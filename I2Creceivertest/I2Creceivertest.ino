#include <Wire.h>
//#include <LiquidCrystal_I2C.h>

#define I2C_SLAVE_ADDRESS 0x03 // Address of the slave device
#define Baud 9600

char Valid_Code[7] = {"012345"};
char receivedCode[7];
bool codeReceived = false;
uint8_t response;
volatile uint32_t counter = 0;

// Forward declarations
void receiveEvent(int howMany);
void requestEvent();
int checkCode();

void setup() {
  Serial.begin(Baud);
  Wire.begin(I2C_SLAVE_ADDRESS);  // Initialize the I2C bus as a slave
  Wire.onReceive(receiveEvent);   // Register the receive event handler
  Wire.onRequest(requestEvent);   // Register the request event handler
  Serial.println("Setting up");
}

void loop() {
  if (codeReceived) {
    Serial.println("Checking code");
    codeReceived = false;
    response = checkCode(); // Check the received code and set the response
  }
}

void receiveEvent(int howMany) {
  Serial.print("I2C received: ");
  int i = 0;
  while (Wire.available() && i < 6) {
    receivedCode[i] = Wire.read();
    Serial.print(receivedCode[i]); // Print each received character
    i++;
  }
  receivedCode[i] = '\0'; // Null-terminate the string
  Serial.print(" Complete code: ");
  Serial.println(receivedCode); // Print the complete code
  codeReceived = true;
}

void requestEvent() {
  Serial.print("I2C request. Response: ");
  Serial.println(response);
  Wire.write(response);
  //response = '\0';
}

int checkCode() {
  response = '\0';
  if (strcmp(receivedCode, Valid_Code) == 0) {
    //response = 'Y';
    return 2;
  } else {
    //response = 'N';
    return 1;
  }
  Serial.print("Code checked response: ");
  Serial.println(response);
}
