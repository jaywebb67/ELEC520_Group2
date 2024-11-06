#ifndef COMMUNICATION_PROTOCOL_H
#define COMMUNICATION_PROTOCOL_H

#include <Arduino.h>
#include <SoftwareSerial.h>

// ****PUT THIS LINE IN EACH SKETCH****THEN REMOVE FROM HERE****//
//#define board NANO    

#define START_BYTE          0x02
#define END_BYTE            0x03
//#define CE                  4
// #define Bus_Monitor_Pin_A   2
// #define Bus_Monitor_Pin_E
#define RX_Pin_A            10
#define TX_Pin_A            11
#define RX_Pin_E
#define TX_Pin_E
#define ESP                 1
#define NANO                0

struct TX_Payload {
  unsigned char length;
  char message[35];
};

struct Set_Up_Pins {
  uint16_t RX;
  uint16_t TX;
  uint16_t CE;
  uint16_t Monitor;
};

extern uint8_t Home_Address;
extern uint8_t Destination_Address;
extern uint8_t Sender_Address;
extern volatile bool Bus_Busy;
extern bool Safe_To_Transmit;
extern uint16_t Bus_Montor_Pin;
extern struct Set_Up_Pins Nano;
extern struct Set_Up_Pins Esp;

extern SoftwareSerial RS485Serial;


void Print_Message(unsigned char* message, unsigned char length);
unsigned char Calculate_Checksum(struct TX_Payload* data);
void Assemble_Message(struct TX_Payload* data, unsigned char* message);
unsigned char Calculate_RX_Checksum(unsigned char* data, unsigned char length);
unsigned char Decode_Message(unsigned char* message, unsigned char* Sender_Address, unsigned char* payload);
bool Clear_To_Send();
bool Collision_Avoidance();
void Bus_Monitor_Pin_interrupt();
void Comms_Setup_Nano(struct Set_Up_Pins* pin);
void Comms_Setup_Esp(struct Set_Up_Pins* pin);
void Comms_Set_Up();



#endif