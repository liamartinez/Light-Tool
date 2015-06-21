/*

0 - Mode
1 - Rate
2 - Width
3 - Direction
4 - Color1 hue
5 - Color1 sat
6 - Color1 val
7 - Color2 hue
8 - color2 sat
9 - Color2 val

*/

#include <FastLED.h>
#include <math.h>//for breathing
#include <Wire.h>
#include "Adafruit_Trellis.h"
#include <SD.h>
#include <SPI.h>

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
int rateVal, widthVal, dirVal;
int mode, modeVal;

//for smoothing
const int numReadings = 10;
int readings[numReadings];      // the readings from the analog input
int index = 0;                  // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

//timer
long startTime = 0;
long currentTime;
long interval = 100;
int numPresses = 0;

//saved settings
#define NUMSPOTS 16
#define NUMPARAMS 10
int saved[NUMSPOTS][NUMPARAMS];
File myFile;
boolean buttonPressed = false;
boolean buttonDone = false;
long buttonStart = 0;
long buttonInterval = 1000;
int curNum, lastNum;
boolean saveNow; 

void setup() {

  trellis.begin(0x70);  // only one
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  delay( 2000 ); // power-up safety delay

  Serial.begin (9600);
  Serial.setTimeout(50);

  //int data[NUMPARAMS] = {2, 0, 0, 0, 100, 500, 500, 0, 0, 0};
  int data[NUMPARAMS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  Serial.print("Initializing SD card...");
  pinMode(53, OUTPUT);
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  /*
   Serial.println("Clearing data...");
   SD.remove("settings.txt");
   myFile = SD.open("settings.txt", FILE_WRITE);

   // if the file opened okay, write to it:
   if (myFile) {
     Serial.print("Writing to settings.txt...");
     myFile.print("S");
     for (int i = 0; i < NUMPARAMS; i++) {
     myFile.print(String(data[i]));
     myFile.print(",");
     }
     myFile.println("");
     //myFile.println(data);
     // close the file:
     myFile.close();
     Serial.println("done.");
   } else {
     // if the file didn't open, print an error:
     Serial.println("error opening settings.txt");
   }
   */
  // re-open the file for reading:
  myFile = SD.open("settings.txt");
  if (myFile) {
    // Serial.write(myFile.read());
    Serial.println("settings.txt");

    while (myFile.available()) {
      //this doesnt work with a loop so we have to enter it manually
      data[0] = myFile.parseInt();
      data[1] = myFile.parseInt();
      data[2] = myFile.parseInt();
      data[3] = myFile.parseInt();
      data[4] = myFile.parseInt();
      data[5] = myFile.parseInt();
      data[6] = myFile.parseInt();
      data[7] = myFile.parseInt();
      data[8] = myFile.parseInt();
      data[9] = myFile.parseInt();
      Serial.print(myFile.parseInt());
    }

    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }

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
      saved [i][j] = -1;
    }
  }

  //test config
  for (int i = 0; i < NUMPARAMS; i++) {
    saved[15][i] = data[i];
    Serial.print ("saved: ");
    Serial.print (saved[16][i]);
  }
 
}

