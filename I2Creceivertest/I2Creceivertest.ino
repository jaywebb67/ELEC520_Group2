#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define I2C_SLAVE_ADDRESS 0x03 // Address of the slave device
#define Baud 9600

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
}

void loop() {
  if (codeReceived) {
    codeReceived = false;
    checkCode(); // Check the received code and set the response
  }
}

void receiveEvent(int howMany) {
  int i = 0;
  while (Wire.available() && i < 6) {
    receivedCode[i] = Wire.read();
    i++;
  }
  receivedCode[i] = '\0'; // Null-terminate the string
  codeReceived = true;
}

void requestEvent() {
  Wire.write(response);
}

void checkCode() {
  // Implement your code validation logic here
  if (strcmp(receivedCode, Valid_Code) == 0) {
    response = 'Y';
  } else {
    response = 'N';
  }
}
