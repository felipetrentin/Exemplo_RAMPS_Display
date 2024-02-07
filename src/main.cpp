#include <Arduino.h>
#include "pins_RAMPS.h"
#include <U8glib-HAL.h>

U8GLIB_ST7920_128X64_4X tela(LCD_PINS_SCK, LCD_PINS_MOSI, LCD_PINS_CS);

void setup(){
  tela.begin();
  pinMode(BTN_ENC, INPUT);
}

void loop(){
  tela.firstPage();
  do {
    tela.setFont(u8g_font_10x20);
    tela.drawStr(0,24,"Hello World!");
    if(digitalRead(BTN_ENC)){
      tela.drawCircle(10, 10, 5);
    }
  } while ( tela.nextPage() );
}
