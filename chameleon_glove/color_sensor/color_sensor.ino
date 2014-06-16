#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <Adafruit_NeoPixel.h>
 
#define PIN 9
#define TPIXEL 113 //The total amount of pixel's/led's in your connected strip/stick (Default is 60)
 
int switchPin = 10; // switch is connected to pin 10
int val; // variable for reading the pin status
int val2;
int buttonState; // variable to hold the button state
int lightMode = 0; // how many times the button has been pressed
 
Adafruit_NeoPixel strip = Adafruit_NeoPixel(TPIXEL, PIN, NEO_GRB + NEO_KHZ800);
// our RGB -> eye-recognized gamma color
byte gammatable[256];
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
 
void setup() {
  Serial.begin(9600); // Set up serial communication at 9600bps
  pinMode(switchPin, INPUT_PULLUP); // Set the switch pin as input
  pinMode(PIN, OUTPUT);
  strip.setBrightness(80); //adjust brightness here
  buttonState = digitalRead(switchPin); // read the initial state
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1); // halt!
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
  
  for (int i=0; i<3; i++) { //this sequence flashes the first pixel three times as a countdown to the color reading.
    strip.setPixelColor (0, strip.Color(188, 188, 188)); //white, but dimmer-- 255 for all three values makes it blinding!
    strip.show();
    delay(1000);
    strip.setPixelColor (0, strip.Color(0, 0, 0));
    strip.show();
    delay(500);
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
