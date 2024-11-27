#ifndef MQTT_HPP
#define MQTT_HPP

#include <cstring>  // Ensure this library is included for strcmp and memset
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


//NetworkClientSecure client;
NetworkClientSecure client;
PubSubClient mqttClient(client);



const char* isrg_root_x1_cert;
const int mqttPort;
const char* mqttServer;
long lastReconnectAttempt;
bool restartFlag;
String clientId;
String ping;
String payload;

struct MqttMessage {
  String topic;
  String payload;  
};

void mqttSetUp();

void MQTT_task();

void reconnect(int onSetUp);

void setClock();

void mqttCallback(char* topic, byte* payload, unsigned int length);

void mqttHandler(void* pvParameters);





#endif