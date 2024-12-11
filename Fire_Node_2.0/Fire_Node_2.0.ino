/*
 *ELEC_520 
 *authored by Alex Meredith
*/

#include "Communication_Protocol.h"
#include "DHT.h"

#define DHTPIN 5     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define Alarm_Pin 3 
#define Sensor_Read_Interval  2000
#define RedPin  7

   //need to send location byte may need to be variable to fit in struct
// uint8_t Location;
uint8_t Home_Node_Type = 0x32;
uint8_t Home_Address = 0x32;    //Default address for initial set up
uint8_t Destination_Address = 0x13;   //Default address for initial set up
uint8_t Node_3 = 0x33;
const char Respond_Cmd[8] = "RESPOND";
const char Reset_Cmd[6] = "RESET";
volatile bool alarm_Pressed = false;
String LED = "LED";
const uint32_t Max_Temp = 45;
unsigned long Last_Time = 0;
uint32_t count = 0; 


const struct TX_Payload Fire_1 = {9, "Fire Call"};
const struct TX_Payload Fire_2 = {12, "Sensor error"};
const struct TX_Payload Fire_3 = {10, "Heat Alarm"};
const struct TX_Payload Alive = {11, "Fire online"};
struct TX_Payload Hello = {2, "Hi"};

DHT dht(DHTPIN, DHTTYPE);

void alarm_Pressed_interrupt() {
  alarm_Pressed = true;
  //Serial.println("ISR");
}

void Test_Heat_Sensor() {
  // Reading temperature takes about 250 milliseconds!
  float t = dht.readTemperature();
  count++;
  if (isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    Serial.println();
    Transmit_To_Bus(&Fire_2);
  }

  // Serial.print("Temperature read = ");
  // Serial.println(t);
  t += random(1, 25); // Simulate temperature fluctuation for testing
  // Serial.print("New Temperature = ");
  // Serial.println(t);
  
  if (t > Max_Temp) {
    Serial.println("Maximum allowable temperature exceeded");
    Transmit_To_Bus(&Fire_3);
    digitalWrite(RedPin, HIGH);
  }
  
  else if (count == 10) {
      struct TX_Payload Fire_temp;


      // Check for NAN before proceeding
      if (isnan(t)) {
          Serial.println("Error: Temperature is NAN");
          return;  // Exit early to avoid corrupt data
      }

      // Convert float to char array using dtostrf
      char tempBuffer[16];  // Buffer for the temperature string
      dtostrf(t, 6, 2, tempBuffer); // Width = 6, Precision = 2 (e.g., " 23.45")

      // Safely copy into the payload structure
      Fire_temp.length = strlen(tempBuffer);  // Length of the message
      strncpy(Fire_temp.message, tempBuffer, sizeof(Fire_temp.message));
      Fire_temp.message[sizeof(Fire_temp.message) - 1] = '\0';  // Ensure null termination

      // Debug print to verify the conversion
      Serial.print("Formatted Temperature String: ");
      Serial.println(tempBuffer);

      // Save the current destination address and transmit the payload
      uint8_t temp = Destination_Address;
      Destination_Address = 0x0A;
      Transmit_To_Bus(&Fire_temp);
      Destination_Address = temp;

      count = 0;  // Reset the counter
  }




}

void setup() {
  Serial.begin(9600);
  Comms_Set_Up();
  pinMode(Alarm_Pin, INPUT);
  pinMode(RedPin, OUTPUT);
  dht.begin();
  attachInterrupt(digitalPinToInterrupt(Alarm_Pin),alarm_Pressed_interrupt, FALLING);
  alarm_Pressed = false;
  
}

void loop() {
  if(RS485Serial.available()){
    Addressee = Read_Serial_Port();
    //Serial.println((char*)RX_Message_Payload);
  
    if(Addressee == Home_Address){
      Serial.println((char*)RX_Message_Payload);
      // Serial.print("Sender's address: ");
      // Serial.println(Sender_Address, HEX);
      // Serial.print("Sender is a node of type: ");
      // Serial.println(Sender_Node_Type, HEX);
      // Handle specific commands
      if (strcmp((char*)RX_Message_Payload, Reset_Cmd) == 0) {
        Serial.println("Restting alarm...");
        alarm_Pressed = false;
        digitalWrite(RedPin, LOW);
      }
      if (strcmp((char*)RX_Message_Payload, Respond_Cmd) == 0) {
        Serial.println("responding");
        uint8_t temp = Destination_Address;
        Destination_Address = 0x0A;
        Transmit_To_Bus(&Alive);
        Destination_Address = temp;
      }
    }
    else if((Sender_Address == Node_3)&&(Addressee == Home_Address )){
      Serial.println((char*)RX_Message_Payload);
      Serial.print("Sender's address: ");
      Serial.println(Sender_Address, HEX);
    }
    else{
      // Serial.print("Address mismatch: Received ");
      // Serial.print(Addressee, HEX);
      // Serial.print(", Expected ");       
      // Serial.println(Home_Address, HEX);
    }
  }

  if (alarm_Pressed) {
  Transmit_To_Bus(&Fire_1);
  digitalWrite(RedPin, HIGH);
  alarm_Pressed = false;
  }

  unsigned long Current_Time = millis();
  if(Current_Time - Last_Time >= Sensor_Read_Interval) {
    Test_Heat_Sensor();
    Last_Time = Current_Time;
  }
}
