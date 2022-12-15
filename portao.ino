#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>

#define SERVO 6
#define MAX 180
#define MIN 0
#define ledPin 3
#define TEMPO_ABERTO 500

String strID;
String cadastrados[1]={
  "73:95:52:4d",
};
bool fechado = false, aberto = false, fim_de_curso = false, acionado = false;
byte i, pos = 0x00;
int contador = 0;
MFRC522 rfid(10, 9);
Servo servo1;

void setup(){

  TCCR1A = 0x00;
  TCCR1B = 0x03;
  TCNT1 = 0xF63C;
  
  // 100Hz 10ms -> Ps 64 -> Inicializar TCNT1 com 63036 ou 0xF63C
  // 40Hz 25ms  -> Ps 64 -> Inicializar TCNT1 com 59286 ou 0xE796
  // 20Hz 50ms -> Ps 256 -> Inicializar TCNT1 com 62411 ou 0xF3CB
  // 10Hz 100ms -> Ps 64 -> Inicializar TCNT1 com 40536 ou 0x9E58

  servo1.attach(SERVO);
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
}

void loop(){

  if(!acionado)
    if(getID())
      if(validar()){
        acionado = true;
        //TIMSK1 |= (1 << TOIE1);
      }     
      else
        invalido();

}

void invalido(){
  //sinaliza erro, sonoro
  return;
}

bool validar(){
  for(i=0; i<1; i++)
    if(strID.indexOf(cadastrados[i]) >= 0) return true;
  return false;
}

bool getID(){
  if(!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return false;
  strID = "";
  for(i=0; i < 4; i++)
    strID += (rfid.uid.uidByte[i] < 0x10 ? "0" : "") + String(rfid.uid.uidByte[i], HEX) + (i!=3 ? ":" : "");
  rfid.PICC_HaltA();~
  Serial.println(strID);
  return true;
}

ISR(TIMER1_OVF_vect){
  TCNT1 = 0xF63C;

  if(fechado){
    if(pos < MAX){
      servo1.write(pos);
      pos++;
    }
    else{
      fim_de_curso = true;
      fechado = false;
    }
  }

  if(fim_de_curso){
      if(contador < TEMPO_ABERTO){
        contador++;
      }
      else{
        fim_de_curso = false;
        aberto = true;
      }
    }

  if(aberto){
      if(pos > MIN){
        servo1.write(pos);
        pos--;
      }
      else{
        fechado = true;        
        acionado = false;
        TIMSK1 &= (0 << TOIE1);
      }
    }
}


//digitalWrite(ledPin, digitalRead(ledPin)^1);