#include "Communication_Protocol.h"
//#include "Node_Config.h"
//.cpp v 2.3

struct Set_Up_Pins Nano = {RX_Pin_A, TX_Pin_A, Max485_CE, Bus_Monitor_Pin};
struct Set_Up_Pins Esp  = {RX_Pin_E, TX_Pin_E, Max485_CE, Bus_Monitor_Pin};

#if defined(ARDUINO_AVR_NANO)
SoftwareSerial RS485Serial(10, 11);
#else
HardwareSerial RS485Serial(2);    //creadte hardwarSerial object  attached to UART 2
volatile unsigned char buffer[MESSAGE_LENGTH];
volatile unsigned char bufferIndex = 0;
QueueHandle_t RX_Queue;
TaskHandle_t RX_Message_Handle;
#endif


//global variable for bus busy state
volatile bool Bus_Busy = 0;
//global variable for safe to transmit state
bool Safe_To_Transmit = 0;
//global variable for rewrite memory flag
bool ReW_Mem;

//message format: Start byte, Sender address byte, Destination address byte, Sender device type code, Length byte, 33 message characters, Checksum byte, End byte.
unsigned char TX_Message[40]; // sized for start byte,
unsigned char RX_Message[40];
unsigned char Forward[40];
unsigned char RX_Message_Payload[35];
unsigned char IntroMessage[6] = {'I', 'n', 't', 'r', 'o', Home_Address};
unsigned char Device_ID[10];
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


uint8_t ADD_Reset;
uint8_t Sender_Address;
uint8_t Sender_Node_Type;
uint8_t Addressee;

const struct TX_Payload Intro = {6, IntroMessage};


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
  RX_Queue = xQueueCreate(10, MESSAGE_LENGTH * sizeof(char));

  // Create task 3 (rus on core 0)
  xTaskCreatePinnedToCore(
     RX_Message_Process,  // Task function. 
    "RX_Message_Process",     // name of task. 
    10000,                    // Stack size of task 
    NULL,                     // parameter of the task 
    2,                        // priority of the task 
    &RX_Message_Handle,      // Task handle to keep track of created task 
    0                         // pin task to core 0 
    );       

  // Attach UART interrupt 
 
  #endif
  attachInterrupt(digitalPinToInterrupt(Bus_Monitor_Pin), Bus_Monitor_Pin_interrupt, CHANGE);
  delay(1000);
  // Load_Vitals();
}


