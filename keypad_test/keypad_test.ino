#include <Arduino.h>

////ESP32 S3 pins
// const int rowPins[4] = {8, 3, 46, 9};     // Row pins connected to the keypad
// const int colPins[4] = {10, 11, 12, 13};     // Column pins connected to the keypad

//ESP32 WROOM pins
const int rowPins[4] = {18, 19, 21, 22};     // Row pins connected to the keypad
const int colPins[4] = {23, 25, 26, 27};     // Column pins connected to the keypad

unsigned long debounceDelay = 200;        // Debounce time in milliseconds
volatile int keypresses = 0;
volatile unsigned long lastInterruptTime = 0;
volatile bool isPressed = false;
char Input_Key_Code[6];

TaskHandle_t Keypad_Reader = NULL;

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
      //Notify_Alarm_Node();
      return 2;
    } 
    else {
      Serial.println("Exit Goodbye");
      Current_Codes_In_Use.deleteEntry(x); 
      //Notify_Alarm_Node();
      return 3;
    }
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
       //notify_User(2, y);
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



void setup() {
// Start Serial communication
  Serial.begin(9600);

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
}


void loop() {
  // put your main code here, to run repeatedly:

}
