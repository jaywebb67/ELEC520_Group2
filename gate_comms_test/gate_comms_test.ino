#include "Communication_Protocol.h"

//ESP32//
uint8_t Home_Address = 0x28;
uint8_t Destination_Address = 0x12;
uint8_t Node_Type = 0x11;

struct TX_Payload Gate_1 = {11, "New entrant"};

void setup() {
  //call Comms_Set_Up function this will set up the software serial object and initallise the pins for the correct board
  Comms_Set_Up();
  Serial.begin(9600); // Initialize serial communication
  
}

void loop() {
  //Serial.println("loop entry");
 if(RS485Serial.available()){
    //Serial.println("first if passed");
    //Read_Serial_Data();
    int numBytes = RS485Serial.readBytes(RX_Message, sizeof(RX_Message) - 1); // Read bytes into buffer 
    RX_Message[numBytes] = '\0'; 
    Addressee = Decode_Message(RX_Message, &Sender_Address, &Sender_Node_Type, RX_Message_Payload);

    if(Addressee == Home_Address){
      Serial.println((char*)RX_Message_Payload);
      Serial.print("Sender's address: ");
      Serial.println(Sender_Address, HEX);
      Serial.print("Sender is a node of type: ");
      Serial.println(Sender_Node_Type, HEX);
    }
    else{
      Serial.print("Address mismatch: Received ");
      Serial.println(Addressee, HEX);
      Serial.print(", Expected ");       
      Serial.println(Home_Address, HEX);
    }
  }
  //Serial.println("lets assemble");
  //To transmit call Assemble_Message replace Fire_1 with your TX_Payload variable name leave the &, and TX_Message
  Assemble_Message(&Gate_1, TX_Message); // Assemble message
  // Serial.print("TX_Message: ");
  // Print_Message(TX_Message, 7 + Gate_1.length);
  //Then call Clear_To_Send this will check the bus, wait for it to be quiet, and do the collision avoidance
  if(Clear_To_Send()){
    //Serial.println("All clear");
    digitalWrite(Max485_CE, HIGH);
    RS485Serial.write(TX_Message, 7 + Gate_1.length); //replace Fire_1 with your TX_Payload variable name leave the .length 
    RS485Serial.flush();
    digitalWrite(Max485_CE, LOW);
  }
  //Serial.println("I'm here");
  delay(2000);

}