unsigned char Read_Serial_Port() {
  int index = 0;
  bool startByteFound = false;
  bool endByteFound = false;

  // Clear RX_Message buffer, Forward buffer, and RX_Message_Payload buffer to prevent message overlaps
  memset(RX_Message, 0, sizeof(RX_Message));
  memset(Forward, 0, sizeof(Forward));
  memset(RX_Message_Payload, 0, sizeof(RX_Message_Payload));

  // Read bytes until the complete message with start and end bytes is found
  while (RS485Serial.available()>0) {
    char incomingByte = RS485Serial.read();
    delayMicroseconds(100);
    // Serial.print("Received byte: 0x");
    // Serial.println(incomingByte, HEX);

    if (incomingByte == 0x02) {
      // Reset buffers and start from the beginning if the start byte is found
      memset(RX_Message, 0, sizeof(RX_Message));
      memset(Forward, 0, sizeof(Forward));
      index = 0;
      startByteFound = true;
      endByteFound = false;
      RX_Message[index] = incomingByte;
      Forward[index] = incomingByte;
      index++;
    } else if (incomingByte == 0x03) {
      if (startByteFound) {
        RX_Message[index] = incomingByte;
        Forward[index] = incomingByte;
        index++;
        endByteFound = true;
        break;
      }
    } //else if (startByteFound && incomingByte != 0x00) {
      else{
      RX_Message[index] = incomingByte;
      Forward[index] = incomingByte;
      index++;
      // Check for buffer overflow
      if (index >= sizeof(RX_Message) - 1) {
        Serial.println("Buffer overflow");
        break;
      }
    }
  }

  if (!endByteFound) {
    Serial.println("Invalid start or end byte");
  }

  // Null-terminate the string
  RX_Message[index] = '\0';
  Forward[index] = '\0';

  // Process the message if both start and end bytes were found
  unsigned char address = 0;
  if (startByteFound && endByteFound) {
    address = Decode_Message(RX_Message, &Sender_Address, &Sender_Node_Type, RX_Message_Payload);
  } else {
    Serial.println("Message was not processed due to incomplete start/end bytes");
  }

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
  RS485Serial.begin(Baud);
  digitalWrite(pin->CE, LOW);
  #else
  pinMode(pin->Monitor, INPUT);
  pinMode(pin->CE, OUTPUT);
  RS485Serial.begin(Baud, SERIAL_8N1, pin->RX, pin->TX);  //hardware serial for ESP
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

// void Load_Vitals(){
//    // Initialize storedValue to check if the EEPROM has been written
//   int checkValue = EEPROM.read(location_address);

//   // Check if the stored value is valid (i.e., not uninitialized EEPROM content)
//   if (checkValue != 255) { // EEPROM is initially set to 255 (default erased state)
//     Home_Address = checkValue;
//     Serial.print("Home Address read from EEPROM: ");
//     Serial.println(Home_Address);
//   } else {
//     Serial.println("No stored value found in EEPROM, initializing...");
//     //call the introduction  function
//     Introduction();
//   }
// }

// void Introduction() {
//   bool reply_received = 0;
//   Transmit_To_Bus((TX_Payload*)&Intro);
//   while (!reply_received) {
//     if(RS485Serial.available()){
//       Addressee = Read_Serial_Port();
//       if(Addressee == Home_Address) {
//         Home_Address = RX_Message_Payload[0];
//         EEPROM.write(location_address, Home_Address);
//         reply_received = 1;
//       }
//     }
//   }
// }
void Load_Vitals(){
   // Initialize storedValue to check if the EEPROM has been written
  int checkValue = EEPROM.read(location_address);

  // Check if the stored value is valid (i.e., not uninitialized EEPROM content)
  if (checkValue != 255) { // EEPROM is initially set to 255 (default erased state)
    Home_Address = checkValue;
    Serial.print("Home Address read from EEPROM: ");
    Serial.println(Home_Address);
    int j = 0;
    for(int i = ID_Address; i <  ID_Address + EEPROM.read(Size_Address); i++){
      Device_ID[j] = EEPROM.read(i);
      j++;
    }
  } else {
    Serial.println("No stored value found in EEPROM, initializing...");
    //call the introduction  function
    Introduction();
  }
}

void Introduction() {
  bool reply_received = 0;
  Transmit_To_Bus((TX_Payload*)&Intro);
  while (!reply_received) {
    if(RS485Serial.available()){
      Addressee = Read_Serial_Port();
      if(Addressee == Home_Address) {
        int colonIndex = -1; 
        for (int i = 0; i < sizeof(RX_Message_Payload); i++) { 
          Device_ID[i] = RX_Message_Payload[i];
          if (RX_Message_Payload[i] == ':') { 
            colonIndex = i; 
            break; 
          } 
        }
        Home_Address = RX_Message_Payload[colonIndex+1];
        int Size_Of = sizeof(RX_Message_Payload);
        EEPROM.write(location_address, Home_Address);
        EEPROM.write(Size_Address, Size_Of);
        for(int i = ID_Address; i < ID_Address + Size_Of; i++){
          EEPROM.write(i, Device_ID[i]);
        }
        reply_received = 1;
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

#if !defined(ARDUINO_AVR_NANO)
// ISR for multi thread applications
void IRAM_ATTR onUartRx() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  char data = RS485Serial.read(); // Read data from HardwareSerial

  if (bufferIndex < 40) {
    buffer[bufferIndex++] = data;

    // Check if the full message is received
    if (bufferIndex == MESSAGE_LENGTH || data == END_BYTE) {
      xQueueSendFromISR(RX_Queue, (const void*)buffer, &xHigherPriorityTaskWoken); // Cast buffer to const void*
      bufferIndex = 0; // Reset buffer index for the next message
    }
  }

  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

// RS485 serial port task
void RX_Message_Process(void *pvParameters) {
  unsigned char receivedMessage[MESSAGE_LENGTH];
  while (1) {
    if (xQueueReceive(RX_Queue, &receivedMessage, portMAX_DELAY)) {
      memset(RX_Message_Payload, 0, sizeof(RX_Message_Payload));
      Addressee = Decode_Message(receivedMessage, &Sender_Address, &Sender_Node_Type, RX_Message_Payload);
      // Process received message
      Serial.print("Received message: ");
      for (int i = 0; i < MESSAGE_LENGTH; i++) {
        Serial.print(receivedMessage[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      Serial.println((char*)RX_Message_Payload);
    }
  }
}
#endif


// //write function to reset address if reset_address message is received
// void Reset_Params(bool x){
//   if(x){
//     EEPROM.write(location_address, Home_Address);
//   }
//   else{
//     EEPROM.write(location_address, 0xFF);   //reset eeprom to default setting 11111111 this will force device to ask for a new address on boot
//   }
// }

//write function to reset address if reset_address message is received
void Reset_Params(bool x){
  if(x){
    int Size_Of = sizeof(RX_Message_Payload);
        EEPROM.write(location_address, Home_Address);
        EEPROM.write(Size_Address, Size_Of);
        for(int i = ID_Address; i < ID_Address + Size_Of; i++){
          EEPROM.write(i, Device_ID[i]);
        }
  }
  else{
    EEPROM.write(location_address, 0xFF);   //reset eeprom to default setting 11111111 this will force device to ask for a new address on boot
    for(int i = ID_Address; i <  ID_Address + EEPROM.read(Size_Address); i++){
        EEPROM.write(i, 0xFF);
    }
    EEPROM.write(Size_Address, 0xFF);

  }
}




















