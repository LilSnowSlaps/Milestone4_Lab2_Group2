
// 
// MSE2202 Lab 2 Team 2: Colour Sensor with Servo code, reworked to use the greenFlag. Essentially demo code.
// 
//  Language: Arduino (C++)
//  Target:   ESP32-S3
//  Author:   Rolex Brabander, Foster Deighton, Jathur ...., Jehanzeb .....
//  Date:     2024 03 29 
//

//#define PRINT_COLOUR                                  // uncomment to turn on output of colour sensor data

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <SPI.h>
#include "Adafruit_TCS34725.h"
#include <MSE2202_Lib.h>

// Function declarations
void doHeartbeat();
void setServoPosition(int degrees);
int degreesToDutyCycle(int degrees);


// Constants
const int cHeartbeatInterval = 75;                    // heartbeat update interval, in milliseconds
const int cSmartLED          = 21;                    // when DIP switch S1-4 is on, SMART LED is connected to GPIO21
const int cSmartLEDCount     = 1;                     // number of Smart LEDs in use
const int cSDA               = 47;                    // GPIO pin for I2C data
const int cSCL               = 48;                    // GPIO pin for I2C clock
const int cTCSLED            = 14;                    // GPIO pin for LED on TCS34725
const int cLEDSwitch         = 46;                    // DIP switch S1-2 controls LED on TCS32725    
const int cServoPin          = 41;                    // GPIO pin for servo motor
const int cServoPin2         = 42;
const int cServoChannel      = 5;                     // PWM channel used for the RC servo motor
const int cServoChannel2     = 6;

// Variables
boolean heartbeatState       = true;                  // state of heartbeat LED
unsigned long lastHeartbeat  = 0;                     // time of last heartbeat state change
unsigned long curMillis      = 0;                     // current time, in milliseconds
unsigned long prevMillis     = 0;                     // start time for delay cycle, in milliseconds

unsigned long colorReadStart = 0;
unsigned long colorReadInterval = 2000;
long timecount500ms = 0;
bool timeup500ms = false;
long currenttime =0;


// Declare SK6812 SMART LED object
//   Argument 1 = Number of LEDs (pixels) in use
//   Argument 2 = ESP32 pin number 
//   Argument 3 = Pixel type flags, add together as needed:
//     NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//     NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//     NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//     NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//     NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel SmartLEDs(cSmartLEDCount, cSmartLED, NEO_RGB + NEO_KHZ800);

// Smart LED brightness for heartbeat
unsigned char LEDBrightnessIndex = 0; 
unsigned char LEDBrightnessLevels[] = {0, 0, 0, 5, 15, 30, 45, 60, 75, 90, 105, 120, 135, 
                                       150, 135, 120, 105, 90, 75, 60, 45, 30, 15, 5, 0};

// TCS34725 colour sensor with 2.4 ms integration time and gain of 4
// see https://github.com/adafruit/Adafruit_TCS34725/blob/master/Adafruit_TCS34725.h for all possible values
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_4X);
bool tcsFlag = 0;                                     // TCS34725 flag: 1 = connected; 0 = not found

bool greenFlag = 0; // 0 means green is not detected, 1 means green is detected

void setup() {
  Serial.begin(115200);                               // Standard baud rate for ESP32 serial monitor

  // Set up SmartLED
  SmartLEDs.begin();                                  // initialize smart LEDs object
  SmartLEDs.clear();                                  // clear pixel
  SmartLEDs.setPixelColor(0, SmartLEDs.Color(0,0,0)); // set pixel colours to black (off)
  SmartLEDs.setBrightness(0);                         // set brightness [0-255]
  SmartLEDs.show();                                   // update LED
  Wire.setPins(cSDA, cSCL);                           // set I2C pins for TCS34725
  pinMode(cTCSLED, OUTPUT);                           // configure GPIO to control LED on TCS34725
  pinMode(cLEDSwitch, INPUT_PULLUP);                  // configure GPIO to set state of TCS34725 LED 
  pinMode(cServoPin, OUTPUT);                      // configure servo GPIO for output
  pinMode(cServoPin2, OUTPUT);                      // configure servo GPIO for output
  ledcSetup(cServoChannel, 50, 14);                // setup for channel for 50 Hz, 14-bit resolution
  ledcSetup(cServoChannel2, 50, 14);                // setup for channel for 50 Hz, 14-bit resolution
  ledcAttachPin(cServoPin, cServoChannel);         // assign servo pin to servo channel
    ledcAttachPin(cServoPin2, cServoChannel2);         // assign servo pin to servo channel


  if (tcs.begin()) {
    Serial.printf("Found TCS34725 colour sensor\n");
    tcsFlag = true;
  } 
  else {
    Serial.printf("No TCS34725 found ... check your connections\n");
    tcsFlag = false;
  }
}

