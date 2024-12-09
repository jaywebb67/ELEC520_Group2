
// volatile unsigned char buffer[MESSAGE_LENGTH];
// volatile unsigned char bufferIndex = 0;

// void IRAM_ATTR onUartRx() {
//   char data = UART2.read();
//   if (bufferIndex < MESSAGE_LENGTH) {
//     buffer[bufferIndex++] = data;
    
//     if (bufferIndex == MESSAGE_LENGTH || data == END_BYTE) {
//       BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//       xQueueSendFromISR(RX_Queue, buffer, &xHigherPriorityTaskWoken);
//       bufferIndex = 0; // Reset buffer index for the next message
//       if (xHigherPriorityTaskWoken) {
//         portYIELD_FROM_ISR();
//       }
//     }
//   }
// }


// void RX_Message_Process_Code(void *pvParameters) {
//   unsigned char receivedMessage[MESSAGE_LENGTH];
//   while (1) {
//     if (xQueueReceive(RX_Queue, &receivedMessage, portMAX_DELAY)) {
//       // Process the complete message outside ISR
//       Serial.print("Received message: ");
//       for (int i = 0; i < MESSAGE_LENGTH; i++) {
//         Serial.print(receivedMessage[i], HEX);
//         Serial.print(" ");
//       }
//       Serial.println();
//     }
//   }
// }


// #include <Arduino.h>

// // UART Configuration


// #define START_BYTE 0x02
// #define END_BYTE 0x03


// HardwareSerial RS485Serial(2);








// void setup() {
//   Serial.begin(115200);

//   // Configure UART
//   RS485Serial.begin(9600, SERIAL_8N1, pin->RX, pin->TX);

  

 


//   // Create tasks
//   xTaskCreatePinnedToCore(
//     RX_Message_Process_Code,   /* Task function. */
//     "RX_Message_Process",     /* name of task. */
//     10000,                 /* Stack size of task */
//     NULL,                  /* parameter of the task */
//     1,                     /* priority of the task */
//     &RX_Message_Process,      /* Task handle to keep track of created task */
//     0);                    /* pin task to core 0 */

//   if (RX_Message_Process == NULL) {
//     Serial.println("Task creation failed.");
//   } else {
//     Serial.println("Task created successfully.");
//   }
// }

// void loop() {
//   // Your main loop code
// }

// void RX_Message_Process_Code(void *pvParameters) {
//   unsigned char receivedMessage[MESSAGE_LENGTH];
//   while (1) {
//     if (xQueueReceive(RX_Queue, &receivedMessage, portMAX_DELAY)) {

//       Addressee = Decode_Message(receivedMessage, &Sender_Address, &Sender_Node_Type, RX_Message_Payload);
//       // Process received message
//       Serial.print("Received message: ");
//       for (int i = 0; i < MESSAGE_LENGTH; i++) {
//         Serial.print(receivedMessage[i], HEX);
//         Serial.print(" ");
//       }
//       Serial.println();
//     }
//   }
// }



// void keyPrompt(){
//   lcd.setCursor(0,0);
//   lcd.print("   Enter Key:   ");
//    // Calculate how many underscores to display
//   int remainingSpaces = passwordLength - currentInput.length();

//   lcd.setCursor(4,1);
//   lcd.print("[");
//   lcd.setCursor(11,1);
//   lcd.print("]");
//   lcd.setCursor(5,1);
//   lcd.print(currentInput);
//   Serial.println(currentInput);
//     // Display remaining underscores
//   for (int i = 0; i < remainingSpaces; i++) {
//     lcd.print("_");}
// }

// void checkPassword(){
//     lcd.setCursor(0,0);
//     lcd.print("     Access     ");
//     lcd.setCursor(0,1);
//     if (currentInput == setPassword) {
//       lcd.print("     Granted    ");
//       delay(2);
//       loop();
//     } else 
//     {
//       lcd.print("     Denied     ");
//       delay(2);
//       loop();

//     }
//     return;
// }





























