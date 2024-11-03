#include "Communication_Protocol.h"

unsigned char Calculate_Checksum(struct TX_Payload* data) {
    unsigned char checksum = 0;
    for (unsigned char i = 0; i < data->length; i++) {
        checksum ^= data->message[i];
    }
    return checksum;
}

void Assemble_Message(struct TX_Payload* data, unsigned char* message) {
    message[0] = START_BYTE;             // Start byte
    message[1] = Home_Address;           // Sender Address byte
    message[2] = Destination_Address;    // Destination Address byte
    message[3] = data->length;           // Length byte
    
    for (unsigned char i = 0; i < data->length; i++) {
        message[4 + i] = data->message[i]; // Payload
    }
    
    
    message[4 + data->length] = Calculate_Checksum(data); // Checksum
    message[5 + data->length] = END_BYTE;                // End byte
}

void Print_Message(unsigned char* message, unsigned char length) {
  for (unsigned char i = 0; i < length; i++) {
    Serial.print("0x");
    if (message[i] < 0x10) Serial.print("0");
    Serial.print(message[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}