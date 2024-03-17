#include<Arduino.h>
#include<U8glib-HAL.h>

void changeVarFloat(U8GLIB* scr, float* value, float increment, String title){
    scr->setFont(u8g_font_courB10);
    scr->drawStr(0, 20, title.c_str());
}

