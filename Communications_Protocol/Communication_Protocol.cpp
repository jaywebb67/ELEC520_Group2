#include "Communication_Protocol.h"

//global variable for bus busy state
volatile bool Bus_Busy = 0;
//global variable for safe to transmit state
bool Safe_To_Transmit = 0;

uint16_t Bus_Monitor_Pin;
extern uint8_t board;

struct Set_Up_Pins Nano = {10, 11, 4, 2};
struct Set_Up_Pins Esp  = {3, 1, 5, 4};

SoftwareSerial RS485Serial(Nano.RX, Nano.TX); // Initialize globally

unsigned char Calculate_Checksum(struct TX_Payload* data) {
  unsigned char checksum = 0;
  for (unsigned char i = 0; i < data->length; i++) {
      checksum ^= data->message[i];
  }
  return checksum;
}

unsigned char Calculate_RX_Checksum(unsigned char* data, unsigned char length) {
  unsigned char checksum = 0;
  for (unsigned char i = 0; i < length; i++) {
      checksum ^= data[i];
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

unsigned char Decode_Message(unsigned char* message, unsigned char* Sender_Address, unsigned char* payload){
  if (message[0] != START_BYTE || message[5 + message[3]] != END_BYTE) {
      return 0; // Error: Invalid start or end byte
  }
  
  *Sender_Address = message[1];
  unsigned char address = message[2]; // Destination Address byte
  unsigned char length = message[3]; // Use unsigned char for consistency

  // Bounds check for payload length
  if (length > 35) {
      return 0; // Error: Payload length exceeds buffer size
  }

  for (unsigned char i = 0; i < length; i++) {
      payload[i] = message[4 + i];
  }
  unsigned char received_checksum = message[4 + length];
  unsigned char calculated_checksum = Calculate_RX_Checksum(&message[1], length + 3);

  if (received_checksum != calculated_checksum) {
      return 0; // Error: Checksum mismatch
  }

  return address; // Return the address for further processing
}

bool Clear_To_Send(){
  if(Collision_Avoidance()){
    delay(random(1, 10));
    return 1;
  }
  else return 0;
}

bool Collision_Avoidance(){
  uint32_t x = 50;
  for (uint32_t delay_time = 100; delay_time<10000; delay_time *= 2){
    Bus_Busy = 0;
    delay(1);
    if(!Bus_Busy){
      return 1;
    }
    delay(random(x, delay_time));
    x *=2;
  }
}

// ISR for bus monitoring pin
void Bus_Monitor_Pin_interrupt() {
  Bus_Busy = 1;
}

void Comms_Setup_Nano(struct Set_Up_Pins* pin){
  pinMode(pin->Monitor, INPUT);
  pinMode(pin->CE, OUTPUT);
  RS485Serial = SoftwareSerial(pin->RX, pin->TX); // Reinitialize with Nano pins
  Bus_Monitor_Pin = pin->Monitor;
  digitalWrite(pin->CE, LOW);
}

void Comms_Setup_Esp(struct Set_Up_Pins* pin){
  pinMode(pin->Monitor, INPUT);
  pinMode(pin->CE, OUTPUT);
  RS485Serial = SoftwareSerial(pin->RX, pin->TX); // Reinitialize with ESP pins
  Bus_Monitor_Pin = pin->Monitor;
  digitalWrite(pin->CE, LOW);
}

void Comms_Set_Up(){
  if(board == NANO){
    Comms_Setup_Nano(&Nano);
  }
  else {
    Comms_Setup_Esp(&Esp);
  }
  attachInterrupt(digitalPinToInterrupt(Bus_Monitor_Pin), Bus_Monitor_Pin_interrupt, CHANGE);
  RS485Serial.begin(9600);
  
}






