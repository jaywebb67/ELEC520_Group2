#include "MQTT.hpp"

String stringLocation = "";
String clientId = "";
String ping = "";
bool restartFlag = false;
bool I_am_Forwarder = false;
bool deviceConfig = false;

TaskHandle_t mqttTaskHandle = NULL;
TaskHandle_t mqttPublishHandle = NULL;
TaskHandle_t mqttReceiveHandle = NULL;

QueueHandle_t mqttPublishQueue;
QueueHandle_t mqttMessageQueue;

// Create a CharBuffer object with 10 entries, each of size 6 characters 
CharBuffer Valid_Entrance_Codes(100, 15);
CharBuffer Current_Codes_In_Use(100, 15);

WiFiClientSecure client;
PubSubClient mqttClient(client);

// Move MQTT server details into the function
const char* mqttServer = "40c06ef97ec5427eb54aa49e5c03c12c.s1.eu.hivemq.cloud";
const uint16_t mqttPort = 8883;

// WiFi credentials
const char* ssid = "Jays_WiFi";//BT-CJC2PH";//
const char* password = "jaywebb1";//"NfECRbGtfV37Hd";//

//topic for recieving user credential updates
String usersTopic = "";

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

void MQTT_SetUp(){
    Serial.print("Attempting to connect to SSID (max 10 secs): ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    int attempts = 0;

    while (attempts < 10 && WiFi.status() != WL_CONNECTED) {
        //Serial.print(".");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected. Returning to set up 5 seconds...");
        vTaskDelay(pdMS_TO_TICKS(5000));
        return; // Restart the ESP32 to attempt a clean reconnection
    }

    Serial.println("WiFi connected");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (!LittleFS.begin()) {
        Serial.println("Failed to mount LittleFS");
        return;
    } 
    setClock();

    client.setCACert(isrg_root_x1_cert); // Use the certificate content
    mqttClient.setServer(mqttServer,mqttPort);
    mqttClient.setCallback(mqttCallback);

    mqttMessageQueue = xQueueCreate(15, sizeof(MqttMessage));
    if (mqttMessageQueue == NULL) {
        Serial.println("Failed to create mqttMessageQueue!");
        while (true); // Halt execution
    }

    mqttPublishQueue = xQueueCreate(10, 100); 

    // Check if the queue was created successfully
    if (mqttPublishQueue == NULL) {
      Serial.println("Failed to create MQTT publish queue");
      //while (1); // Stop execution
    }

    xTaskCreatePinnedToCore(mqttPublisher, "mqttPublisher", 5000, NULL, 1, &mqttPublishHandle, 1);
    if (mqttTaskHandle == NULL) {
        Serial.println("Failed to create mqttHandler task.");
    } else {
        Serial.println("mqttHandler task created successfully.");
    }

    xTaskCreatePinnedToCore(mqttHandler, "mqttHandler", 5000, NULL, 2, &mqttReceiveHandle, 1);
    if (mqttTaskHandle == NULL) {
        Serial.println("Failed to create mqttHandler task.");
    } else {
        Serial.println("mqttHandler task created successfully.");
    }
}


void MQTT_task(void* pvParameters) {
    reconnect(1); // Initial connection attempt
    unsigned long lastPingTime = millis();
    unsigned long lastConnectionAttempt = millis();

    while (true) {
        if (restartFlag) {
            esp_restart(); // Restart the ESP if the restart flag is set
        }

        // Check if MQTT client is connected
        if (mqttClient.connected()) {
            // Send ping message every 10 seconds
            if (millis() - lastPingTime >= 10000) {
                lastPingTime = millis();
                if (!mqttClient.publish("ELEC520/devicePing", ping.c_str())) {
                    // Uncomment for debugging
                    // Serial.println("Failed to send ping message");
                }
                mqttClient.loop(); // Ensure MQTT client processes incoming messages
            }
        } else {
            // Retry reconnection every 5 minutes
            if (millis() - lastConnectionAttempt >= 300000) { // 5 minutes
                lastConnectionAttempt = millis();
                reconnect(1); // Attempt to reconnect
            }
        }
    }
}


void mqttPublisher(void* parameter) {
  MqttMessage message; // Create a struct instance to hold topic and payload
  struct TX_Payload txPayload;
  while (true) {
    // Wait for an MqttMessage from the queue
    if (xQueueReceive(mqttPublishQueue, &message, portMAX_DELAY) == pdPASS) {
      // Publish the payload to the specified MQTT topic
      if(message.topic == "ELEC520/userAccess"){
        String payload = message.payload +" "+ clientId;
        mqttClient.publish(message.topic.c_str(), payload.c_str());
      }
      else if((message.topic == "ELEC520/alarm")){
        if(!mqttClient.connected()){
            uint8_t temp = Destination_Address;
            Destination_Address = 0x13;
            struct TX_Payload msg;
            if(message.payload == "Alarm Disabled"){
              msg = {14, 'Vuser'};
            }
            else{
              msg = {13, 'NVuser'};
            }
            Transmit_To_Bus(&msg);
            Destination_Address = temp;
        }
        else if((message.topic == "ELEC520/alarm")){
          if(!mqttClient.connected()){
              uint8_t temp = Destination_Address;
              Destination_Address = 0x13;
              struct TX_Payload msg;
              if(message.payload == "Alarm Disabled"){
                msg = {14, 'Alarm Disabled'};
              }
              else{
                msg = {13, 'Alarm Enabled'};
              }
              Transmit_To_Bus(&msg);
              Destination_Address = temp;
          }
          else{
            mqttClient.publish(message.topic.c_str(), message.payload.c_str());
          }
        }
        else if(!mqttClient.connected()){
          txPayload.length = min(message.payload.length(), sizeof(message.payload) - 1); // Set length
          strncpy(txPayload.message, message.payload.c_str(), txPayload.length);          // Copy payload
          txPayload.message[txPayload.length] = '\0'; // Ensure null-termination
          uint8_t temp = Destination_Address;
          Destination_Address = MQTT_Address;
          Transmit_To_Bus(&txPayload);
          Destination_Address = temp;
        }
        
      // Log the published message
      // Serial.print("Published to topic: ");
      // Serial.print(message.topic.c_str());
      // Serial.print(" | Payload: ");
      // Serial.println(message.payload);
    

    }
  }
}





void reconnect(bool onSetUp) {
    if(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, password);
      int attempts = 0;

      while (attempts < 10 && WiFi.status() != WL_CONNECTED) {
          Serial.print(".");
          vTaskDelay(5000 / portTICK_PERIOD_MS);
          attempts++;
      }
      if(WiFi.status() != WL_CONNECTED){
        return;
      }
      setClock();
      client.setCACert(isrg_root_x1_cert); // Use the certificate content
      Serial.println("Starting connection to MQTT server...");
      mqttClient.setCallback(mqttCallback);
      if (mqttMessageQueue == NULL) {
          mqttMessageQueue = xQueueCreate(10, sizeof(MqttMessage));
          if (mqttMessageQueue == NULL) {
              Serial.println("Failed to create mqttMessageQueue!");
              while (true); // Halt execution
          }
      }
      if (mqttTaskHandle == NULL) {
          xTaskCreatePinnedToCore(mqttHandler, "mqttHandler", 5000, NULL, 1, &mqttTaskHandle, 1);
          if (mqttTaskHandle == NULL) {
              Serial.println("Failed to create mqttHandler task.");
          } else {
              Serial.println("mqttHandler task created successfully.");
          }
      }
    }
    if(LittleFS.exists("/userCredentials.txt")) {
        File file = LittleFS.open("/userCredentials.txt", "r");
        if (file) {
            Serial.println("Reading user credentials from file...");

            while (file.available()) {
                String line = file.readStringUntil('\n'); // Read each line until newline
                line.trim(); // Remove trailing whitespace or newlines

                if (!line.isEmpty()) {
                    char msg[15]; // Buffer to store the credential as a C-string
                    line.toCharArray(msg, 15); // Convert String to char array
                    Valid_Entrance_Codes.addEntry(msg); // Add the entry to the buffer
                    Serial.print("User Credential added to buffer: ");
                    Serial.println(msg); // Debugging: Print the added code
                }
            }

            file.close(); // Close the file after reading
            Serial.println("Finished loading user credentials.");
        } else {
            Serial.println("Failed to open userCredentials.txt");
        }
    }
    // Check and load device ID
    if (LittleFS.exists("/deviceConfig.txt")) {
        deviceConfig = false;
        File file = LittleFS.open("/deviceConfig.txt", "r");
        if (file) {
            String line = file.readStringUntil('\n'); // Read the line as a string
            file.close();
            line.trim(); // Remove any trailing newline or whitespace

            int separatorIndex = line.indexOf(':'); // Find the index of the colon separator
            if (separatorIndex != -1) {
                clientId = line.substring(0, separatorIndex); // Extract client ID
                stringLocation = line.substring(separatorIndex + 1); // Extract stringLocation
                clientId.trim(); // Trim any whitespace
                stringLocation.trim(); // Trim any whitespace

                ping = clientId + " online";
                usersTopic = "ELEC520/users/update/" + clientId;

                Serial.print("Loaded client ID: ");
                Serial.println(clientId);
                Serial.print("Loaded stringLocation: ");
                Serial.println(stringLocation);
            } else {
                Serial.println("Invalid format in deviceConfig.txt. Expected 'clientID:stringLocation'.");

            }
        } else {
            Serial.println("Failed to read deviceConfig.txt");
        }
    } else {
        clientId = "Gate" + String(random(0xff), HEX);
        stringLocation = "Unknown"; // Default stringLocation if file doesn't exist
        Serial.print("Generated client ID: ");
        Serial.println(clientId);
        Serial.print("Default stringLocation: ");
        Serial.println(stringLocation);
        deviceConfig = true;
    }

    if (mqttClient.connected()) {
        mqttClient.disconnect();
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    if (!mqttClient.connected()) {
        String willMessage = clientId + " offline";
        if (mqttClient.connect(clientId.c_str(), "gateID", "Gatepass1", "ELEC520/devicePing", 0, true, willMessage.c_str())) {
            Serial.println("MQTT connected");

            if (deviceConfig) { 
                if (mqttClient.subscribe("ELEC520/devices/update", 0)) {
                    Serial.println("Subscribed to ELEC520/devices/update with QoS 0");
                } else {
                    Serial.println("Failed to subscribe to ELEC520/devices/update");
                }
                mqttClient.publish("ELEC520/devices/view", "Gate");
            }
            if (onSetUp) {
                if (mqttClient.subscribe("ELEC520/users/updateNeeded", 0)) {
                    Serial.println("Subscribed to ELEC520/users/updateNeeded with QoS 0");
                } else {
                    Serial.println("Failed to subscribe to ELEC520/users/updateNeeded");
                }
                mqttClient.publish("ELEC520/users/needUpdate", clientId.c_str());
            }
            mqttClient.subscribe("ELEC520/alarm", 0);
            mqttClient.subscribe("ELEC520/deviceConfig", 0);
            mqttClient.subscribe("ELEC520/users/add",0);
            mqttClient.subscribe("ELEC520/users/view",0);
            mqttClient.subscribe("ELEC520/forwarder",0);            
        } else {
            Serial.print("MQTT connection failed, rc=");
            Serial.println(mqttClient.state());
            Serial.println("Retrying in 5 seconds");
        }
    }
}

void setClock() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");

    Serial.print("Waiting for NTP sync");
    while (time(nullptr) < 8 * 3600 * 2) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        Serial.print(F("."));
    }
    Serial.println();

    setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0/2", 1);
    tzset();

    struct tm timeinfo;
    getLocalTime(&timeinfo);
    Serial.print("Current time: ");
    Serial.println(asctime(&timeinfo));
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char payloadBuffer[256];
    strncpy(payloadBuffer, (char*)payload, length);
    payloadBuffer[length] = '\0';

    MqttMessage msg = {String(topic), String(payloadBuffer)};
    if (xQueueSend(mqttMessageQueue, &msg, portMAX_DELAY) != pdTRUE) {
      //  Serial.println("Failed to send message to queue");
    }
}

