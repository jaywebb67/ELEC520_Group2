#include <HardwareSerial.h>

HardwareSerial SerialPort(2); // use UART2

char TX_message[] = {"Hello world"};
void setup()  
{
  SerialPort.begin(15200, SERIAL_8N1, 16, 17); 
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("first passed");
  digitalWrite(4, HIGH);
  SerialPort.write(TX_message, 11);
  SerialPort.flush();
  digitalWrite(4, LOW);
  delay(2000);
}

