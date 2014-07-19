#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <Adafruit_NeoPixel.h>
#include <Adafruit_LSM303.h>


// Number of the pin LED are connected to
#define LEDPIN 9

// Finger LENGTHs in pixels
#define INDEX_FINGER_LENGTH 19
#define MIDDLE_FINGER_LENGTH 20
#define RING_FINGER_LENGTH 19
#define PINKY_LENGTH 19
#define THUMB_LENGTH 19

#define INDEX_FINGER_BASE 0
#define MIDDLE_FINGER_BASE 14
#define RING_FINGER_BASE 13
#define PINKY_BASE 10
#define THUMB_BASE 3

#define RING_LENGTH 16

// The total amount of pixels (ring + fingers starting from index ending with thumb)
#define TPIXEL RING_LENGTH + INDEX_FINGER_LENGTH + MIDDLE_FINGER_LENGTH + RING_FINGER_LENGTH + PINKY_LENGTH + THUMB_LENGTH

// finger parameters
int finger_start[5];
int finger_end[5];
int finger_base[5];

// Color sensor pin on the index finger
#define COLOR_SENSOR_PIN 10 // switch is connected to pin 10

// On-off velcro switch
#define ON_PIN 12

// Mode switch pin
#define MODE_PIN 6

// Modes in the app
#define OFF_MODE 0
#define COLOR_THIEF_MODE 1
#define COMPASS_MODE 2

// Mode switching variables
#define TOTAL_MODES 2
#define START_MODE COLOR_THIEF_MODE;

int current_mode = START_MODE;
int mode_button_state = 0;
boolean mode_initialized = false;

// our RGB -> eye-recognized gamma color
byte gammatable[256];

// Peripherals 
Adafruit_NeoPixel strip = Adafruit_NeoPixel(TPIXEL, LEDPIN, NEO_GRB + NEO_KHZ800);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
Adafruit_LSM303 compass;

void setup() {
  initFingersArray();

  Serial.begin(9600); // Set up serial communication at 9600bps

  // Set the switch pins as inputs
  pinMode(ON_PIN, INPUT_PULLUP);
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
  if (!compass.begin())
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

  /*
  Serial.print("Red: ");         Serial.print(red); Serial.print(" ");
  Serial.print("Green: ");       Serial.print(green); Serial.print(" ");
  Serial.print("Blue: ");        Serial.print(blue); Serial.print(" ");
  Serial.print("Intencity: ");   Serial.print(clear); Serial.print(" ");
  */

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

  if (digitalRead(ON_PIN) != LOW) {
    colorWipe(0);
    current_mode = START_MODE;
    mode_initialized = false;

    delay(100);
    return;
  }

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
        current_mode = START_MODE;
      }
      mode_initialized = false;
    }
  }

  if (!mode_initialized) {
    initMode(current_mode);
  }

  if (current_mode == COLOR_THIEF_MODE) {
    // read input value twice and store it to see if button was pressed continiously
    val = digitalRead(COLOR_SENSOR_PIN);
    delay (20);
    val2 = digitalRead(COLOR_SENSOR_PIN);

    /*
    Serial.print("Measuring color: ");
    Serial.print(val);
    Serial.println(val2);
    */

    // if same value after 20 millis and it's pressed, trigger
    if (val == val2 && val == LOW) {
      readSensorAndSetColor();
    }
  } else if (current_mode == COMPASS_MODE) {
    compass.read();

//    Serial.print("Accel ");
//    Serial.print("X: "); Serial.print((int)compass.accelData.x);       Serial.print(" ");
//    Serial.print("Y: "); Serial.print((int)compass.accelData.y);       Serial.print(" ");
//    Serial.print("Z: "); Serial.println((int)compass.accelData.z);     Serial.print("");
//    Serial.print("Mag ");
//    Serial.print("X: "); Serial.print((int)compass.magData.x);         Serial.print(" : ");
//    Serial.print("Y: "); Serial.print((int)compass.magData.y);         Serial.print(" -> ");
//    Serial.print("Z: "); Serial.println((int)compass.magData.z);       Serial.print("\n");

    // Angle from 0 to 360 degrees
    double angle = (degrees(atan2(compass.magData.x, compass.magData.y)) + 180);

    // Adjusting compass position
    #define ANGLE_OFFSET 0
    angle += ANGLE_OFFSET;
    if (angle > 360) {
      angle = angle - 360;
    }

    int bearing_led = 15 - (int)(angle * 16 / 360);
//    Serial.print("LED: "); Serial.print(bearing_led);         Serial.print("\n");

    for (int led = 0; led < 16; led++) {
      if (led == bearing_led) {
        lightUpLEDAndFinger(led, 255); // north
        strip.setPixelColor((led + 12) % 16, sensorToColor(255, 255, 0, 255)); // east
        strip.setPixelColor((led + 8) % 16, sensorToColor(255, 0, 0, 255)); // south
        strip.setPixelColor((led + 4) % 16, sensorToColor(0, 255, 0, 255));
      } else {
        lightUpLEDAndFinger(led, 0);
      }
    }
    strip.show();
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
        strip.setPixelColor(j, sensorToColor(255, 255, 255, 255));
        strip.setPixelColor(j-1, 0);
        strip.setPixelColor(j+1, 0);
      }
      delay(60);
      strip.show();
    }
    colorWipe(0);
    readSensorAndSetColor();
  } else if (mode == COMPASS_MODE) {
    for (int i = 1; i < 16; i++) {
      delay(50);

/*
      strip.setPixelColor((i - 1) % 16, 0); // north
      strip.setPixelColor((i + 3) % 16, 0); // east
      strip.setPixelColor((i + 7) % 16, 0); // south
      strip.setPixelColor((i + 11) % 16, 0); // west
*/
      strip.setPixelColor(i % 16, sensorToColor(0, 0, 255, 255)); // north
      strip.setPixelColor((i + 12) % 16, sensorToColor(255, 255, 0, 255)); // east
      strip.setPixelColor((i + 8) % 16, sensorToColor(255, 0, 0, 255)); // south
      strip.setPixelColor((i + 4) % 16, sensorToColor(0, 255, 0, 255)); // west

      strip.show();
    }
  }

  mode_initialized = true;
}

