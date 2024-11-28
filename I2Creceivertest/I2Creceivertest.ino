#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define I2C_SLAVE_ADDRESS 0x03 // Address of the slave device
#define Baud    9600

//LiquidCrystal_I2C lcd(0x27, 16, 2); // Change 0x27 to the address of your LCD
char Valid_Code[7] = {"012345"};
char receivedCode[7];
bool codeReceived = false;
char response;

// Forward declarations
void receiveEvent(int howMany);
void requestEvent();
void checkCode();

void setup() {
  Serial.begin(Baud);
  Wire.begin(I2C_SLAVE_ADDRESS);  // Initialize the I2C bus as a slave
  Wire.onReceive(receiveEvent);   // Register the receive event handler
  Wire.onRequest(requestEvent);   // Register the request event handler

  // // Initialize the LCD
  // lcd.init();
  // lcd.backlight();
  // lcd.clear();
  // lcd.setCursor(0, 0);
  // lcd.print("Waiting for code");
}

void loop() {
  if (codeReceived) {
    codeReceived = false;
    checkCode(); // Check the received code and set the response
    // updateLCD(); // Update the LCD based on the response
  }
}

void receiveEvent(int howMany) {
  for (int i = 0; i < 7; i++) {
    if (Wire.available()) {
      receivedCode[i] = Wire.read();
    }
  }
  codeReceived = true;
}

void requestEvent() {
  Wire.write(response);
}

void checkCode() {
  // Implement your code validation logic here
  if (strcmp((char*)receivedCode, Valid_Code) == 0) {
      response = 'Y';
  }
  else {
    response = 'N';
  }
  // if (isValidCode(receivedCode)) {
  //   response = 'Y'; // Code Accepted
  // } else if (isDeniedCode(receivedCode)) {
  //   response = 'N'; // Code Denied
  // } else {
  //   response = 'I'; // Invalid Code
  // }
}

// void updateLCD() {
//   lcd.clear();
//   if (response == 'Y') {
//     lcd.setCursor(0, 0);
//     lcd.print("Access");
//     lcd.setCursor(0, 1);
//     lcd.print("Granted");
//   } else if (response == 'N') {
//     lcd.setCursor(0, 0);
//     lcd.print("Access");
//     lcd.setCursor(0, 1);
//     lcd.print("Denied");
//   } else {
//     lcd.setCursor(0, 0);
//     lcd.print("Invalid");
//     lcd.setCursor(0, 1);
//     lcd.print("Input");
//   }
// }

// Dummy functions for code validation
bool isValidCode(char* code) {
  // Implement your code validation logic here
  return true;
}

bool isDeniedCode(char* code) {
  // Implement your code validation logic here
  return false;
}
