#include "Communication_Protocol.h"

//.cpp v 2.0

struct Set_Up_Pins Nano = {RX_Pin_A, TX_Pin_A, Max485_CE, Bus_Monitor_Pin};
struct Set_Up_Pins Esp  = {RX_Pin_E, TX_Pin_E, Max485_CE, Bus_Monitor_Pin};

#if defined(ARDUINO_AVR_NANO)
SoftwareSerial RS485Serial(10, 11);
#else
HardwareSerial RS485Serial(2);
#endif


//global variable for bus busy state
volatile bool Bus_Busy = 0;
//global variable for safe to transmit state
bool Safe_To_Transmit = 0;

//uint16_t Bus_Monitor_Pin;
extern uint8_t board;

//message format: Start byte, Sender address byte, Destination address byte, Sender device type code, Length byte, 33 message characters, Checksum byte, End byte.
unsigned char TX_Message[40]; // sized for start byte,
unsigned char RX_Message[40];
unsigned char RX_Message_Payload[35];
uint8_t Sender_Address;
uint8_t Sender_Node_Type;
uint8_t Addressee;




 

// unsigned char Calculate_Checksum(struct TX_Payload* data) {
//   unsigned char checksum = 0;
//   for (unsigned char i = 0; i < data->length; i++) {
//       checksum ^= data->message[i];
//   }
//   Serial.print("Calculated TX Checksum: "); 
//   Serial.println(checksum, HEX);
//   return checksum;
// }

// unsigned char Calculate_RX_Checksum(unsigned char* data, unsigned char length) {
//   unsigned char checksum = 0;
//   for (unsigned char i = 0; i < length; i++) {
//       checksum ^= data[i];
//   }
//   Serial.print("Calculated RX Checksum: "); 
//   Serial.println(checksum, HEX);
//   return checksum;
// }
unsigned char Calculate_Checksum(struct TX_Payload* data) {
  unsigned char checksum = 0;
  for (unsigned char i = 0; i < data->length; i++) {
    checksum ^= data->message[i];
    // Serial.print("Step ");
    // Serial.print(i);
    // Serial.print(": ");
    // Serial.println(checksum, HEX);
  }
  // Serial.print("Calculated TX Checksum: ");
  // Serial.println(checksum, HEX);
  return checksum;
}

unsigned char Calculate_RX_Checksum(unsigned char* data, unsigned char length) {
  unsigned char checksum = 0;
  for (unsigned char i = 0; i < length; i++) {
    checksum ^= data[i];
    // Serial.print("Step ");
    // Serial.print(i);
    // Serial.print(": ");
    // Serial.println(checksum, HEX);
  }
  // Serial.print("Calculated RX Checksum: ");
  // Serial.println(checksum, HEX);
  return checksum;
}


