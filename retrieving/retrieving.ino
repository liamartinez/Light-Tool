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

//libraries
#include <FastLED.h>
#include <math.h>//for breathing
#include <Wire.h>
#include "Adafruit_Trellis.h"
#include <SD.h>
#include <SPI.h>

//trellis
Adafruit_Trellis matrix0 = Adafruit_Trellis();
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0);
#define NUMTRELLIS 1
#define numKeys (NUMTRELLIS * 16)

//led string
#define NUM_LEDS 12
#define DATA_PIN 2
#define CLOCK_PIN 3
CRGB leds[NUM_LEDS];

//consoleMode
#define CONPIN 22
int consoleMode; //0 is tweak, 1 is saved modes

//sensor pins
#define SLIDER1 A3 // hue
#define SLIDER2 A2 // saturation
#define SLIDER3 A0 //brightness
#define SLIDER4 A8 //offset hue
#define SLIDER5 A9 //offset brightness
#define SLIDER6 A10 //offset saturation
#define MODESWITCH A1 //dial
#define POT1 A12 //rate
#define POT2 A13 //width
#define POT3 A14 //direction

//sensor values
int dialVal;
int brightness, hueVal, saturation;
int offsetBright, offsetHue, offsetSat;
int rate, width, dir;
int offset; //for beam
int val, val2, val3, val4, val5, val6;
int rateVal, widthVal, dirVal;
int mode, modeVal;

//levels
#define MAXBRIGHT 255

//math
int breatheMath;
float breatheVal;

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
int curNum;
boolean saveNow;
int delNum, oldVal;
boolean deleteMode = false;

int counter;

void setup() {
  Serial.begin (9600);
  Serial.setTimeout(100);//can probably make this shorter

  initSD();
  openSD();

  trellis.begin(0x70);  // only one
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  delay( 2000 ); // power-up safety delay

  animateLights();
}

void loop() {
  modeVal = analogRead(MODESWITCH);
  consoleMode = digitalRead(CONPIN);
  if (consoleMode == 0) {
    tinkerMode(); 
  }
  else if (consoleMode == 1) {
    saveMode();
  }
  FastLED.show();
}



//----------------console modes -------------------------------//

