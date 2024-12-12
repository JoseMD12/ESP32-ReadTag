#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define RST_PIN         22
#define SS_PIN          5

MFRC522 mfrc522(SS_PIN, RST_PIN);

const char* ssidMovel = "iPhone de José";
const char* passwordMovel = "jose0606";

const char* serverUrlMovel = "http://172.20.10.2:3000/product/event";

const char* connectionTesteMovel = "http://172.20.10.2:3000/rfid";

const char* depositId = "5bad4fc3-8562-4f8f-8ef9-e54d3dc602a9";

void setup() {
  Serial.begin(9600);
  SPI.begin();

  Serial.println("Conectando ao Wi-Fi...");
  //URL
  WiFi.begin(ssidMovel, passwordMovel);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(". ");
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    //URL
    http.begin(connectionTesteMovel);
    http.GET();
    http.end();
  }

  Serial.println("\nConectado ao Wi-Fi!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());

  mfrc522.PCD_Init();
  Serial.println(F("Read item code from a MIFARE PICC "));
}

void loop() {
  Serial.flush();

  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print(F("Card UID:"));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  byte buffer[18];
  byte block;
  MFRC522::StatusCode status;
  byte len;
  String cardData = "";

  for (int i = 0; i < 2; i++) {
    block = 1 + i;
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }

    len = sizeof(buffer);
    status = mfrc522.MIFARE_Read(block, buffer, &len);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }

    for (byte j = 0; j < 16; j++) {
      cardData += (char)buffer[j];
    }
  }

  cardData = trimString(cardData);

  Serial.print("Dados lidos: ");
  Serial.println(cardData);

  sendToServer(cardData);

  Serial.println();
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1(); 
}

void sendToServer(String data) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    //URL
    http.begin(serverUrlMovel);
    http.addHeader("Content-Type", "application/json"); // Define o tipo de conteúdo

    String jsonPayload = String("{\"actualDepositId\": \"") + depositId + "\", \"tagList\": [\"" + data + "\"]}";

    Serial.print("Enviando dados ao servidor: ");
    Serial.println(jsonPayload);

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("Resposta do servidor: ");
      Serial.println(response);
    } else {
      Serial.print("Erro na requisição: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Erro: Wi-Fi desconectado!");
  }
}

String trimString(String str) {
  while (str.length() > 0 && isWhitespace(str[0])) {
    str.remove(0, 1);
  }

  while (str.length() > 0 && isWhitespace(str[str.length() - 1])) {
    str.remove(str.length() - 1);
  }

  return str;
}

bool isWhitespace(char c) {
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}
