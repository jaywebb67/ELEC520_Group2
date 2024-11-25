#include <Arduino.h>
#include <BluetoothSerial.h>
#include <LiquidCrystal_I2C.h>
#include "Char_Buffer.h"
#include "Communication_Protocol.h"
//#include "Gate_Function.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define DEVICE_NAME "Gate 1"

BluetoothSerial SerialBT;

//const uint8_t Home_Node_Type = 0x35;
uint8_t Home_Address = 0x13;
uint8_t Destination_Address = 0x28;
// Create a CharBuffer object with 10 entries, each of size 6 characters 
CharBuffer Valid_Entrance_Codes(100, 6);
CharBuffer Current_Codes_In_Use(100, 6);

//ESP32 WROOM pins
const int rowPins[4] = {18, 19, 21, 22};     // Row pins connected to the keypad
const int colPins[4] = {23, 25, 26, 27};     // Column pins connected to the keypad
char Input_Key_Code[7];
char BT_Key_Code[7];
char Start_Pass[7] = {"012345"};
// volatile unsigned char buffer[MESSAGE_LENGTH];
// volatile unsigned char bufferIndex = 0;
volatile uint32_t bytes_Received = 0;
unsigned long debounceDelay = 200;        // Debounce time in milliseconds
volatile int keypresses = 0;

volatile unsigned long lastInterruptTime = 0;
volatile bool isPressed = false;

// Define task handles
TaskHandle_t Keypad_Reader = NULL;
TaskHandle_t Bluetooth_Task_Handle = NULL; // Task handle for Bluetooth task


