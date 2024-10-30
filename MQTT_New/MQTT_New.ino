#include <LiquidCrystal_I2C.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Keypad.h>
#include <cstring>  // Ensure this library is included for strcmp and memset
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
  String topic;
  String payload;
};

// MQTT server details
const char* mqttServer = "40c06ef97ec5427eb54aa49e5c03c12c.s1.eu.hivemq.cloud";  // MQTT broker IP or URL
const int mqttPort = 8883;                    // Port for secure MQTT (typically 8883)

// WiFi credentials
// Update these with values suitable for your network.
const char* ssid = "BT-CJC2PH";       // your network SSID (WiFi name)
const char* password = "NfECRbGtfV37Hd";   // your network password

//NetworkClientSecure client;
NetworkClientSecure client;
PubSubClient mqttClient(client);

////keypad setup
//keypad dimensions
const byte rows = 4;
const byte cols = 4;
//keypad button values
 char keys[rows][cols] = {
   {'1', '2', '3', 'A'},
   {'4', '5', '6', 'B'},
   {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

//setting the arduino pins that are used for the columns and rows
byte colPins[cols] = {26, 25, 33, 32};
byte rowPins[rows] = {13, 12, 14, 27};

//keypad initialisation
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);

////LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

////password management
const int passwordMaxLength = 6;
String setPassword = "123456";  // Define setPassword as a String
char currentInput[passwordMaxLength] = {0};

char key;
bool accessGranted = false;
int idx = 0; 

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

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect("esp32","esp32","sub123")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic","hello world");
      // ... and resubscribe
      mqttClient.subscribe("esp32/test",1);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void getInput(){
  key = keypad.getKey();
  if (key){
    currentInput[idx] = key;
    idx++;
  }
}


void keyPrompt(){
  lcd.setCursor(0,0);
  lcd.print("   Enter Key:   ");
   // Calculate how many underscores to display
  int remainingSpaces = passwordMaxLength - idx;

  lcd.setCursor(4,1);
  lcd.print("[");
  lcd.setCursor(11,1);
  lcd.print("]");
  lcd.setCursor(5,1);
  lcd.print(currentInput);
    // Display remaining underscores
  if(idx<6){
    for (int i = 0; i < remainingSpaces; i++) {
      lcd.print("_");
    }
  }
}


void checkPassword() {
    lcd.setCursor(0, 0);
    lcd.print("     Access     ");
    lcd.setCursor(0, 1);

    // Open the credentials file from LittleFS
    File file = LittleFS.open("/userCredentials.txt", "r");
    if (!file) {
        Serial.println("Failed to open credentials file");
        lcd.print("   Error       ");
        delay(1000);
        return;
    }

    bool matchFound = false;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        int delimiterPos = line.indexOf(':');
        
        // Extract username and password from the line
        String username = line.substring(0, delimiterPos);
        String password = line.substring(delimiterPos + 1);
        password.trim(); // Remove newline and whitespace

        // Check if entered password matches any stored password
        if (strcmp(currentInput, password.c_str()) == 0) {
            lcd.print("     Granted    ");
            String message = username + " logged in";  // Customize message for MQTT
            mqttClient.publish("esp32/test", message.c_str());  // Publish username
            matchFound = true;
            delay(1000);
            break;
        }
    }

    file.close();

    if (!matchFound) {
        lcd.print("     Denied     ");
        delay(1000);
    }

    // Reset currentInput for the next attempt
    memset(currentInput, 0, sizeof(currentInput));
    idx = 0;
}






// Callback function that runs on Core 0
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  MqttMessage msg;
  msg.topic = String(topic);
  msg.payload = "";

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
      Serial.print("Processing message on Core 0 - Topic: ");
      Serial.print(receivedMsg.topic);
      Serial.print(", Message: ");
      Serial.println(receivedMsg.payload);
      
      // Add additional processing code here if needed
    }
  }
}

void setup() {
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

  lcd.init();
  lcd.setBacklight(255);
  lcd.backlight();

  // Create a queue to hold up to 10 messages
  messageQueue = xQueueCreate(10, sizeof(MqttMessage));
  
  // Create a task that runs on Core 0
  xTaskCreatePinnedToCore(
    mqttHandler,            // Function to run
    "mqttHandler",         // Name of the task
    10000,            // Stack size (in words)
    NULL,             // Task input parameter
    1,                // Priority of the task
    NULL,             // Task handle
    0);               // Core 1
}


void loop() {
  // your code to interact with the server here
  if (!mqttClient.connected()) {
    reconnect();
  }
  if (idx < passwordMaxLength){
      getInput();
      keyPrompt();
  } else {
    checkPassword();
  }
  mqttClient.loop();
}


