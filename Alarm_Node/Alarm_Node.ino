#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Communication_Protocol.h"
//#include <freertos/semphr.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <LittleFS.h>


const uint8_t Home_Node_Type = 0x35;
const uint8_t Home_Address = 0x13;
uint8_t Destination_Address = 0x28;

// Define a queue to hold messages
QueueHandle_t messageQueue;
QueueHandle_t RX_Queue;

TaskHandle_t RX_Message_Handle;

String payload = "";

// MQTT server details
const char* mqttServer = "40c06ef97ec5427eb54aa49e5c03c12c.s1.eu.hivemq.cloud";  // MQTT broker IP or URL
const int mqttPort = 8883;                    // Port for secure MQTT (typically 8883)
String clientId = "";
// WiFi credentials
// Update these with values suitable for your network.
const char* ssid = "BT-CJC2PH";//"Jays_WiFi";       // your network SSID (WiFi name)
const char* password = "NfECRbGtfV37Hd";//"jaywebb1";   // your network password

//NetworkClientSecure client;
NetworkClientSecure client;
PubSubClient mqttClient(client);

//SemaphoreHandle_t xMutex;


bool alarmEnabled = false;
bool restartFlag = 0;

void setClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov","time.google.com");
  
  // Wait for time synchronization
  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  
  // Set timezone to GMT0 with BST daylight saving rules
  setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0/2", 1);
  tzset();  // Apply the new TZ environment setting

  struct tm timeinfo;
  getLocalTime(&timeinfo);  // Use getLocalTime for better accuracy in ESP32
  Serial.print(F("Current local time: "));
  Serial.print(asctime(&timeinfo));
}

void reconnect(int onSetUp) {
  // Check if `deviceID.txt` exists in LittleFS
  if (LittleFS.exists("/deviceID.txt")) {
    // Open the file in read mode
    File file = LittleFS.open("/deviceID.txt", "r");
    if (file) {
      // Read the device ID from the file and set it as `clientID`
      clientId = file.readStringUntil('\n');  // Read the device ID as a String
      clientId.trim();  // Remove any trailing newline or whitespace
      file.close();
      Serial.println("Loaded client ID from deviceID.txt: " + clientId);
    } else {
      Serial.println("Failed to open deviceID.txt for reading.");
    }
  } else {
    // If `deviceID.txt` does not exist, generate a random client ID
    clientId = "Alarm" + String(random(0xffff), HEX);
    Serial.println("No deviceID.txt found. Generated random client ID: " + clientId);
  }
  if(mqttClient.connected()){
    mqttClient.disconnect();
    delay(500);
  }

  // Attempt to connect to the MQTT broker with the client ID
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection with clientId: ");
    Serial.println(clientId);
    if (mqttClient.connect(clientId.c_str(), "alarm", "alarmpassword")) {
      Serial.println("connected");
      //delay(100);
      // Subscribe to relevant topics based on setup phase
      if (onSetUp) {
        if (!LittleFS.exists("/deviceID.txt")){
          Serial.println("Initialising deviceID");
          mqttClient.subscribe("ELEC520/devices/update", 1);
          //delay(100);
          mqttClient.publish("ELEC520/devices/view", "Alarm");
        }
      }
      mqttClient.subscribe("ELEC520/#",1);
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(1000);
    }
  }
}

// Callback function that runs on Core 0
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    MqttMessage msg;
    msg.topic = String(topic); // Ensure the topic is correctly stored
    for (int i = 0; i < length; i++) {
        msg.payload += (char)payload[i];
    }
    // Serial.print("Received topic: ");
    // Serial.println(msg.topic);
    // Serial.print("Received payload: ");
    // Serial.println(msg.payload);
    if (xQueueSend(messageQueue, &msg, portMAX_DELAY) != pdTRUE) {
        Serial.println("Failed to send message to queue");
    }
}


