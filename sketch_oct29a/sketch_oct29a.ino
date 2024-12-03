
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
  Serial.println(message->size);
}

void loop() {
  // put your main code here, to run repeatedly:

}


