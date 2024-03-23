#include <Arduino.h>
#include "pins_RAMPS.h"
#include <U8glib-HAL.h>

#include "newStepper.h"
#include "physical.h"

U8GLIB_ST7920_128X64_4X tela(LCD_PINS_SCK, LCD_PINS_MOSI, LCD_PINS_CS);

void updateScreen();
void updateEncoder();
int puxarPapel();
int estadoAtual = 0;
int selecao = 0;
bool advanceState;
bool backState;
bool lastAdvanceState;
bool lastBackState;

int index;

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
    0,  // 0000 //semMov
    -1, // 0001
    1,  // 0010
    0,  // 0011 //dois pulsos sem dir
    1,  // 0100
    0,  // 0101 //semMov
    -2, // 0110
    -1, // 0111
    -1, // 1000
    2,  // 1001
    0,  // 1010 //semMov
    1,  // 1011
    0,  // 1100 //dois pulsos sem dir
    1,  // 1101
    -1, // 1110
    0   // 1111 //semMov
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
  TCCR4A = 0;
  TCCR4B = 0;
  TCNT4 = 0;

  OCR4A = 64;

  TCCR4B |= (1 << WGM12);
  TCCR4B |= (1 << CS12) | (1 << CS10);
  TIMSK4 |= (1 << OCIE4A);
  sei();

  setupSteppers();
}

ISR(TIMER4_COMPA_vect)
{
  updateEncoder();
}

// big font u8g_font_10x20
void loop()
{
  loopTime = millis();
  updateScreen();
}

void updateEncoder()
{
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
  controlar
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
        tela.drawStr(0, 20, "Tempo de execussao: ");
        tela.drawStr(0, 30, String(endTime - startTime).c_str());
        if(running){

        }else{
          if(!digitalRead(X_MIN_PIN)){
            tela.drawStr(0, 40, "Papel preso!");
          }
          if(digitalRead(X_MAX_PIN)){
            tela.drawStr(0, 50, "Sem papel!");
          }
          if(digitalRead(X_MIN_PIN) && !digitalRead(X_MAX_PIN)){
            tela.drawStr(0, 10, "Maquina Pronta!");
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
        tela.drawStr(0, 10, "Configurar maquina");
        if (backState && !lastBackState){
          estadoAtual = menuPrincipal;
          counter = 0;
        }
        break;
      
      case controlar:
        index = abs(selecao) % 4;
        tela.drawStr(0, 10, "Controle manual");
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
            puxarPapel();
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
  prepareMovement(1, 1000);
  runAndWait();
  // desliga o puxador e separa o papel girando sem embreagem
  digitalWrite(MOSFET_C_PIN, LOW);
  digitalWrite(MOSFET_A_PIN, HIGH);

  // 4000 step
  prepareMovement(1, 4000);
  runSteppers();
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