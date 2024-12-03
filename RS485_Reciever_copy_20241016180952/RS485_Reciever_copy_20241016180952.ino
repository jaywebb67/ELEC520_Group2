// Receiver Code
#include <SoftwareSerial.h>
 
// Define the pins for the MAX485
#define CE 4
#define Bus_Monitor_Pin 2
int BUS;
bool Bus_Busy = 0;
String LED = "LED";
 
// Create a SoftwareSerial object to communicate with the MAX485
SoftwareSerial RS485Serial(10, 11); // RX, TX

void Bus_Monitor_Pin_interrupt() {
  Bus_Busy = 1;
}
  
 void Transmit_To_Bus() {
  Bus_Busy = 0;
  delay(1);
  if(!Bus_Busy) {
    Serial.write();
  }
  else {
      delay(random(10; 50));
  }

 }

void setup() {
  randomSeed(analogRead(0));
  // Initialize the serial communication
  Serial.begin(9600);
  RS485Serial.begin(9600);
  // Set the DE and RE pins as outputs
  pinMode(CE, OUTPUT);
  pinMode(Bus_Monitor_Pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(Bus_Monitor_Pin), Bus_Monitor_Pin_interrupt, CHANGE);
  // Set DE and RE low to enable receiving mode
  digitalWrite(CE, LOW);
}
 
void loop() {
  if (RS485Serial.available()) {
    Serial.print("Hi");
    // Read the received data
    int receivedData = RS485Serial.read();
 
    // Print the received data to the serial monitor
    Serial.print("Temperature: ");
    Serial.println(receivedData);
 
    // Print a successful message
    Serial.println("Data successfully received.");

      digitalWrite(CE, HIGH);
  RS485Serial.write("LED");
  digitalWrite(CE, LOW);
  }
  // 
    BUS = digitalRead(8);
    Serial.print("Bus is: ");
    Serial.println(BUS);
    delay(100);

 
}
