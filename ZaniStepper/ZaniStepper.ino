/********************************************************
** Download Library from:                              **
** http://www.alhin.de/arduino/index.php?n=48          **
********************************************************/

#include <AH_Pololu.h>


// written by marc Schaefer
// 24.06.2013
// v 0.1
// license is GPL v2 or later


// D2 -> EncoderChanA
// D3 -> Switch
// D4 -> EncoderChanB
// D5 -> DIR
// D6 -> STEP
// D7 -> SLP
// D8 -> RST
// D9 -> MS1
// D10-> MS2
// D11-> MS3
// D12-> ENABLE

// define PINs for layout
const int ChanAPin  = 2;
const int Switch    = 3;
const int ChanBPin  = 4;
const int DIR       = 5;
const int STEP      = 6;
const int SLP       = 7;
const int RST       = 8;
const int MS1       = 9;
const int MS2       = 10;
const int MS3       = 11;
const int ENABLE    = 12;



// define paramter for stepper speed
const int MaxRPM = 100;
const int MinRPM = 3;
const int IncrementWidth = 1;

// 1 = clock wise turn, 0 = counter clock wise
volatile boolean DIRECTION = 1;
// 1 = commit changes, 0 = no changes
volatile boolean doAction = 1;

// variable to store the actual speed and direction
volatile int EncoderSpeedRPM = 0;

// variable to store the press time for Swtich
volatile int ButtonPressedTime = 0;

// enabled/disabled state
// 1 = engine enbaled, 0 = engine dead
volatile int active = 0;

// variables to store the actual activr state and speed
int ActualEncoderSpeedRPM = EncoderSpeedRPM;

// 1 = engine enbaled, 0 = engine dead
int ActualActive = 0;

#define DEBOUNCE 10  // button debouncer, how many ms to debounce, 5+ ms is usually plenty

// here is where we define the buttons that we'll use. button "1" is the first, button "6" is the 6th, etc
byte buttons[] = {Switch}; // the analog 0-5 pins are also known as 14-19
// This handy macro lets us determine how big the array up above is, by checking the size
#define NUMBUTTONS sizeof(buttons)
// we will track if a button is just pressed, just released, or 'currently pressed' 
byte pressed[NUMBUTTONS], justpressed[NUMBUTTONS], justreleased[NUMBUTTONS];



//AH_Pololu(int RES, int DIR, int STEP, int MS1, int MS2, int MS3, int SLEEP, int ENABLE, int RESET);
AH_Pololu stepper(200,DIR,STEP,MS1,MS2,MS3,SLP,ENABLE,RST);   // init with all functions


void check_switches()
{
  static byte previousstate[NUMBUTTONS];
  static byte currentstate[NUMBUTTONS];
  static long lasttime;
  byte index;

  if (millis()) {// we wrapped around, lets just try again
     lasttime = millis();
  }
  
  if ((lasttime + DEBOUNCE) > millis()) {
    // not enough time has passed to debounce
    return; 
  }
  // ok we have waited DEBOUNCE milliseconds, lets reset the timer
  lasttime = millis();
  
  for (index = 0; index < NUMBUTTONS; index++) { // when we start, we clear out the "just" indicators
    justreleased[index] = 0;     
    currentstate[index] = digitalRead(buttons[index]);   // read the button
    
    /*     
    Serial.print(index, DEC);
    Serial.print(": cstate=");
    Serial.print(currentstate[index], DEC);
    Serial.print(", pstate=");
    Serial.print(previousstate[index], DEC);
    Serial.print(", press=");
    */
    
    if (currentstate[index] == previousstate[index]) {
      if ((pressed[index] == LOW) && (currentstate[index] == LOW)) {
          // just pressed
          justpressed[index] = 1;
      }
      else if ((pressed[index] == HIGH) && (currentstate[index] == HIGH)) {
          // just released
          justreleased[index] = 1;
      }
      pressed[index] = !currentstate[index];  // remember, digital HIGH means NOT pressed
    }
    //Serial.println(pressed[index], DEC);
    previousstate[index] = currentstate[index];   // keep a running tally of the buttons
  }
}