// Task function to process messages on Core 1
void mqttHandler(void* pvParameters) {
  MqttMessage receivedMsg;
  
  while (true) {
    // Check if there’s a message in the queue
    if (xQueueReceive(messageQueue, &receivedMsg, portMAX_DELAY) == pdTRUE) {

      // Add additional processing code here if needed
      // Check if the message is for adding a new user credential

      if (receivedMsg.topic == "ELEC520/users/view") {
        // Clear the file at the start when viewing/updating users from database
        File file = LittleFS.open("/userCredentials.txt", "w");
        if (file) {
            file.close();  // Close immediately to clear contents
            Serial.println("User credentials file cleared.");
        } else {
            Serial.println("Failed to open /userCredentials.txt for clearing.");
        }
      }
      else if (receivedMsg.topic == "ELEC520/devices/update") {
        Serial.println(receivedMsg.payload);
        // Open the credentials file in append mode
        File file = LittleFS.open("/deviceID.txt", "w");
        if (file) {
          // Write the payload in 'user:password' format to the file
          file.println(receivedMsg.payload); // Each entry goes on a new line
          file.flush();  // Ensure data is written immediately
          Serial.println("Device ID written to file: " + receivedMsg.payload);
          clientId = receivedMsg.payload;
          file.close();
          return;
        } else {
          Serial.println("Failed to open and write 'deviceID.txt'.");
        }
        file.close();
        restartFlag = 1;
      }
      else if (receivedMsg.topic == "ELEC520/test") {
        Serial.println(receivedMsg.payload);
      }
      else if (receivedMsg.topic == "ELEC520/alarm") {
        Serial.println(receivedMsg.payload);
        receivedMsg.payload.trim();
        if (receivedMsg.payload == "Alarm Disabled"){
          //Serial.println("Alarm disarmed");
          alarmEnabled = false;
        }
        else{
          //Serial.println("Alarm disarmed");
          alarmEnabled = true;
          // Serial.print("alarmEnabled is set to: ");
          // Serial.println(alarmEnabled);
        } 
      }
    }
  }
}

void setup(){
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  delay(100);

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(".");
    // wait 1 second for re-trying
    delay(1000);
  }
  Serial.println("WiFi connected");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  setClock();

  client.setCACert(isrg_root_x1_cert);
  //client.setInsecure();
  Serial.println("\nStarting connection to server...");
  if (!client.connect(mqttServer, mqttPort)) {
    Serial.println("Connection failed!");
  }else{
    Serial.println("Connected to server!");
  }
  mqttClient.setBufferSize(512);
  mqttClient.setCallback(mqttCallback);
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return;
  } 
  Serial.println("LittleFS mounted successfully");

  // Create a queue to hold up to 10 messages
  messageQueue = xQueueCreate(10, sizeof(MqttMessage));
  
  // xMutex = xSemaphoreCreateMutex();

  // // Check if mutex was created successfully
  // if (xMutex == NULL) {
  //     // Mutex creation failed
  //     Serial.println("Mutex creation failed!");
  // } else {
  //     // Mutex created successfully
  //     Serial.println("Mutex created successfully!");
  // }

  // Create a task that runs on Core 0
  xTaskCreatePinnedToCore(
    mqttHandler,            // Function to run
    "mqttHandler",         // Name of the task
    10000,            // Stack size (in words)
    NULL,             // Task input parameter
    1,                // Priority of the task
    NULL,             // Task handle
    0);               // Core 0
    reconnect(1);
}


void loop() {
  // your code to interact with the server here
  if (restartFlag){
    delay(1000);
    esp_restart();
  }

  if (!mqttClient.connected()) {
    reconnect(0);
  }
  if(alarmEnabled == true){
    tone(speakerPin,400,500);
    delay(1000);
  }
  // if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
  //   if(alarmEnabled){
  //     xSemaphoreGive(xMutex);  
  //   }
  // }
  mqttClient.loop();

}
