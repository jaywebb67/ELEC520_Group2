#ifndef MQTT_HPP
#define MQTT_HPP

#include <cstring>  // Ensure this library is included for strcmp and memset
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


//NetworkClientSecure client;
extern NetworkClientSecure client;
extern PubSubClient mqttClient;



extern const char* isrg_root_x1_cert;
extern const int mqttPort;
extern const char* mqttServer;
extern long lastReconnectAttempt;
extern bool restartFlag;
extern String clientId;
extern String ping;
extern String payload;
extern const char* ssid;
extern const char* password;

struct MqttMessage {
  String topic;
  String payload;  
};

void mqttSetUp();

void MQTT_task(void* pvParameters);

void reconnect(int onSetUp);

void setClock();

void mqttCallback(char* topic, byte* payload, unsigned int length);

void mqttHandler(void* pvParameters);






#endif