void EncoderAction() {
   if (digitalRead(ChanAPin) == HIGH) {   // found a low-to-high on channel A
   if (digitalRead(ChanBPin) == LOW) {  // check channel B to see which way
                                            // encoder is turning
     EncoderSpeedRPM = min(EncoderSpeedRPM + IncrementWidth, MaxRPM);         // CCW
   }
   else {
     EncoderSpeedRPM = max(EncoderSpeedRPM - IncrementWidth, MinRPM);         // CW
   }
 }
 else                                        // found a high-to-low on channel A
 {
   if (digitalRead(ChanBPin) == LOW) {   // check channel B to see which way
                                             // encoder is turning
      EncoderSpeedRPM = max(EncoderSpeedRPM - IncrementWidth, MinRPM);          // CW
   }
   else {
      EncoderSpeedRPM = min(EncoderSpeedRPM + IncrementWidth, MaxRPM);          // CCW
   }

 }
}


void setup() {
 /////////////////////////////////////////////////////////////////
 // Setup stepper driver
 stepper.resetDriver();                  // reset driver
 stepper.disableDriver();                // disbale driver
 stepper.setMicrostepping(2);            // 0 -> Full Step
                                         // 1 -> 1/2 microstepping
                                         // 2 -> 1/4 microstepping
                                         // 3 -> 1/8 microstepping
                                         // 4 -> 1/16 microstepping
 // set speed to medium speed
 EncoderSpeedRPM = (MaxRPM+MinRPM)/2;
 ActualEncoderSpeedRPM = EncoderSpeedRPM;
 stepper.setSpeedRPM(ActualEncoderSpeedRPM);   // set speed in RPM, rotations per minute
 /////////////////////////////////////////////////////////////////


 /////////////////////////////////////////////////////////////////
 // setup switch input
 byte i;
 // Make input & enable pull-up resistors on switch pins
 for (i=0; i< NUMBUTTONS; i++) {
    pinMode(buttons[i], INPUT);
    digitalWrite(buttons[i], HIGH);
 }
 //pinMode(Switch, INPUT);
 // enable pullup resitor on switch
 //digitalWrite(Switch, HIGH);
 // attach interrrupt 1 on pin 3 to button in digital port
 //attachInterrupt(1, ButtonAction, CHANGE);
 /////////////////////////////////////////////////////////////////

 /////////////////////////////////////////////////////////////////
 //setup Encoder
 // setup ChanA and ChanB input
 pinMode(ChanAPin, INPUT);
 pinMode(ChanBPin, INPUT);
 // enable pullup on ChanA and ChanB
 digitalWrite(ChanAPin, HIGH);
 digitalWrite(ChanBPin, HIGH);
 // attach interrrupt 0 on pin 2 to button in digital port
 attachInterrupt(0, EncoderAction, CHANGE);
 /////////////////////////////////////////////////////////////////


 //Serial.begin (9600);
 //Serial.println("start");                // a personal quirk
 //Serial.print("Active: ");
 //Serial.println(ActualActive);

 active = 0;
 ActualActive = 0;
 stepper.enableDriver();
 stepper.sleepON();
}

void loop() {
  doAction = 0;
  check_switches();      // when we check the switches we'll get the current state
  for (byte i = 0; i < NUMBUTTONS; i++) {
    if (justpressed[i]) {
      Serial.print(i, DEC);
      Serial.println(" Just pressed"); 
      // remember, check_switches() will CLEAR the 'just pressed' flag
      ButtonPressedTime = millis();
    }
    if (justreleased[i]) {
      Serial.print(i, DEC);
      Serial.println(" Just released");
      // remember, check_switches() will CLEAR the 'just pressed' flag
      ButtonPressedTime = millis() - ButtonPressedTime;
      doAction = 1;
    }
    if (pressed[i]) {
      Serial.print(i, DEC);
      Serial.println(" pressed");
      // is the button pressed down at this moment
      
      
    }  
  }  
  if (doAction) {
    if (ButtonPressedTime > 500) { // long push -> change direction
      DIRECTION = !DIRECTION;
      //Serial.println("LongPush ");
    }
    else { // short push toggle enable/disable stepper
      active = !active;
    }
  }
  
  if (ActualActive != active) {
    ActualActive = active;
    if (active) {
      stepper.sleepOFF();
    }
    else {
      stepper.sleepON();
    }
  }
  if (ActualEncoderSpeedRPM != EncoderSpeedRPM) {
    ActualEncoderSpeedRPM = EncoderSpeedRPM;
    //Serial.println(EncoderSpeedRPM);
    stepper.setSpeedRPM(EncoderSpeedRPM);
  }
  //stepper.rotate(360);
  stepper.move(1,DIRECTION);
}
