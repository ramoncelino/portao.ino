/*------------------------------------------------------------------------*\

  Descrição: O código é responsável por controlar o acesso a um portão
             automático, este módulo permite se comunicar. Se comunica com
             o transmissor e atuar sobre dois servo motores que abrem e fecham
             o portão.

  Criado por: Ramon Celino
              Ygor Kauan

\*------------------------------------------------------------------------*/

// Bibliotecas usadas
#include <WiFi.h>
#include <esp_now.h>

// Constantes
#define SERVO1 13
#define SERVO2 12

// -- Variáveis --
bool acionar = false;

// Estrutuda do tipo de dados usada para comunicação
typedef struct struct_message {
  bool acionamento;
} struct_message;

// Criação de uma struct_message denominada msg
struct_message msg;

// Função callback quando dado é recebido
void onDataReceiver(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&msg, incomingData, sizeof(msg));
  acionar = msg.acionamento;
}

void setup() {

  // Definição dos modo de operação dos pinos usados
  pinMode(SERIAL, OUTPUT);
  pinMode(SERVO2, OUTPUT);

  // Definição do modo de operação do ESP32, modo estação
  WiFi.mode(WIFI_STA);
  
  // Definição da função chamada para realizar o recebimento de dados
  esp_now_register_recv_cb(onDataReceiver);
  
  // Antes de iniciar a operar fecha o portão caso esteja aberto
  for(char i=0;i<100;i++) servoFechado();
}

void loop() {

  if (acionar) {               // Quando recebe uma msg true (msg.acionamento)
    for(char i=0;i<100;i++) { 
      servoAberto();           // Move o servo1 da posição 0º para a posição 180º
                               // Move o servo2 da posição 180º para a posição 0º
    }
    delay(1000);
    for(char i=0;i<20;i++) { 
      servoFechado();          // Move o servo1 da posição 0º para a posição 180º
                               // Move o servo1 da posição 0º para a posição 180º
    }
    acionar = false;           // Reinicia a variável para um novo ciclo
  }
}

// Função que rotaciona dois servos motores, cada um em sentido oposto, abrindo o portão
// Cada movimento precisa de um pulso com período respectivo a seu movimento;
// Para se mover de 0 a 180, um pulso de 0.6ms e permanece ~19ms inativo,
// Para se mover de 180 a 0, um pulso de 2.4ms e permanece ~17ms inativo.
void servoFechado() {
   digitalWrite(SERVO1, HIGH);  //pulso do servo
   delayMicroseconds(600);      //0.6ms
   digitalWrite(SERVO1, LOW);   //completa periodo do servo
   for(int i=0;i<32;i++)delayMicroseconds(600);
                                // 20ms = 20000us
                                // 20000us - 600us = 19400us
   digitalWrite(SERVO2, HIGH);  //pulso do servo
   delayMicroseconds(2400);     //2.4ms
   digitalWrite(SERVO2, LOW);   //completa periodo do servo
   for(int i=0;i<7;i++)delayMicroseconds(2400);
                                // 20ms = 20000us
                                // 20000us - 2400us = 17600us
}

// Função que rotaciona dois servos motores, cada um em sentido oposto, fechando o portão
void servoAberto() {
   digitalWrite(SERVO1, HIGH);  //pulso do servo
   delayMicroseconds(2400);     //2.4ms
   digitalWrite(SERVO1, LOW);   //completa periodo do servo
   for(int i=0;i<7;i++)delayMicroseconds(2400);
                                // 20ms = 20000us
                                // 20000us - 2400us = 17600us
   
   digitalWrite(SERVO2, HIGH);  //pulso do servo
   delayMicroseconds(600);      //0.6ms
   digitalWrite(SERVO2, LOW);   //completa periodo do servo
   for(int i=0;i<32;i++)delayMicroseconds(600);
                                // 20ms = 20000us
                                // 20000us - 600us = 19400us
}