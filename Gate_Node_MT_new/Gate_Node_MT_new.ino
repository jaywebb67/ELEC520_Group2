#include <Arduino.h>
#include <BluetoothSerial.h>
#include <LiquidCrystal_I2C.h>
#include "Char_Buffer.h"
#include "Communication_Protocol.h"
#include "Gate_Function.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "driver/gpio.h"

// #define MESSAGE_LENGTH 40
#define DEVICE_NAME "Gate 1"

//const uint8_t Home_Node_Type = 0x35;
uint8_t Home_Address = 0x13;
uint8_t Destination_Address = 0x28;

BluetoothSerial SerialBT;


QueueHandle_t LCD_Queue;

//create LCD object
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Create a CharBuffer object with 10 entries, each of size 6 characters 
CharBuffer Valid_Entrance_Codes(100, 6);
CharBuffer Current_Codes_In_Use(100, 6);

const int rowPins[4] = {8, 3, 46, 9};     // Row pins connected to the keypad
const int colPins[4] = {10, 11, 12, 13};     // Column pins connected to the keypad
char Input_Key_Code[6];
char BT_Key_Code[6];
// volatile unsigned char buffer[MESSAGE_LENGTH];
// volatile unsigned char bufferIndex = 0;
unsigned long debounceDelay = 200;        // Debounce time in milliseconds
volatile int keypresses = 0;

 struct TX_Payload User = {4, "User"};    //put messages to transmit here
 struct TX_Payload NoUser = {7, "No User"};

typedef void (*FunctionPointer)(void);

typedef struct {
  FunctionPointer func;
} QueueItem;



// Define task handles
TaskHandle_t Keypad_Reader = NULL;
TaskHandle_t Bluetooth_Task_Handle = NULL; // Task handle for Bluetooth task
// TaskHandle_t RX_Message_Handle;
TaskHandle_t LCD_Thread_Handle;

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





void Enter_Mess() {
  lcd.setCursor(0,0);
  lcd.print("   Enter Key:   ");
}

void Access_G() {
  lcd.setCursor(0,0);
  lcd.print("     Access     ");
  lcd.setCursor(0,1);
  lcd.print("     Granted    ");
}

void Access_D() {
  lcd.setCursor(0,0);
  lcd.print("     Access     ");
  lcd.setCursor(0,1);
  lcd.print("     Denied     ");
}

void Bye() {
  lcd.setCursor(0,0);
  lcd.print("     So Long    ");
  lcd.setCursor(0,1);
  lcd.print("    Farewell    ");
}

void Invalid_Mess(){
  lcd.setCursor(0,0);
  lcd.print("    Invalid     ");
  lcd.setCursor(0,1);
  lcd.print("     Input      ");
}

void Broken(){
  lcd.setCursor(0,0);
  lcd.print("    I Need A    ");
  lcd.setCursor(0,1);
  lcd.print("    COFFEE      ");
}



void Send_To_LCD_Queue(FunctionPointer func) {
  QueueItem item = {func};
  if (xQueueSend(LCD_Queue, &item, portMAX_DELAY) != pdPASS) {
    Serial.println("Failed to send to queue.");
  }
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
      Notify_Alarm_Node();
      return 2;
    } 
    else {
      Serial.println("Exit Goodbye");
      Current_Codes_In_Use.deleteEntry(x); 
      Notify_Alarm_Node();
      return 3;
    }
  }
}

void Notify_Alarm_Node() {
   int Current_Users = Current_Codes_In_Use.getCurrentIndex();
        if(Current_Users == 1) {
          Transmit_To_Bus(&User);
        }
        else if(Current_Users == 0) {
          Transmit_To_Bus(&NoUser);
        }
}
void notify_User(int x, int y) {
  switch (x) {
    case 1:     // Bluetooth thread call
      switch (y) {
        case 1:
          SerialBT.println("Access Denied");
          break;
        case 2:
          SerialBT.println("Access Granted");
          break;
        case 3:
          SerialBT.println("Toodle Pip");
          break;
        default:
          SerialBT.println("I Need A COFFEE");
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
      SerialBT.println("I Need A COFFEE");
      Send_To_LCD_Queue(Broken);
      break;
  }
}



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
      Send_To_LCD_Queue(Invalid_Mess);
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
        notify_User(2, y);
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
      Send_To_LCD_Queue(Enter_Mess);
  }
}

// Task 2 function 
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
      SerialBT.println("Invalid Code");
      bytes_Received = 0; // Reset bytes_Received
      // Send to LCD or turn on a red LED to indicate the error
      while (SerialBT.available()) { // Clear the buffer
        SerialBT.read();
      }
    } else if (bytes_Received != 6) {
      Serial.println("Invalid Code");
      SerialBT.println("Invalid Code");
      bytes_Received = 0; // Reset bytes_Received
      // Send to LCD or turn on a red LED to indicate the error
    } 
    else {
      // Process the valid 6-byte code
      int y = Test_Entry_Code(Input_Key_Code);
      notify_User(1, y);
      bytes_Received = 0; // Reset bytes_Received for the next code
    }
  }
}

// // RS485 serial port task
// void RX_Message_Process(void *pvParameters) {
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
  //LCD setup
  lcd.init();
  lcd.setBacklight(255);
  lcd.backlight();

  // Initialize queue
  // RX_Queue = xQueueCreate(10, MESSAGE_LENGTH * sizeof(char));
  LCD_Queue = xQueueCreate(10, sizeof(QueueItem));
  
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

  // // Create task 3 (rus on core 0)
  // xTaskCreatePinnedToCore(
  //    RX_Message_Process,  // Task function. 
  //   "RX_Message_Process",     // name of task. 
  //   10000,                    // Stack size of task 
  //   NULL,                     // parameter of the task 
  //   2,                        // priority of the task 
  //   &RX_Message_Handle,      // Task handle to keep track of created task 
  //   0                         // pin task to core 0 
  //   );                       

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
  
  // Initialize row & column pins 
  for (int i = 0; i < 4; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], LOW);  // Keep rows LOW initially (inactive)
    pinMode(colPins[i], INPUT_PULLUP);
  }

  // Bluetooth device name
   SerialBT.begin(DEVICE_NAME);
  Serial.println("The device started, now you can pair it with Bluetooth!");

  // Attach the interrupt to the Bluetooth serial available method
  attachInterrupt(digitalPinToInterrupt(SerialBT.available()), Bluetooth_ISR, RISING);
  
  // Set up interrupts on the column pins to detect a key press
  for (int i = 0; i < 4; i++) {
    attachInterrupt(digitalPinToInterrupt(colPins[i]), Key_Pressed_ISR, FALLING);
  }

  // // Attach UART interrupt 
  // RS485Serial.onReceive(onUartRx); // Attach the interrupt handler

  Enter_Mess();
}



void loop() {
  // Main loop can be used to handle other tasks or configurations
  // For this example, we're just letting the tasks handle the output.
  delay(1000);
}

