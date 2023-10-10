#include <ArduinoJson.h>
#include <Arduino.h> 
#include <DueTimer.h>  
#include <XYscope.h>

// -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~ Button code Begin -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~
#define BTN23_PIN 23
#define BTN25_PIN 25
#define BTN27_PIN 27
#define BTN29_PIN 29

// DEBOUCE STUFF
// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 1000;    // the debounce time; increase if the output flickers
// -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~ Button code End -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~

// -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~ XYscope code Begin -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~
XYscope XYscope;

void DACC_Handler(void) {
  //	DACC_Handler Interrupt Service Routine. This routine
  //	provides a 'wrapper' to link the AVR DAC INTERRUPT
  //	to the 'XYscope-class' ISR routine.
  XYscope.dacHandler();  //Link the AVR DAC ISR/IRQ to the XYscope.
                         //It is called whenever the DMA controller
                         //'DAC_ready_for_More_data' event occurs
}

void paintCrt_ISR(void) {
  //	paintCrtISR  Interrupt Service Routine. This routine
  //	provides a 'wrapper' to link the Timer3.AttachedInterrupt()
  //	function to the 'XYscope-class' ISR routine.
  XYscope.initiateDacDma();  //Start the DMA transfer to paint the CRT screen
}

//	Define/initialize critical global constants and variables

double dacClkRateHz, dacClkRateKHz;

int EndOfSetup_Ptr;
double TimeForRefresh;


char shiftVal = 0;
uint32_t dispTimer = 0;
int MovingX = 2048, MovingY = 2048;


float angle = 0;

#include <malloc.h>  //Required for RAM usage monitoring routines

// The following lines are used for RAM monitoring routines
extern char _end;
extern "C" char *sbrk(int i);
char *ramstart = (char *)0x20070000;
char *ramend = (char *)0x20088000;
// -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~ XYscope code End -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~




StaticJsonDocument<500> doc;

const byte numChars = 128;
char receivedChars[numChars];

boolean newData = false;

void setup() {
  Serial.begin(9600);
  Serial.println("<Arduino is ready>");
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);

// Button Setup
pinMode(BTN23_PIN, INPUT); 
pinMode(BTN25_PIN, INPUT); 
pinMode(BTN27_PIN, INPUT); 
pinMode(BTN29_PIN, INPUT); 




// -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~ XYscope code Begin -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~
  double DmaFreq = 800000;  //800000 (Hz) is the default startup value for the DMA frequency.
                            //You can try various alternate values to find an optimal
                            //value that works best for your scope, setup, & application.
                            //Use program menu option "c" & "C" to vary the frequency while
                            //watching your scope display; Using "c" & "C" options will allow you
                            //to see how frequency changes effect the display quality in real-time.

  XYscope.begin(DmaFreq);

  //Timer3 is used as the CRT refresh timer.  This timer is setup inside of XYscope.begin( ).
  //However, paintCRT_ISR must be "attached" to timer 3.  To be properly link to the
  //refresh-screen XYscope interupt service routine, it must be linked in the Arduino
  //setup() code as follows:

  Timer3.attachInterrupt(paintCrt_ISR);
// -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~ XYscope code End -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~


}

void loop() {
  digitalWrite(5, HIGH);
  recvWithStartEndMarkers();
  showNewData();

// -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~ Button code Begin -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~

  if(buttonPressed(BTN23_PIN)) {
    Serial.print("Button 2 pressed!");
  }
  if(buttonPressed(BTN25_PIN)) {
    Serial.print("Button 3 pressed!");
  }
    if(buttonPressed(BTN27_PIN)) {
    Serial.print("Button 4 pressed!");
  }
    if(buttonPressed(BTN29_PIN)) {
    Serial.print("Button 5 pressed!");
  }
  delay(100);
// -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~ Button code End -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~
}

void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;

  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();

    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      } else {
        receivedChars[ndx] = '\0';  // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }

    else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}

void showNewData() {
  if (newData == true) {
    Serial.print("(receivedChars = ");
    delay(500);
    Serial.println(receivedChars);

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, receivedChars);

    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      digitalWrite(6, HIGH);
      delay(500);
      return;
    }

    const char* display = doc["display"];
    const char* line1 = doc["line1"];
    const char* line2 = doc["line2"];
    const char* line3 = doc["line3"];
    const char* line4 = doc["line4"];

    Serial.println(line1);
    Serial.println(line2);
    Serial.println(line3);
    Serial.println(line4);


    
    // TEST STRING    <{'display': 'True', 'line1': 'OPTION1', 'line2': 'Option2', 'line3': 'namehereOption3', 'line4': 'namehereOption4'}>

    // If TEST STRING is too long, it will cut off end of srtring and cause "deserializeJson() failed: " error!

    if (strcmp(display,"True")==0){
      XYscope.plotClear();
      XYscope.printSetup(700, 700, 200, 100);
      XYscope.print((char *)line1, false);

      XYscope.printSetup(700, 500, 200, 100);
      XYscope.print((char *)line2, false);

      XYscope.printSetup(700, 300, 200, 100);
      XYscope.print((char *)line3, false);

      XYscope.printSetup(700, 100, 200, 100);
      XYscope.print((char *)line4, false);

      newData = false;
    }
    
    // This is probably not needed. The screen does not need to be cleared by a seperate function before displaying something new b/c XYscope.plotClear() is now run at the beginning of "display == True"

    if (strcmp(display,"False")==0){
      digitalWrite(7, HIGH);
      XYscope.plotClear();
      delay(1000);
      digitalWrite(7, LOW);
      newData = false;
    }

    
  }
}

// -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~ Button code Begin -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~
// Frankenstein debounce function. Not sure how all this works, or enen if it is all needed to work, but it works.
int buttonPressed(uint8_t button) {
  static uint16_t lastStates = 0;
  uint8_t state = digitalRead(button);
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (state != ((lastStates >> button) & 1)) {
      lastStates ^= 1 << button;
      lastDebounceTime = millis();
      return state == HIGH;
    }
  }
  return false;
}


// // Working, non-debouced code
// // Generic function to check if a button is pressed
// int buttonPressed(uint8_t button) {
//   static uint16_t lastStates = 0;
//   uint8_t state = digitalRead(button);
//   if (state != ((lastStates >> button) & 1)) {
//     lastStates ^= 1 << button;
//     return state == HIGH;
//   }
//   return false;
// }
// -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~ Button code End -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~

