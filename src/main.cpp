#include <Arduino.h>
#include <EEPROM.h>
#include <U8glib-HAL.h>

#include "pins_RAMPS.h"
#include "newStepper.h"
#include "physical.h"


#define screenRefreshMS 70

PROGMEM const uint8_t lockimg[] ={
  B11100000,B00111111,
  B11110000,B01111111,
  B01110000,B01110000,
  B00110000,B01100000,
  B00110000,B01100000,
  B00110000,B01100000,
  B11111000,B11111111,
  B11111000,B11111000,
  B11111000,B11111000,
  B11111000,B11111101,
  B11111000,B11111101,
  B11111000,B11111111,
  B11110000,B01111111
};

struct params{
  int larguraBandeirinha = 165;
  int distanciaEntre = 300;
  int velPuxador = 60;
  int velEsteira = 90;
  int velbomba = 150;
  int velFeed = 50;
  int uLbandeirinha = 30;
  int distCorte = 100;
} parameters;

U8GLIB_ST7920_128X64_4X tela(LCD_PINS_SCK, LCD_PINS_MOSI, LCD_PINS_CS);

void updateScreen();
int puxarPapel();
void tryScreenUpdate();

int returnState;

String errMsg = "_____";

enum statesFirstMenu
{
  menuStatus,
  menuPrincipal,
  configurar,
  configurarVelocidade,
  configurarTamanho,
  controlar,
  displayErro,
  editVal,
  
};

String editValName;
int* editValPtr;
int lastEditVal;

int estadoAtual = 0;
int selecao = 0;
bool advanceState;
bool backState;
bool lastAdvanceState;
bool lastBackState;

int index;

unsigned int mmMade = 0;

bool running = false;

unsigned long startTime;
unsigned long endTime;

long counter = 0;
byte encState = 0;

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
/**
 * return -1 = falha
 * return  1 = erro de recurso
 * return  2 = interrupted
 * return  0 = ok
 */
String errorMsgs[] = {
  "Falha ao executar.",
  "Sem recursos disponiveis",
  "Interrompido pelo usuário"
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

  if(!(digitalRead(KILL_BTN_PIN) || digitalRead(BTN_ENC))){
    eeprom_write_block(&parameters, 0, sizeof(parameters));
    digitalWrite(BEEPER_PIN, HIGH);
    delay(1000);
    digitalWrite(BEEPER_PIN, LOW);
    while(true){
      tela.firstPage();
      do{
        tela.setFont(u8g_font_ncenR08r);
        tela.drawStr(0, 10, "PADRAO RESTAURADO");
        tela.drawLine(0, 14, 128, 14);
        tela.drawStr(0, 30, "reinicie para continuar.");
      } while (tela.nextPage());
    }
  }

  // carrega os parametros
  eeprom_read_block(&parameters, 0, sizeof(parameters));
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

  //eeprom_read_block(&parameters, 0, sizeof(parameters));

}


// big font u8g_font_10x20
void loop()
{
  if(running){
    startTime = millis();
    returnState = puxarPapel();
    if(!running){
      digitalWrite(X_ENABLE_PIN, HIGH);
      digitalWrite(Y_ENABLE_PIN, HIGH);
      digitalWrite(E0_ENABLE_PIN, HIGH);
    }
    if(returnState != -1){
      estadoAtual = displayErro;
      errMsg = errorMsgs[returnState];
      digitalWrite(X_ENABLE_PIN, HIGH);
      digitalWrite(Y_ENABLE_PIN, HIGH);
      digitalWrite(E0_ENABLE_PIN, HIGH);
      running = false;
    }
    endTime = millis();
  }
  tryScreenUpdate();
}
unsigned long updateTime;
unsigned long lastUpdateTime;

