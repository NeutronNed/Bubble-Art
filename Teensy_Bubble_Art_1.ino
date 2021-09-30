/*  Bubble Art/Spectrum Analyzer using a Teensy 3.2, (TBD) Neopixel LEDs and the Teensy Audio 
     Adapter Board. 
    
    The analog signals from the left and right line inputs are mixed and 
    then processed to create an array of frequency magnitudes.  These are
    used to create visual patterns on the Neopixel LEDs.

    For information on the Teensy Audio adapter: http://www.pjrc.com/teensy/td_libs_Audio.html

    High-level summary:
    1) The audio signals from the L & R channels are mixed and binned by frequency into 1024 bins
    2) The expanded set of spectrum data is grouped into 12 octave bands.
    3) Each of the TBD LEDS has calculated a color and intensity based on the appropriate spectrum value
    5) The strip is refreshed
    6) Repeat forever
    
    Revision: 1
    Date: 30-Sep-2021
    Leon Durivage
*/

#include <Adafruit_NeoPixel.h>  // LED library
#include <Audio.h>              // Teensy Audio library
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// *************************************************************************************** LED PARAMETERS
#define LED_PIN     2          // Digital pin on the Teensy connected to the NeoPixel string
#define NUMPIXELS  72          // How many NeoPixels are attached (TBD)

byte LEDoffset = 36;           // the offset position for the first LED
byte colorScale = 5;           // the variation in the color map - 0=one color, 5=full spectrum

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);


// ***************************************************************************** AUDIO ADAPTER PARAMETERS

const int my_input = AUDIO_INPUT_LINEIN; //Glass head uses audio line input from adapter board

// Create the Audio components.  
// These should be created in the order data flows: inputs/sources -> processing -> outputs

AudioInputI2S          audio_input;         // Teensy Audio Adapter: input is both channels L&R
AudioMixer4            mixer;               // a mixer object to mix the L & R channels
AudioAnalyzeFFT1024    my_FFT;              // Compute a 1024 point Fast Fourier Transform (FFT)
                                            //  frequency analysis, with real value (magnitude) 
                                            //  output. The frequency resolution is 43 Hz, useful 
                                            //  for detailed audio visualization
AudioConnection        patchCord1(audio_input, 0, mixer, 0);  // send L to mixer input 0
AudioConnection        patchCord2(audio_input, 1, mixer, 1);  // send R to mixer input 1
AudioConnection        patchCord3(mixer, my_FFT);             // send mixed signal to FFT object
AudioOutputI2S         audio_output;        // output to the adapter's headphones & line-out

AudioControlSGTL5000   audio_adapter;       // object to send control signals to the audio adapter

// An array to hold the 16 frequency bands
float level[16];

// ************************************************************************************************ SETUP
void setup() {  // Set up code runs once - on power up and reset

// Debug monitor mode - comment out for final code
//  Serial.begin(9600);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(255);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audio_adapter.enable();
  audio_adapter.inputSelect(my_input);
  audio_adapter.volume(0.5);

  // Configure the window algorithm to use
  my_FFT.windowFunction(AudioWindowHanning1024);

}

// ************************************************************************************************* LOOP
void loop() {

// Code start
//    colorMap = (colorMap + 1) % 255;    // increment color Map index

// FFT ********************************************************************************************** FFT  
  if (my_FFT.available()) 
  {
    // each time new FFT data is available
    // print it all to the Arduino Serial Monitor
    // 7.6msec my_FFT.windowFunction(AudioWindowHamming1024);

    level[0] =  my_FFT.read(0, 1) * 1.5;
    level[1] =  my_FFT.read(2, 3) * 1.5;
    level[2] =  my_FFT.read(4, 6) * 1.4;
    level[3] =  my_FFT.read(7, 11) * 1.3;
    level[4] =  my_FFT.read(12, 19) * 1.2;
    level[5] =  my_FFT.read(20, 31) * 1.1;
    level[6] =  my_FFT.read(32, 50);
    level[7] =  my_FFT.read(51, 78);
    level[8] =  my_FFT.read(79, 119);
    level[9] =  my_FFT.read(120, 176);
    level[10] = my_FFT.read(177, 254); 
    level[11] = my_FFT.read(255, 511) * .7; 

    
    //Serial.print("FFT: ");
    //for (i=0; i<16; i++) {
    //  if (level[i] >= 0.01) {
    //    Serial.print(level[i]);
    //    Serial.print(" ");
    //  } else {
    //    Serial.print("  -  "); // don't print "0.00"
    //  }
    //}
    //Serial.println();

  }

// LED Display ***************************************************************************** LED Display

// Set the overall brightness of the LEDs based on the left touch switch

  
  for (int i = 0; i < NUMPIXELS/2; i++){   // cycle through all 72 LEDs
    byte Color = i * colorScale;  // this results in a color map position
    byte Channel = i / 4 ;     // there are 12 frequency bands, which scales to 48/12 = 4
 
    byte Red1 = (WheelR(Color) * level[Channel]) ;   // brightness proportionally to the loudness
    byte Green1 = (WheelG(Color) * level[Channel]) ; //  of the individual channel
    byte Blue1 = (WheelB(Color) * level[Channel]) ;    
    
    strip.setPixelColor(i+36, Red1, Green1, Blue1);  // Bottom to Top


  }
  strip.show();   //show the new strip colors 



}

// ****************************************************************************************** SUBROUTINES

/*******************************************************************************************/
/* WheelR: same as above but just returned the Red byte (not the entire RGB word)          */
byte WheelR(byte WheelPos) {
  if(WheelPos < 85) {
    return WheelPos * 3; 
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return 255 - WheelPos * 3;
  } else {
    return 0;
  }
}
/*******************************************************************************************/
/* WheelG: same as above but just returned the Green byte (not the entire RGB word)        */
byte WheelG(byte WheelPos) {
  if(WheelPos < 85) {
   return 255 - WheelPos * 3;
  } else if(WheelPos < 170) {
   return  0;
  } else {
   WheelPos -= 170;
   return  WheelPos * 3;
  }
}
/*******************************************************************************************/
/* WheelB: same as above but just returned the Blue byte (not the entire RGB word)         */
byte WheelB(byte WheelPos) {
  if(WheelPos < 85) {
   return 0;
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return  WheelPos * 3;
  } else {
   WheelPos -= 170;
   return  255 - WheelPos * 3;
  }
}
