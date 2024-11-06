#include "DHT.h"
//#include <SoftwareSerial.h>
#include <SoftwareSerial.h>
#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define CE 4
#define Alarm_Pin 3 
#define LED_B  13

// int maxHum = 60;
// int receivedData = 40;
bool alarm_Pressed = false;
String LED = "LED";

int t = 4675;
SoftwareSerial RS485Serial(10, 11);

void alarm_Pressed_interrupt() {
  alarm_Pressed = true;
}

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  pinMode(CE, OUTPUT);
  pinMode(Alarm_Pin, INPUT);
  pinMode(LED_B, OUTPUT);
  Serial.begin(9600); 
  RS485Serial.begin(9600);
  dht.begin();
  digitalWrite(CE, LOW);
  attachInterrupt(digitalPinToInterrupt(Alarm_Pin),alarm_Pressed_interrupt, FALLING);
}

void loop() {
  // Wait a few seconds between measurements.
  delay(2000);

  // if (RS485Serial.available()) {
  //   //Serial.print("Hi");
  //   // Read the received data
  //   String receivedData = RS485Serial.readString();
  //   //Serial.println(receivedData);
 
  //   if(receivedData == LED) 
  //     digitalWrite(LED_B, !digitalRead(LED_B));
  // }
  // // Reading temperature or humidity takes about 250 milliseconds!
  // // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // float h = dht.readHumidity();
  // // Read temperature as Celsius
  // // float t = dht.readTemperature();
  // int t = 39765;
  
  // // Check if any reads failed and exit early (to try again).
  // if (isnan(h) || isnan(t)) {
  //   Serial.println("Failed to read from DHT sensor!");
  //   return;
  // }

  // if(alarm_Pressed) {
  //   Serial.println("Fire alarm triggered");
  // }
  
  Serial.println(t);
  Serial.println("I'm here");
  digitalWrite(CE, HIGH);
  RS485Serial.write(t);
  digitalWrite(CE, LOW);

    // Print the received data to the serial monitor
    //Serial.println(receivedData);
   

}
