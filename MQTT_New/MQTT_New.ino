#include <LiquidCrystal_I2C.h>

#include <WiFi.h>

#include <Keypad.h>

#include "./MQTT.hpp"



// WiFi credentials
// Update these with values suitable for your network.
const char* ssid = "Galaxy S20+ 5G 1bc4";  //       // your network SSID (WiFi name)
const char* password = "jaywebb1"; //your network password




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
        if (delimiterPos == -1) {
            continue; // Skip invalid lines
        }
        
        // Extract username and password from the line
        String username = line.substring(0, delimiterPos);
        String password = line.substring(delimiterPos + 1);
        password.trim(); // Remove newline and whitespace

        // Check if entered password matches any stored password
        if (strcmp(currentInput, password.c_str()) == 0) {
            lcd.print("     Granted    ");
            String message = username + " " + clientId ;  // Customize message for MQTT
            mqttClient.publish("ELEC520/userAccess", message.c_str());  // Publish username
            mqttClient.publish("ELEC520/alarm", "Alarm Disabled");  // Publish username
            matchFound = true;
            delay(1000);
            break;
        }
    }
  
    file.close();

    if (!matchFound) {
        lcd.print("     Denied     ");
        mqttClient.publish("ELEC520/alarm", "Alarm Enabled");  // Publish username
        delay(1000);
    }

    // Reset currentInput for the next attempt
    memset(currentInput, 0, sizeof(currentInput));
    idx = 0;
}




void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);


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
    1);               // Core 1
    reconnect(1);
}


void loop() {
  // your code to interact with the server here
  if (restartFlag){
    esp_restart();
  }

  if (!mqttClient.connected()) {
    reconnect(0);
  }
  if (idx < passwordMaxLength){
      getInput();
      keyPrompt();
  } else {
    checkPassword();
  }

}


