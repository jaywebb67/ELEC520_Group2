// #define START_BYTE 0x02
// #define END_BYTE 0x03
#include "Communication_Protocol.h"

uint8_t Home_Address = 0x12;
uint8_t Destination_Address = 0x00;

unsigned char message[40]; // Increased size to accommodate additional bytes

// struct TX_Payload {
//   unsigned char length;
//   char message[35];
// };

struct TX_Payload Fire_1 = {23, "Call button x activated"};

void setup() {
  Serial.begin(9600); // Initialize serial communication
}

void loop() {
  Assemble_Message(&Fire_1, message); // Assemble message
  Serial.write(message, 5 + Fire_1.length); // Print the message byte array
  Serial.println();
  Print_Message(message, 6 + Fire_1.length); // Print the message in hex format
  delay(2000); // Delay for 2 seconds
}

// void Print_Message(unsigned char* message, unsigned char length) {
//   for (unsigned char i = 0; i < length; i++) {
//     Serial.print("0x");
//     if (message[i] < 0x10) Serial.print("0");
//     Serial.print(message[i], HEX);
//     Serial.print(" ");
//   }
//   Serial.println();
// }



// unsigned char Calculate_Checksum(struct TX_Payload* data) {
//     unsigned char checksum = 0;
//     for (unsigned char i = 0; i < data->length; i++) {
//         checksum ^= data->message[i];
//     }
//     return checksum;
// }

// void Assemble_Message(struct TX_Payload* data, unsigned char* message) {
//     message[0] = START_BYTE;             // Start byte
//     message[1] = Home_Address;           // Sender Address byte
//     message[2] = Destination_Address;    // Destination Address byte
//     message[3] = data->length;           // Length byte
    
//     for (unsigned char i = 0; i < data->length; i++) {
//         message[4 + i] = data->message[i]; // Payload
//     }
    
    
//     message[4 + data->length] = Calculate_Checksum(data); // Checksum
//     message[5 + data->length] = END_BYTE;                // End byte
// }
