/*------------------------------------------------------------------------*\

  Descrição: O código é responsável por controlar o acesso a um portão
             automático, este módulo permite a autenticação de usuários
             através de um módulo RFID MFRC522, o seguinte código cadastra
             e exclui cartões/tags compatíveis com o módulo. Se comunica com 
             o receptor, outro ESP32 que irá controlar o acionamento do portão.

  Criado por: Ramon Celino
              Ygor Kauan

\*------------------------------------------------------------------------*/

// Bibliotecas usadas
#include <SPI.h>
#include <MFRC522.h>
#include <esp_now.h>
#include <WiFi.h>

// Constantes
#define BOTAO_GRAVAR 12
#define BOTAO_APAGAR 33
#define LED_ONBOARD 2
#define LED_VERMELHO 4
#define LED_VERDE 5
#define BUZZER 32

// -- Variáveis --
uint8_t peerAddress[] = {0xCC, 0x50, 0xE3, 0x5D, 0x7A, 0x0C};       // Endereço MAC do receptor
uint8_t leitura_cartao[4];                                          // Variável que armazena endereço a ser comparado
uint8_t cadastrados[100][4];                                        // Vetor bidimensional para representar os cartões gravados
uint8_t cont_cadastros = 0;                                         // Referência para número de cadastrados
bool modo_gravar = false;                                           // Identificador booleano do modo gravação
bool modo_apagar = false;                                           // Identificador booleano do modo exclusão

// Acesso ao módulo RFID
MFRC522 rfid(21, 22);                                               // Pino de seleção ss_pin. Pino do reset, rst_pin

// Estrutuda do tipo de dados que será enviada ao receptor
typedef struct struct_message {
  bool acionamento;
} struct_message;

// Criação de uma struct_message denominada msg
struct_message msg;

// Carrega informações do receptor
esp_now_peer_info_t peerInfo;

// Função callback quando dado é enviado
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  status == ESP_NOW_SEND_SUCCESS ? digitalWrite(LED_ONBOARD, LOW) : digitalWrite(LED_ONBOARD, HIGH);
}

// Função de interrupção chamada no evento do clique do botão ligado ao pino 12, modo gravação
void IRAM_ATTR botao_gravar() {
  Serial.println("Entrando modo gravar");
  if (!modo_apagar) modo_gravar = true;
}

// Função de interrupção chamada no evento do clique do botão ligado ao pino 33, modo excluir
void IRAM_ATTR botao_apagar() {
  Serial.println("Entrando modo apagar");
  if (!modo_gravar) modo_apagar = true;
}

