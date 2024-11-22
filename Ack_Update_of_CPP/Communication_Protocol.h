#ifndef COMMUNICATION_PROTOCOL_H
#define COMMUNICATION_PROTOCOL_H

#include <Arduino.h>

#if defined(ARDUINO_AVR_NANO)
#include <SoftwareSerial.h>
extern SoftwareSerial RS485Serial;
#else
#include <HardwareSerial.h>
 extern HardwareSerial RS485Serial;
#endif

// ****PUT THIS LINE IN EACH SKETCH****THEN REMOVE FROM HERE****//
//#define board NANO    
#define Home_Node_Type      0x32
#define START_BYTE          0x02
#define END_BYTE            0x03
#define ENQ                 0X05
#define ACK                 0x06
#define NAK                 0x15
#define Max485_CE           4
#define RX_Pin_A            10
#define TX_Pin_A            11
#define RX_Pin_E            16
#define TX_Pin_E            17
#define Bus_Monitor_Pin     2

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
//extern uint16_t Bus_Montor_Pin;
extern struct Set_Up_Pins Nano;
extern struct Set_Up_Pins Esp;
extern unsigned char TX_Message[40];
extern unsigned char RX_Message[40];
extern unsigned char RX_Message_Payload[35];
extern uint8_t Sender_Address;
extern uint8_t Sender_Node_Type;
extern uint8_t Addressee;
extern uint8_t Sender_Node_Type;
extern uint8_t Response;


void Print_Message(unsigned char* message, unsigned char length);
unsigned char Calculate_Checksum(struct TX_Payload* data);
void Assemble_Message(const struct TX_Payload* data, unsigned char* message, const unsigned char* dest, unsigned char* ack = NAK) ;
void Transmit_To_Bus(const struct TX_Payload* data, unsigned char* dest = Destination_Address, const unsigned char* ack = NAK, unsigned char* message = TX_Message);
void Respond_to_Message(const struct TX_Payload* data, const unsigned char* dest);
unsigned char Calculate_RX_Checksum(unsigned char* data, unsigned char length);
unsigned char Decode_Message(unsigned char* message, unsigned char* Sender_Address, unsigned char* Sender_Node_Type, unsigned char* payload, unsigned char* ack);
bool Clear_To_Send();
bool Collision_Avoidance();
void Bus_Monitor_Pin_interrupt();
void Comms_Setup_Nano(struct Set_Up_Pins* pin);
void Comms_Setup_Esp(struct Set_Up_Pins* pin);
void Comms_Set_Up();
void Read_Serial_Data();
void Board_Select();
void print_Struct(struct Set_Up_Pins* message);
unsigned char Read_Serial_Port();
unsigned char Process_RX_Transmission();


#endif