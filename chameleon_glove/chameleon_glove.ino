#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <Adafruit_NeoPixel.h>
#include <Adafruit_LSM303.h>


#define PIN 9
#define TPIXEL 113 //The total amount of pixel's/led's in your connected strip/stick (Default is 60)
 
int switchPin = 10; // switch is connected to pin 10
int val; // variable for reading the pin status
int val2;
int buttonState; // variable to hold the button state
int lightMode = 0; // how many times the button has been pressed
// our RGB -> eye-recognized gamma color
byte gammatable[256];

// Peripherals 
Adafruit_NeoPixel strip = Adafruit_NeoPixel(TPIXEL, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
Adafruit_LSM303 lsm;

void setup() {
  Serial.begin(9600); // Set up serial communication at 9600bps

  pinMode(switchPin, INPUT_PULLUP); // Set the switch pin as input
  buttonState = digitalRead(switchPin); // read the initial state

  // NEopixels
  pinMode(PIN, OUTPUT);
  strip.setBrightness(80); //adjust brightness here
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  // Color Sensor
  if (!tcs.begin()) {
    Serial.println("No TCS34725 found ... check your connections");
    while (1); // halt!
  }
  
  // Accelerometer / magnetometer
  // Try to initialise and warn if we couldn't detect the chip
  if (!lsm.begin())
  {
    Serial.println("Oops ... unable to initialize the LSM303. Check your wiring!");
    while (1);
  }

  // thanks PhilB for this gamma table!
  // it helps convert RGB colors to what humans see
  for (int i=0; i<256; i++) {
    float x = i;
    x /= 255;
    x = pow(x, 2.5);
    x *= 255;
    gammatable[i] = x;
    //Serial.println(gammatable[i]);
  }

  readSensorAndSetColor();
}

void readSensorAndSetColor() {
  uint16_t clear, red, green, blue;
 
  tcs.setInterrupt(false); // turn on LED
 
  delay(60); // takes 50ms to read

  tcs.getRawData(&red, &green, &blue, &clear);
  tcs.setInterrupt(true); // turn off LED
  Serial.print("C:\t"); Serial.print(clear);
  Serial.print("\tR:\t"); Serial.print(red);
  Serial.print("\tG:\t"); Serial.print(green);
  Serial.print("\tB:\t"); Serial.print(blue);

  colorWipe(sensorToColor(red, green, blue, clear), 0);
}

uint32_t sensorToColor(uint16_t red, uint16_t green, uint16_t blue, uint16_t clear) {
  // Figure out some basic hex code for visualization
  uint32_t sum = red;
  sum += green;
  sum += blue;
  sum = clear;
  float r, g, b;
  r = red; r /= sum;
  g = green; g /= sum;
  b = blue; b /= sum;
  r *= 256; g *= 256; b *= 256;
/*
  Serial.print("\t");
  Serial.print((int)r, HEX); Serial.print((int)g, HEX); Serial.print((int)b, HEX);
  Serial.println();
 
  Serial.print((int)r );
  Serial.print(" ");
  Serial.print((int)g);
  Serial.print(" ");
  Serial.println((int)b);
*/  
  return strip.Color(gammatable[(int)r], gammatable[(int)g], gammatable[(int)b]);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();

    //delay(20);
  }
}
 
void loop() {
  val = digitalRead(switchPin); // read input value and store it in val
  delay (20);
  val2 = digitalRead(switchPin);
  
  Serial.print("LightMode: ");
  Serial.print(lightMode);  
  Serial.print("\n");

  Serial.print("Switch: ");
  Serial.print(val);
  Serial.print("\t");
  Serial.print(val2);
  Serial.print("\n");
  
  if (val == val2) {
    if (val==LOW) { // the button state has changed!
      if (lightMode == 0) {
        lightMode = 1;
      }
    }
  }

  // show color
  if (lightMode == 0) {
    strip.show();
    Serial.print("Showing Color\n");
  }

  // measuring light
  if (lightMode == 1) {
    Serial.print("Sensing Color\n");
    readSensorAndSetColor();
    lightMode = 0;
  }
}
