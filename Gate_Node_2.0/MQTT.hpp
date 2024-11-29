#ifndef MQTT_HPP
#define MQTT_HPP

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "Char_Buffer.h"


//NetworkClientSecure client;
extern NetworkClientSecure client;
extern PubSubClient mqttClient;

extern CharBuffer Valid_Entrance_Codes;
extern CharBuffer Current_Codes_In_Use;
extern bool restartFlag;
extern String clientId;
extern String ping;

struct MqttMessage {
  String topic;
  String payload;  
};
struct Mqtt_user_Message {
  String topic;
  String payload;  
  int location;
};

struct Mqtt_newNode_Message {
  String topic;
  int location;
  int node_type;
  int address;
  int dest_address;

};

void mqttSetUp();

void MQTT_task(void* pvParameters);

void reconnect(int onSetUp);

void reconnect(bool onSetUp);

void setClock();

void mqttCallback(char* topic, byte* payload, unsigned int length);

void mqttHandler(void* pvParameters);



#endif