void tinkerMode() {
   //openSaveMode = true;
    saveNow = true;
    deleteMode = false;

    val = analogRead(SLIDER1);
    val2 = analogRead (SLIDER2);
    val3 = analogRead (SLIDER3);
    val4 = analogRead (SLIDER4);
    val5 = analogRead (SLIDER5);
    val6 = analogRead (SLIDER6);
    rateVal = analogRead(POT1);
    widthVal = analogRead (POT2);
    dirVal = analogRead(POT3);

    offsetHue = map (val5, 0, 1023, 0, 255);//switched with bright for now
    
    //offsetBright = map(val5, 0, 1023, 0, 255);
    offsetBright = 0; 
    offsetSat = map (val6, 0, 1023, 255, 0);
    int pot1 = analogRead(POT1);
    //int mode = map(analogRead(MODESWITCH), 0, 1023, 8, 1);

  Serial.println (offsetSat); 

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
    brightness = map(val3, 0, 1023, 255, 0);
    //brightness = 255;

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
        runAround (widthVal, rateVal, hueVal, saturation, brightness);
        break;
        
       case 3:
        runAround2 (widthVal, rateVal, hueVal, saturation, brightness, offsetHue, offsetSat, offsetBright);

      case 4://beam
        beam (widthVal, dirVal, hueVal, saturation, brightness);
        break;

      case 5: //sparkle
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

//--------
void saveMode() {
  
  if (!deleteMode) {
    delay(30); // 30ms delay is required, dont remove me!
    if (saveNow) {
      dialVal = -2; //dont blink anything
    }
    
    for (int i = 0; i < NUMSPOTS; i++) {
        if (saved[i][0] != -1) {
          trellis.setLED(i);
          trellis.writeDisplay();
        }
      }
      
    if (trellis.readSwitches()) {
      // go through every button
      for (uint8_t i = 0; i < numKeys; i++) {
        // if it was pressed, turn it on
        if (trellis.justPressed(i)) {
          Serial.println ("pressed");
          trellis.setLED(i);
          Serial.println (i);
          dialVal = i;
          if (saveNow) {
            //save something
            if (saved[dialVal][0] == -1) {
              Serial.println ("saving...");
              save (dialVal, mode, rateVal, widthVal, dirVal, hueVal, saturation, brightness, offsetHue, offsetSat, offsetBright);
              saveNow = false;
            } else {
              saveNow = false;
            }
          }
          if (dialVal == 15) deleteMode = true;
        } else {
          trellis.clrLED(i);
        }
      }
      // tell the trellis to set the LEDs we requested

      trellis.writeDisplay();
    }
    if (dialVal > -1 && dialVal < NUMSPOTS) curNum = dialVal;

    for (int i = 0; i < NUMSPOTS; i++) {
      if (curNum == i) {

        //saveNow = false;
        switch (saved[i][0]) {

          case 0:
            steady (saved[i][4], saved[i][5], saved[i][6]);
            break;

          case 1:
            pulse (saved[i][1], saved[i][4], saved[i][5]);
            break;

          case 2:
            runAround (saved[i][2], saved[i][1], saved[i][4], saved[i][5], saved[i][6]);
            break;

          case 3:
            runAround2 (saved[i][2], saved[i][1], saved[i][4], saved[i][5], saved[i][6], saved[i][7], saved[i][8], saved[i][9]);

          case 4://beam
            beam (saved[i][2], saved[i][3], saved[i][4], saved[i][5], saved[i][6]);
            break;

          case 5: //sparkle
            sparkle (saved[i][2], saved[i][1], saved[i][4], saved[i][5], saved[i][6], saved[i][7], saved[i][8], saved[i][9]);
            break;
        }
      }
    }
  } else if (deleteMode) {
    delay(30); // 30ms delay is required, dont remove me!
    trellis.setLED(15);
    trellis.writeDisplay();
    delay (600);
    trellis.clrLED(15);
    trellis.writeDisplay();
    delay (600);

    if (trellis.readSwitches()) {
      for (uint8_t i = 0; i < numKeys; i++) {
        if (trellis.justPressed(i)) {
          curNum = i;
        }
      }
    }

    if (curNum != 15) {
      Serial.println ("OK NA");
      del (curNum);
    }
  }

}

//--------------- animation modes---------------------------------//

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

void runAround (int width_, int rate_, int h, int s, int b) {
  int w = map (width_, 1023, 0, 1, NUM_LEDS / 3);
  int r = map (rate_, 0, 1024, 10, 100);
  for (int i = 0; i < NUM_LEDS; i ++) {
    if (i < NUM_LEDS - w) {
      leds[i + w] = CHSV( h, s, b);
    } else {
      leds[i + w] = CHSV( h, s, b);
      leds[width - (NUM_LEDS - i)] = CHSV( h, s, b);
    }
    FastLED.show();
    delay(r);
    leds[i] = CRGB::Black;
  }
}

void runAround2 (int width_, int rate_, int h, int s, int b, int h2, int s2, int b2) {
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


//------------------lights ----------------------------//

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
    //myFile.print("S");
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
        if (j < NUMPARAMS - 1) myFile.print (",");
      }
      myFile.println("");
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

void del (int d) {
  for (int i = 0; i < NUMPARAMS; i++) {
    saved[d][i] = -1;
  }

  for (int i = 0; i < NUMSPOTS; i++) {
    if (saved[i][0] != -1) {
      trellis.setLED(i);
      trellis.writeDisplay();
    }

  }
  Serial.print (d);
  Serial.println (" is deleted.");

}

void initSD() {
  Serial.print("Initializing SD card...");
  pinMode(53, OUTPUT);
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
}

void openSD() {
  counter = 0;
  myFile = SD.open("settings.txt");
  if (myFile) {
    Serial.println("Opening settings.txt");
    while (myFile.available()) {
      //Serial.write(myFile.read());
      for (int i = 0; i < NUMPARAMS; i++) {
        saved[counter][i] = myFile.parseInt();
      }
      counter++;
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }

  for (int i = 0; i < NUMSPOTS; i++) {
    for (int j = 0; j < NUMPARAMS; j++) {
      Serial.println(saved [i][j]);
    }
  }
}

