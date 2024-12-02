#include "Communication_Protocol.h"
//#include "Node_Config.h"
//.cpp v 2.4

struct Set_Up_Pins Nano = {RX_Pin_A, TX_Pin_A, Max485_CE, Bus_Monitor_Pin};
struct Set_Up_Pins Esp  = {RX_Pin_E, TX_Pin_E, Max485_CE, Bus_Monitor_Pin};

#if defined(ARDUINO_AVR_NANO)
SoftwareSerial RS485Serial(10, 11);
#else
HardwareSerial RS485Serial(2);    //creadte hardwarSerial object  attached to UART 2
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
unsigned char Forward[40];
unsigned char RX_Message_Payload[35];
unsigned char Ack_message[9] = { 
  START_BYTE, 
  Home_Address, 
  Destination_Address, 
  Home_Node_Type, 
  1, // Length byte 
  ACK, // Data 
  ACK, // Checksum 
  END_BYTE, // End byte 
  '\0' // Null-terminator if needed 
  };

uint8_t Fire_Node = 0x32;
uint8_t Intrusion_Node = 0x42;
uint8_t Home_Address = 0x13;
uint8_t Destination_Address = 0x28;
uint8_t Location;
uint8_t Sender_Address;
uint8_t Sender_Node_Type;
uint8_t Addressee;

const struct TX_Payload Intro = {1, {Location}};


//*****USER FUNCTIONS*****//

// Call this function to transmit data the message field is predefined in the header 
void Transmit_To_Bus(struct TX_Payload* data, unsigned char* message){  
  Assemble_Message(data, message);
  if(Clear_To_Send()){
    digitalWrite(Max485_CE, HIGH);
    RS485Serial.write(message, 7 + data->length);  
    #if !defined(ARDUINO_AVR_NANO)
    RS485Serial.flush();  //only required for ESP32 forces the cpu to block on the sending
    #endif
    digitalWrite(Max485_CE, LOW);
    // Serial.print("TX_Message: ");
    // Print_Message(message, 7 + message[4]);
  }
}


//Call this function in setup to initialise serial communications infrastructure
void Comms_Set_Up(){
  Serial.println("Comms setup");
  #if defined(ARDUINO_AVR_NANO)
  Board_Select(&Nano);
  #else 
  Serial.println("ESP32 Comms setup");
  Board_Select(&Esp); 
  #endif
  attachInterrupt(digitalPinToInterrupt(Bus_Monitor_Pin), Bus_Monitor_Pin_interrupt, CHANGE);
  delay(1000);
  //Introduction();
  Serial.println("End of ESp32 Comms setup");
}


/*Call this function to read the serial port message is stored to RX_Message_Payload, sender address, and sender node type are stored
   function returns the intended recipiant address*/