void lightUpLEDAndFinger(int led, int ledcolor) {
  strip.setPixelColor(led, ledcolor);

  for (int finger = 0; finger < 5; finger++) {
/*
    Serial.print("LED#: ");     Serial.print(led);                    Serial.print(" ");
    Serial.print("Finger: ");   Serial.print(finger);                 Serial.print(" ");
    Serial.print("Start: ");    Serial.print(finger_start[finger]);   Serial.print(" ");
    Serial.print("End: ");      Serial.print(finger_end[finger]);     Serial.print(" ");
    Serial.print("Base: ");     Serial.print(finger_base[finger]);    Serial.print("\n");
*/
    if (led == finger_base[finger]) {
      for (int l = finger_start[finger]; l < finger_end[finger]; l++) {
        strip.setPixelColor(l, ledcolor);
      }
    }
  }
}

// helper function for calculating finger parameters
void initFingersArray() {
  // index finger
  finger_start[0] = RING_LENGTH;
  finger_end[0] = RING_LENGTH + INDEX_FINGER_LENGTH;
  finger_base[0] = INDEX_FINGER_BASE;

  // middle finger
  finger_start[1] = RING_LENGTH + INDEX_FINGER_LENGTH;
  finger_end[1] = RING_LENGTH + INDEX_FINGER_LENGTH + MIDDLE_FINGER_LENGTH;
  finger_base[1] = MIDDLE_FINGER_BASE;

  // ring finger
  finger_start[2] = RING_LENGTH + INDEX_FINGER_LENGTH + MIDDLE_FINGER_LENGTH;
  finger_end[2] = RING_LENGTH + INDEX_FINGER_LENGTH + MIDDLE_FINGER_LENGTH + RING_FINGER_LENGTH;
  finger_base[2] = RING_FINGER_BASE;

  // pinky
  finger_start[3] = RING_LENGTH + INDEX_FINGER_LENGTH + MIDDLE_FINGER_LENGTH + RING_FINGER_LENGTH;
  finger_end[3] = RING_LENGTH + INDEX_FINGER_LENGTH + MIDDLE_FINGER_LENGTH + RING_FINGER_LENGTH + PINKY_LENGTH;
  finger_base[3] = PINKY_BASE;

  // thumb
  finger_start[4] = RING_LENGTH + INDEX_FINGER_LENGTH + MIDDLE_FINGER_LENGTH + RING_FINGER_LENGTH + PINKY_LENGTH;
  finger_end[4] = RING_LENGTH + INDEX_FINGER_LENGTH + MIDDLE_FINGER_LENGTH + RING_FINGER_LENGTH + PINKY_LENGTH + THUMB_LENGTH;
  finger_base[4] = THUMB_BASE;
}
