#ifndef COMMUNICATION_PROTOCOL_H
#define COMMUNICATION_PROTOCOL_H

#include <Arduino.h>

#define START_BYTE 0x02
#define END_BYTE   0x03

extern uint8_t Home_Address;
extern uint8_t Destination_Address;

struct TX_Payload {
  unsigned char length;
  char message[35];
};

void Print_Message(unsigned char* message, unsigned char length);
unsigned char Calculate_Checksum(struct TX_Payload* data);
void Assemble_Message(struct TX_Payload* data, unsigned char* message);

#endif