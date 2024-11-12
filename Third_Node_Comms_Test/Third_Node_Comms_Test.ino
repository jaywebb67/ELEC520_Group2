#include "Communication_Protocol.h"

uint8_t Home_Address = 0x33;
uint8_t Destination_Address = 0x12;
uint8_t Node_Type = 0x11;
unsigned char ADD_1 = 0x12;
unsigned char ADD_2 = 0x28;
const char Hello[3] = "HI";

const struct TX_Payload message_1 = {14, "12 say's Hello"};
const struct TX_Payload message_2 = {14, "28 say's Hello"};
void setup() {
  Comms_Set_Up();
  Serial.begin(9600);

}

void loop() {
  delay(1000);
  if(RS485Serial.available()){
    Addressee = Read_Serial_Port();
    Serial.print("Addressee: ");
    Serial.println(Addressee, HEX);
    Serial.print("Sender's address: ");
      Serial.println(Sender_Address, HEX);

    if(Addressee == Home_Address){
      Serial.println("Made it");
      if(Sender_Address == ADD_1){
        Serial.println("sent from 12");
        Destination_Address = 0x28;
        Transmit_To_Bus(&message_1);
      }
      if(Sender_Address == ADD_2){
        Serial.println("sent from 28");
        Destination_Address = 0x12;
        Transmit_To_Bus(&message_2);
      }
    }
  }

}
