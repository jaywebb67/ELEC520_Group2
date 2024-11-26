#include "Communication_Protocol.h"
#include "DHT.h"
#include "Node_Config.h"
#include <EnableInterrupt.h>
#include <TaskScheduler.h>

Scheduler runner;
   //need to send location byte may need to be variable to fit in struct

const uint8_t Home_Node_Type = 0x32;
uint8_t Home_Address = 0x0F;    //Default address for initial set up
uint8_t Destination_Address = 0xFF;   //Default address for initial set up
uint8_t Node_3 = 0x33;
const char Respond_Cmd[7] = "RESPOND";
const char Reset_Cmd[6] = "RESET";
volatile bool alarm_Pressed = false;
String LED = "LED";
const uint32_t Max_Temp = 40;
bool RX_Flag = false;


const struct TX_Payload Fire_1 = {21, "Call button activated"};
const struct TX_Payload Fire_2 = {12, "Sensor error"};
const struct TX_Payload Fire_3 = {21, "Fire sensor activated"};
const struct TX_Payload Fire_4 = {19, "Nothing to see here"};
struct TX_Payload Hello = {2, "Hi"};

DHT dht(DHTPIN, DHTTYPE);

Task Task1(2000, TASK_FOREVER, &Read_Sensor);
Task Task2(100, TASK_FOREVER, &Serial_RX);

void alarm_Pressed_interrupt() {
  alarm_Pressed = true;
}

void RX_ISR() {
  RX_Flag = true;
}

void Read_Sensor () {
  float t = dht.readTemperature();

  if (isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    Serial.println();
    Transmit_To_Bus(&Fire_2);
  }

  Serial.print("Temperature read = ");
  Serial.println(t);
  t += random(1, 30); // Simulate temperature fluctuation for testing
  Serial.print("New Temperature = ");
  Serial.println(t);
  
  if (t > Max_Temp) {
    Serial.println("Maximum allowable temperature exceeded");
    Transmit_To_Bus(&Fire_3);
  }
  
  if (alarm_Pressed) {
    Serial.println("Fire alarm triggered");
    Transmit_To_Bus(&Fire_1);
  }

   int x = random(1, 100);
  if(x>90){
    Destination_Address = 0x33;
    Transmit_To_Bus(&Hello);
    Destination_Address = 0x28;
  }
}

void Serial_RX() {
  Addressee = Read_Serial_Port();
  
  if(Addressee == Home_Address){
    Serial.println((char*)RX_Message_Payload);
    Serial.print("Sender's address: ");
    Serial.println(Sender_Address, HEX);
    Serial.print("Sender is a node of type: ");
    Serial.println(Sender_Node_Type, HEX);
    // Handle specific commands
    if (strcmp((char*)RX_Message_Payload, Reset_Cmd) == 0) {
      alarm_Pressed = false;
    }
    if (strcmp((char*)RX_Message_Payload, Respond_Cmd) == 0) {
      Transmit_To_Bus(&Fire_4);
    }
  }
  else if((Sender_Address == Node_3)&&(Addressee == Home_Address )){
    Serial.println((char*)RX_Message_Payload);
    Serial.print("Sender's address: ");
    Serial.println(Sender_Address, HEX);
  }
  else{
    Serial.print("Address mismatch: Received ");
    Serial.print(Addressee, HEX);
    Serial.print(", Expected ");       
    Serial.println(Home_Address, HEX);
  }
  RX_Flag = false;
}

void setup() {
  Serial.begin(9600);
  Comms_Set_Up();
  pinMode(Alarm_Pin, INPUT);
  pinMode(LED_B, OUTPUT);
  dht.begin();
  attachInterrupt(digitalPinToInterrupt(Alarm_Pin),alarm_Pressed_interrupt, FALLING);
  EnableInterrupt(RX_Pin_A , RX_ISR, CHANGE); // Set up pin change interrupt on pin 10
  alarm_Pressed = false;

}

void loop() {
  // put your main code here, to run repeatedly:

}
