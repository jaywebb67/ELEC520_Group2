#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/semphr.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <LittleFS.h>

// ISRG Root X1 certificate (PEM format)
const char* isrg_root_x1_cert = R"literal(-----BEGIN CERTIFICATE-----
MIIFBjCCAu6gAwIBAgIRAIp9PhPWLzDvI4a9KQdrNPgwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAw
WhcNMjcwMzEyMjM1OTU5WjAzMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg
RW5jcnlwdDEMMAoGA1UEAxMDUjExMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB
CgKCAQEAuoe8XBsAOcvKCs3UZxD5ATylTqVhyybKUvsVAbe5KPUoHu0nsyQYOWcJ
DAjs4DqwO3cOvfPlOVRBDE6uQdaZdN5R2+97/1i9qLcT9t4x1fJyyXJqC4N0lZxG
AGQUmfOx2SLZzaiSqhwmej/+71gFewiVgdtxD4774zEJuwm+UE1fj5F2PVqdnoPy
6cRms+EGZkNIGIBloDcYmpuEMpexsr3E+BUAnSeI++JjF5ZsmydnS8TbKF5pwnnw
SVzgJFDhxLyhBax7QG0AtMJBP6dYuC/FXJuluwme8f7rsIU5/agK70XEeOtlKsLP
Xzze41xNG/cLJyuqC0J3U095ah2H2QIDAQABo4H4MIH1MA4GA1UdDwEB/wQEAwIB
hjAdBgNVHSUEFjAUBggrBgEFBQcDAgYIKwYBBQUHAwEwEgYDVR0TAQH/BAgwBgEB
/wIBADAdBgNVHQ4EFgQUxc9GpOr0w8B6bJXELbBeki8m47kwHwYDVR0jBBgwFoAU
ebRZ5nu25eQBc4AIiMgaWPbpm24wMgYIKwYBBQUHAQEEJjAkMCIGCCsGAQUFBzAC
hhZodHRwOi8veDEuaS5sZW5jci5vcmcvMBMGA1UdIAQMMAowCAYGZ4EMAQIBMCcG
A1UdHwQgMB4wHKAaoBiGFmh0dHA6Ly94MS5jLmxlbmNyLm9yZy8wDQYJKoZIhvcN
AQELBQADggIBAE7iiV0KAxyQOND1H/lxXPjDj7I3iHpvsCUf7b632IYGjukJhM1y
v4Hz/MrPU0jtvfZpQtSlET41yBOykh0FX+ou1Nj4ScOt9ZmWnO8m2OG0JAtIIE38
01S0qcYhyOE2G/93ZCkXufBL713qzXnQv5C/viOykNpKqUgxdKlEC+Hi9i2DcaR1
e9KUwQUZRhy5j/PEdEglKg3l9dtD4tuTm7kZtB8v32oOjzHTYw+7KdzdZiw/sBtn
UfhBPORNuay4pJxmY/WrhSMdzFO2q3Gu3MUBcdo27goYKjL9CTF8j/Zz55yctUoV
aneCWs/ajUX+HypkBTA+c8LGDLnWO2NKq0YD/pnARkAnYGPfUDoHR9gVSp/qRx+Z
WghiDLZsMwhN1zjtSC0uBWiugF3vTNzYIEFfaPG7Ws3jDrAMMYebQ95JQ+HIBD/R
PBuHRTBpqKlyDnkSHDHYPiNX3adPoPAcgdF3H2/W0rmoswMWgTlLn1Wu0mrks7/q
pdWfS6PJ1jty80r2VKsM/Dj3YIDfbjXKdaFU5C+8bhfJGqU3taKauuz0wHVGT3eo
6FlWkWYtbt4pgdamlwVeZEW+LM7qZEJEsMNPrfC03APKmZsJgpWCDWOKZvkZcvjV
uYkQ4omYCTX5ohy+knMjdOmdH9c7SpqEWBDC86fiNex+O0XOMEZSa8DA
-----END CERTIFICATE-----)literal";

// Define a queue to hold messages
QueueHandle_t messageQueue;

// Struct to store topic and message information
struct MqttMessage {
  String topic = "";
  String payload = "";
};

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

SemaphoreHandle_t xMutex;

bool alarmEnabled = false;


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
    clientId = "Gate" + String(random(0xffff), HEX);
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
    if (mqttClient.connect(clientId.c_str(), "esp32", "sub123")) {
      Serial.println("connected");
      delay(100);
      // Subscribe to relevant topics based on setup phase
      if (onSetUp) {
        if (!LittleFS.exists("/deviceID.txt")){
          Serial.println("Initialising deviceID");
          mqttClient.subscribe("ELEC520/devices/update", 1);
          delay(100);
          mqttClient.publish("ELEC520/devices/view", "Gate");
        }
      }
      mqttClient.subscribe("ELEC520/#",1);
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Callback function that runs on Core 0
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  MqttMessage msg;

  // Convert payload to String
  for (int i = 0; i < length; i++) {
    msg.payload += (char)payload[i];
  }

  // Send the message to the queue
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
      // Serial.print("Topic: ");
      // Serial.print(receivedMsg.topic);
      // Serial.print(", Message: ");  
      
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
        delay(1000);
        esp_restart();
      }
      else if (receivedMsg.topic == "ELEC520/test") {
        Serial.println(receivedMsg.payload);
      }
      else if (receivedMsg.topic == "ELEC520/alarm") {
        Serial.println(receivedMsg.payload);
        String payload = receivedMsg.payload;
        if (payload == "Alarm Disabled"){
          alarmEnabled == false;
        }
        else{
          alarmEnabled == true;
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
  
  xMutex = xSemaphoreCreateMutex();

  // Check if mutex was created successfully
  if (xMutex == NULL) {
      // Mutex creation failed
      Serial.println("Mutex creation failed!");
  } else {
      // Mutex created successfully
      Serial.println("Mutex created successfully!");
  }

  // Create a task that runs on Core 0
  xTaskCreatePinnedToCore(
    mqttHandler,            // Function to run
    "mqttHandler",         // Name of the task
    10000,            // Stack size (in words)
    NULL,             // Task input parameter
    1,                // Priority of the task
    NULL,             // Task handle
    0);               // Core 1
    reconnect(1);
}


void loop() {
  // your code to interact with the server here
  if (!mqttClient.connected()) {
    reconnect(0);
  }
  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    if(alarmEnabled){
      xSemaphoreGive(xMutex);  
    }
  }
  mqttClient.loop();

}
