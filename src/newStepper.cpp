#include <Arduino.h>
#include "pins_RAMPS.h"
#include "newStepper.h"

struct stepperInfo {
  // externally defined parameters
  float acceleration;
  volatile unsigned int minStepInterval;   // ie. max speed, smaller is faster
  void (*dirFunc)(int);
  void (*stepFunc)();


  long stepPosition;              // current position of stepper (total of all movements taken so far)

  // per movement variables (only changed once per movement)
  volatile int dir;                        // current direction of movement, used to keep track of position
  volatile unsigned int totalSteps;        // number of steps requested for current movement

  volatile unsigned short period; //inverse of frequency. how many times to run the timer to actually step. (period of 10 is 1/10 of the timer freq.)
  volatile unsigned short count; //how many timers have been run. 
  // per iteration variables (potentially changed every interrupt)
  volatile unsigned int stepCount;
  
};

void xStep() {
  X_STEP_HIGH
  X_STEP_LOW
}
void xDir(int dir) {
  digitalWrite(X_DIR_PIN, dir);
  digitalWrite(Y_DIR_PIN, dir);
}

/*
void yStep() {
  Y_STEP_HIGH
  Y_STEP_LOW
}
void yDir(int dir) {
  digitalWrite(Y_DIR_PIN, dir);
}
*/

void zStep() {
  Z_STEP_HIGH
  Z_STEP_LOW
}
void zDir(int dir) {
  digitalWrite(Z_DIR_PIN, dir);
}

void aStep() {
  E0_STEP_HIGH
  E0_STEP_LOW
}
void aDir(int dir) {
  digitalWrite(E0_DIR_PIN, dir);
}

#define NUM_STEPPERS 3

volatile stepperInfo steppers[NUM_STEPPERS];

void setupSteppers() {

  pinMode(X_STEP_PIN,   OUTPUT);
  pinMode(X_DIR_PIN,    OUTPUT);
  pinMode(X_ENABLE_PIN, OUTPUT);

  pinMode(Y_STEP_PIN,   OUTPUT);
  pinMode(Y_DIR_PIN,    OUTPUT);
  pinMode(Y_ENABLE_PIN, OUTPUT);

  pinMode(Z_STEP_PIN,   OUTPUT);
  pinMode(Z_DIR_PIN,    OUTPUT);
  pinMode(Z_ENABLE_PIN, OUTPUT);

  pinMode(E0_STEP_PIN,   OUTPUT);
  pinMode(E0_DIR_PIN,    OUTPUT);
  pinMode(E0_ENABLE_PIN, OUTPUT);

  digitalWrite(X_ENABLE_PIN, HIGH);
  digitalWrite(Y_ENABLE_PIN, HIGH);
  digitalWrite(Z_ENABLE_PIN, HIGH);
  digitalWrite(E0_ENABLE_PIN, HIGH);

  noInterrupts();
  TCCR3A = 0;
  TCCR3B = 0;
  TCNT3  = 0;

  OCR3A = 25;                             // compare value
  TCCR3B |= (1 << WGM12);                   // CTC mode
  TCCR3B |= ((1 << CS11) | (1 << CS10));    // 64 prescaler

  TCCR4A = 0;
  TCCR4B = 0;
  TCNT4  = 0;

  OCR4A = 25;                             // compare value
  TCCR4B |= (1 << WGM12);                   // CTC mode
  TCCR4B |= ((1 << CS11) | (1 << CS10));    // 64 prescaler

  TCCR5A = 0;
  TCCR5B = 0;
  TCNT5  = 0;

  OCR5A = 25;                             // compare value
  TCCR5B |= (1 << WGM12);                   // CTC mode
  TCCR5B |= ((1 << CS11) | (1 << CS10));    // 64 prescaler

  interrupts();
  TIMER3_INTERRUPTS_ON
  TIMER4_INTERRUPTS_OFF
  TIMER5_INTERRUPTS_OFF

  steppers[0].dirFunc = xDir;
  steppers[0].stepFunc = xStep;
  steppers[0].acceleration = 500;
  steppers[0].minStepInterval = 4;

  steppers[1].dirFunc = zDir;
  steppers[1].stepFunc = zStep;
  steppers[1].acceleration = 500;
  steppers[1].minStepInterval = 2;

  steppers[2].dirFunc = aDir;
  steppers[2].stepFunc = aStep;
  steppers[2].acceleration = 300;
  steppers[2].minStepInterval = 1;
}

volatile byte remainingSteppersFlag = 0;


void prepareMovement(int whichMotor, int steps) {
  volatile stepperInfo& si = steppers[whichMotor];
  si.dirFunc( steps < 0 ? HIGH : LOW );
  si.dir = steps > 0 ? 1 : -1;
  si.totalSteps = abs(steps);

  //reset stepper
  si.stepCount = 0;
  si.period = si.minStepInterval;
  remainingSteppersFlag |= (1 << whichMotor);
}

ISR(TIMER3_COMPA_vect)
{
  for (int i = 0; i < NUM_STEPPERS; i++) {
    //for each stepper:

    if (!((1 << i) & remainingSteppersFlag) )
      // currently not running
      continue;

    volatile stepperInfo& s = steppers[i];

    s.count ++;
    if(s.count >= s.period){
      s.count = 0;
      //actually step motor
      if ( s.stepCount < s.totalSteps ) {
        s.stepFunc();
        s.stepCount++;
        s.stepPosition += s.dir;
        if ( s.stepCount >= s.totalSteps ) {
          // done moving (reached steps)
          // set this stepper as not remaining
          remainingSteppersFlag &= ~(1 << i);
        }
      }
    }
  }
}

ISR(TIMER4_COMPA_vect){

}

ISR(TIMER5_COMPA_vect){

}


void runAndWait() {
  //setNextInterruptInterval();
  while ( remainingSteppersFlag );
}

void runSteppers() {
  //setNextInterruptInterval();
}

bool isRunning(int i) {
  return remainingSteppersFlag & (1<<i);
}