void Assemble_Message(struct TX_Payload* data, unsigned char* message) {
  message[0] = START_BYTE;             // Start byte
  message[1] = Home_Address;           // Sender Address byte
  message[2] = Destination_Address;    // Destination Address byte
  message[3] = Home_Node_Type;         // type of device e.g. fie, gate etc
  message[4] = data->length;           // Length byte
  
  for (unsigned char i = 0; i < data->length; i++) {
      message[5 + i] = data->message[i]; // Payload
  }
  
  
  message[5 + data->length] = Calculate_Checksum(data); // Checksum
  message[6 + data->length] = END_BYTE;                // End byte
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

void Transmit_To_Bus(struct TX_Payload* data, unsigned char* message){  
  Assemble_Message(data, message);
  if(Clear_To_Send()){
    digitalWrite(Max485_CE, HIGH);
    RS485Serial.write(message, 7 + data->length);  
    #if !defined(ARDUINO_AVR_NANO)
    RS485Serial.flush();  //only required for ESP32 
    #endif
    digitalWrite(Max485_CE, LOW);
  }
  delay(50);
}
// unsigned char Decode_Message(unsigned char* message, unsigned char* Sender_Address, unsigned char* Sender_Node_Type, unsigned char* payload){
//   Serial.print("RX_Message: ");
//   Print_Message(message, message[7+message[4]]);

//   if (message[0] != START_BYTE || message[6 + message[4]] != END_BYTE) {
//       return 0; // Error: Invalid start or end byte
//   }
  
//   *Sender_Address = message[1];       //place into variable
//   unsigned char address = message[2]; // Destination Address byte
//   *Sender_Node_Type = message[3];     //place into variable
//   unsigned char length = message[4]; // Use unsigned char for consistency

//   // Bounds check for payload length
//   if (length > 34) {
//       return 0; // Error: Payload length exceeds buffer size
//   }

//   for (unsigned char i = 0; i < length; i++) {
//       payload[i] = message[5 + i];
//   }
//   unsigned char received_checksum = message[5 + length];
//   unsigned char calculated_checksum = Calculate_RX_Checksum(&message[1], length + 4);

//   if (received_checksum != calculated_checksum) {
//       return 0; // Error: Checksum mismatch
//   }

//   return address; // Return the address for further processing
// }

unsigned char Decode_Message(unsigned char* message, unsigned char* Sender_Address, unsigned char* Sender_Node_Type, unsigned char* payload) {
  // Serial.print("RX_Message: ");
  // Print_Message(message, 7 + message[4]);

  if (message[0] != START_BYTE || message[6 + message[4]] != END_BYTE) {
    Serial.println("Invalid start or end byte");
    return 0; // Error: Invalid start or end byte
  }

  *Sender_Address = message[1];       // Place into variable
  unsigned char address = message[2]; // Destination Address byte
  *Sender_Node_Type = message[3];     // Place into variable
  unsigned char length = message[4];  // Use unsigned char for consistency

  // Bounds check for payload length
  if (length > 34) {
    Serial.println("Payload length exceeds buffer size");
    return 0; // Error: Payload length exceeds buffer size
  }

  for (unsigned char i = 0; i < length; i++) {
    payload[i] = message[5 + i];
  }
  
  unsigned char received_checksum = message[5 + length];
  unsigned char calculated_checksum = Calculate_RX_Checksum(&message[5], length);  // Adjust this line

  // Serial.print("Received Checksum: ");
  // Serial.println(received_checksum, HEX);
  // Serial.print("Calculated RX Checksum: ");
  // Serial.println(calculated_checksum, HEX);

  if (received_checksum != calculated_checksum) {
    Serial.println("Checksum mismatch");
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

void Board_Select(struct Set_Up_Pins* pin){
  #if defined(ARDUINO_AVR_NANO)
  Serial.println("I'm not board");
  print_Struct(&Nano);
  pinMode(pin->Monitor, INPUT);
  pinMode(pin->CE, OUTPUT);
  RS485Serial = SoftwareSerial(pin->RX, pin->TX); // Reinitialize with Nano pins
  RS485Serial.begin(9600);
  //Bus_Monitor_Pin = pin->Monitor;
  digitalWrite(pin->CE, LOW);
  #else
  pinMode(pin->Monitor, INPUT);
  pinMode(pin->CE, OUTPUT);
  RS485Serial.begin(9600, SERIAL_8N1, pin->RX, pin->TX);
  //Bus_Monitor_Pin = pin->Monitor;
  digitalWrite(pin->CE, LOW);
  #endif
}

void Comms_Set_Up(){
  Serial.println("Lets set up the comms!");
  #if defined(ARDUINO_AVR_NANO)
  Board_Select(&Nano);
  #else 
    Board_Select(&Esp);
  #endif
  attachInterrupt(digitalPinToInterrupt(Bus_Monitor_Pin), Bus_Monitor_Pin_interrupt, CHANGE);
  
}

void Read_Serial_Data() {
  int index = 0;
  while (RS485Serial.available() && index < sizeof(RX_Message)) {
    RX_Message[index++] = RS485Serial.read();
  }
}

void print_Struct(struct Set_Up_Pins* message) {
  Serial.println(message->RX);
  Serial.println(message->TX);
  Serial.println(message->CE);
  Serial.println(message->Monitor); // Print the actual message
}

//   unsigned char Read_Serial_Port(){
//   int numBytes = RS485Serial.readBytes(RX_Message, sizeof(RX_Message) - 1); // Read bytes into buffer 
//     RX_Message[numBytes] = '\0'; 
//     Serial.print("Bytes read: "); 
//     Serial.println(numBytes); 
//     Print_Message(RX_Message, numBytes);
//     unsigned char address = Process_RX_Transmission();
//     return address;
// }

unsigned char Read_Serial_Port() {
  int index = 0;

  // Clear RX_Message buffer
  memset(RX_Message, 0, sizeof(RX_Message));
  // Clear RX_Message_Payload buffer 
  memset(RX_Message_Payload, 0, sizeof(RX_Message_Payload));

  // Read bytes until end byte (0x03) is found
  while (RS485Serial.available()) {
    char incomingByte = RS485Serial.read();
    RX_Message[index++] = incomingByte;

    // Check for buffer overflow
    if (index >= sizeof(RX_Message) - 1) {
      Serial.println("Buffer overflow");
      break;
    }

    // Check for end byte
    if (incomingByte == 0x03) {
      break;
    }
  }

  // Null-terminate the string
  RX_Message[index] = '\0';

  // Serial.print("Bytes read: ");
  // Serial.println(index);
  // Print_Message(RX_Message, index);

  // Process the message
  unsigned char address = Process_RX_Transmission();
  return address;
}

unsigned char Process_RX_Transmission(){
  return Decode_Message(RX_Message, &Sender_Address, &Sender_Node_Type, RX_Message_Payload);
}