void tryScreenUpdate(){
  updateTime = millis();
  if(updateTime >= lastUpdateTime + screenRefreshMS){
    lastUpdateTime = updateTime;
    updateScreen();
  }
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
        index = abs(selecao) % (running ? 2 : 3);
        tela.drawStr(0, 10, "Configurar Maquina");
        tela.drawLine(0, 14, 128, 14);
        tela.drawStr(displayMargin, 30, "Config. Distancias");
        tela.drawStr(displayMargin, 40, "Config. Velocidades");
        if(running){
          tela.drawXBMP(110, 0, 16, 13, lockimg);
        }else{
          tela.drawStr(displayMargin, 50, "Salvar Configuracoes");
        }
        tela.drawStr(0, 30 + index * 10, "=>");
        if(advanceState && !lastAdvanceState){
          counter = 0;
          switch (index)
          {
          case 0:
            estadoAtual = configurarTamanho;
            break;
          case 1:
            estadoAtual = configurarVelocidade;
            break;
          case 2:
            digitalWrite(BEEPER_PIN, HIGH);
            eeprom_write_block(&parameters, 0, sizeof(parameters));
            delay(50);
            digitalWrite(BEEPER_PIN, LOW);
            break;
          }
        }
        if (backState && !lastBackState){
          estadoAtual = menuPrincipal;
          counter = 0;
        }
        break;
      case configurarTamanho:
        tela.drawStr(0, 10, "Configurar Distancias");
        tela.drawLine(0, 14, 128, 14);

        tela.drawStr(displayMargin - 5, 30, "largura: [mm]");
        tela.drawStr(displayMargin + 80, 30, String(parameters.larguraBandeirinha).c_str());

        tela.drawStr(displayMargin - 5, 40, "dist. entre: [mm]");
        tela.drawStr(displayMargin + 80, 40, String(parameters.distanciaEntre).c_str());

        tela.drawStr(displayMargin - 5, 50, "cola: [uL]");
        tela.drawStr(displayMargin + 80, 50, String(parameters.uLbandeirinha).c_str());

        tela.drawStr(displayMargin - 5, 60, "dist. corte [cm]:");
        tela.drawStr(displayMargin + 80, 60, String(parameters.distCorte).c_str());

        if(!running){
          index = abs(selecao) % 4;
          tela.drawStr(0, 30 + index * 10, ">");
          if(advanceState && !lastAdvanceState){
            //editar valores
            switch (index)
            {
            case 0:
              editValPtr = &parameters.larguraBandeirinha;
              editValName = "largura da bandeirinha";
              break;
            case 1:
              editValPtr = &parameters.distanciaEntre;
              editValName = "dist. entre bandeiras";
              break;
            case 2:
              editValPtr = &parameters.uLbandeirinha;
              editValName = "quantidade de cola";
              break;
            case 3:
              editValPtr = &parameters.distCorte;
              editValName = "distancia do corte";
              break;
            }
            lastEditVal = *editValPtr;
            counter = *editValPtr * 4;
            estadoAtual = editVal;
          }
        }else{
          tela.drawXBMP(110, 0, 16, 13, lockimg);
        }
        if (backState && !lastBackState){
          estadoAtual = configurar;
          counter = 0;
        }
        break;
      
      case configurarVelocidade:
        tela.drawStr(0, 10, "Configurar Distancias");
        tela.drawLine(0, 14, 128, 14);

        tela.drawStr(displayMargin - 5, 30, "vel. cola:");
        tela.drawStr(displayMargin + 80, 30, String(parameters.velbomba).c_str());

        tela.drawStr(displayMargin - 5, 40, "vel. puxador");
        tela.drawStr(displayMargin + 80, 40, String(parameters.velPuxador).c_str());

        tela.drawStr(displayMargin - 5, 50, "vel. esteira");
        tela.drawStr(displayMargin + 80, 50, String(parameters.velEsteira).c_str());

        tela.drawStr(displayMargin - 5, 60, "vel. avanco");
        tela.drawStr(displayMargin + 80, 60, String(parameters.velFeed).c_str());

        if(!running){
          index = abs(selecao) % 4;
          tela.drawStr(0, 30 + index * 10, ">");
          if(advanceState && !lastAdvanceState){
            //editar valores
            switch (index)
            {
            case 0:
              editValPtr = &parameters.velbomba;
              editValName = "velocidade cola";
              break;
            case 1:
              editValPtr = &parameters.velPuxador;
              editValName = "velocidade puxador";
              break;
            case 2:
              editValPtr = &parameters.velEsteira;
              editValName = "velocidade esteira";
              break;
            case 3:
              editValPtr = &parameters.velFeed;
              editValName = "velocidade avanco";
              break;
            }
            lastEditVal = *editValPtr;
            counter = *editValPtr * 4;
            estadoAtual = editVal;
          }
        }else{
          tela.drawXBMP(110, 0, 16, 13, lockimg);
        }
        if (backState && !lastBackState){
          estadoAtual = configurar;
          counter = 0;
        }
        break;

      case controlar:
        index = abs(selecao) % (running ? 2 : 4);
        tela.drawStr(0, 10, "Controle Manual");
        tela.drawLine(0, 13, 128, 13);
        tela.drawStr(displayMargin, 30, "Limpar bomba");
        tela.drawStr(displayMargin, 40, "Puxar cola");
        if(!running){
          tela.drawStr(displayMargin, 50, "Seq. de puxar papel");
          tela.drawStr(displayMargin, 60, "mover esteira");
        }
        tela.drawStr(0, 30 + index * 10, "=>");
        if (advanceState && !lastAdvanceState){
          switch (index){
          case 0:
            setInterval(2, 50);
            prepareMovement(2, 10000);
            runSteppers();
            while(isRunning(2)){
              digitalWrite(E0_ENABLE_PIN, LOW);
            }
            digitalWrite(E0_ENABLE_PIN, HIGH);
            break;
          case 1:
            setInterval(2, 100);
            prepareMovement(2, -10000);
            runSteppers();
            while(isRunning(2)){
              digitalWrite(E0_ENABLE_PIN, LOW);
            }
            digitalWrite(E0_ENABLE_PIN, HIGH);
            break;
          case 2:
            startTime = millis();
            returnState = puxarPapel();
            if(returnState != -1){
              estadoAtual = displayErro;
              errMsg = errorMsgs[returnState];
            }
            runAndWait();
            digitalWrite(X_ENABLE_PIN, HIGH);
            digitalWrite(Y_ENABLE_PIN, HIGH);
            digitalWrite(E0_ENABLE_PIN, HIGH);
            endTime = millis();
            break;
          case 3:
            digitalWrite(X_ENABLE_PIN, LOW);
            digitalWrite(Y_ENABLE_PIN, LOW);
            prepareMovement(0, 1000);
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

        tela.drawStr(3, 25, errMsg.c_str());

        tela.drawBox(50, 50, 30, 10);
        tela.setColorIndex(0);
        tela.drawStr(57, 59, "OK");
        tela.setColorIndex(1);
        if ((backState && !lastBackState) || (advanceState && !lastAdvanceState)){
          estadoAtual = menuStatus;
        }
        break;
      
      case editVal:
        tela.drawStr(0, 10, editValName.c_str());
        tela.drawLine(0, 13, 128, 13);
        *editValPtr = counter/4;
        tela.drawStr(0, 40, String(*editValPtr).c_str());
        if((backState && !lastBackState)){
          *editValPtr = lastEditVal;
          estadoAtual = configurar;
          counter = 0;
        }
        if((advanceState && !lastAdvanceState)){
          estadoAtual = configurar;
          counter = 0;
        }
        break;
    }

    lastAdvanceState = advanceState;
    lastBackState = backState;
  } while (tela.nextPage());
}


