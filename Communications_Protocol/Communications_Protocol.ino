#include "Communication_Protocol.h"

//****CHANGE THE VALUE OF THIS VARIABLE TO EITHER NANO OR ESP****//
uint8_t board = NANO;


uint8_t Home_Address = 0x12;
uint8_t Destination_Address = 0x00;
uint8_t Sender_Address;

//message format: Start byte, Sender address byte, Destination address byte, Length byte, 34 message characters, Checksum byte, End byte.
unsigned char TX_Message[40]; // sized for start byte,
unsigned char RX_Message[40];
unsigned char RX_Message_Payload[35];

//Create a TX_Payload object for each message {number of characters, "message max 34 characters"}
struct TX_Payload Fire_1 = {23, "Call button x activated"};

void setup() {
  //call Comms_Set_Up function this will set up the software serial object and initallise the pins for the correct board
  Comms_Set_Up();
  Serial.begin(9600); // Initialize serial communication
}

void loop() {
  //To transmit call Assemble_Message replace Fire_1 with your TX_Payload variable name leave the &, and TX_Message
  Assemble_Message(&Fire_1, TX_Message); // Assemble message
  //Then call Clear_To_Send this will check the bus, wait for it to be quiet, and do the collision avoidance
  if(Clear_To_Send()){
    RS485Serial.write(TX_Message, 6 + Fire_1.length); //replace Fire_1 with your TX_Payload variable name leave the .length 
  }
  
}

