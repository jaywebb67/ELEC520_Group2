#include<Wire.h>
#include "Communication_Protocol.h"

#define RedPin    5
#define GreenPin  9
#define trigPin   6
#define echoPin   7
#define mSensPin  8

//const uint8_t Home_Node_Type = 0x32;
uint8_t Home_Address = 0x0F;    //Default address for initial set up
uint8_t Destination_Address = 0xFF;   //Default address for initial set up
uint8_t Node_3 = 0x33;
const char Respond_Cmd[8] = "RESPOND";
const char Reset_Cmd[6] = "RESET";
const char User_Cmd[6] = "Vuser";
const char No_User_Cmd[7] = "NVuser";
bool Occupied = false;

const struct TX_Payload Intrusion = {14, "Intruder alarm"};
const struct TX_Payload Alive = {8, "I'm here"};
//// Ultrasonic Setup
// pin config
//const int trigPin = 6;
//const int echoPin = 7;
// variables
bool ultraSonicAlert = false;


//// LED setup
// pin config
//const int RedPin = 5;
// variables
long duration;


//// IMU Setup
// Stuff For Registers n' Wire etc 
const int MPU = 0x68;
// variables
float AccX, AccY, AccZ;
bool imuAlert = false;


//// Motion sensor setup
// pin config
//const int mSensPin = 8;
// variables
int mSensValue = 0;


//// called on loading
void setup() {

  // configuring pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(RedPin, OUTPUT);
  pinMode(mSensPin, INPUT);
  pinMode(GreenPin, OUTPUT);

  digitalWrite(RedPin, LOW);
  digitalWrite(GreenPin, HIGH);

  //begining connection to serial with 19200 baud rate
  Serial.begin(Baud);

  //initialising IMU registers
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);
  
  //
  Comms_Set_Up();
}


// called on repeat
void loop() {
  if(RS485Serial.available()){
    Addressee = Read_Serial_Port();

    if(Addressee == Home_Address){
      Serial.println((char*)RX_Message_Payload);
      Serial.print("Sender's address: ");
      Serial.println(Sender_Address, HEX);
      Serial.print("Sender is a node of type: ");
      Serial.println(Sender_Node_Type, HEX);
      // Handle specific commands
      if (strcmp((char*)RX_Message_Payload, User_Cmd) == 0) {
        Occupied = true;
        digitalWrite(GreenPin, LOW);
      }
      else if (strcmp((char*)RX_Message_Payload, No_User_Cmd) == 0) {
        Occupied = false;
        digitalWrite(GreenPin, HIGH);
      }
      else if (strcmp((char*)RX_Message_Payload, Respond_Cmd) == 0) {
        Transmit_To_Bus(&Alive);
      } 
    }
  }

  if(!Occupied) {
    UltraSonic();
    AccXYZ();
    MotionSensor();
    if (imuAlert || ultraSonicAlert || mSensValue){
      digitalWrite(RedPin, HIGH);  
      Transmit_To_Bus(&Intrusion);  
    } else {
      digitalWrite(RedPin, LOW);
    }
  }
}

void AccXYZ(){
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);

  AccX = (Wire.read() << 8 | Wire.read()) / 16384.0;
  AccY = (Wire.read() << 8 | Wire.read()) / 16384.0;
  AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0;

  if (AccZ > 1.1 || AccZ < 0.8 || AccX > 0.15 || AccX < -0.15 || AccY > 0.15 || AccY < -0.15){
    imuAlert = true;
    Transmit_To_Bus(&Intrusion);
  } else {
    imuAlert = false;
  }

}

void UltraSonic(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);

  //Serial.println();
  //Serial.print(duration);

  if (duration < 500){
    ultraSonicAlert = true;
  } else{
    ultraSonicAlert = false;
  }
}

void MotionSensor(){
  mSensValue = digitalRead(mSensPin);
  Serial.println(mSensValue);
}