/**
 * return  0 = falha
 * return  1 = erro de recurso
 * return  2 = interrupted
 * return  -1 = ok
 */
int puxarPapel()
{
  if (digitalRead(X_MAX_PIN))
  {
    // se n tiver papel na entrada
    return 1;
  }
  if (!digitalRead(X_MIN_PIN))
  {
    // já tem papel na saída
    return 0;
  }
  // ################## INICIO PUXAR PAPEL ##################
  //normal operation
  setInterval(0, parameters.velEsteira);
  setInterval(1, parameters.velPuxador);
  setInterval(2, parameters.velbomba);


  //liga motor do puxador
  digitalWrite(Z_ENABLE_PIN, LOW);

  // liga o puxador e gira um pouco
  digitalWrite(MOSFET_C_PIN, HIGH);
  digitalWrite(MOSFET_A_PIN, HIGH);

  delay(50);
  prepareMovement(1, 5000);
  runSteppers();

  while(getMovementStep(1) < 1000){
    if(!digitalRead(KILL_BTN_PIN)){
      digitalWrite(MOSFET_C_PIN, LOW);
      digitalWrite(MOSFET_A_PIN, LOW);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      digitalWrite(E0_ENABLE_PIN, HIGH);
      digitalWrite(Y_ENABLE_PIN, HIGH);
      digitalWrite(X_ENABLE_PIN, HIGH);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      return 2;
    }
    tryScreenUpdate();
  }

  // desliga o puxador e separa o papel
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
      digitalWrite(E0_ENABLE_PIN, HIGH);
      digitalWrite(Y_ENABLE_PIN, HIGH);
      digitalWrite(X_ENABLE_PIN, HIGH);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      return 2;
    }
    if (!isRunning(1))
    {
      digitalWrite(MOSFET_C_PIN, LOW);
      digitalWrite(MOSFET_A_PIN, LOW);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      digitalWrite(E0_ENABLE_PIN, HIGH);
      digitalWrite(Y_ENABLE_PIN, HIGH);
      digitalWrite(X_ENABLE_PIN, HIGH);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      return 0;
    }
    tryScreenUpdate();
  }
  prepareMovement(1, 0);
  // ################## FIM PUXAR PAPEL ##################

  while(isRunning(0)){
    if (!digitalRead(KILL_BTN_PIN))
    {
      digitalWrite(MOSFET_C_PIN, LOW);
      digitalWrite(MOSFET_A_PIN, LOW);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      digitalWrite(E0_ENABLE_PIN, HIGH);
      digitalWrite(Y_ENABLE_PIN, HIGH);
      digitalWrite(X_ENABLE_PIN, HIGH);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      return 2;
    }
    tryScreenUpdate();
  }
  digitalWrite(X_ENABLE_PIN, LOW);
  digitalWrite(Y_ENABLE_PIN, LOW);
  prepareMovement(1, (steps_mm_puxador * parameters.larguraBandeirinha));
  prepareMovement(0, 2000);
  runSteppers();

  while(getMovementStep(1) < 500){
    if(!digitalRead(KILL_BTN_PIN)){
      digitalWrite(MOSFET_C_PIN, LOW);
      digitalWrite(MOSFET_A_PIN, LOW);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      digitalWrite(E0_ENABLE_PIN, HIGH);
      digitalWrite(Y_ENABLE_PIN, HIGH);
      digitalWrite(X_ENABLE_PIN, HIGH);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      return 2;
    }
    tryScreenUpdate();
  }

  //Serial.println("ligada esteira");
  prepareMovement(0, (steps_mm_esteira * parameters.larguraBandeirinha));
  runSteppers();

  while(getMovementStep(0) < 1700){
    if(!digitalRead(KILL_BTN_PIN)){
      digitalWrite(MOSFET_C_PIN, LOW);
      digitalWrite(MOSFET_A_PIN, LOW);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      digitalWrite(E0_ENABLE_PIN, HIGH);
      digitalWrite(Y_ENABLE_PIN, HIGH);
      digitalWrite(X_ENABLE_PIN, HIGH);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      return 2;
    }
  }
  digitalWrite(E0_ENABLE_PIN, LOW);
  prepareMovement(2, -(parameters.uLbandeirinha * steps_uL_bomba + 1000));
  runSteppers();
  while(isRunning(2)){
    if(!digitalRead(KILL_BTN_PIN)){
      digitalWrite(MOSFET_C_PIN, LOW);
      digitalWrite(MOSFET_A_PIN, LOW);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      digitalWrite(E0_ENABLE_PIN, HIGH);
      digitalWrite(Y_ENABLE_PIN, HIGH);
      digitalWrite(X_ENABLE_PIN, HIGH);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      return 2;
    }
    tryScreenUpdate();
  }
  //Serial.println("retraindo bomba");
  prepareMovement(2, 1000);
  setInterval(2, 20);
  runSteppers();
  while(isRunning(0)){
    if(!digitalRead(KILL_BTN_PIN)){
      digitalWrite(MOSFET_C_PIN, LOW);
      digitalWrite(MOSFET_A_PIN, LOW);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      digitalWrite(E0_ENABLE_PIN, HIGH);
      digitalWrite(Y_ENABLE_PIN, HIGH);
      digitalWrite(X_ENABLE_PIN, HIGH);
      digitalWrite(Z_ENABLE_PIN, HIGH);
      return 2;
    }
    tryScreenUpdate();
  }

  setInterval(0, parameters.velFeed);
  prepareMovement(0, steps_mm_esteira * parameters.distanciaEntre);
  runSteppers();
  digitalWrite(MOSFET_C_PIN, LOW);
  digitalWrite(MOSFET_A_PIN, LOW);
  digitalWrite(Z_ENABLE_PIN, HIGH);
  //Serial.println("fim do ciclo");
  return -1;
}