void setup() {

  // Definição dos modo de operação dos pinos usados
  pinMode(BOTAO_GRAVAR, INPUT_PULLUP);
  pinMode(BOTAO_APAGAR, INPUT_PULLUP);
  pinMode(LED_ONBOARD, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // Inicialização das saídas
  digitalWrite(LED_ONBOARD, LOW);
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(BUZZER, LOW);

  // Definição das interruções externas
  attachInterrupt(digitalPinToInterrupt(BOTAO_GRAVAR), botao_gravar, FALLING);
  attachInterrupt(digitalPinToInterrupt(BOTAO_APAGAR), botao_apagar, FALLING);

  // Adicionando a mensagem a ser enviada
  msg.acionamento = true;

  // Definição do modo de operação do ESP32, modo estação
  WiFi.mode(WIFI_STA);

  // Definição da função chamada para realizar o envio de dados
  esp_now_register_send_cb(OnDataSent);

  // Registro do receptor, definição do canal usado e desabilitação da criptografia
  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = 1;  
  peerInfo.encrypt = false;

  // Inicialização da comunicação
  SPI.begin();
  rfid.PCD_Init();  
}

void loop() {
  
  if (!modo_gravar && !modo_apagar) {                                                     // Se nenhum botão seja precionado
    if (getID())                                                                          // Verifica se algum cartão se aproximou
      if (validar() != 100) {                                                             // Verifica se o cartão foi cadastrado
        esp_err_t result = esp_now_send(peerAddress, (uint8_t *) &msg, sizeof(msg));      // Chamada da função que envia msg para abertura do portão
      } else
        erro();                                                                           // Cartão não cadastrado
    } else
      modificar_registro();                                                               // Um dos botões foi pressionado
      
  delay(100);

}

// Função responsável por alterar cadastro do banco de cartãoes/tags
void modificar_registro() {
  uint8_t cartao;

  if (modo_gravar) {                                                                        // Entra no modo Gravar
    digitalWrite(LED_VERDE, HIGH);                                                          // Liga o led verde
    while (!getID()) {};                                                                    // Aguarda a aproximação do cartão
    delay(500);
    cartao = validar();                                                                     // Faz a verificação se o cartão existe no bando de cartões
    digitalWrite(LED_VERDE, LOW);                                                           // Apaga o led verde
    if (cartao == 100) {                                                                    // Caso o cartão não exista será adicionado
      for (uint8_t i = 0; i < 4; i++) cadastrados[cont_cadastros][i] = leitura_cartao[i];
      cont_cadastros++;
      validado();
    } else {
      erro();                                                                               // Cartão a ser adicionado já consta cadastrado
    }    
    modo_gravar = false;

  } else {                                                                                  // Entra no modo apagar
    digitalWrite(LED_VERMELHO, HIGH);                                                       // Liga o led vermelho
    while (!getID()) {};                                                                    // Aguarda a aproximação do cartão  
    delay(500);
    cartao = validar();                                                                     // Faz a verificação se o cartão existe no bando de cartões
    digitalWrite(LED_VERMELHO, LOW);                                                        // Apaga o led vermelho
    if (cartao != 100) {                                                                    // Caso o cartão exista será excluido
      cont_cadastros--;
      for (uint8_t i = 0; i < 4; i++) cadastrados[cartao][i] = cadastrados[cont_cadastros][i];
      validado();
    } else {
      erro();                                                                               // Cartão não existe no banco de cartões
    }    
    modo_apagar = false;
  }
  return;
}

// Função que sinaliza erro de operação, pisca o led vermelho por 5 vezes e aciona o buzzes por 700ms
void erro() {
  digitalWrite(BUZZER, HIGH);
  for (uint8_t i = 0; i < 10; i++){
    digitalWrite(4, digitalRead(4)^1);
    delay(70);
  }
  digitalWrite(BUZZER, LOW);
  return;
}

// Função que sinaliza operação bem sucedida, pisca o led verde por 5 vezes e aciona o buzzes por 700ms
void validado() {
  digitalWrite(BUZZER, HIGH);
  for (uint8_t i = 0; i < 10; i++){
      digitalWrite(5, digitalRead(5)^1);
      delay(70);
  }
  digitalWrite(BUZZER, LOW);
  return;  
}

// Função que percorre o banco de cartões e verifica se existe um igual ao aproximado (leitura_cartao)
// Retorna o índice da matriz cadastrados caso o cartão exista
// Retorna 100 caso contrário
uint8_t validar() {
  for (uint8_t i = 0; i < cont_cadastros; i++){
    if (cadastrados[i][0] == leitura_cartao[0] && cadastrados[i][1] == leitura_cartao[1] && cadastrados[i][2] == leitura_cartao[2] && cadastrados[i][3] == leitura_cartao[3]) return i;
  }
  return 100;
}

// Função responsável por fazer a rotina de captura do ID de um cartão aproximado
// Retorna false caso o cartão proximo foi lido por último
// Caso contrário retorna true
bool getID() {
  for (uint8_t i = 0; i < 4; i++) leitura_cartao[i] = 0;
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return false;
  for (uint8_t i = 0; i < 4; i++) {
    leitura_cartao[i] = rfid.uid.uidByte[i];
  }
  rfid.PICC_HaltA();
  return true;
}