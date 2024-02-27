#include <Arduino.h>
#include "pins_RAMPS.h"
#include <U8glib-HAL.h>

U8GLIB_ST7920_128X64_4X tela(LCD_PINS_SCK, LCD_PINS_MOSI, LCD_PINS_CS);

void setup(){
  tela.begin();
  Serial.begin(9600);
  pinMode(BEEPER_PIN, OUTPUT);
  pinMode(BTN_ENC, INPUT_PULLUP);
  pinMode(MOSFET_A_PIN, OUTPUT);
  pinMode(MOSFET_C_PIN, OUTPUT);
  pinMode(KILL_BTN_PIN, INPUT_PULLUP);
  digitalWrite(MOSFET_A_PIN, LOW);
  digitalWrite(MOSFET_C_PIN, LOW);
  pinMode(X_MIN_PIN, INPUT);
  pinMode(X_MAX_PIN, INPUT_PULLUP);
  tone(BEEPER_PIN, 440, 50);
  delay(50);
  tone(BEEPER_PIN, 440*2, 50);
  delay(50);
  tone(BEEPER_PIN, 440*3, 50);
}

// big font u8g_font_10x20
void loop(){
  tela.firstPage();
  do {
    tela.setFont(u8g_font_courB14r);
    tela.drawStr(0,20,"Bandeirnhas");
    tela.drawStr(0,44,digitalRead(X_MAX_PIN)  ? "off" : "on ");
    tela.drawStr(50,44,digitalRead(X_MIN_PIN) ? "off" : "on ");
    if(!digitalRead(BTN_ENC)){
      tela.drawCircle(100, 50, 5);
    }
  } while ( tela.nextPage() );
  digitalWrite(MOSFET_C_PIN, !digitalRead(BTN_ENC));
  digitalWrite(MOSFET_A_PIN, !digitalRead(KILL_BTN_PIN));
}
