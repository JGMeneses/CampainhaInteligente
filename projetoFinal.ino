#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

int led = 32;        // LED indicador de proximidade
int ledResp = 34;    // LED de resposta do vigia
int trigPin = 26; 
int echoPin = 27;

const char* ssid = "Sheilinha";
const char* password = "12345678";
String botToken = "7317902933:AAHhIjBtF8lXop5D_Uv7bu9xjHvu0Eoh5oc";  
String chatID = "5406423642";  
int lastUpdateID = 0;  
int lastMessageID = 0; // Salvar o ID da última mensagem confirmada

void sendMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + message;
    http.begin(url.c_str());
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.println("Mensagem enviada com sucesso");
    } else {
      Serial.print("Erro ao enviar a mensagem: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi não está conectado!");
  }
}

void deleteMessage(int messageID) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/deleteMessage?chat_id=" + chatID + "&message_id=" + String(messageID);
    http.begin(url.c_str());
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.println("Mensagem deletada com sucesso");
    } else {
      Serial.print("Erro ao deletar a mensagem: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi não está conectado!");
  }
}

void blinkLED(int durationMs) {
  unsigned long startTime = millis();
  while (millis() - startTime < durationMs) {
    digitalWrite(ledResp, HIGH);
    delay(500);
    digitalWrite(ledResp, LOW);
    delay(500);
  }
}

int readDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(trigPin, LOW);
  unsigned long duration = pulseIn(echoPin, HIGH);
  return duration / 58;
}

bool checkForGuardResponse() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/getUpdates?offset=" + String(lastUpdateID + 1);
    http.begin(url.c_str());
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println("Payload recebido: ");
      Serial.println(payload);

      // Usando a biblioteca ArduinoJson para analisar o JSON
      StaticJsonDocument<2048> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print("Erro ao analisar o JSON: ");
        Serial.println(error.f_str());
        return false;
      }

      JsonArray updates = doc["result"].as<JsonArray>();
      for (JsonObject update : updates) {
        int updateID = update["update_id"];
        if (updateID > lastUpdateID) {
          lastUpdateID = updateID;
          JsonObject message = update["message"];
          String text = message["text"];
          int messageID = message["message_id"];

          if (text == "Estou indo!" && messageID != lastMessageID) {
            Serial.println("Comando recebido: Estou indo!");

            // Atualiza o ID da última mensagem confirmada
            lastMessageID = messageID;

            // Deleta a mensagem após o processamento
            deleteMessage(messageID);
            return true;
          }
        }
      }
    } else {
      Serial.print("Erro ao verificar mensagens: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi não está conectado!");
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  pinMode(ledResp, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  WiFi.begin(ssid, password);
  Serial.println("Conectando ao WiFi");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Conectado ao WiFi com o IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  int distancia = readDistance();
  Serial.print("Distancia: ");
  Serial.print(distancia);
  Serial.println(" cm");

  if (distancia <= 10) {
    digitalWrite(led, HIGH);  // Liga o LED indicando proximidade
    sendMessage("Objeto detectado a 10 cm ou menos!");

    // Aguarda a resposta "Estou indo!" do vigia
    while (!checkForGuardResponse()) {
      delay(2000);  // Espera 2 segundos antes de verificar novamente
    }

    // Pisca o LED por 1 minuto após receber a resposta do vigia
    //blinkLED(60000);

    // Desliga o LED de proximidade após o ciclo de piscar
    digitalWrite(led, LOW);
  } else {
    digitalWrite(led, LOW);  // Desliga o LED se a distância for maior que 10 cm
  }

  delay(400);  // Intervalo de 400ms entre leituras do sensor
}
