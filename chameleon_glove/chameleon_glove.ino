#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <Adafruit_NeoPixel.h>
#include <Adafruit_LSM303.h>


// Number of the pin LED are connected to
#define LEDPIN 9

// The total amount of pixels (ring + fingers starting from index ending with thumb)
#define TPIXEL 16 + 19 + 20 + 19 + 19 + 19

// Color sensor pin on the index finger
#define COLOR_SENSOR_PIN 10 // switch is connected to pin 10

// Mode switch pin
#define MODE_PIN 6

// Modes in the app
#define OFF_MODE 0
#define COLOR_THIEF_MODE 1
#define COMPASS_MODE 2

// Mode switching variable
#define TOTAL_MODES 2
int current_mode = OFF_MODE;
int mode_button_state = 0;
boolean mode_initialized = false;

// our RGB -> eye-recognized gamma color
byte gammatable[256];

// Peripherals 
Adafruit_NeoPixel strip = Adafruit_NeoPixel(TPIXEL, LEDPIN, NEO_GRB + NEO_KHZ800);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
Adafruit_LSM303 lsm;

void setup() {
  Serial.begin(9600); // Set up serial communication at 9600bps

  // Set the switch pins as inputs
  pinMode(MODE_PIN, INPUT_PULLUP);
  pinMode(COLOR_SENSOR_PIN, INPUT_PULLUP);

  // Neopixels
  pinMode(LEDPIN, OUTPUT);
  strip.setBrightness(80); //adjust brightness here
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  // Color Sensor
  // Try to initialise and warn if we couldn't detect the chip
  if (!tcs.begin()) {
    Serial.println("No TCS34725 found ... check your connections");
    while (1); // halt!
  }

  tcs.setInterrupt(true); // turn off color sensor's LED

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
  }
}

void readSensorAndSetColor() {
  uint16_t clear, red, green, blue;
 
  tcs.setInterrupt(false); // turn on LED
  delay(60); // takes 50ms to read
  tcs.getRawData(&red, &green, &blue, &clear);
  tcs.setInterrupt(true); // turn off LED
 
  colorWipe(sensorToColor(red, green, blue, clear));
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
// This blocks input until colors are all set
void colorWipe(uint32_t c) {
  for (uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
  }
}
 
void loop() {
  // pair of variables for switch value reading
  int val, val2;

  val = digitalRead(MODE_PIN);

  if (val != mode_button_state) {
    // if button was already pressed in previous loop, don't continue switching the mode
    mode_button_state = val;

    if (mode_button_state == LOW) {
      // switching mode
      current_mode += 1;
      if (current_mode >= TOTAL_MODES) {
        current_mode = 0;
      }
      mode_initialized = false;
    }
  }

  if (!mode_initialized) {
    initMode(current_mode);
  }

  val = digitalRead(COLOR_SENSOR_PIN);
  Serial.print("Color pin: ");
  Serial.println(val);

  if (current_mode == COLOR_THIEF_MODE) {
    // read input value twice and store it to see if button was pressed continiously
    delay (20);
    val2 = digitalRead(COLOR_SENSOR_PIN);

    Serial.print("Measuring color: ");
    Serial.print(val);
    Serial.println(val2);

    // if same value after 20 millis and it's pressed, trigger
    if (val == val2 && val == LOW) {
      readSensorAndSetColor();
    }
  }
}

void initMode(int mode) {
  int i, j, endofline;
  colorWipe(0);

  if (mode == COLOR_THIEF_MODE) {
    for (i = 1; i < 35; i++) {
      endofline = i + 10;
      if (endofline > 35) {
        tcs.setInterrupt(false); // turn on LED
        endofline = 35;
      }
      for (j = i; j < endofline; j+=2) {
        strip.setPixelColor(j, sensorToColor(255,255,255,255));
        strip.setPixelColor(j-1, 0);
        strip.setPixelColor(j+1, 0);
      }
      delay(60);
      strip.show();
    }
    colorWipe(0);
    readSensorAndSetColor();
  } else if (mode == COMPASS_MODE) {
    strip.setPixelColor(35, 255);
    strip.setPixelColor(36, 255);
    strip.setPixelColor(37, 255);
    strip.show();
  }

  mode_initialized = true;
}
