#include "MQTT.hpp"
#include "Communication_Protocol.h"

#define RedPin_1    23
#define RedPin_2    22
#define YellowPin   21
#define resolution  8

volatile unsigned char buffer[MESSAGE_LENGTH];
volatile unsigned char bufferIndex = 0;
bool I_am_Forwarder = false;
bool Forward_To_MQTT = false;

void RX_Message_Process(void *pvParameters);
void IRAM_ATTR onUartRx();

QueueHandle_t RX_Queue;



TaskHandle_t RX_Message_Handle;

void setup() {
  // // Start Serial communication
  Serial.begin(9600);
  //call function to set up correct communication pins and serial port for the board in use
  Comms_Set_Up();
  Serial.println("Hello");
  mqttSetUp();
  ledcAttach(RedPin_1, 4, resolution);
  ledcAttach(RedPin_2, 4, resolution);
  ledcAttach(YellowPin, 4, resolution);

  RX_Queue = xQueueCreate(10, MESSAGE_LENGTH * sizeof(char));
  // Create task (runs on core 0)
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
  RS485Serial.onReceive(onUartRx); // Attach the interrupt handler
}


void loop() {
  // put your main code here, to run repeatedly:

}

// RS485 serial port task
void RX_Message_Process(void *pvParameters) {
  unsigned char receivedMessage[MESSAGE_LENGTH];
  while (1) {
    if (xQueueReceive(RX_Queue, &receivedMessage, portMAX_DELAY)) {
      memset(RX_Message_Payload, 0, sizeof(RX_Message_Payload));
      Addressee = Decode_Message(receivedMessage, &Sender_Address, &Sender_Node_Type, RX_Message_Payload);
      // Process received message

      if((Addressee == MQTT_Address) && (I_am_Forwarder)) {
        // parse message into correct
        } mqtt message struct
        Forward_To_MQTT = true;  //set flag for mqtt thread
      }
      if(Addressee == Home_Address){
        
      }
      if(Sender_Node_Type == Intrusion_Node || Fire_Node){
        if(RX_Message_Payload == "Alarm"){
          ledcWrite(RedPin_1, 128);
          ledcWrite(RedPin_2, 64);
        }
      }
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

// // PWM configuration
// const int pwmFrequency = 5000; // Frequency in Hz
// const int pwmResolution = 8;   // Resolution in bits
// const int led1Channel = 0;     // PWM channel for LED 1
// const int led2Channel = 1;     // PWM channel for LED 2

// void setup() {
//   // Configure PWM for LED 1
//   ledcSetup(led1Channel, pwmFrequency, pwmResolution);
//   ledcAttachPin(led1Pin, led1Channel);
  
//   // Configure PWM for LED 2
//   ledcSetup(led2Channel, pwmFrequency, pwmResolution);
//   ledcAttachPin(led2Pin, led2Channel);

//   // Set initial duty cycles
//   ledcWrite(led1Channel, 128); // 50% duty cycle
//   ledcWrite(led2Channel, 64);  // 25% duty cycle
// }

// void loop() {
//   // Do other tasks while PWM runs in the background
//   Serial.println("Doing other tasks...");
//   delay(1000); // Simulate other tasks with a delay
// }
