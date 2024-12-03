#include <SoftwareSerial.h>


SoftwareSerial RS485Serial(10,11);

unsigned char myAddress = 255;
String myPurpose = "gate";
char* message;

const int whitePin = 4;



void setup() {
  Serial.begin(9600);
  RS485Serial.begin(9600);
  ConnectToNetwork();
}

void loop() {
  // put your main code here, to run repeatedly:

}

void ConnectToNetwork(){
  //Serial.print("signal to adress 0");
  //assembleMessage(myAddress, myPurpose);

  RS485Serial.write("purpose and address");

  while (myAddress == 255) {
    digitalWrite(whitePin, HIGH); // sets the LED on
    delay(500);                // waits for a second
    digitalWrite(whitePin, LOW);  // sets the LED off
    delay(500);
    if (true){ // RS485Serial.available()
      String receivedData = RS485Serial.readString();
      message = undefinedFunction(receivedData);
      Serial.println((unsigned int)(message[2]));
      if ((unsigned int)message[2] == 65535) {
        myAddress = 100;
        digitalWrite(whitePin, HIGH); 
        Serial.println("hello");
      }
    }

  }

}

char* undefinedFunction(String data){
  char msg[4] = {0, 0, 255, 0};
  return msg;
}

