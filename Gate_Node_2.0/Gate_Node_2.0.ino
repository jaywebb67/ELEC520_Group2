//#include <BluetoothSerial.h>
#include <LiquidCrystal_I2C.h>
#include "Communication_Protocol.h"
#include "LCD_Manager.h"
#include "MQTT.hpp"
#include "driver/uart.h"
#include "driver/gpio.h"

#define DEVICE_NAME "Gate 1"

//BluetoothSerial //SerialBT;

//const uint8_t Home_Node_Type = 0x35;
uint8_t Home_Address = 0x13;
uint8_t Destination_Address = 0x28;

bool alarmEnabled = true;

//ESP32 WROOM pins
const int rowPins[4] = {18, 19, 13, 32};     // Row pins connected to the keypad
const int colPins[4] = {23, 25, 26, 27};     // Column pins connected to the keypad

char Input_Key_Code[7];
char BT_Key_Code[7];
char Start_Pass[7] = {"012345"};
// volatile unsigned char buffer[MESSAGE_LENGTH];
// volatile unsigned char bufferIndex = 0;
volatile uint32_t bytes_Received = 0;
unsigned long debounceDelay = 250;        // Debounce time in milliseconds
volatile uint8_t Valid_Input_Presses = 0;
volatile unsigned long lastInterruptTime = 0;
volatile bool isPressed = false;

// Define task handles
TaskHandle_t Keypad_Reader = NULL;
TaskHandle_t Bluetooth_Task_Handle = NULL; // Task handle for Bluetooth task
TaskHandle_t LCD_Thread_Handle;
//TaskHandle_t RX_Message_Handle;

// Timer handle 
TimerHandle_t Keypad_Timeout_Timer;


// Timer callback function 
void TimeoutCallback(TimerHandle_t xTimer) { 
  Serial.println("Timeout occurred. Resetting Valid_Input_Presses."); 
  Valid_Input_Presses = 0; 
  Send_To_LCD_Queue(T_Out);
  Send_To_LCD_Queue(Enter_Mess);
}

void IRAM_ATTR Key_Pressed_ISR() {
  if(!isPressed) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(Keypad_Reader, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // Perform a context switch if needed
  }
}

// //alternative keypad ISR more stable but wrong in every way
// void IRAM_ATTR Key_Pressed_ISR() {
//   unsigned long interruptTime = millis();
//   // Debounce logic
//   if ((interruptTime - lastInterruptTime > debounceDelay) && !isPressed) {
//     isPressed = true;
//     keypresses++;
//     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//     vTaskNotifyGiveFromISR(Keypad_Reader, &xHigherPriorityTaskWoken);
//     portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // Perform a context switch if needed
//   }
//   lastInterruptTime = interruptTime;
// }

// void onBluetoothDataReceived(const uint8_t *data, size_t len) {
//   Serial.print("Received data length: ");
//   Serial.println(len);

//   uint32_t validBytesReceived = 0; // Counter for valid characters

//   // Copy received data to global buffer while filtering out control characters
//   for (size_t i = 0; i < len && validBytesReceived < sizeof(BT_Key_Code) - 1; i++) {
//     if (data[i] != '\r' && data[i] != '\n') { // Ignore newline and carriage return characters
//       BT_Key_Code[validBytesReceived++] = data[i];
//     }
//   }
//   BT_Key_Code[validBytesReceived] = '\0'; // Null-terminate the string

//   bytes_Received = (len-2); // Update bytes_Received with the count of valid characters

//   Serial.print("Filtered data length: ");
//   Serial.println(bytes_Received);

//   // Check if total characters received, including control characters, exceed 6
//   if (validBytesReceived != 6) {
//     Serial.println("Invalid Code");
//     //SerialBT.println("Invalid Code");
//   } else {
//     // Notify the Bluetooth task
//     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//     vTaskNotifyGiveFromISR(Bluetooth_Task_Handle, &xHigherPriorityTaskWoken);
//     portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // Perform a context switch if needed
//   }
// }


void notify_User(int x, int y) {
  switch (x) {
    case 1:     // Bluetooth thread call
      switch (y) {
        case 1:
          //SerialBT.println("Access Denied");
          break;
        case 2:
          //SerialBT.println("Access Granted");
          break;
        case 3:
          //SerialBT.println("Toodle Pip");
          break;
        default:
          //SerialBT.println("I Need A COFFEE");
          break;
      }
      break;
    case 2:     // Keypad thread call
      switch (y) {
        case 1:
          Send_To_LCD_Queue(Access_D);
          break;
        case 2:
          Send_To_LCD_Queue(Access_G);
          break;
        case 3:
          Send_To_LCD_Queue(Bye);
          break;
        default:
          Send_To_LCD_Queue(Broken);
          break;
      }
      break;
    default:
      //SerialBT.println("I Need A COFFEE");
      Send_To_LCD_Queue(Broken);
      break;
  }
}

