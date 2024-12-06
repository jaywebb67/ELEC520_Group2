#ifndef MQTT_HPP
#define MQTT_HPP

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "Char_Buffer.h"
#include "Communication_Protocol.h"

#define QUEUE_WAIT_TIME pdMS_TO_TICKS(100) // Wait for 100 milliseconds

//NetworkClientSecure client;
extern WiFiClientSecure client;
extern PubSubClient mqttClient;

extern TaskHandle_t mqttTaskHandle;
extern TaskHandle_t mqttReceiveHandle;

extern QueueHandle_t mqttPublishQueue;

extern bool I_am_Forwarder;
extern bool restartFlag;
extern String clientId;
extern String ping;

extern struct TX_Payload Reset; 

// Create a CharBuffer object with 10 entries, each of size 6 characters 
extern CharBuffer Valid_Entrance_Codes;
extern CharBuffer Current_Codes_In_Use;


struct MqttMessage {
  String topic;
  String payload;  
};

void MQTT_SetUp();

void MQTT_task(void* pvParameters);

void mqttPublisher(void* parameter);

void reconnect(bool onSetUp);

void setClock();

void mqttCallback(char* topic, byte* payload, unsigned int length);

void mqttHandler(void* pvParameters);



#endif