#include <Arduino.h>
#include "pins_RAMPS.h"
#include <U8glib-HAL.h>

#include "newStepper.h"
#include "physical.h"

U8GLIB_ST7920_128X64_4X tela(LCD_PINS_SCK, LCD_PINS_MOSI, LCD_PINS_CS);

void updateScreen();
int puxarPapel();

String errMsg = "_____";

int estadoAtual = 0;
int selecao = 0;
bool advanceState;
bool backState;
bool lastAdvanceState;
bool lastBackState;

int index;

unsigned int mmMade = 0;

int cutSize = 10000;

bool running = false;
unsigned long lastTime = 0;
unsigned long loopTime = 0;

unsigned long startTime;
unsigned long endTime;

int counter = 0;
byte encState = 0;

bool constantVel = false;

int larguraBandeirinha = 165;

#define displayMargin 15

static int8_t lookup_table[] = {
    0,  // 0000 //sem movivento
    -1, // 0001
    1,  // 0010
    0,  // 0011 //dois pulsos sem direção conhecida
    1,  // 0100
    0,  // 0101 //sem movivento
    -2, // 0110 // caso meio zuado, estamos ignorando no cod. final.
    -1, // 0111
    -1, // 1000
    2,  // 1001 // caso meio zuado, estamos ignorando no cod. final.
    0,  // 1010 //sem movivento
    1,  // 1011
    0,  // 1100 //dois pulsos sem direção conhecida
    1,  // 1101
    -1, // 1110
    0   // 1111 //sem movivento
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

  digitalWrite(BEEPER_PIN, HIGH);
  delay(100);
  digitalWrite(BEEPER_PIN, LOW);

  cli();
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;

  OCR2A = 64;

  TCCR2B |= (1 << WGM12);
  TCCR2B |= (1 << CS12) | (1 << CS10);
  TIMSK2 |= (1 << OCIE2A);
  sei();

  setupSteppers();
}


// big font u8g_font_10x20
void loop()
{
  loopTime = millis();
  updateScreen();
}

