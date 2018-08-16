//Programa : RFID - Controle de Acesso por leitor RFID
//Autor : João Paulo

#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>

#define SS_PIN 10
#define RST_PIN 9
#define BUZZER 6
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.

char st[20];

String tipo [] = {
  "Cartão",   //0
  "Chaveiro", //1
};

String ids_permitidos [][3] = {
  {"46 3D B2 AC", "1", "João Paulo de Melo"},
  {"30 22 72 A3", "0", "Fulano"},
};

void setup()
{
  Serial.begin(9600); // Inicia a serial
  SPI.begin();    // Inicia  SPI bus
  mfrc522.PCD_Init(); // Inicia MFRC522
  pinMode(BUZZER, OUTPUT); // Inicia o Buzzer
  //Define o número de colunas e linhas do LCD:
  mensagem_boasVindas();
}

void loop()
{
  bool acesso = false;
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  String chave = get_UID().substring(1);

  for (int i = 0; i < sizeof (ids_permitidos) / sizeof (ids_permitidos[0]); i++)
    if (chave == ids_permitidos[i][0]) //UID 1 - Chaveiro
    {
      acesso = true;
      chave.replace(" ", "-");
      //Serial.println("Seja bem vindo " + ids_permitidos[i][2]);
      //Serial.println("Seu tipo de acesso é: " + tipo[ids_permitidos[i][1].toInt()]);
      Serial.println(/*"Tag UID: " + */chave);
      som_sucesso();
      delay(2000);
      mensagem_boasVindas();
      break;
    }

  if (!acesso) {
    Serial.println("Tag Inválida.");
    Serial.println("Tag UID: " + chave);
    som_erro();
    delay(2000);
    mensagem_boasVindas();
  }
}

void mensagem_boasVindas()
{
  //Serial.println("Aproxime o seu Cartão do leitor...");
}

String get_UID() {
  String conteudo = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  conteudo.toUpperCase();
  return conteudo;
}

void som_erro() {
  for(int i = 0; i < 3; i++){
    tone(BUZZER, 1000);   //Define a frequência em 1440
    delay(250);
    noTone(BUZZER);
    delay(250);
  }
}

void som_sucesso() {
  for(int i = 0; i < 2; i++){
    tone(BUZZER, 1800);   //Define a frequência em 1440
    delay(80);
    noTone(BUZZER);
    delay(80);
  }
}