void mqttHandler(void* pvParameters) {
    MqttMessage receivedMsg;

    while (true) {
        if (xQueueReceive(mqttMessageQueue, &receivedMsg, portMAX_DELAY) == pdTRUE) {
            if (receivedMsg.topic == "ELEC520/users/add") {
                File file = LittleFS.open("/userCredentials.txt", "a");
                if (file) {
                    file.println(receivedMsg.payload);
                    file.flush();
                    file.close();
                    Serial.println("User credential added.");
                } else {
                    Serial.println("Failed to open userCredentials.txt");
                }
            }
            else if (receivedMsg.topic == "ELEC520/users/view") {
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
                File file = LittleFS.open("/deviceConfig.txt", "w");
                if (file) {
                    // Write the payload in 'user:password' format to the file
                    file.print(receivedMsg.payload); // Each entry goes on a new line
                    file.print(":Unknown");
                    file.flush();  // Ensure data is written immediately
                    file.close();
                    Serial.println("Device ID written to file: " + receivedMsg.payload);
                    clientId = receivedMsg.payload;
                    ping = clientId + " online"; 
                    usersTopic = "ELEC520/users/update/" + clientId;
                    
                } else {
                    Serial.println("Failed to open and write 'deviceID.txt'.");
                }
      
                restartFlag = 1;
            }
            else if (receivedMsg.topic == "ELEC520/forwarder") {
              Serial.println(receivedMsg.payload);
              if(receivedMsg.payload == clientId.c_str()){
                I_am_Forwarder = true;
              }
              else{
                I_am_Forwarder = false;
              }
            
            }
            else if (receivedMsg.topic == "ELEC520/users/updateNeeded") {
                if(receivedMsg.payload == "1"){
                  if (mqttClient.subscribe(usersTopic.c_str(), 0)) {
                      Serial.print("Subscribed to ");
                      Serial.print(usersTopic.c_str());
                      Serial.println(" with QoS 0");
                  } else {
                      Serial.println("Failed to subscribe to ELEC520/users/update");
                  }
                  mqttClient.publish("ELEC520/users/view", clientId.c_str());
                }
                else {
                    Serial.println("No update needed");
                }

            }
            else if (receivedMsg.topic == usersTopic) {
                Serial.println(receivedMsg.payload);
                // Check if payload starts with 'lastUpdate:'
                if (!receivedMsg.payload.startsWith("lastUpdated:")) {
                    // Ignore this message and exit the processing block
                  char msg[15];
                  receivedMsg.payload.toCharArray(msg, 15);
                  Valid_Entrance_Codes.addEntry(msg);  
                  int separatorIndex = receivedMsg.payload.indexOf(':');  // Assuming a colon delimiter

                  if (separatorIndex != -1) {
                      // Open the credentials file in append mode
                      File file = LittleFS.open("/userCredentials.txt", "a");
                      //file.setBufferSize(1024);  // Set buffer size if needed

                      if (file) {
                          // Write username, then colon, then password to the same line but separately
                          file.println(receivedMsg.payload); // Write the password and move to a new line
                          file.flush();  // Ensure all data is written to storage
                          // Close the file
                          file.close();
                          Serial.println("User credentials updated in file.");
                      
                      } else {
                          Serial.println("Failed to open /userCredentials.txt for writing.");
                      }
                  } else {
                        Serial.println("Invalid payload format. Expected 'username:password'");
                  }    
                }  
            }
            else if (receivedMsg.topic == "ELEC520/deviceConfig") {
              Serial.println(receivedMsg.payload);
              // Clear the file at the start when viewing/updating users from database
              File file = LittleFS.open("/deviceConfig.txt", "w");
              if (file) {
                  String line = ""; // Read the line as a string
                  receivedMsg.payload.trim();
                  file.println(receivedMsg.payload); // Write the password and move to a new line
                  file.flush();  // Ensure all data is written to storage
                  // Close the file
                  file.close();
                  line =  receivedMsg.payload;// Remove any trailing newline or whitespace

                  int separatorIndex = line.indexOf(':'); // Find the index of the colon separator
                  if (separatorIndex != -1) {
                      clientId = line.substring(0, separatorIndex); // Extract client ID
                      stringLocation = line.substring(separatorIndex + 1); // Extract stringLocation
                      clientId.trim(); // Trim any whitespace
                      stringLocation.trim(); // Trim any whitespace
                      Serial.print("Loaded client ID: ");
                      Serial.println(clientId);
                      Serial.print("Loaded stringLocation: ");
                      Serial.println(stringLocation);
                  } else {
                      Serial.println("Invalid format in deviceConfig.txt. Expected 'clientID:stringLocation'.");
                  }
              } else {
                  Serial.println("Failed to open /deviceConfig.txt for clearing.");
              }
            }
            else if (receivedMsg.topic == "ELEC520/alarm") {
                Serial.println(receivedMsg.payload);
            }
            receivedMsg.topic = "";
            receivedMsg.payload = "";
        }
    }
}

// void copy_keycode(const char* source, char* dest){
//   size_t length = strlen(source);
//   size_t FirstChar = (length>6) ? length-6 : 0;
//   memcpy(dest, source + FirstChar, 6);
//   dest[6] = '\0';
// }
