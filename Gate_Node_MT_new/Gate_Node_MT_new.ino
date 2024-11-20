#include <Arduino.h>
#include <BluetoothSerial.h>
#include "Char_Buffer.h"
#include "Communication_Protocol.h"
//#include <cstring>

BluetoothSerial SerialBT;
volatile bool btMessageFlag = false; // Global flag

// Create a CharBuffer object with 10 entries, each of size 6 characters 
CharBuffer Valid_Entrance_Codes(100, 6);
CharBuffer Current_Codes_In_Use(100, 6);

const int rowPins[4] = {8, 3, 46, 9};     // Row pins connected to the keypad
const int colPins[4] = {10, 11, 12, 13};     // Column pins connected to the keypad
char Input_Key_Code[6];
char BT_Key_Code[6];
unsigned long debounceDelay = 200;        // Debounce time in milliseconds
volatile int keypresses = 0;

const struct TX_Payload;    //put messages to transmit here

// Define two task handles
TaskHandle_t Keypad_Reader = NULL;
TaskHandle_t Task2Handle = NULL;
TaskHandle_t Bluetooth_Task_Handle = NULL; // Task handle for Bluetooth task

volatile unsigned long lastInterruptTime = 0;
volatile bool isPressed = false;

void IRAM_ATTR Bluetooth_ISR() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(Bluetooth_Task_Handle, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // Perform a context switch if needed
}


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




// Task 1 function (runs on Core 0 by default)
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
    // Disable further interrupts to avoid multiple triggers for the same key press
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
        int x = Valid_Entrance_Codes.searchEntry(Input_Key_Code);
        if (x < 0) {
          Serial.println("Access Denied");
        } else {
          x = Current_Codes_In_Use.searchEntry(Input_Key_Code);
          if (x < 0) {
            Serial.println("Access Granted");
            Current_Codes_In_Use.addEntry(Input_Key_Code);
          } else {
            Serial.println("Exit Goodbye");
            Current_Codes_In_Use.deleteEntry(x); 
          }
        }
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

      // Reset pressed flag 
      isPressed = false;
    }
  }
}

// Task 2 function (runs on Core 0)
void Process_BT_Message(void *pvParameters) {
  uint32_t bytes_Received = 0;
  while (true) {
    // Wait for the notification from ISR 
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    while (SerialBT.available() && bytes_Received < 6) {
      char incomingByte = SerialBT.read();
      BT_Key_Code[bytes_Received] = incomingByte;
      Serial.print("Received: ");
      Serial.println(incomingByte);
      bytes_Received++;
    }

    // Check if more than 6 bytes are received
    if (bytes_Received > 6 || SerialBT.available()) {
      Serial.println("Invalid Code");
      bytes_Received = 0; // Reset bytes_Received
      // Send to LCD or turn on a red LED to indicate the error
      while (SerialBT.available()) { // Clear the buffer
        SerialBT.read();
      }
    } else if (bytes_Received != 6) {
      Serial.println("Invalid Code");
      bytes_Received = 0; // Reset bytes_Received
      // Send to LCD or turn on a red LED to indicate the error
    } else {
      // Process the valid 6-byte code
      int x = Valid_Entrance_Codes.searchEntry(BT_Key_Code);
      if (x < 0) {
        Serial.println("Access Denied");
      } else {
        x = Current_Codes_In_Use.searchEntry(BT_Key_Code);
        if (x < 0) {
          Serial.println("Access Granted");
          Current_Codes_In_Use.addEntry(BT_Key_Code);
        } else {
          Serial.println("Exit Goodbye");
          Current_Codes_In_Use.deleteEntry(x); 
        }
      }
      bytes_Received = 0; // Reset bytes_Received for the next code
    }
  }
}
  


void setup() {
  // Start Serial communication
  Serial.begin(9600);
  //call function to set up correct communication pins and serial port for the board in use
  Comms_Set_Up();
  
  // Create Task 1 (this will run on Core 0 by default)
  xTaskCreatePinnedToCore(
    Keypad_Read,               // Task function
    "Keypad_Read",            // Task name
    10000,               // Stack size (bytes)
    NULL,                // Parameters passed to the task
    1,                   // Task priority (higher is higher priority)
    &Keypad_Reader,        // Task handle
    0                    // Core 0
  );

  // Create Task 2 (this will run on Core 0)
  xTaskCreatePinnedToCore(
     Process_BT_Message,        // Function to implement the task
    "Process_BT_Message",     // Name of the task
    10000,                    // Stack size in words
    NULL,                     // Task input parameter
    1,                        // Priority of the task
    &Bluetooth_Task_Handle,   // Task handle
    0                         // Core where the task should run
  );

  // Initialize row pins as OUTPUT
  for (int i = 0; i < 4; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], LOW);  // Keep rows LOW initially (inactive)
  }

  // Initialize column pins as INPUT with internal pull-ups
  for (int i = 0; i < 4; i++) {
    pinMode(colPins[i], INPUT_PULLUP);
  }

  SerialBT.begin("ESP32"); // Bluetooth device name
  Serial.println("The device started, now you can pair it with Bluetooth!");

  // Attach the interrupt to the Bluetooth serial available method
  attachInterrupt(digitalPinToInterrupt(SerialBT.available()), Bluetooth_ISR, RISING);
  
  // Set up interrupts on the column pins to detect a key press
  for (int i = 0; i < 4; i++) {
    attachInterrupt(digitalPinToInterrupt(colPins[i]), Key_Pressed_ISR, FALLING);
  }
}

