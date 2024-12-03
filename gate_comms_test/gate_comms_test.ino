#include "Communication_Protocol.h"

//.cpp v1.0
//ESP32//
uint8_t Home_Address = 0x28;
uint8_t Destination_Address = 0x12;
uint8_t Node_Type = 0x11;
uint8_t Node_3 = 0x33;
const char incoming_fire_call[22] = "Call button activated";

struct TX_Payload Gate_1 = {11, "New entrant"};
struct TX_Payload Reset = {5, "RESET"};
struct TX_Payload Hello = {2, "HI"};

void setup() {
  //call Comms_Set_Up function this will set up the software serial object and initallise the pins for the correct board
  Comms_Set_Up();
  Serial.begin(9600); // Initialize serial communication
  
}

void loop() {
  //Serial.println("loop entry");
 if(RS485Serial.available()){
    
    Addressee = Read_Serial_Port(); 

    if(Addressee == Home_Address){
      Serial.println((char*)RX_Message_Payload);
      Serial.print("Sender's address: ");
      Serial.println(Sender_Address, HEX);
      Serial.print("Sender is a node of type: ");
      Serial.println(Sender_Node_Type, HEX);
      if(strcmp((char*)RX_Message_Payload, incoming_fire_call) == 0){
        Transmit_To_Bus(&Reset);
      }
    }
    else{
      Serial.print("Address mismatch: Received ");
      Serial.println(Addressee, HEX);
      Serial.print(", Expected ");       
      Serial.println(Home_Address, HEX);
    }
  }
  
  int x = random(1, 100);
  if(x<10){
    Destination_Address = 0x33;
    Transmit_To_Bus(&Hello);
    Destination_Address = 0x12;
  }
  //Transmit_To_Bus(&Gate_1);
  //Serial.println("I'm here");
  delay(2000);

}
