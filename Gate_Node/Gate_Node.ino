////importing libraries for interfacing with the LCD and the Keypad:
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include "Communication_Protocol.h"
#include "BluetoothSerial.h"
#include <string>


// Check if Bluetooth configs are enabled
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Check Serial Port Profile
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Port Profile for Bluetooth is not available or not enabled. It is only available for the ESP32 chip.
#endif

uint8_t Entrycodes[100];

uint8_t Home_Address = 0x12;
uint8_t Destination_Address = 0x28;
uint8_t Node_Type = 0x10;
uint32_t Users_Present = 0;
bool first_user_flag = 0;

// // Handle received and sent messages
String message = "";
char incomingChar;

// Bluetooth Serial object
BluetoothSerial SerialBT;

////keypad setup
//keypad dimensions
const uint8_t rows = 4;
const uint8_t cols = 4;
//keypad button values
 char keys[rows][cols] = {
   {'1', '2', '3', 'A'},
   {'4', '5', '6', 'B'},
   {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
//setting the arduino pins that are used for the columns and rows
uint8_t colPins[cols] = {26, 25, 33, 32};
uint8_t rowPins[rows] = {13, 12, 14, 27};

//keypad initialisation
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);

////LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

////password management
int passwordLength = 6;
String setPassword = "123456";
String currentInput = "";
char key;
bool accessGranted = false;


void setup() {
  Serial.begin(9600);
  Comms_Set_Up();
  lcd.init();
  lcd.setBacklight(255);
  lcd.backlight();
  SerialBT.begin("ESP32-BT-Slave");
  Serial.println("The device started, now you can pair it with bluetooth!");
}

void loop() {
  if (currentInput.length() < passwordLength){
      getInput();
      keyPrompt();
  } else {
    checkPassword();
  }
  if (SerialBT.available()){                //should this be a while(incoming char != '\n')
    char incomingChar = SerialBT.read();
    if (incomingChar != '\n'){
      message += String(incomingChar);
    }
    else{
      message = "";
    }
  if(message != 0){
    checkPassword();
  }
  
  if()
  

}

void getInput(){
  key = keypad.getKey();
  if (key){
    currentInput += key;
    Serial.println(currentInput);
  }
}


void keyPrompt(){
  lcd.setCursor(0,0);
  lcd.print("   Enter Key:   ");
   // Calculate how many underscores to display
  int remainingSpaces = passwordLength - currentInput.length();

  lcd.setCursor(4,1);
  lcd.print("[");
  lcd.setCursor(11,1);
  lcd.print("]");
  lcd.setCursor(5,1);
  lcd.print(currentInput);
    // Display remaining underscores
  for (int i = 0; i < remainingSpaces; i++) {
    lcd.print("_");}
}

bool checkPassword(){
  bool Entry_Granted = 0;
    lcd.setCursor(0,0);
    lcd.print("     Access     ");
    lcd.setCursor(0,1);
    x = stoi(message);
    for(int i = 0; i < length(Entrycodes); i++){
      if(Entrycodes[i] == x){
        Entry_Granted = 1;
        return;
      }
    }
    if (Entry_Granted) {
      lcd.print("     Granted    ");
      delay(2);
      loop();       //why this loop
      return 1;
    } else 
    {
      lcd.print("     Denied     ");
      delay(2);
      loop();

    }
    return 0;
}

void get_Key_Code() {
  delay(10);
  pin_a = digitalRead();
  pin_b = digitalRead();
  pin_c = digitalRead();
  pin_d = digitalRead();
  pin_e = digitalRead();
  pin_f = digitalRead();
  pin_g = digitalRead();
  pin_h = digitalRead();

}







