#ifndef MQTT_HPP
#define MQTT_HPP

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "Communication_Protocol.h"
#include "Char_Buffer.h"

#define QUEUE_WAIT_TIME pdMS_TO_TICKS(100) // Wait for 100 milliseconds

//NetworkClientSecure client;
extern WiFiClientSecure client;
extern PubSubClient mqttClient;

extern TaskHandle_t mqttTaskHandle;
extern TaskHandle_t mqttReceiveHandle;

extern QueueHandle_t mqttPublishQueue;

extern bool restartFlag;
extern String clientId;
extern String ping;
extern bool alarmTriggered;
extern bool I_am_Forwarder;

extern CharBuffer Valid_Admin_Codes;

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

extern struct TX_Payload Reset; 

extern void Disable_Alarm();

void MQTT_SetUp();

void MQTT_task(void* pvParameters);

void mqttPublisher(void* parameter);

void reconnect(bool onSetUp);

void setClock();

void mqttCallback(char* topic, byte* payload, unsigned int length);

void mqttHandler(void* pvParameters);

extern TaskHandle_t LED_Flash;

#endif