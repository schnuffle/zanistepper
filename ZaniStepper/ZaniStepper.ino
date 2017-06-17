/********************************************************
** Download Library from:                              **
** http://www.alhin.de/arduino/index.php?n=48          **
********************************************************/

#include <AH_Pololu.h>

////////////////////////////////////////////////////////////////
// Button check adopted from
// https://blog.adafruit.com/2009/10/20/example-code-for-multi-button-checker-with-debouncing/
//

// written by marc Schaefer
// 24.06.2013
// v 0.2
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
const int MaxRPM = 15;
const int MinRPM = 2;
const int DeltaRPMS = 1;
const int IncrementWidth = 1;

// 1 = clock wise turn, 0 = counter clock wise
static boolean DIRECTION = 1;
static boolean AcDIR = 1;
// 1 = commit changes, 0 = no changes
static boolean doAction = 0;

// variable to store the actual speed
volatile int EncoderSpeedRPM = 0;
// variables to store the actual speed
volatile int ActualEncoderSpeedRPM = EncoderSpeedRPM;


// variable to store the press time for the Switch
static long ButtonPressedTime = 0;

// enabled/disabled state
// 1 = engine enabled, 0 = engine dead
static int active = 0;
// 1 = engine enabled, 0 = engine dead
static int ActualActive = 0;

#define DEBOUNCE 10  // button debouncer, how many ms to debounce, 5+ ms is usually plenty
// we will track if a button is just pressed, just released, or 'currently pressed' 
static byte pressed, justpressed, justreleased;



//AH_Pololu(int RES, int DIR, int STEP, int MS1, int MS2, int MS3, int SLEEP, int ENABLE, int RESET);
AH_Pololu stepper(200,DIR,STEP,MS1,MS2,MS3,SLP,ENABLE,RST);   // init with all functions


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

void check_switch()
{
  static byte previousstate;
  static byte currentstate;
  static long lasttime;

  if (!lasttime) {// we wrapped around, lets just try again
     lasttime = millis();
  }
  
  if ((lasttime + DEBOUNCE) > millis()) {
    // not enough time has passed to debounce
    //Serial.print("Not enough time has passed to debounce");
    return; 
  }
  // ok we have waited DEBOUNCE milliseconds, lets reset the timer
  lasttime = millis();
   
  justreleased = 0;                     // when we start, we clear out the "just" indicators
  // justpressed = 0;
  currentstate = digitalRead(Switch);   // read the button
             
  //Serial.print("cstate=");
  //Serial.print(currentstate, DEC);
  //Serial.print(", pstate=");
  //Serial.print(previousstate, DEC);
  //Serial.print(", press=");
    
    
  if (currentstate == previousstate) {
    if ((pressed == LOW) && (currentstate == LOW)) {
        // just pressed
        justpressed = 1;
    }
    else if ((pressed == HIGH) && (currentstate == HIGH)) {
        // just released
        justreleased = 1;
    }
    pressed = !currentstate;  // remember, digital HIGH means NOT pressed
  }
  //Serial.println(pressed, DEC);
  previousstate = currentstate;   // keep a running tally of the buttons
} // check_switches

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