void loop() {

  //values
  modeVal = analogRead(MODESWITCH);


  consoleMode = digitalRead(CONPIN);

  if (consoleMode == 0) {
    
    //openSaveMode = true; 
    saveNow = true; 
    
    val = analogRead(SLIDER1);
    val2 = analogRead (SLIDER2);
    val3 = analogRead (SLIDER3);
    val4 = analogRead (SLIDER4);
    val5 = analogRead (SLIDER5);
    val6 = analogRead (SLIDER6);
    rateVal = analogRead(POT1);
    widthVal = analogRead (POT2);
    dirVal = analogRead(POT3);

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
    if (modeVal < 90) mode = 8;

    //map
    hueVal = map(val, 0, 920, 0, 255);
    hueVal = smooth(hueVal);
    saturation = map(val2, 0, 1023, 255, 0);
    //brightness = map(val3, 0, 1023, 255, 0);
    brightness = 255;

    for (uint8_t i = 0; i < numKeys; i++) {
      trellis.clrLED(i);
      trellis.writeDisplay();
    }

    switch (mode) {

      case 0:
        steady (hueVal, saturation, brightness);
        break;

      case 1:
        pulse (rateVal, hueVal, saturation);
        break;

      case 2:
        runAround (widthVal, rateVal, hueVal, saturation, brightness, offsetHue, offsetSat, offsetBright);
        break;

      case 3://beam
        beam (widthVal, dirVal, hueVal, saturation, brightness);
        break;

      case 4: //sparkle
        sparkle (widthVal, rateVal, hueVal, saturation, brightness, offsetHue, offsetSat, offsetBright);
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

    //+++ add a function here to save the parameters in an array
    int testArray[NUMPARAMS] = {4, 500, 500, 500, 100, 100, 500, 500, 500, 500};


    if (saveNow) {
      dialVal = -2; //dont blink anything
    }
    
    if (saveNow) Serial.println ("OPEN"); 
    else Serial.println ("NO"); 
    
    //removing this breaks the buttons. WHY???
    for (int i = 0; i < 5; i++) {
      if (i != dialVal) {
        trellis.clrLED(i);
        trellis.writeDisplay();
      }
    }


    for (int i = 0; i < NUMSPOTS; i++) {
      if (saved[i][0] != -1) {
        trellis.setLED(i);
        trellis.writeDisplay();
      }

    }
    

    /* //blinking
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
          Serial.println ("pressed");
          trellis.setLED(i);
          dialVal = i;
          if (openSaveMode) {
            //save something
            if (saved[dialVal][0] == -1) {
            Serial.println ("saving...");
            save (dialVal, mode, rateVal, widthVal, dirVal, hueVal, saturation, brightness, offsetHue, offsetSat, offsetBright);
            saveNow = false; 
            } else {
            saveNow = false; 
            }
          }
        }
        // if it was released, turn it off
        if (trellis.justReleased(i)) {
          Serial.println ("released");
          trellis.clrLED(i);
        }
      }
      // tell the trellis to set the LEDs we requested
  
      trellis.writeDisplay();
    }
    
    if (dialVal > -1 && dialVal < NUMSPOTS) curNum = dialVal; 
    
    for (int i = 0; i < NUMSPOTS; i++) {
      Serial.println (curNum); 
      if (curNum == i) {

        saveNow = false; 
        switch (saved[i][0]) {

          case 0:
            steady (saved[i][4], saved[i][5], saved[i][6]);
            break;

          case 1:
            pulse (saved[i][1], saved[i][4], saved[i][5]);
            break;

          case 2:
            runAround (saved[i][2], saved[i][1], saved[i][4], saved[i][5], saved[i][6], saved[i][7], saved[i][8], saved[i][9]);
            break;

          case 3://beam
            beam (saved[i][2], saved[i][3], saved[i][4], saved[i][5], saved[i][6]);
            break;

          case 4: //sparkle
            sparkle (saved[i][2], saved[i][1], saved[i][4], saved[i][5], saved[i][6], saved[i][7], saved[i][8], saved[i][9]);
            break;
        }
      }
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


//------------------save--------------------------------//

int save (int i, int m, int r, int w, int d, int h1, int s1, int b1, int h2, int s2, int b2) {
  saved[i][0] = m;
  saved[i][1] = r;
  saved[i][2] = w;
  saved[i][3] = d;
  saved[i][4] = h1;
  saved[i][5] = s1;
  saved[i][6] = b1;
  saved[i][7] = h2;
  saved[i][8] = s2;
  saved[i][9] = b2;
  
      for (int i = 0; i < NUMSPOTS; i++) {
        for (int j = 0; j < NUMPARAMS; j++) {
        Serial.print (String (saved[i][j])); 
        Serial.print (","); 
        }
        Serial.println(); 
      }
  
   Serial.println("Clearing data...");
   SD.remove("settings.txt");
   myFile = SD.open("settings.txt", FILE_WRITE);

   // if the file opened okay, write to it:
   if (myFile) {
     Serial.print("Writing to settings.txt...");
     myFile.print("S");
     /*
     for (int i = 0; i < NUMPARAMS; i++) {
     myFile.print(String(data[i]));
     myFile.print(",");
     }
     myFile.println("");
*/
      for (int i = 0; i < NUMSPOTS; i++) {
        for (int j = 0; j < NUMPARAMS; j++) {
        myFile.print (String (saved[i][j])); 
        myFile.print (","); 
        }
        myFile.println(); 
      }


     // close the file:
     myFile.close();
     Serial.println("done.");
   } else {
     // if the file didn't open, print an error:
     Serial.println("error opening settings.txt");
   }
  
  
  Serial.println ("saved");
}

//------------------helper ----------------------------//

//this should be a class so you can make separate functions for everything you use it for
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