unsigned char Read_Serial_Port() {
  int index = 0;
  // Clear RX_Message buffer, Forward buffer and RX_Message_Payload buffer to prevent message overlaps
  memset(RX_Message, 0, sizeof(RX_Message));
  memset(Forward, 0, sizeof(Forward));
  // Clear 
  memset(RX_Message_Payload, 0, sizeof(RX_Message_Payload));

  // Read bytes until end byte (0x03) is found
  while (RS485Serial.available()) {
    char incomingByte = RS485Serial.read();
    RX_Message[index++] = incomingByte;
    Forward[index++] = incomingByte;

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
  Forward[index] = '\0';
  // Process the message
  unsigned char address = Decode_Message(RX_Message, &Sender_Address, &Sender_Node_Type, RX_Message_Payload);
  return address;
}


//Function to send acknowledgement byte as a reply
void Acknowledge(unsigned char* dest, unsigned char* message){
  message[2] = *dest;
  if(Clear_To_Send()){
  digitalWrite(Max485_CE, HIGH);
  RS485Serial.write(message, 9);  
  #if !defined(ARDUINO_AVR_NANO)
  RS485Serial.flush();  //only required for ESP32 forces the cpu to block on the sending
  #endif
  digitalWrite(Max485_CE, LOW);
  }
}
//*****END OF USER FUNCTIONS*****//


// calculates the checksum value for the Tx message
unsigned char Calculate_Checksum(struct TX_Payload* data) {
  unsigned char checksum = 0;
  for (unsigned char i = 0; i < data->length; i++) {
    checksum ^= data->message[i];
  }
  return checksum;
}


//Calculates the checksum value from the received message
unsigned char Calculate_RX_Checksum(unsigned char* data, unsigned char length) {
  unsigned char checksum = 0;
  for (unsigned char i = 0; i < length; i++) {
    checksum ^= data[i];
  }
  return checksum;
}


//Assemble the transmission into the correct format
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


/* Function to to extract the relevant information from the received transmission
   Automatically responds to requests for acknowledgement*/
unsigned char Decode_Message(unsigned char* message, unsigned char* Sender_Address, unsigned char* Sender_Node_Type, unsigned char* payload) {
  // Serial.print("RX_Message: ");
  // Print_Message(message, 7 + message[4]);
  if (message[0] != START_BYTE || message[6 + message[4]] != END_BYTE) {
    Serial.println("Invalid start or end byte");
    return 0; // Error: Invalid start or end byte
  }

  *Sender_Address = message[1];       // Place into variable
  unsigned char address = message[2]; // Destination Address byte to be returned
  *Sender_Node_Type = message[3];     // Place into variable
  unsigned char length = message[4];  // in function use only

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

  if (received_checksum != calculated_checksum) {
    Serial.println("Checksum mismatch");
    return 0; // Error: Checksum mismatch
  }
  if (payload[0] == ACK){
    Acknowledge(Sender_Address);
  }

  #if !defined(ARDUINO_AVR_NANO)
  if(address == 0x0F) {
    Forward_Messasage();
  }
  #endif
  return address; // Return the destination address for further processing
}

bool Clear_To_Send(){
  if(Collision_Avoidance()){
    delay(random(1, 10));
    return 1;
  }
  else return 0;
}


//Function to check that the serial bus is not busy, and determine when it is safe to transmit
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


//Sets up the correct communication infrastructure pins and platform specific serial objects
void Board_Select(struct Set_Up_Pins* pin){
  #if defined(ARDUINO_AVR_NANO)
  print_Struct(&Nano);
  pinMode(pin->Monitor, INPUT);
  pinMode(pin->CE, OUTPUT);
  RS485Serial = SoftwareSerial(pin->RX, pin->TX); // software serial for nano
  RS485Serial.begin(9600);
  digitalWrite(pin->CE, LOW);
  #else
  pinMode(pin->Monitor, INPUT);
  pinMode(pin->CE, OUTPUT);
  RS485Serial.begin(9600, SERIAL_8N1, pin->RX, pin->TX);  //hardware serial for ESP
  digitalWrite(pin->CE, LOW);
  #endif
}




//prints the pins being used in setup.....debugging
void print_Struct(struct Set_Up_Pins* message) {
  Serial.println(message->RX);
  Serial.println(message->TX);
  Serial.println(message->CE);
  Serial.println(message->Monitor); // Print the actual message
}

//prints the char array messages.....debugging
void Print_Message(unsigned char* message, unsigned char length) {
  for (unsigned char i = 0; i < length; i++) {
    Serial.print("0x");
    if (message[i] < 0x10) Serial.print("0");
    Serial.print(message[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}


//unused
unsigned char Process_RX_Transmission(){
  return Decode_Message(RX_Message, &Sender_Address, &Sender_Node_Type, RX_Message_Payload);
}

//unused
void Read_Serial_Data() {
  int index = 0;
  while (RS485Serial.available() && index < sizeof(RX_Message)) {
    RX_Message[index++] = RS485Serial.read();
  }
}

void Introduction() {
  bool reply_received = 0;
  Transmit_To_Bus((TX_Payload*)&Intro);                 //transmit intrduction message
  while (!reply_received) {                             //loop until a reply is received
    if(RS485Serial.available()){
      Addressee = Read_Serial_Port();                   //wait for reply
      if(Addressee == Home_Address) {
        Home_Address = RX_Message_Payload[0];           //set up node parameters
        Destination_Address = RX_Message_Payload[1];
        Location = RX_Message_Payload[2];
        reply_received = 1;                             //set exit condition
      }
    }
  }
}

void Forward_Messasage() {
 int x = sizeof(Forward);

  if(Clear_To_Send()){
    digitalWrite(Max485_CE, HIGH);
    RS485Serial.write(Forward, x);  
    #if !defined(ARDUINO_AVR_NANO)
    RS485Serial.flush();  //only required for ESP32 forces the cpu to block on the sending
    #endif
    digitalWrite(Max485_CE, LOW);
  }
}

























