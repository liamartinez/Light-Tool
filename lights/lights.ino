#include <FastLED.h>
#include <math.h>//for breathing

#include <Wire.h>
#include "Adafruit_Trellis.h"
//---
Adafruit_Trellis matrix0 = Adafruit_Trellis();
Adafruit_Trellis matrix1 = Adafruit_Trellis();
Adafruit_Trellis matrix2 = Adafruit_Trellis();
Adafruit_Trellis matrix3 = Adafruit_Trellis();

Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0, &matrix1, &matrix2, &matrix3);

#define NUMTRELLIS 1

#define numKeys (NUMTRELLIS * 16)
//---

//consoleMode
#define CONPIN 22
int consoleMode; //0 is tweak, 1 is saved modes
boolean openSaveMode = false;

#define NUM_LEDS    16
#define DATA_PIN 2
#define CLOCK_PIN 3
CRGB leds[NUM_LEDS];

//sensor pins
#define SLIDER1 A3 // hue
#define SLIDER2 A2 // saturation
#define SLIDER3 A0 //brightness
#define SLIDER4 A8 //offset hue
#define SLIDER5 A9 //offset brightness
#define SLIDER6 A10 //offset saturation

#define MODESWITCH A1
#define POT1 A12 //rate
#define POT2 A13 //width
#define POT3 A14 //direction

int dialVal;

//levels
#define MAXBRIGHT 255

int brightness, hueVal, saturation;
int offsetBright, offsetHue, offsetSat;
int breatheMath;
float breatheVal;
int rate, width;
int dir;
int offset; //for beam
int val, val2, val3, val4, val5, val6;
int mode, modeVal;

//for smoothing
const int numReadings = 10;

int readings[numReadings];      // the readings from the analog input
int index = 0;                  // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

//saved settings
#define NUMSPOTS 16
#define NUMPARAMS 10
int saved[NUMSPOTS][NUMPARAMS];

void setup() {
  // begin() with the addresses of each panel in order
  // I find it easiest if the addresses are in order
  trellis.begin(0x70);  // only one
  // trellis.begin(0x70, 0x71, 0x72, 0x73);  // or four!


  // put your setup code here, to run once:
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  delay( 3000 ); // power-up safety delay
  Serial.begin (9600);
  brightness = MAXBRIGHT;
  saturation = 255;
  breatheVal = 5;

  //consoleMode = 1;
  offset = 3; //for beam

  animateLights();

  // initialize all the readings to 0:
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }

  for (int i = 0; i < NUMSPOTS; i++) {
    for (int j = 0; j < NUMPARAMS; j++) {
      saved [i][j] = 0;
    }
  }

  //test config
  int data[NUMPARAMS] = {2, 0, 0, 0, 500, 500, 500, 0, 0, 0};
  for (int i = 0; i < NUMPARAMS; i++) {
    saved[16][i] = data[i];
  }

}

void loop() {

  //values
  modeVal = analogRead(MODESWITCH);
  mode;

  consoleMode = digitalRead(CONPIN);

  if (consoleMode == 0) {

    Serial.println ("TINKER!!");

    val = analogRead(SLIDER1);
    val2 = analogRead (SLIDER2);
    val3 = analogRead (SLIDER3);
    val4 = analogRead (SLIDER4);
    val5 = analogRead (SLIDER5);
    val6 = analogRead (SLIDER6);

    offsetHue = map (val4, 0, 1023, 0, 255);

    offsetBright = 0;
    offsetSat = map (val6, 0, 1023, 255, 0);
    int pot1 = analogRead(POT1);
    //int mode = map(analogRead(MODESWITCH), 0, 1023, 8, 1);

    if (modeVal > 950) mode = 0;
    if (modeVal > 850 && modeVal < 950) mode = 1;
    if (modeVal > 700 && modeVal < 850) mode = 2;
    if (modeVal > 600 && modeVal < 700) mode = 3;
    if (modeVal > 450 && modeVal < 600) mode = 4;
    if (modeVal > 300 && modeVal < 450) mode = 5;
    if (modeVal > 150 && modeVal < 300) mode = 6;
    if (modeVal > 90 && modeVal < 150) mode = 7;
    if (modeVal < 90) mode = 7;


    //map
    hueVal = map(val, 0, 920, 0, 255);
    hueVal = smooth(hueVal);
    saturation = map(val2, 0, 1023, 255, 0);
    //brightness = map(val3, 0, 1023, 255, 0);
    brightness = 255;


    openSaveMode = false; //reset

    for (uint8_t i = 0; i < numKeys; i++) {
      trellis.clrLED(i);
      trellis.writeDisplay();
    }

    switch (mode) {

      case 1:
        steady (hueVal, saturation, brightness);
        break;

      case 2:
        pulse (analogRead(POT1), hueVal, saturation);
        break;

      case 3:
        runAround (analogRead (POT2), analogRead (POT1), hueVal, saturation, brightness, offsetHue, offsetSat, offsetBright);
        break;

      case 4://beam
        beam (analogRead (POT2), analogRead(POT3), hueVal, saturation, brightness);
        break;

      case 5: //sparkle
        sparkle (analogRead (POT2),analogRead (POT1), hueVal, saturation, brightness, offsetHue, offsetSat, offsetBright);        
        break;


      default:
        FastLED.clear();
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = CRGB::Black;

        }
        FastLED.show();

    }

  }


  if (consoleMode == 1) {

    if (!openSaveMode) {
      dialVal = 20; //dont blink anything
      openSaveMode = true;
    }

    for (int i = 0; i < 5; i++) {
      if (i != dialVal) {
        trellis.setLED(i);
        trellis.writeDisplay();
      }
    }

    /*//blinking
    trellis.setLED(dialVal);
    trellis.writeDisplay();
    delay (600);
    trellis.clrLED(dialVal);
    trellis.writeDisplay();
    delay (600);
    */

    // If a button was just pressed or released...
    if (trellis.readSwitches()) {
      // go through every button
      for (uint8_t i = 0; i < numKeys; i++) {
        // if it was pressed, turn it on
        if (trellis.justPressed(i)) {
          Serial.print("v"); Serial.println(i);

          dialVal = i;
          trellis.setLED(i);
        }
        // if it was released, turn it off
        if (trellis.justReleased(i)) {
          Serial.print("^"); Serial.println(i);
          trellis.clrLED(i);
        }
      }
      // tell the trellis to set the LEDs we requested
      trellis.writeDisplay();
    }


    switch (dialVal) {
      case 0:
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = 0x0078D7;
        }
        break;

      case 1:
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = 0x008285;
        }
        break;

      case 2:
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = 0x00F8A3D;
        }
        break;

      case 3:
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = 0xFF4242;
        }
        break;

      case 4:
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = 0xEB005E;
        }
        break;

      case 5:
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = 0x8C6BC2;
        }
        break;

      case 15:
        pulse (saved[16][1], saved[16][4], saved[16][5]);
        break;
    }

  }
  FastLED.show();

}