void loop() {
  // Main loop can be used to handle other tasks or configurations
  // For this example, we're just letting the tasks handle the output.
  delay(1000);
}

/*
volatile unsigned char buffer[MESSAGE_LENGTH];
volatile unsigned char bufferIndex = 0;

void IRAM_ATTR onUartRx() {
  char data = UART2.read();
  if (bufferIndex < MESSAGE_LENGTH) {
    buffer[bufferIndex++] = data;
    
    if (bufferIndex == MESSAGE_LENGTH || data == END_BYTE) {
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      xQueueSendFromISR(uartQueue, buffer, &xHigherPriorityTaskWoken);
      bufferIndex = 0; // Reset buffer index for the next message
      if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
      }
    }
  }
}


void TaskProcessDataCode(void *pvParameters) {
  unsigned char receivedMessage[MESSAGE_LENGTH];
  while (1) {
    if (xQueueReceive(uartQueue, &receivedMessage, portMAX_DELAY)) {
      // Process the complete message outside ISR
      Serial.print("Received message: ");
      for (int i = 0; i < MESSAGE_LENGTH; i++) {
        Serial.print(receivedMessage[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
}


#include <Arduino.h>

// UART Configuration
#define RXD_PIN 16
#define TXD_PIN 17

#define START_BYTE 0x02
#define END_BYTE 0x03
#define MESSAGE_LENGTH 9

TaskHandle_t TaskProcessData;
QueueHandle_t uartQueue;

unsigned char buffer[MESSAGE_LENGTH];
volatile unsigned char bufferIndex = 0;

void IRAM_ATTR onUartRx() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  char data = UART2.read();

  if (bufferIndex < MESSAGE_LENGTH) {
    buffer[bufferIndex++] = data;

    // Check if the full message is received
    if (bufferIndex == MESSAGE_LENGTH || data == END_BYTE) {
      xQueueSendFromISR(uartQueue, buffer, &xHigherPriorityTaskWoken);
      bufferIndex = 0; // Reset buffer index for the next message
    }
  }

  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

void setup() {
  Serial.begin(115200);

  // Configure UART
  UART2.begin(9600, SERIAL_8N1, RXD_PIN, TXD_PIN);

  // Initialize queue
  uartQueue = xQueueCreate(10, MESSAGE_LENGTH * sizeof(char));

  // Attach UART interrupt
  UART2.onReceive(onUartRx);

  // Create tasks
  xTaskCreatePinnedToCore(
    TaskProcessDataCode,   /* Task function. */
    "TaskProcessData",     /* name of task. */
    10000,                 /* Stack size of task */
    NULL,                  /* parameter of the task */
    1,                     /* priority of the task */
    &TaskProcessData,      /* Task handle to keep track of created task */
    0);                    /* pin task to core 0 */

  if (TaskProcessData == NULL) {
    Serial.println("Task creation failed.");
  } else {
    Serial.println("Task created successfully.");
  }
}

void loop() {
  // Your main loop code
}

void TaskProcessDataCode(void *pvParameters) {
  unsigned char receivedMessage[MESSAGE_LENGTH];
  while (1) {
    if (xQueueReceive(uartQueue, &receivedMessage, portMAX_DELAY)) {
      // Process received message
      Serial.print("Received message: ");
      for (int i = 0; i < MESSAGE_LENGTH; i++) {
        Serial.print(receivedMessage[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
}


#include <Arduino.h>

// UART Configuration
#define RXD_PIN 16
#define TXD_PIN 17

TaskHandle_t TaskProcessData;
QueueHandle_t uartQueue;

void IRAM_ATTR onUartRx() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  char data = UART2.read();
  xQueueSendFromISR(uartQueue, &data, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

void setup() {
  Serial.begin(115200);

  // Configure UART
  UART2.begin(9600, SERIAL_8N1, RXD_PIN, TXD_PIN);

  // Initialize queue
  uartQueue = xQueueCreate(10, sizeof(char));

  // Attach UART interrupt
  UART2.onReceive(onUartRx);

  // Create tasks
  xTaskCreatePinnedToCore(
    TaskProcessDataCode,   /* Task function. */
    "TaskProcessData",     /* name of task. */
    10000,                 /* Stack size of task */
    NULL,                  /* parameter of the task */
    1,                     /* priority of the task */
    &TaskProcessData,      /* Task handle to keep track of created task */
    0);                    /* pin task to core 0 */

  // To ensure proper task creation
  if (TaskProcessData == NULL) {
    Serial.println("Task creation failed.");
  } else {
    Serial.println("Task created successfully.");
  }
}

void loop() {
  // Your main loop code
}

void TaskProcessDataCode(void *pvParameters) {
  char receivedChar;
  while (1) {
    if (xQueueReceive(uartQueue, &receivedChar, portMAX_DELAY)) {
      // Process received character
      Serial.print("Received: ");
      Serial.println(receivedChar);
    }
  }
}
