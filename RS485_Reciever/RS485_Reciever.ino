// Receiver Code
#include <SoftwareSerial.h>
 


int BUS;
String LED = "LED";

//****** TX PROTOCOL SECTION******
// Define the pins for the MAX485 and bus monitor
#define CE 4
#define Bus_Monitor_Pin 2

//global variable for bus busy
bool Bus_Busy = 0;
// ISR for bus monitoring pin
void Bus_Monitor_Pin_interrupt() {
  Bus_Busy = 1;
}
  

//function to determine if the RS485 bus is busy
bool Collision_Avoidance() {
  Bus_Busy = 0;
  delay(1);
  if(!Bus_Busy) {
    return 1;
  }
  else {
    delay(random(100, 200));
    if(!Bus_Busy) {
      return 1;
    }
    else {
      delay(random(200, 300));
      if(!Bus_Busy) {
        return 1;
      }
      else {
        delay(random(300, 500));
        if(!Bus_Busy) {
          return 1;
        }
        else {
          delay(random(500, 1000));
          if(!Bus_Busy) {
            return 1;
          }
          else
          return 0;
        }
      }
    }
  }
}

//Tx protocol function
bool Transmit_To_Bus() {
  bool Safe_To_Transmit = Collision_Avoidance();

  if(Safe_To_Transmit){
    delay(random(1, 10));
    return 1;
  }
  else return 0;
}

//******END OF TX PROTOCOL******

// Create a SoftwareSerial object to communicate with the MAX485
SoftwareSerial RS485Serial(10, 11); // RX, TX

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
    // BUS = digitalRead(8);
    // Serial.print("Bus is: ");
    // Serial.println(BUS);
    // delay(100);

 
}