void IRAM_ATTR Key_Pressed_ISR() {
  unsigned long interruptTime = millis();
  // Debounce logic
  if ((interruptTime - lastInterruptTime > debounceDelay) && !isPressed) {
    isPressed = true;
    keypresses++;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(Keypad_Reader, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // Perform a context switch if needed
  }
  lastInterruptTime = interruptTime;
}


// void onBluetoothDataReceived(const uint8_t *data, size_t len) {
//   //Serial.println("Bluetooth data received");
//   // Notify the Bluetooth task
//   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//   vTaskNotifyGiveFromISR(Bluetooth_Task_Handle, &xHigherPriorityTaskWoken);
//   portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // Perform a context switch if needed
// }
void onBluetoothDataReceived(const uint8_t *data, size_t len) {
  // Copy received data to global buffer
  bytes_Received = (len < sizeof(BT_Key_Code)) ? len : sizeof(BT_Key_Code) - 1; // Ensure no buffer overflow
  memcpy(BT_Key_Code, data, bytes_Received);
  BT_Key_Code[bytes_Received] = '\0'; // Null-terminate the string

  // Notify the Bluetooth task
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(Bluetooth_Task_Handle, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // Perform a context switch if needed
}


//Function to check recieved keycodes
int Test_Entry_Code(const char* code){
  int x = Valid_Entrance_Codes.searchEntry(code);
  if (x < 0) {
    Serial.println("Access Denied");
    return 1;
  } 
  else {
    x = Current_Codes_In_Use.searchEntry(code);
    if (x < 0) {
      Serial.println("Access Granted");
      Current_Codes_In_Use.addEntry(code);
      Current_Codes_In_Use.printBuffer();
      //Notify_Alarm_Node();
      return 2;
    } 
    else {
      Serial.println("Exit Goodbye");
      Current_Codes_In_Use.deleteEntry(x); 
      Current_Codes_In_Use.printBuffer();
      //Notify_Alarm_Node();
      return 3;
    }
  }
}

// const int rowPins[4] = {12, 14, 27, 26};     // Row pins connected to the keypad
// const int colPins[4] = {25, 23, 32, 35};     // Column pins connected to the keypad
// Task 1 function 
void Keypad_Read(void *pvParameters) {
  bool keyPressed = false;
  uint8_t Valid_Input_Presses = 0;

  // Keypad layout (row-major order)
  char keypad[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
  };
  int pressedRow = -1;  // Row where the key press occurred
  int pressedCol = -1; // Column where the key press occurred
  bool multipleKeysPressed = false; // Flag for multiple key presses

  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //wait for flag
    // Code for task 1
    Serial.println("Entering task 1.");
   
    for (int i = 0; i < 4; i++) {
      //detachInterrupt(digitalPinToInterrupt(colPins[i]));  // Disable interrupts on all columns
      digitalWrite(rowPins[i], HIGH); // Reset rows to HIGH (idle state)
    }
   
    //vTaskDelay(pdMS_TO_TICKS(debounceDelay)); // Sleep for 50ms to debounce switch
     //Serial.println("Interrups should be disabled.");
    // Scan for key press in all rows and columns
    int pressedCount = 0;
    int foundRow = -1;
    int foundCol = -1;

    //Scan all rows and columns to determine which key was pressed
    for (int row = 0; row < 4; row++) {
      digitalWrite(rowPins[row], LOW); // Activate current row

      for (int col = 0; col < 4; col++) {
        if (digitalRead(colPins[col]) == LOW) {
          pressedCount++;
          foundRow = row;
          foundCol = col;
        }
      }

      digitalWrite(rowPins[row], HIGH); // Deactivate row
    }

    // Process single or multiple key presses 
    if (pressedCount > 1) { 
      //Send_To_LCD_Queue(Invalid_Mess);
      Serial.println("Multiple keys detected, rejecting input."); 
    } 
    else if (pressedCount == 1) 
    { char key = keypad[foundRow][foundCol]; // Set the key 
    Input_Key_Code[Valid_Input_Presses] = key; 
    Valid_Input_Presses++; 
    Serial.print("Key pressed: "); 
    Serial.println(key);

      if(Valid_Input_Presses == 6){
        // Process the valid 6-byte code
         int y = Test_Entry_Code(Input_Key_Code);
        // notify_User(2, y);
        Serial.println(y);
        Serial.println(Input_Key_Code);
        Valid_Input_Presses = 0;
      }
      // Reset key press flag and update debounce time
      keyPressed = false;
    }
    // Reattach interrupts after debounce delay to avoid immediate retriggering
    for (int i = 0; i < 4; i++) {
      //attachInterrupt(digitalPinToInterrupt(colPins[i]), Key_Pressed_ISR, FALLING);
      digitalWrite(rowPins[i], LOW); // Reset rows to LOW for interrupt to work
    }
    // Reset pressed flag 
      isPressed = false;
      //Send_To_LCD_Queue(Enter_Mess);
  }
}


// // Task 2 function 
// void Process_BT_Message(void *pvParameters) {
//   uint32_t bytes_Received = 0;
//   while (true) {
//     // Wait for the notification from ISR 
//     ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//     Serial.println("BT task entered");
//     while (SerialBT.available() && bytes_Received < 6) {
//       char incomingByte = SerialBT.read();
//       BT_Key_Code[bytes_Received] = incomingByte;
//       Serial.print("Received: ");
//       Serial.println(incomingByte);
//       bytes_Received++;
//     }

//     // Check if more than 6 bytes are received
//     if (bytes_Received > 6 || SerialBT.available()) {
//       Serial.println("Invalid Code_1");
//       SerialBT.println("Invalid Code_1");
//       bytes_Received = 0; // Reset bytes_Received
//       // Send to LCD or turn on a red LED to indicate the error
//       while (SerialBT.available()) { // Clear the buffer
//         SerialBT.read();
//       }
//     } else if (bytes_Received != 6) {
//       Serial.println("Invalid Code_2");
//       SerialBT.println("Invalid Code_2");
//       SerialBT.println(bytes_Received);
//       bytes_Received = 0; // Reset bytes_Received
//       // Send to LCD or turn on a red LED to indicate the error
//     } 
//     else {
//       // Process the valid 6-byte code
//       int y = Test_Entry_Code(Input_Key_Code);
//       //notify_User(1, y);
//       bytes_Received = 0; // Reset bytes_Received for the next code
//     }
//   }
// }
// void Process_BT_Message(void *pvParameters) {
//   // uint32_t bytes_Received = 0;
//   while (true) {
//     // Wait for the notification from ISR
//     ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//     //Serial.println("BT task entered");

//     bytes_Received = 0; // Ensure bytes_Received is reset each time task is entered

//     // Small delay to give the buffer time to fill
//     //vTaskDelay(pdMS_TO_TICKS(50)); 

//     while (SerialBT.available() && bytes_Received < 6) {
//       char incomingByte = SerialBT.read();
//       BT_Key_Code[bytes_Received] = incomingByte;
//       Serial.print("Received: ");
//       Serial.println(incomingByte);
//       bytes_Received++;
//     }

//     // Debug statement to show bytes received
//     Serial.print("Bytes received: ");
//     Serial.println(bytes_Received);

//     // Check if more than 6 bytes are received
//     if (bytes_Received > 6 || SerialBT.available()) {
//       Serial.println("Invalid Code_1");
//       SerialBT.println("Invalid Code_1");
//       bytes_Received = 0; // Reset bytes_Received
//       // Send to LCD or turn on a red LED to indicate the error
//       while (SerialBT.available()) { // Clear the buffer
//         SerialBT.read();
//       }
//     } else if (bytes_Received != 6) {
//       Serial.println("Invalid Code_2");
//       SerialBT.println("Invalid Code_2");
//       SerialBT.println(bytes_Received);
//       bytes_Received = 0; // Reset bytes_Received
//       // Send to LCD or turn on a red LED to indicate the error
//     } else {
//       // Process the valid 6-byte code
//       int y = Test_Entry_Code(BT_Key_Code);
//       // notify_User(1, y);
//       bytes_Received = 0; // Reset bytes_Received for the next code
//     }
//   }
// }

void Process_BT_Message(void *pvParameters) {
  while (true) {
    // Wait for the notification from ISR 
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    Serial.println("BT task entered");

    // Check if the length of received data is 6
    if (bytes_Received == 6) {
      // Process the valid 6-byte code
      int y = Test_Entry_Code(BT_Key_Code);
      //notify_User(1, y);
      SerialBT.println("Valid Code");
    } else {
      Serial.println("Invalid Code");
      SerialBT.println("Invalid Code");
    }

    bytes_Received = 0; // Reset bytes_Received for the next code
  }
}




void setup() {
 // Start Serial communication
  Serial.begin(9600);
  //call function to set up correct communication pins and serial port for the board in use
  //Comms_Set_Up();
  Serial.println("Hello");
  // //LCD setup
  // lcd.init();
  // lcd.setBacklight(255);
  // lcd.backlight();

  // Initialize queue
  // RX_Queue = xQueueCreate(10, MESSAGE_LENGTH * sizeof(char));
  // LCD_Queue = xQueueCreate(10, sizeof(QueueItem));
  
  // Create Task 1 (runs on Core 0 by default)
  xTaskCreatePinnedToCore(
    Keypad_Read,               // Task function
    "Keypad_Read",            // Task name
    10000,               // Stack size (bytes)
    NULL,                // Parameters passed to the task
    3,                   // Task priority (higher is higher priority)
    &Keypad_Reader,        // Task handle
    0                    // Core 0
  );
  // Initialize row & column pins 
  for (int i = 0; i < 4; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], LOW);  // Keep rows LOW initially (inactive)
    pinMode(colPins[i], INPUT_PULLUP);
  }
   // Set up interrupts on the column pins to detect a key press
  for (int i = 0; i < 4; i++) {
    attachInterrupt(digitalPinToInterrupt(colPins[i]), Key_Pressed_ISR, FALLING);
  }

  Valid_Entrance_Codes.addEntry(Start_Pass);
  Valid_Entrance_Codes.printBuffer();
  // Create Task 2 (runs on Core 0)
  xTaskCreatePinnedToCore(
     Process_BT_Message,        // Function to implement the task
    "Process_BT_Message",     // Name of the task
    10000,                    // Stack size in words
    NULL,                     // Task input parameter
    3,                        // Priority of the task
    &Bluetooth_Task_Handle,   // Task handle
    0                         // Core where the task should run
  );

  // // // Create task 3 (rus on core 0)
  // // xTaskCreatePinnedToCore(
  // //    RX_Message_Process,  // Task function. 
  // //   "RX_Message_Process",     // name of task. 
  // //   10000,                    // Stack size of task 
  // //   NULL,                     // parameter of the task 
  // //   2,                        // priority of the task 
  // //   &RX_Message_Handle,      // Task handle to keep track of created task 
  // //   0                         // pin task to core 0 
  // //   );                       

  // // Create task 4 (runs on core 0)
  // xTaskCreatePinnedToCore(
  //    LCD_Thread,            //Task function
  //   "LCD_Thread",               //name of task
  //   10000,                      //stacksize of task
  //   NULL,                       //parameter of task
  //   1,                          //priority of task
  //   &LCD_Thread_Handle,                //Task handle to keep track of created task 
  //   0                           //pin task to core 0
  // );
  
  

  // Initialize BluetoothSerial and set up the callback 
  SerialBT.onData(onBluetoothDataReceived);
  SerialBT.begin(DEVICE_NAME);
  Serial.println("The device started, now you can pair it with Bluetooth!");

  // Attach the interrupt to the Bluetooth serial available method
  //attachInterrupt(digitalPinToInterrupt(SerialBT.available()), Bluetooth_ISR, RISING);
  // Initialize BluetoothSerial and set up the callback SerialBT.onData(onBluetoothDataReceived);
 

  // // // Attach UART interrupt 
  // // RS485Serial.onReceive(onUartRx); // Attach the interrupt handler

  // Enter_Mess(); LCD default screen
}

void loop() {
  // put your main code here, to run repeatedly:

}