void setNewSpeedDirectionActive() {
  static long dtimer;

  if (!dtimer) {// we wrapped around, lets just try again
     dtimer = millis();
  }
 
  if ( millis() - dtimer > 100) {
    //Serial.println(ActualActive); //," Active: ",active," ActualEncoderSpeedRPM: ",ActualEncoderSpeedRPM);
    //Serial.println(active);
    //Serial.println(ActualEncoderSpeedRPM);

    if ((ActualActive) && (active)) {
      if (DIRECTION==AcDIR) {        
        if (ActualEncoderSpeedRPM < EncoderSpeedRPM) {
          ActualEncoderSpeedRPM = ActualEncoderSpeedRPM + DeltaRPMS;
          stepper.setSpeedRPM(ActualEncoderSpeedRPM);
        } else if (ActualEncoderSpeedRPM > EncoderSpeedRPM) {
          ActualEncoderSpeedRPM = ActualEncoderSpeedRPM - DeltaRPMS;
          stepper.setSpeedRPM(ActualEncoderSpeedRPM);
        }
      } else {
        ActualEncoderSpeedRPM  = ActualEncoderSpeedRPM - DeltaRPMS;
        stepper.setSpeedRPM(ActualEncoderSpeedRPM);
        if (ActualEncoderSpeedRPM < MinRPM) {
          AcDIR=!AcDIR;
        }  
      }
    } 
    if ((ActualActive) && (!active)) {
      if (ActualEncoderSpeedRPM >  MinRPM) {
        ActualEncoderSpeedRPM = ActualEncoderSpeedRPM - DeltaRPMS;
        stepper.setSpeedRPM(ActualEncoderSpeedRPM);
      } else {
        ActualEncoderSpeedRPM = 0;
        ActualActive = 0;
        stepper.setSpeedRPM(ActualEncoderSpeedRPM);
        //stepper.sleepON();
      }    
    }
    if ((!ActualActive) && (active)) {
      ActualActive = 1;
      ActualEncoderSpeedRPM = MinRPM;
      // EncoderSpeedRPM = ActualEncoderSpeedRPM;
      stepper.setSpeedRPM(ActualEncoderSpeedRPM);   
      DIRECTION = AcDIR; 
      stepper.sleepOFF();
    }
    if ((!ActualActive) && (!active)) {
        ActualEncoderSpeedRPM = 0;
        stepper.setSpeedRPM(ActualEncoderSpeedRPM);
        stepper.sleepON();
    }
     dtimer = millis();
  }
 
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

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
} // EncoderAction




/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

void setup() {
 // Setup stepper driver
 stepper.resetDriver();                  // reset driver
 stepper.disableDriver();                // disbale driver
 stepper.setMicrostepping(4);            // 0 -> Full Step
                                         // 1 -> 1/2 microstepping
                                         // 2 -> 1/4 microstepping
                                         // 3 -> 1/8 microstepping
                                         // 4 -> 1/16 microstepping
 // set speed to medium speed
 //dtimer = 0;
 EncoderSpeedRPM = (MaxRPM+MinRPM)/2;
 ActualEncoderSpeedRPM = 0;
 stepper.setSpeedRPM(ActualEncoderSpeedRPM);   // set speed in RPM, rotations per minute
 /////////////////////////////////////////////////////////////////


 /////////////////////////////////////////////////////////////////
 // setup switch input
 // Make input & enable pull-up resistors on switch pins
  pinMode(Switch, INPUT);
  digitalWrite(Switch, HIGH);

 
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


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

void loop() {
  check_switch();      // when we check the switches we'll get the current state
  if (justpressed) {
    //Serial.println(" Just pressed"); 
    // remember, check_switches() will CLEAR the 'just pressed' flag
    ButtonPressedTime = millis();
    justpressed = 0;
  }
  if (justreleased) {
    //Serial.println(" Just released");
    // remember, check_switches() will CLEAR the 'just pressed' flag
    ButtonPressedTime = millis() - ButtonPressedTime;
    justreleased = 0;
    pressed = 0;
    doAction = 1;
    
  }
  if (pressed) {
    //Serial.println(" pressed");
    // is the button pressed down at this moment            
  }    

  if (doAction) {
    if (ButtonPressedTime > 500) { // long push -> change direction
      DIRECTION = !DIRECTION;
      //Serial.println("LongPush ");
    }
    else { // short push -> toggle enable/disable stepper
      active = !active;
      //Serial.println(active);
    }
    // Reset doAction as it has been done
    doAction = 0;
  } // doAction

  setNewSpeedDirectionActive();
  if (ActualActive) {
    stepper.move(1,AcDIR);
  }
}
