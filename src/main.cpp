#include <Arduino.h>
#include "pins_RAMPS.h"
#include <U8glib-HAL.h>

U8GLIB_ST7920_128X64_4X tela(LCD_PINS_SCK, LCD_PINS_MOSI, LCD_PINS_CS);

void updateScreen();
void updateEncoder();

unsigned long lastTime = 0;
unsigned long loopTime = 0;
int counter = 0;
byte encState = 0;
static int8_t lookup_table[] = {
  0,  //0000 //semMov
  -1,  //0001
  1,  //0010
  0,  //0011 //dois pulsos sem dir
  1,  //0100
  0,  //0101 //semMov
  -2,  //0110
  -1,  //0111
  -1,  //1000
  2,  //1001
  0,  //1010 //semMov
  1,  //1011
  0, //1100 //dois pulsos sem dir
  1,  //1101
  -1,  //1110
  0   //1111 //semMov
  };

void setup()
{
  tela.begin();
  Serial.begin(9600);
  // tela:
  pinMode(BEEPER_PIN, OUTPUT);
  pinMode(BTN_ENC, INPUT_PULLUP);
  pinMode(BTN_EN1, INPUT_PULLUP);
  pinMode(BTN_EN2, INPUT_PULLUP);
  pinMode(KILL_BTN_PIN, INPUT_PULLUP);

  // sensor
  pinMode(X_MIN_PIN, INPUT);
  pinMode(X_MAX_PIN, INPUT_PULLUP);

  // atuador
  pinMode(MOSFET_A_PIN, OUTPUT);
  pinMode(MOSFET_C_PIN, OUTPUT);
  digitalWrite(MOSFET_A_PIN, LOW);
  digitalWrite(MOSFET_C_PIN, LOW);

  pinMode(E0_ENABLE_PIN, OUTPUT);
  digitalWrite(E0_ENABLE_PIN, LOW);

  digitalWrite(BEEPER_PIN, HIGH);
  delay(100);
  digitalWrite(BEEPER_PIN, LOW);

  cli();
  TCCR4A = 0;
  TCCR4B = 0;
  TCNT4 = 0;

  OCR4A = 64;

  TCCR4B |= (1 << WGM12);
  TCCR4B |= (1 << CS12) | (1 << CS10);
  TIMSK4 |= (1 << OCIE4A);

  //timer 3 for steppers
  // Clear registers
  TCCR3A = 0;
  TCCR3B = 0;
  TCNT3 = 0;

  // 1000000 Hz (16000000/((1+1)*8))
  OCR3A = 1;
  // CTC
  TCCR3B |= (1 << WGM32);
  // Prescaler 8
  TCCR3B |= (1 << CS31);
  // Output Compare Match A Interrupt Enable
  TIMSK3 |= (1 << OCIE3A);
  sei();
}

ISR(TIMER3_COMPA_vect)
{
  
}

ISR(TIMER4_COMPA_vect)
{
  updateEncoder();
}

// big font u8g_font_10x20
void loop()
{
  loopTime = millis();

  digitalWrite(MOSFET_C_PIN, !digitalRead(BTN_ENC));
  digitalWrite(MOSFET_A_PIN, !digitalRead(KILL_BTN_PIN));
  updateScreen();
}

void updateEncoder(){
  //escrever no bit menos significativo
  encState = encState | digitalRead(BTN_EN2);
  // shift e escreve no menos sig.
  encState = (encState << 1) | digitalRead(BTN_EN1);

  //os dois bits mais sig. são da leitura anterior, então comparamos com a leitura atual para ver se deu update
  if((encState >> 2) != (encState & 0b11)){
    switch (lookup_table[encState])
    {
    case 1:
      counter++;
      break;

    case -1:
      counter--;
      break;
    }
    
  }
  //agora o futuro virou passado, faz o bitshift.
  encState = (encState & 0b11) << 1;
}

void updateScreen(){
  tela.firstPage();
  do{
    tela.setFont(u8g_font_5x8r);
    tela.drawStr(0, 20, "Bandeirinhas");
    tela.drawStr(0, 44, digitalRead(X_MAX_PIN) ? "off" : "on ");
    tela.drawStr(50, 44, digitalRead(X_MIN_PIN) ? "off" : "on ");
    tela.drawStr(50, 10, String(counter).c_str());
    if (!digitalRead(BTN_ENC))
    {
      tela.drawCircle(100, 50, 5);
    }
  } while (tela.nextPage());
}

class StepperDriver{
  //time unit is microseconds
  uint8_t dirPin = 0;
  uint8_t stepPin = 0;
  uint8_t enPin = 0;
  int freqTarget = 0;
  int currFreq = 0;
  int period = 0;
  int accel = 200;
  unsigned long currMicros = 0;
  unsigned long lastMicros = 0;

  StepperDriver(uint8_t _dirPin, uint8_t _stepPin, uint8_t _enPin){
    this->dirPin = _dirPin;
    this->stepPin = _stepPin;
    this->enPin = _enPin;
    pinMode(dirPin, OUTPUT);
    pinMode(stepPin, OUTPUT);
    pinMode(enPin, OUTPUT);
  }

  void update(){
    lastMicros = micros();
  }

  void setSpeed(int _frequency){
    this->freqTarget = _frequency;
  }

  void updateAccel(){
    int diff = freqTarget - currFreq;
    int incTime = (diff*1000000)/accel;
    int incAmount = diff/incTime;
    currFreq =+ incAmount;
    this->period = 1000000/currFreq;
  }
};

void updateSteppers(){

}