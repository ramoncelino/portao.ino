#include <SPI.h>
#include <MFRC522.h>
#include <esp_now.h>
#include <WiFi.h>

uint8_t peerAddress[] = {0x10, 0x52, 0x1C, 0x5E, 0x09, 0xEC};

typedef struct struct_message {
  bool acionamento;
} struct_message;

struct_message msg;

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
}

uint8_t leitura_cartao[4];
uint8_t cadastrados[20][4];
uint8_t cont_cadastros = 0;

MFRC522 rfid(14, 13);  // sda_pin , rs_pin
bool modo_gravar = false, modo_apagar = false;

void IRAM_ATTR botao_gravar() {
  Serial.println("Entrando modo gravar");
  if (!modo_apagar) modo_gravar = true;
}

void IRAM_ATTR botao_apagar() {
  Serial.println("Entrando modo apagar");
  if (!modo_gravar) modo_apagar = true;
}

void setup() {

  pinMode(12, INPUT_PULLUP);
  pinMode(27, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(12), botao_gravar, FALLING);
  attachInterrupt(digitalPinToInterrupt(27), botao_apagar, FALLING);

  pinMode(16, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(26, OUTPUT);
  digitalWrite(16, LOW);
  digitalWrite(5, LOW);
  digitalWrite(26, LOW);
  
  msg.acionamento = true;

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = 1;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    return;
  }

  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();  
}

void loop() {

  if (!modo_gravar && !modo_apagar) {
    if (getID())
      if (validar() != 100) {
        esp_err_t result = esp_now_send(peerAddress, (uint8_t *) &msg, sizeof(msg));
        delay(200);
      } else
        erro();
    } else
      modificar_registro();
      
  delay(100);  
}

void modificar_registro() {
  uint8_t cartao;

  if (modo_gravar) {
    digitalWrite(5, HIGH);
    while (!getID()) {};
    delay(500);
    cartao = validar();
    digitalWrite(5, LOW);
    if (cartao == 100) {
      for (uint8_t i = 0; i < 4; i++) cadastrados[cont_cadastros][i] = leitura_cartao[i];
      cont_cadastros++;
      validado();
    } else {
      erro();
    }    
    modo_gravar = false;

  } else {  // modo_apagar
    digitalWrite(16, HIGH);
    while (!getID()) {};
    delay(500);
    cartao = validar();
    digitalWrite(16, LOW);
    if (cartao != 100) {
      cont_cadastros--;
      for (uint8_t i = 0; i < 4; i++) cadastrados[cartao][i] = cadastrados[cont_cadastros][i];
      validado();
    } else {
      erro();
    }    
    modo_apagar = false;
  }
  return;
}

void erro() {
  digitalWrite(26, HIGH);
  for (uint8_t i = 0; i < 10; i++){
    digitalWrite(16, digitalRead(16)^1);
    delay(70);
  }
  digitalWrite(26, LOW);
  return;
}

void validado() {
  digitalWrite(26, HIGH);
  for (uint8_t i = 0; i < 10; i++){
      digitalWrite(5, digitalRead(5)^1);
      delay(70);
  }
  digitalWrite(26, LOW);
  return;  
}

uint8_t validar() {
  for (uint8_t i = 0; i < cont_cadastros; i++){
    if (cadastrados[i][0] == leitura_cartao[0] && cadastrados[i][1] == leitura_cartao[1] && cadastrados[i][2] == leitura_cartao[2] && cadastrados[i][3] == leitura_cartao[3]) return i;
  }
  return 100;
}

bool getID() {
  for (uint8_t i = 0; i < 4; i++) leitura_cartao[i] = 0;
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return false;
  for (uint8_t i = 0; i < 4; i++) {
    leitura_cartao[i] = rfid.uid.uidByte[i];
  }
  rfid.PICC_HaltA();
  return true;
}