void animateLights() {
  // light up all the LEDs in order
  for (uint8_t i = 0; i < numKeys; i++) {
    trellis.setLED(i);
    trellis.writeDisplay();
    delay(50);
  }
  // then turn them off
  for (uint8_t i = 0; i < numKeys; i++) {
    trellis.clrLED(i);
    trellis.writeDisplay();
    delay(50);
  }
}


//--------------- modes---------------------------------//

void steady(int h, int s, int b) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV( h, s, b);
  }
}

void pulse(int v, int h, int s) {
  breatheVal = map(v, 0, 1023, 500.0, 4000.0);
  Serial.println (breatheVal);
  breatheMath = int((exp(sin(millis() / breatheVal * PI)) - 0.36787944) * 108.0);
  //http://sean.voisen.org/blog/2011/10/breathing-led-with-arduino/

  float b = map (breatheMath, 0, 255, 50, MAXBRIGHT); //leds are not great with the low values
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV( h, s, b);
  }
}

void runAround (int width_, int rate_, int h, int s, int b, int h2, int s2, int b2) {
  int w = map (width_, 1023, 0, 1, NUM_LEDS / 2);
  int r = map (rate_, 0, 1024, 10, 100);
  for (int i = 0; i < NUM_LEDS; i ++) {
    if (i < NUM_LEDS - w) {
      leds[i + w] = CHSV( h + h2, s + s2, b + b2);
    } else {
      leds[i + w] = CHSV( h + h2, s + s2, b + b2);
      leds[width - (NUM_LEDS - i)] = CHSV(h + h2, s + s2, b + b2);
    }
    FastLED.show();
    delay(r);
    leds[i] = CHSV( h, s, b);
  }
}

void beam(int width_, int dir_, int h, int s, int b) {
  width = map (width_, 1024, 0, 1, NUM_LEDS / 4);
  dir = map(dir_, 1023, 0, 0, NUM_LEDS);

  for (int i = 0; i < NUM_LEDS; i++) {
  leds[i] = CHSV( h, s, 0);
  }

  for (int i = dir - width; i < dir + width; i++) {
  leds[i] = CHSV( h, s, b);
  }
}

void sparkle(int width_, int rate_, int h, int s, int b, int h2, int s2, int b2) {
        width = map (width_, 0, 1024, 1, NUM_LEDS);
        rate = map (rate_, 0, 1024, 10, 200);
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = CHSV( h, s, b);
        }
        delay(rate);
        for (int j = 0; j < width; j++) {
          leds[random(0, NUM_LEDS - 1)] = CHSV( h + h2, s + s2, b + b2);
        }
}

//------------------helper ----------------------------//

int smooth (int r) {
  // subtract the last reading:
  total = total - readings[index];
  // read from the sensor:
  readings[index] = r;
  // add the reading to the total:
  total = total + readings[index];
  // advance to the next position in the array:
  index = index + 1;

  // if we're at the end of the array...
  if (index >= numReadings)
    // ...wrap around to the beginning:
    index = 0;

  // calculate the average:
  average = total / numReadings;
  // send it to the computer as ASCII digits

  delay(1);        // delay in between reads for stability
  return average;
}
