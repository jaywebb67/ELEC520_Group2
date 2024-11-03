#define START_BYTE 0x02
#define END_BYTE 0x03
#define ADDRESS_BYTE 0x01



//data structure set up for each nodes transmit messages
struct TX_Payload {
  unsigned char length;
  char message[35];
}

//unsigned char Calculate_Checksum(unsigned char* data, unsigned char length) {
unsigned char Calculate_Checksum(struct TX_Payload* data, unsigned char length) {
    unsigned char checksum = 0;
    for (unsigned char i = 0; i < length; i++) {
        //checksum ^= data[i];
        checksum ^= data->message[i];
    }
    return checksum;
}


//void Assemble_Message(unsigned char* payload, unsigned char payload_length, unsigned char* message) {
void Assemble_Message(struct TX_Payload* data, unsigned char* message) {
    message[0] = START_BYTE;             // Start byte
    message[1] = Home_Address;           // Sender Address byte
    message[2] = Destination_Address;    // Destination Address byte
    //message[3] = payload_length;
    message[3] = data->length;         // Length byte
    
    for (unsigned char i = 0; i < payload_length; i++) {
        //message[4 + i] = payload[i];     // Payload
        message[4 + i] = data->message[i];     // Payload
    }
    
    //message[4 + payload_length] = Calculate_Checksum(&message[1], payload_length + 2); // Checksum
    message[4 + data->length] = Calculate_Checksum(&message[1], data->length + 3); // Checksum
    //message[5 + payload_length] = END_BYTE;       // End byte
    message[5 + data->length] = END_BYTE;       // End byte
}


unsigned char payload[] = {'H', 'e', 'l', 'l', 'o'};
unsigned char message[10]; // Adjust size based on maximum payload length

Assemble_Message(payload, 5, message);

// Now `message` contains the assembled message ready for transmission


//**Rx code**//

#define START_BYTE 0x02
#define END_BYTE 0x03


unsigned char Decode_Message(unsigned char* message, unsigned char* payload, unsigned char* length) {
    if (message[0] != START_BYTE || message[4 + message[2]] != END_BYTE) {
        return 0; // Error: Invalid start or end byte
    }

    *length = message[2]; // Length byte
    unsigned char address = message[1]; // Address byte

    for (unsigned char i = 0; i < *length; i++) {
        payload[i] = message[4 + i];
    }
    unsigned char received_checksum = message[4 + *length];
    unsigned char calculated_checksum = Calculate_Checksum(&message[1], *length + 3);

    if (received_checksum != calculated_checksum) {
        return 0; // Error: Checksum mismatch
    }
    
    return address; // Return the address for further processing
}

unsigned char message[] = {0x02, 0x01, 0x07, 0x05, 'H', 'e', 'l', 'l', 'o', 0x5A, 0x03};
unsigned char payload[5];
unsigned char length;

unsigned char address = Decode_Message(message, payload, &length);

if (address) {
    // Successfully decoded the message
    // `payload` contains the data, and `length` is the number of bytes in the payload
} else {
    // Handle decoding error
}



void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}

// demo code to access struct data
struct greeting {
  unsigned char size;
  char salutation[25];
};

struct greeting hello = {10, "hello world"};

void setup() {
  Serial.begin(9600); // Initialize serial communication
  print_Struct(&hello); // Pass pointer to hello
}

void print_Struct(struct greeting* message) {
  Serial.println(message->salutation); // Print the actual message
}