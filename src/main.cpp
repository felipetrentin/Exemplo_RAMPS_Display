#include <Arduino.h>
#include "pins_RAMPS.h"
#include <U8glib-HAL.h>

U8GLIB_ST7920_128X64_4X tela(LCD_PINS_SCK, LCD_PINS_MOSI, LCD_PINS_CS);

void setup()
{
  tela.begin();
  Serial.begin(9600);
  // tela:
  pinMode(BEEPER_PIN, OUTPUT);
  pinMode(BTN_ENC, INPUT_PULLUP);
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
  pinMode(E0_DIR_PIN, OUTPUT);
  pinMode(E0_STEP_PIN, OUTPUT);

  digitalWrite(BEEPER_PIN, HIGH);
  delay(100);
  digitalWrite(BEEPER_PIN, LOW);

  cli();
  TCCR4A = 0;
  TCCR4B = 0;
  TCNT4 = 0;

  OCR4A = 15624;

  TCCR4B |= (1 << WGM12);
  TCCR4B |= (1 << CS12) | (1 << CS10);
  TIMSK4 |= (1 << OCIE4A);
  sei();

  digitalWrite(E0_ENABLE_PIN, LOW);
}

bool toggle = false;
ISR(TIMER4_COMPA_vect)
{
  digitalWrite(E0_STEP_PIN, toggle);
  toggle = !toggle;
}

unsigned long last_run = 0;
int counter = 0;

void shaft_moved()
{
  if (millis() - last_run > 5)
  {
    if (digitalRead(4) == 1)
    {
      if (counter < 9)
        counter++;
    }
    if (digitalRead(4) == 0)
    {
      if (counter > 0)
        counter--;
    }
    last_run = millis();
  }
}

// big font u8g_font_10x20
void loop()
{
  tela.firstPage();
  do
  {
    tela.setFont(u8g_font_courB14r);
    tela.drawStr(0, 20, "Bandeirnhas");
    tela.drawStr(0, 44, digitalRead(X_MAX_PIN) ? "off" : "on ");
    tela.drawStr(50, 44, digitalRead(X_MIN_PIN) ? "off" : "on ");
    if (!digitalRead(BTN_ENC))
    {
      tela.drawCircle(100, 50, 5);
    }
  } while (tela.nextPage());
  digitalWrite(MOSFET_C_PIN, !digitalRead(BTN_ENC));
  digitalWrite(MOSFET_A_PIN, !digitalRead(KILL_BTN_PIN));
}
