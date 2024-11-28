#ifndef MQTT_HPP
#define MQTT_HPP

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <PubSubClient.h>


//NetworkClientSecure client;
extern NetworkClientSecure client;
extern PubSubClient mqttClient;


extern bool restartFlag;
extern String clientId;
extern String ping;

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