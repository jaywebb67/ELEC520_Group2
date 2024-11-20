#include "Communication_Protocol.h"
#include "DHT.h"


#define DHTPIN 5     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define Alarm_Pin 3 
#define LED_B  13

const uint8_t Home_Node_Type = 0x32;
const uint8_t Home_Address = 0x12;
uint8_t Destination_Address = 0x28;
uint8_t Node_3 = 0x33;
const char Respond_Cmd[7] = "RESPOND";
const char Reset_Cmd[6] = "RESET";
volatile bool alarm_Pressed = false;
String LED = "LED";
const uint32_t Max_Temp = 40;

const struct TX_Payload Fire_1 = {21, "Call button activated"};
const struct TX_Payload Fire_2 = {12, "Sensor error"};
const struct TX_Payload Fire_3 = {21, "Fire sensor activated"};
const struct TX_Payload Fire_4 = {19, "Nothing to see here"};
struct TX_Payload Hello = {2, "Hi"};

DHT dht(DHTPIN, DHTTYPE);

void alarm_Pressed_interrupt() {
  alarm_Pressed = true;
}

void setup() {
  Serial.begin(9600);
  
  pinMode(Alarm_Pin, INPUT);
  pinMode(LED_B, OUTPUT);
  dht.begin();
  attachInterrupt(digitalPinToInterrupt(Alarm_Pin),alarm_Pressed_interrupt, FALLING);
  alarm_Pressed = false;
}

void loop() {
  if(RS485Serial.available()){
    Addressee = Read_Serial_Port();
  

  if(Addressee == Home_Address){
      Serial.println((char*)RX_Message_Payload);
      Serial.print("Sender's address: ");
      Serial.println(Sender_Address, HEX);
      Serial.print("Sender is a node of type: ");
      Serial.println(Sender_Node_Type, HEX);
      // Handle specific commands
      if (strcmp((char*)RX_Message_Payload, Reset_Cmd) == 0) {
        alarm_Pressed = false;
      }
      if (strcmp((char*)RX_Message_Payload, Respond_Cmd) == 0) {
        Transmit_To_Bus(&Fire_4);
      }
    }
    else if((Sender_Address == Node_3)&&(Addressee == Home_Address )){
      Serial.println((char*)RX_Message_Payload);
      Serial.print("Sender's address: ");
      Serial.println(Sender_Address, HEX);
    }
    else{
      Serial.print("Address mismatch: Received ");
      Serial.print(Addressee, HEX);
      Serial.print(", Expected ");       
      Serial.println(Home_Address, HEX);
    }
  }

  // Reading temperature or humidity takes about 250 milliseconds!
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    Serial.println();
    Transmit_To_Bus(&Fire_2);
  }

  Serial.print("Temperature read = ");
  Serial.println(t);
  t += random(1, 30); // Simulate temperature fluctuation for testing
  Serial.print("New Temperature = ");
  Serial.println(t);
  
  if (t > Max_Temp) {
    Serial.println("Maximum allowable temperature exceeded");
    Transmit_To_Bus(&Fire_3);
  }
  
  if (alarm_Pressed) {
    Serial.println("Fire alarm triggered");
    Transmit_To_Bus(&Fire_1);
  }

   int x = random(1, 100);
  if(x>90){
    Destination_Address = 0x33;
    Transmit_To_Bus(&Hello);
    Destination_Address = 0x28;
  }
  delay(2000);
}