int Test_Entry_Code(const char* code) {
  int x = Valid_Entrance_Codes.searchEntry(code);
  if (x < 0) {
    Serial.println("Access Denied");
    return 1;
  } else {
      // Get the username corresponding to the code
      String username = Valid_Entrance_Codes.findUsername(x);
      if (username.isEmpty()) {
        Serial.println("Error retrieving username.");
        return -1; // Error case
      }

      // Format "username:password" pair
      const int formattedLength = username.length() + 1 + strlen(code) + 1; // username + ':' + code + '\0'
      char* formattedEntry = new char[formattedLength];
      snprintf(formattedEntry, formattedLength, "%s:%s", username.c_str(), code);

      // Check if the code is already in use
      x = Current_Codes_In_Use.searchEntry(code);
      if (x < 0) {
        Serial.println("Access Granted");
        Current_Codes_In_Use.addEntry(formattedEntry); // Add the formatted "username:password" pair
        delete[] formattedEntry; // Free memory for the formatted entry
        Current_Codes_In_Use.printBuffer();

        // Prepare MQTT message
        MqttMessage msg;
        msg.topic = "ELEC520/userAccess";
        msg.payload = username;
        //delete[] username;
        // Send the message to the queue
        if (xQueueSend(mqttPublishQueue, &msg, portMAX_DELAY) != pdPASS) {
          Serial.println("Failed to send message to MQTT queue");
        }
        if((Current_Codes_In_Use.getCurrentIndex()>0) && (alarmEnabled)){
          MqttMessage msgAlarm;
          msgAlarm.topic = "ELEC520/alarm";
          msgAlarm.payload = "Alarm Disabled";
          // Send the message to the queue
          if (xQueueSend(mqttPublishQueue, &msgAlarm, portMAX_DELAY) != pdPASS) {
            Serial.println("Failed to send message to MQTT queue");
          }
          alarmEnabled = false;
        }
        return 2;
      } else {
        delete[] formattedEntry; // Free memory for the formatted entry
        Serial.println("Exit Goodbye");
        Current_Codes_In_Use.deleteEntry(x); 
        Current_Codes_In_Use.printBuffer();
        // Prepare MQTT message
        MqttMessage msg;
        msg.topic = "ELEC520/userAccess";
        msg.payload = username;
        //delete[] username;
        // Send the message to the queue
        if (xQueueSend(mqttPublishQueue, &msg, portMAX_DELAY) != pdPASS) {
          Serial.println("Failed to send message to MQTT queue");
        }
        if((Current_Codes_In_Use.getCurrentIndex()==0) && (!alarmEnabled)){
          MqttMessage msgAlarm;
          msgAlarm.topic = "ELEC520/alarm";
          msgAlarm.payload = "Alarm Enabled";
          // Send the message to the queue
          if (xQueueSend(mqttPublishQueue, &msgAlarm, portMAX_DELAY) != pdPASS) {
            Serial.println("Failed to send message to MQTT queue");
          }
          alarmEnabled = true;
        }
        return 3;
      }
  }
}



// Task 1 function 
void Keypad_Read(void *pvParameters) {
  
  unsigned long Start_Time;

  // Create the timeout timer (10 seconds) 
  Keypad_Timeout_Timer = xTimerCreate("TimeoutTimer", pdMS_TO_TICKS(10000), pdFALSE, 0, TimeoutCallback);

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

    isPressed = true;
    vTaskDelay(pdMS_TO_TICKS(debounceDelay));
   
    Serial.println("Entering task 1.");
   
    for (int i = 0; i < 4; i++) {
      //set row pins high to enable reading
      digitalWrite(rowPins[i], HIGH); // Reset rows to HIGH (idle state)
    }
   
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
    if(Valid_Input_Presses == 0){
      xTimerStart(Keypad_Timeout_Timer, 0);
    }
    Valid_Input_Presses++; 
    Serial.print("Key pressed: "); 
    Serial.println(key);

      if(Valid_Input_Presses == 6){
        // Process the valid 6-byte code
        int y = Test_Entry_Code(Input_Key_Code);
        notify_User(2, y);
        Serial.println(y);
        Serial.println(Input_Key_Code);
        Valid_Input_Presses = 0;
        xTimerStop(Keypad_Timeout_Timer, 0);
        Send_To_LCD_Queue(Enter_Mess);
      }
    }
    // Reset row pins to low to reenable the interrupt
    for (int i = 0; i < 4; i++) {
      //attachInterrupt(digitalPinToInterrupt(colPins[i]), Key_Pressed_ISR, FALLING);
      digitalWrite(rowPins[i], LOW); // Reset rows to LOW for interrupt to work
    }
    // Reset pressed flag 
      isPressed = false;

      
  }
}


