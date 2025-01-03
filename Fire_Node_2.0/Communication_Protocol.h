#ifndef COMMUNICATION_PROTOCOL_H
#define COMMUNICATION_PROTOCOL_H

#include <Arduino.h>


#if defined(ARDUINO_AVR_NANO)
#include <SoftwareSerial.h>
extern SoftwareSerial RS485Serial;
#else
#include <HardwareSerial.h>
 extern HardwareSerial RS485Serial;
 extern QueueHandle_t RX_Queue;
#endif

  

#define START_BYTE          0x02
#define END_BYTE            0x03
#define ENQ                 0X05
#define ACK                 0x06
#define DEFAULT_ADD         0x1
#define Max485_CE           4
#define RX_Pin_A            10
#define TX_Pin_A            11
#define RX_Pin_E            16
#define TX_Pin_E            17
#define ESP                 1
#define NANO                0
#define Bus_Monitor_Pin     2
#define MESSAGE_LENGTH      40

//.h v2.3
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

extern uint8_t Location;
extern uint8_t Home_Address;
extern uint8_t Home_Node_Type;
extern uint8_t Destination_Address;
extern uint8_t Sender_Address;
extern volatile bool Bus_Busy;
extern bool Safe_To_Transmit;
extern struct Set_Up_Pins Nano;
extern struct Set_Up_Pins Esp;
extern unsigned char TX_Message[40];
extern unsigned char RX_Message[40];
extern unsigned char Forward[40];
extern unsigned char RX_Message_Payload[35];
extern unsigned char Ack_message[9];
extern uint8_t Sender_Address;
extern uint8_t Sender_Node_Type;
extern uint8_t Addressee;
extern uint8_t Sender_Node_Type;
extern const struct TX_Payload Intro;
extern volatile unsigned char buffer[MESSAGE_LENGTH];
extern volatile unsigned char bufferIndex;



void Print_Message(unsigned char* message, unsigned char length);
unsigned char Calculate_Checksum(struct TX_Payload* data);
void Assemble_Message(struct TX_Payload* data, unsigned char* message);
void Transmit_To_Bus(struct TX_Payload* data, unsigned char* message = TX_Message);
unsigned char Calculate_RX_Checksum(unsigned char* data, unsigned char length);
unsigned char Decode_Message(unsigned char* message, unsigned char* Sender_Address, unsigned char* Sender_Node_Type, unsigned char* payload);
bool Clear_To_Send();
bool Collision_Avoidance();
void Bus_Monitor_Pin_interrupt();
void Comms_Setup_Nano(struct Set_Up_Pins* pin);
void Comms_Setup_Esp(struct Set_Up_Pins* pin);
void Comms_Set_Up();
void Read_Serial_Data();
void Board_Select(struct Set_Up_Pins* pin);
void print_Struct(struct Set_Up_Pins* message);
unsigned char Read_Serial_Port();
unsigned char Process_RX_Transmission();
void Acknowledge(unsigned char* dest, unsigned char* message = Ack_message );
void Introduction();
void Forward_Messasage();
void RX_Message_Process(void *pvParameters);
void onUartRx();


#endif