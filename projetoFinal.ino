  #include <WiFi.h>
  #include <HTTPClient.h>
  #include <ArduinoJson.h>

  // Definições de pinos
  int led = 32;        // LED indicador de proximidade
  int ledResp = 25;    // LED de resposta do vigia
  int trigPin = 26; 
  int echoPin = 27;

  // Credenciais Wi-Fi
  const char* ssid = "esp32";
  const char* password = "embarcados";

  // Configuração do bot Telegram
  String botToken = "7317902933:AAHhIjBtF8lXop5D_Uv7bu9xjHvu0Eoh5oc";  
  String chatID = "5406423642";  
  int lastUpdateID = 0;  
  int lastMessageID = 0; // Salvar o ID da última mensagem confirmada

  // Variáveis de tempo
  unsigned long responseStartTime;
  const unsigned long responseTimeout = 60000; // Tempo limite de 60 segundos

  TaskHandle_t Task1;
  TaskHandle_t Task2;
  TaskHandle_t Task3;
  TaskHandle_t Task4;
  SemaphoreHandle_t xMutex;

  QueueHandle_t messageQueue;
  QueueHandle_t deleteQueue;

  void TaskSendMessage(void * parameter) {
    String message;
    for (;;) {
      if (xQueueReceive(messageQueue, &message, portMAX_DELAY)) {
        unsigned long startTime = millis();
        bool success = false;

        while (millis() - startTime < responseTimeout) {
          if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + message;
            http.begin(url.c_str());
            int httpResponseCode = http.GET();

            if (httpResponseCode > 0) {
              Serial.println("Mensagem enviada com sucesso");
              success = true;
              http.end();
              break; // Sai do loop se a mensagem for enviada com sucesso
            } else {
              Serial.print("Erro ao enviar a mensagem: ");
              Serial.println(httpResponseCode);
              http.end();
              vTaskDelay(2000 / portTICK_PERIOD_MS); // Espera 2 segundos antes de tentar novamente
            }
          } else {
            Serial.println("WiFi não está conectado!");
          }
        }

        if (!success) {
          Serial.println("Não foi possível enviar a mensagem após 1 minuto.");
        }
      }
    }
  }

  void TaskDeleteMessage(void * parameter) {
    int messageID;
    for (;;) {
      if (xQueueReceive(deleteQueue, &messageID, portMAX_DELAY)) {
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

              lastMessageID = messageID;
              xQueueSend(deleteQueue, &messageID, portMAX_DELAY);
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

  void resetSystem() {
    Serial.println("Tempo de resposta excedido. Reiniciando o sistema...");
    ESP.restart(); // Reinicia o ESP32
  }

  void TaskReadDistance(void * parameter) {
    for (;;) {
      int distancia;
      if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
        distancia = readDistance();
        Serial.print("Distancia: ");
        Serial.print(distancia);
        Serial.println(" cm");
        xSemaphoreGive(xMutex);
      }

      if (distancia <= 10) {
        digitalWrite(led, HIGH);
        String message = "Objeto detectado a 10 cm ou menos!";
        xQueueSend(messageQueue, &message, portMAX_DELAY);
        
        responseStartTime = millis(); // Inicia o tempo de resposta

        bool responseReceived = false;
        while (millis() - responseStartTime < responseTimeout) {
          if (checkForGuardResponse()) {
            responseReceived = true;
            break;
          }
          vTaskDelay(2000 / portTICK_PERIOD_MS); // Espera 2 segundos antes de verificar novamente
        }

        if (!responseReceived) {
          resetSystem(); // Reseta o sistema se o tempo de resposta exceder o limite
        } else {
          blinkLED(10000);
          digitalWrite(led, LOW);
        }
      } else {
        digitalWrite(led, LOW);
      }

      vTaskDelay(400 / portTICK_PERIOD_MS);
    }
  }

  void TaskWiFi(void * parameter) {
    for (;;) {
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconectando ao WiFi...");
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
          vTaskDelay(500 / portTICK_PERIOD_MS);
          Serial.print(".");
        }
        Serial.println("");
        Serial.print("Conectado ao WiFi com o IP: ");
        Serial.println(WiFi.localIP());
      }
      vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
  }

  void setup() {
    Serial.begin(115200);
    pinMode(led, OUTPUT);
    pinMode(ledResp, OUTPUT);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    xMutex = xSemaphoreCreateMutex();
    
    // Criando filas para mensagens e IDs de mensagens a serem deletadas
    messageQueue = xQueueCreate(10, sizeof(String));
    deleteQueue = xQueueCreate(10, sizeof(int));

    WiFi.begin(ssid, password);
    Serial.println("Conectando ao WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("Conectado ao WiFi com o IP: ");
    Serial.println(WiFi.localIP());

    // Criando tasks
    xTaskCreatePinnedToCore(
      TaskReadDistance,    // Função da tarefa
      "TaskReadDistance",  // Nome da tarefa
      4096,                // Tamanho da pilha
      NULL,                // Parâmetro da tarefa
      2,                   // Prioridade da tarefa
      &Task1,              // Handle da tarefa
      1                    // Núcleo 1
    );

    xTaskCreatePinnedToCore(
      TaskWiFi,            // Função da tarefa
      "TaskWiFi",          // Nome da tarefa
      4096,                // Tamanho da pilha
      NULL,                // Parâmetro da tarefa
      1,                   // Prioridade da tarefa
      &Task2,              // Handle da tarefa
      0                    // Núcleo 0
    );

    xTaskCreatePinnedToCore(
      TaskSendMessage,     // Função da tarefa
      "TaskSendMessage",   // Nome da tarefa
      4096,                // Tamanho da pilha
      NULL,                // Parâmetro da tarefa
      3,                   // Prioridade da tarefa
      &Task3,              // Handle da tarefa
      1                    // Núcleo 1
    );

    xTaskCreatePinnedToCore(
      TaskDeleteMessage,   // Função da tarefa
      "TaskDeleteMessage", // Nome da tarefa
      4096,                // Tamanho da pilha
      NULL,                // Parâmetro da tarefa
      3,                   // Prioridade da tarefa
      &Task4,              // Handle da tarefa
      0                   // Núcleo 1
    );
  }

  void loop() {
    // O loop é vazio, pois o código é gerido pelas tasks
  }