// void Process_BT_Message(void *pvParameters) {
//   while (true) {
//     // Wait for the notification from ISR 
//     ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//     Serial.println("BT task entered");

//     // Check if the length of received data is 6
//     if (bytes_Received == 6) {
//       // Process the valid 6-byte code
//       int y = Test_Entry_Code(BT_Key_Code);
//       notify_User(1, y);
//       //SerialBT.println("Valid Code");
//     } else {
//       Serial.println("Invalid Code");
//       //SerialBT.println("Invalid Code");
//     }
//     // // Process the valid 6-byte code
//     // int y = Test_Entry_Code(BT_Key_Code);
//     // notify_User(1, y);
//     bytes_Received = 0; // Reset bytes_Received for the next code
//   }
// }

// Task 4 function
void LCD_Thread(void *pvParameters) {
  QueueItem item; 
  while (1) { 
    if (xQueueReceive(LCD_Queue, &item, portMAX_DELAY)) { 
      // Call the function 
      item.func(); 
    } 
    // sleep for 2 seconds as minimum LCD display time 
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}


void setup() {
 // Start Serial communication
  Serial.begin(9600);
  //call function to set up correct communication pins and serial port for the board in use
  Comms_Set_Up();
  Serial.println("Hello");
  LCD_Innit(); 
  MQTT_SetUp();
  //xTaskCreatePinnedToCore(MQTT_task, "MQTT_task", 10000, NULL, 1, NULL, 1);

  // // Initialize queue
  // RX_Queue = xQueueCreate(10, MESSAGE_LENGTH * sizeof(char));
  // if (RX_Queue == NULL) {
  //     Serial.println("Failed to create RX_Queue!");
  //     while (true); // Halt execution
  // }
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

  //put in initial value for testing
  Valid_Entrance_Codes.addEntry(Start_Pass);
  Valid_Entrance_Codes.printBuffer();
  // Create Task 2 (runs on Core 0)
  // xTaskCreatePinnedToCore(
  //    Process_BT_Message,        // Function to implement the task
  //   "Process_BT_Message",     // Name of the task
  //   10000,                    // Stack size in words
  //   NULL,                     // Task input parameter
  //   3,                        // Priority of the task
  //   &Bluetooth_Task_Handle,   // Task handle
  //   0                         // Core where the task should run
  // );

  // Create task 3 (rus on core 0)
  xTaskCreatePinnedToCore(
     RX_Message_Process,  // Task function. 
    "RX_Message_Process",     // name of task. 
    10000,                    // Stack size of task 
    NULL,                     // parameter of the task 
    2,                        // priority of the task 
    &RX_Message_Handle,      // Task handle to keep track of created task 
    0                         // pin task to core 0 
    );                       

  // Create task 4 (runs on core 0)
  xTaskCreatePinnedToCore(
     LCD_Thread,            //Task function
    "LCD_Thread",               //name of task
    10000,                      //stacksize of task
    NULL,                       //parameter of task
    1,                          //priority of task
    &LCD_Thread_Handle,                //Task handle to keep track of created task 
    0                           //pin task to core 0
  );
  
  // Initialize BluetoothSerial and set up the callback 
  //SerialBT.onData(onBluetoothDataReceived);
  //SerialBT.begin(DEVICE_NAME);
  //Serial.println("The device started, now you can pair it with Bluetooth!");

  // Attach the interrupt to the Bluetooth serial available method
  //attachInterrupt(digitalPinToInterrupt(SerialBT.available()), Bluetooth_ISR, RISING);
  // Initialize BluetoothSerial and set up the callback SerialBT.onData(onBluetoothDataReceived);
 

  // Attach UART interrupt 
  RS485Serial.onReceive(onUartRx); // Attach the interrupt handler

  Enter_Mess(); //LCD default screen

  xTaskCreatePinnedToCore(MQTT_task, "MQTT_task", 5000, NULL, 1, &mqttTaskHandle, 1);
  if (mqttTaskHandle == NULL) {
      Serial.println("Failed to create mqtt thread.");
  } else {
      Serial.println("MQTT thread task created successfully.");
  }

}

void loop() {
  // put your main code here, to run repeatedly:

}
