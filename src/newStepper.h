#include<Arduino.h>
#include"pins_RAMPS.h"

// For RAMPS 1.4
#define X_STEP_HIGH             PORTF |=  0b01000001;
#define X_STEP_LOW              PORTF &= ~0b01000001;

//#define Y_STEP_HIGH             PORTF |=  0b01000000;
//#define Y_STEP_LOW              PORTF &= ~0b01000000;

#define Z_STEP_HIGH             PORTL |=  0b00001000;
#define Z_STEP_LOW              PORTL &= ~0b00001000;

#define E0_STEP_HIGH             PORTA |=  0b00010000;
#define E0_STEP_LOW              PORTA &= ~0b00010000;

#define TIMER3_INTERRUPTS_ON    TIMSK3 |=  (1 << OCIE3A);
#define TIMER3_INTERRUPTS_OFF   TIMSK3 &= ~(1 << OCIE3A);

#define TIMER4_INTERRUPTS_ON    TIMSK4 |=  (1 << OCIE4A);
#define TIMER4_INTERRUPTS_OFF   TIMSK4 &= ~(1 << OCIE4A);

#define TIMER5_INTERRUPTS_ON    TIMSK5 |=  (1 << OCIE5A);
#define TIMER5_INTERRUPTS_OFF   TIMSK5 &= ~(1 << OCIE5A);


void setupSteppers();
void prepareMovement(int whichMotor, int steps);
void runAndWait();
void runSteppers();
bool isRunning(int i);
int getMovementStep(int i);
void setInterval(int i, unsigned int minStepInterval);