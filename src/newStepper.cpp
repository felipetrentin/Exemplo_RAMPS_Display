#include <Arduino.h>
#include "pins_RAMPS.h"
#include "newStepper.h"

struct stepperInfo {
  // externally defined parameters
  volatile unsigned int minStepInterval;   // ie. max speed, smaller is faster
  void (*dirFunc)(int);
  void (*stepFunc)();


  long stepPosition;              // current position of stepper (total of all movements taken so far)

  // per movement variables (only changed once per movement)
  volatile int dir;                        // current direction of movement, used to keep track of position
  volatile unsigned int totalSteps;        // number of steps requested for current movement

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


  TIMER3_INTERRUPTS_OFF
  TIMER4_INTERRUPTS_OFF
  TIMER5_INTERRUPTS_OFF

  //esteira
  steppers[0].dirFunc = xDir;
  steppers[0].stepFunc = xStep;
  steppers[0].minStepInterval = 90;

  //puxador
  steppers[1].dirFunc = zDir;
  steppers[1].stepFunc = zStep;
  steppers[1].minStepInterval = 60;

  //bomba
  steppers[2].dirFunc = aDir;
  steppers[2].stepFunc = aStep;
  steppers[2].minStepInterval = 70;
}

volatile byte remainingSteppersFlag = 0;


void prepareMovement(int whichMotor, int steps) {
  volatile stepperInfo& si = steppers[whichMotor];
  si.dirFunc( steps < 0 ? HIGH : LOW );
  si.dir = steps > 0 ? 1 : -1;
  si.totalSteps = abs(steps);

  //reset stepper
  si.stepCount = 0;

  //set as running
  remainingSteppersFlag |= (1 << whichMotor);

  switch (whichMotor)
  {
  case 0:
    TCNT3 = 0;
    OCR3A = si.minStepInterval;  
    break;
  case 1:
    TCNT4 = 0;
    OCR4A = si.minStepInterval;  
    break;
  case 2:
    TCNT5 = 0;
    OCR5A = si.minStepInterval;  
    break;
  }
}

ISR(TIMER3_COMPA_vect)
{
  volatile stepperInfo& s = steppers[0];
  if ( s.stepCount < s.totalSteps ) {
    s.stepFunc();
    s.stepCount++;
    s.stepPosition += s.dir;
    if ( s.stepCount >= s.totalSteps ) {
      // done moving (reached steps)
      // set this stepper as not remaining
      remainingSteppersFlag &= ~(1);
      TIMER3_INTERRUPTS_OFF
    }
  }
}

ISR(TIMER4_COMPA_vect){
  volatile stepperInfo& s = steppers[1];
  if ( s.stepCount < s.totalSteps ) {
    s.stepFunc();
    s.stepCount++;
    s.stepPosition += s.dir;
    if ( s.stepCount >= s.totalSteps ) {
      // done moving (reached steps)
      // set this stepper as not remaining
      remainingSteppersFlag &= ~(1 << 1);
      TIMER4_INTERRUPTS_OFF
    }
  }
}

ISR(TIMER5_COMPA_vect){
  volatile stepperInfo& s = steppers[2];
  if ( s.stepCount < s.totalSteps ) {
    s.stepFunc();
    s.stepCount++;
    s.stepPosition += s.dir;
    if ( s.stepCount >= s.totalSteps ) {
      // done moving (reached steps)
      // set this stepper as not remaining
      remainingSteppersFlag &= ~(1 << 2);
      TIMER5_INTERRUPTS_OFF
    }
  }
}


void runAndWait() {
  runSteppers();
  //setNextInterruptInterval();
  while ( remainingSteppersFlag );
}

void runSteppers() {
  TIMER3_INTERRUPTS_ON
  TIMER4_INTERRUPTS_ON
  TIMER5_INTERRUPTS_ON
}

int getMovementStep(int i){
  volatile stepperInfo& s = steppers[i];
  return s.stepCount;
}

bool isRunning(int i) {
  return remainingSteppersFlag & (1<<i);
}

void setInterval(int i, unsigned int minStepInterval){
  steppers[i].minStepInterval = minStepInterval;
}