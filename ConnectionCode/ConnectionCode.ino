#include <SoftwareSerial.h>

SoftwareSerialRS485Serial(10,11);

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
  RS485Serial.write("signal to adress 0")
}