ISR(TIMER2_COMPA_vect)
{
  /**
   * baseado em ma tabela em que os bits são os valores do encoder.
   * a ordem de encState fica:
   * 0bAANN
   * em que A é antigo e N é novo.
   * e também fica
   * 0b2121
   * em que 2 e 1 são ospinos 1 e 2 do encoder.
   */
  // escrever no bit menos significativo
  encState = encState | digitalRead(BTN_EN2);
  // shift e escreve no menos sig.
  encState = (encState << 1) | digitalRead(BTN_EN1);

  // os dois bits mais sig. são da leitura anterior, então comparamos com a leitura atual para ver se deu update
  if ((encState >> 2) != (encState & 0b11)){
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
  // agora o futuro virou passado, faz o bitshift.
  encState = (encState & 0b11) << 1;
}

enum statesFirstMenu
{
  menuStatus,
  menuPrincipal,
  configurar,
  controlar,
  displayErro
};

void updateScreen()
{
  tela.firstPage();
  do
  {
    advanceState = !digitalRead(BTN_ENC);
    backState = !digitalRead(KILL_BTN_PIN);

    tela.setFont(u8g_font_ncenR08r);


    selecao = (counter / 4);

    switch(estadoAtual){
      case menuStatus:
        // desenho ambos menus
        if((endTime - startTime) > 0){
          tela.drawStr(0, 30, (String(((float)60000) / (float)(endTime - startTime)) + " b/min").c_str());
        }else{
          tela.drawStr(0, 25, "N/A b/min");
        }
        tela.drawFrame(0,0,128,14);

        if(running){
          //apenas se executando
          tela.drawStr(2, 10, "Executando...");
        }else{
          //apenas se IDLE
          if(!digitalRead(X_MIN_PIN)){
            tela.drawStr(2, 10, "Papel preso!");
          } else if(digitalRead(X_MAX_PIN)){
            tela.drawStr(2, 10, "Sem papel!");
          } else {
            tela.drawStr(2, 10, "Maquina Pronta!");
          }
        }
        if (advanceState && !lastAdvanceState){
          estadoAtual = menuPrincipal;
          counter = 0;
        }
        break;
      case menuPrincipal:
        index = abs(selecao) % 3;
        tela.drawStr(0, 10, "Menu Principal");
        tela.drawLine(0, 13, 128, 13);
        tela.drawStr(displayMargin, 30, "Configurar");
        tela.drawStr(displayMargin, 40, "Controlar");
        tela.drawStr(displayMargin, 50, running ? "Parar" : "Iniciar");
        tela.drawStr(0, 30 + index * 10, "=>");
        if (advanceState && !lastAdvanceState){
          switch (index){
          case 0:
            estadoAtual = configurar;
            counter = 0;
            break;
          case 1:
            estadoAtual = controlar;
            counter = 0;
            break;
          case 2:
            // codigo para iniciar/parar
            running = !running;
            break;
          }
        }
        if (backState && !lastBackState){
          estadoAtual = menuStatus;
          counter = 0;
        }
        break;
      
      case configurar:
        tela.drawStr(0, 10, "Configurar Maquina");
        tela.drawLine(0, 13, 128, 13);
        if (backState && !lastBackState){
          estadoAtual = menuPrincipal;
          counter = 0;
        }
        break;
      
      case controlar:
        index = abs(selecao) % 4;
        tela.drawStr(0, 10, "Controle Manual");
        tela.drawLine(0, 13, 128, 13);
        tela.drawStr(displayMargin, 30, "Limpar bomba");
        tela.drawStr(displayMargin, 40, "Puxar cola");
        tela.drawStr(displayMargin, 50, "Seq. de puxar papel");
        tela.drawStr(displayMargin, 60, "mover esteira");
        tela.drawStr(0, 30 + index * 10, "=>");
        if (advanceState && !lastAdvanceState){
          switch (index){
          case 0:
            prepareMovement(2, 10000);
            runSteppers();
            while(isRunning(2)){
              digitalWrite(E0_ENABLE_PIN, LOW);
            }
            digitalWrite(E0_ENABLE_PIN, HIGH);
            break;
          case 1:
            prepareMovement(2, -10000);
            runSteppers();
            while(isRunning(2)){
              digitalWrite(E0_ENABLE_PIN, LOW);
            }
            digitalWrite(E0_ENABLE_PIN, HIGH);
            break;
          case 2:
            startTime = millis();
            if(puxarPapel() != 0){
              estadoAtual = displayErro;
              errMsg = "ERRO!";
            }
            endTime = millis();
            break;
          case 3:
            digitalWrite(X_ENABLE_PIN, LOW);
            digitalWrite(Y_ENABLE_PIN, LOW);
            prepareMovement(0, steps_mm_esteira * larguraBandeirinha);
            runAndWait();
            digitalWrite(X_ENABLE_PIN, HIGH);
            digitalWrite(Y_ENABLE_PIN, HIGH);

            break;
          }
        }
        if (backState && !lastBackState){
          estadoAtual = menuPrincipal;
          counter = 0;
        }
        break;
      case displayErro:
        tela.drawFrame(0, 0, 128, 64);
        tela.drawStr(5, 10, "Parada de emergencia!");
        tela.drawLine(0, 13, 128, 13);

        tela.setCursorPos(1, 25);
        tela.print(errMsg.c_str());

        tela.drawBox(50, 50, 30, 10);
        tela.setColorIndex(0);
        tela.drawStr(57, 59, "OK");
        tela.setColorIndex(1);
        if ((backState && !lastBackState) || (advanceState && !lastAdvanceState)){
          estadoAtual = menuStatus;
        }
        break;
    }

    lastAdvanceState = advanceState;
    lastBackState = backState;
  } while (tela.nextPage());
}

int puxarPapel()
{
  if (digitalRead(X_MAX_PIN))
  {
    // se n tiver papel na entrada
    return -1;
  }
  if (!digitalRead(X_MIN_PIN))
  {
    // já tem papel na saída
    return 1;
  }

  //liga motor do puxador
  digitalWrite(Z_ENABLE_PIN, LOW);
  // liga o puxador e gira um pouco
  digitalWrite(MOSFET_C_PIN, HIGH);
  digitalWrite(MOSFET_A_PIN, HIGH);
  // 1000 steps
  delay(50);
  prepareMovement(1, 5000);
  runSteppers();
  delay(400);
  // desliga o puxador e separa o papel girando sem embreagem
  digitalWrite(MOSFET_C_PIN, LOW);
  digitalWrite(MOSFET_A_PIN, HIGH);

  //aguarda o sensor de papel dentro
  while (digitalRead(X_MIN_PIN))
  {
    if (!digitalRead(KILL_BTN_PIN))
    {
      digitalWrite(MOSFET_C_PIN, LOW);
      digitalWrite(MOSFET_A_PIN, LOW);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      return 2;
    }
    if (!isRunning(1))
    {
      digitalWrite(MOSFET_C_PIN, LOW);
      digitalWrite(MOSFET_A_PIN, LOW);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      return -1;
    }
  }
  // anda uma quantidade fixa até realmente sair
  // 7000 steps
  prepareMovement(1, steps_mm_puxador * larguraBandeirinha);
  runSteppers();
  delay(300);
  prepareMovement(0, steps_mm_esteira * larguraBandeirinha);
  digitalWrite(X_ENABLE_PIN, LOW);
  digitalWrite(Y_ENABLE_PIN, LOW);
  runSteppers();
  delay(800);
  digitalWrite(E0_ENABLE_PIN, LOW);
  prepareMovement(2, -2000);
  runSteppers();
  while(isRunning(2)){

  }
  prepareMovement(2, 300);
  runAndWait();
  digitalWrite(MOSFET_C_PIN, LOW);
  digitalWrite(MOSFET_A_PIN, LOW);

  digitalWrite(E0_ENABLE_PIN, HIGH);
  digitalWrite(Y_ENABLE_PIN, HIGH);
  digitalWrite(X_ENABLE_PIN, HIGH);
  digitalWrite(Z_ENABLE_PIN, HIGH);
  return 0;
}