void loop() {

  currenttime = millis();

  timecount500ms = timecount500ms +1;
  if (timecount500ms>2){
    timeup500ms = true;
    timecount500ms = 0;
  }


  uint16_t r, g, b, c;                                // RGBC values from TCS34725
  
  digitalWrite(cTCSLED, !digitalRead(cLEDSwitch));    // turn on onboard LED if switch state is low (on position)
  if (tcsFlag) {                                      // if colour sensor initialized
    tcs.getRawData(&r, &g, &b, &c);                   // get raw RGBC values
#ifdef PRINT_COLOUR            
      Serial.printf("R: %d, G: %d, B: %d, C %d\n", r, g, b, c);
#endif
  }

  // Check if it's time to read the color
  if (millis() - colorReadStart >= colorReadInterval || colorReadStart == 0) {
    // Read color values from the sensor after a 2-second interval for accurate reading
    if (millis() - colorReadStart >= colorReadInterval) {
      tcs.getRawData(&r, &g, &b, &c);          
      Serial.printf("R: %d, G: %d, B: %d, C %d\n", r, g, b, c);
      // Check if the green value is above 15
      if (g > 40 && g < 2000 && c > 160 && c < 200) {        
        Serial.println("Green Detected.");
        greenFlag = 1;
      } else {
        Serial.println("Green not detected");
        greenFlag = 0;
      }
      // Reset the color read start time
      colorReadStart = millis();
    }
  }
  if (greenFlag == 1) {

    if (timeup500ms != true){
    ledcWrite(cServoChannel, degreesToDutyCycle(80));
    ledcWrite(cServoChannel2, degreesToDutyCycle(100));
    }
    else{
    Serial.println("Writing 180 degrees to servo.");
     ledcWrite(cServoChannel, degreesToDutyCycle(180));
    ledcWrite(cServoChannel2, degreesToDutyCycle(0));
    }

  } else {
    // 149 Degrees was the highest we could go before, it was because of the degreesToDutyCycle function being uncalibrated
    // this degreesToDutyCycle function maps 0-180 degrees to 500-2500 pwm. My (Rolex) lab 2 code maps the degrees
    // to 400-2100 pwm.
    Serial.println("Writing 0 degrees to servo.");
    ledcWrite(cServoChannel, degreesToDutyCycle(90));
    ledcWrite(cServoChannel2, degreesToDutyCycle(90));
  }
  doHeartbeat();                                      // update heartbeat LED
}

// update heartbeat LED
void doHeartbeat() {
  curMillis = millis();                               // get the current time in milliseconds
  // check to see if elapsed time matches the heartbeat interval
  if ((curMillis - lastHeartbeat) > cHeartbeatInterval) {
    lastHeartbeat = curMillis;                        // update the heartbeat time for the next update
    LEDBrightnessIndex++;                             // shift to the next brightness level
    if (LEDBrightnessIndex > sizeof(LEDBrightnessLevels)) { // if all defined levels have been used
      LEDBrightnessIndex = 0;                         // reset to starting brightness
    }
    SmartLEDs.setBrightness(LEDBrightnessLevels[LEDBrightnessIndex]); // set brightness of heartbeat LED
    SmartLEDs.setPixelColor(0, SmartLEDs.Color(0, 250, 0)); // set pixel colours to green
    SmartLEDs.show();                                 // update LED
  }
}
void setServoPosition(int degrees) {  //writes the servo to a duty cycle   
  int dutyCycle = degreesToDutyCycle(degrees);
  ledcWrite(cServoChannel, dutyCycle);
}

int degreesToDutyCycle(int degrees) { //maps servo degrees to duty cycle
  // Map degrees (0-180) to duty cycle (400-2100). changed from 500-2500 before
  return map(degrees, 0, 180, 400, 2100);
}

