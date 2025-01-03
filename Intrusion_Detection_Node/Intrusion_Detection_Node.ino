/*
 *ELEC_520 
 *authored by Alex Meredith and Alex Fitzsimons
*/

#include<Wire.h>
#include "Communication_Protocol.h"

#define RedPin    5
#define YellowPin 12
#define trigPin   6
#define echoPin   7
#define mSensPin  8

uint8_t Location;
uint8_t MQTT_Address = 0x0A;
uint8_t Home_Node_Type = 0x42;
uint8_t Home_Address = 0x0F;    //Default address for initial set up
uint8_t Destination_Address = 0x0A;   //Default address for initial set up
uint8_t Node_3 = 0x33;
const char Respond_Cmd[8] = "RESPOND";
const char Reset_Cmd[6] = "RESET";
const char User_Cmd[6] = "Vuser";
const char No_User_Cmd[7] = "NVuser";


const struct TX_Payload Intrusion = {14, "Intruder alarm"};
const struct TX_Payload Alive = {16, "Intrusion online"};

// Ultrasonic Setup
bool ultraSonicAlert = false;

// LED setup
long duration;

// IMU Setup
const int MPU = 0x68;
// variables
float AccX, AccY, AccZ;
bool imuAlert = false;

// Motion sensor setup
int mSensValue = 0;


void setup() {

  // configuring pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(RedPin, OUTPUT);      //indicates alarm triggered
  pinMode(mSensPin, INPUT);
  pinMode(YellowPin, OUTPUT);   //indicates sensors are operating

  digitalWrite(RedPin, LOW);
  digitalWrite(YellowPin, HIGH);

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


void loop() {
  uint32_t Sampling_Period = 5000;
  uint32_t Flash_Freq = 500;
  unsigned long Lost_Time = 0;
  unsigned long Past_Time = 0;
  bool Occupied = false;
  bool ledState = false;
  while(true) {
    if(RS485Serial.available()){
      Addressee = Read_Serial_Port();
      // Serial.print("RX_Message: ");
      // Print_Message(RX_Message, 7 + RX_Message[4]);
      Serial.println((char*)RX_Message_Payload);

      if(Addressee == Home_Address){
        if (strcmp((char*)RX_Message_Payload, Respond_Cmd) == 0) {
          Transmit_To_Bus(&Alive);
        } 
        else if (strcmp((char*)RX_Message_Payload, "New ADD") == 0){
          Home_Address = RX_Message_Payload[7];
          ReW_Mem = true;
          Reset_Params(ReW_Mem);
        }
        else if (strcmp((char*)RX_Message_Payload, "Remove") == 0){
          ReW_Mem = false;
          Reset_Params(ReW_Mem);
        }
      }
      if(Addressee == Home_Node_Type){
        if (strcmp((char*)RX_Message_Payload, Respond_Cmd) == 0) {
          uint8_t temp = Destination_Address;
          Destination_Address = MQTT_Address;
          Transmit_To_Bus(&Alive);
          Destination_Address = temp;
        }
      }
        // Handle specific commands
      else if(Addressee == Location){
        if (strcmp((char*)RX_Message_Payload, User_Cmd) == 0) {
        Occupied = true;
        digitalWrite(YellowPin, LOW);
        }
        else if (strcmp((char*)RX_Message_Payload, No_User_Cmd) == 0) {
          Occupied = false;
          digitalWrite(YellowPin, HIGH);
        }
      }
    }

    if(!Occupied) {
      unsigned long Current_Time = millis();
      UltraSonic();
      //AccXYZ();
      //MotionSensor();
      if (imuAlert || ultraSonicAlert || mSensValue){
        if(Current_Time-Lost_Time >= Sampling_Period){
          digitalWrite(RedPin, HIGH);  
          Transmit_To_Bus(&Intrusion);  
          imuAlert = false;
          ultraSonicAlert = false;
          Lost_Time = Current_Time;
        } 
      }
      else {
        digitalWrite(RedPin, LOW);
      }
    }
    else {
      unsigned long Current_Occ_Time = millis();
      if((Current_Occ_Time - Past_Time)>= Flash_Freq){
        ledState = !ledState; 
        digitalWrite(YellowPin, ledState); 
        Past_Time = Current_Occ_Time;
      }
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
  } 
  Serial.print("Acc X: ");
  Serial.print(AccX);
  Serial.print("  Acc Y: ");
  Serial.print(AccY);
  Serial.print("  Acc Z: ");
  Serial.print(AccZ);
  Serial.print("  Alert: ");
  Serial.println(imuAlert);
}

void UltraSonic(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);

  // Serial.println();
  // Serial.print(duration);

  if (duration < 500){
    ultraSonicAlert = true;
  } 
}

void MotionSensor(){
  mSensValue = digitalRead(mSensPin);
  Serial.println(mSensValue);
}


