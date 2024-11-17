#include <Arduino.h>

const int rowPins[4] = {8, 3, 46, 9};     // Row pins connected to the keypad
const int colPins[4] = {10, 11, 12, 13};     // Column pins connected to the keypad
char Input_Key_Code[6];
unsigned long debounceDelay = 200;        // Debounce time in milliseconds
volatile int keypresses = 0;

// Define two task handles
TaskHandle_t Keypad_Reader = NULL;
TaskHandle_t Task2Handle = NULL;

volatile unsigned long lastInterruptTime = 0;
volatile bool isPressed = false;

void Key_Pressed_ISR() {
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
      detachInterrupt(digitalPinToInterrupt(colPins[i]));  // Disable interrupts on all columns
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

    // // Process single key press 
    // if (pressedCount == 1) { 
    //   pressedRow = foundRow; 
    //   pressedCol = foundCol; 
    //   keyPressed = true; 
    //   multipleKeysPressed = false; 
    // } else if (pressedCount > 1) { 
    //   multipleKeysPressed = true; 
    // }
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

    // if (keyPressed) {
    //   if (!multipleKeysPressed) {
    //     // If only one key is pressed, process it
    //     char key = keypad[pressedRow][pressedCol]; // Set the key
    //     Input_Key_Code[Valid_Input_Presses] = key;
    //     Valid_Input_Presses ++;     //keep track of number of digits input
    //     Serial.print("Key pressed: ");
    //     Serial.println(key);
    //     Serial.print("Number of keypresses: ");
    //     Serial.println(keypresses);
    //   } else {
    //     // If multiple keys are pressed simultaneously, reject input
    //     Serial.println("Multiple keys detected, rejecting.");
    //     //multipleKeysPressed = false;
    //   }

      if(Valid_Input_Presses == 6){
        //notify task that uses keycode
        Serial.println(Input_Key_Code);
        Valid_Input_Presses = 0;
      }
      // Reset key press flag and update debounce time
      keyPressed = false;
    }
    // Reattach interrupts after debounce delay to avoid immediate retriggering
    for (int i = 0; i < 4; i++) {
      attachInterrupt(digitalPinToInterrupt(colPins[i]), Key_Pressed_ISR, FALLING);
      digitalWrite(rowPins[i], LOW); // Reset rows to LOW for interrupt to work

      // Reset pressed flag 
      isPressed = false;
    }
  }
}

// Task 2 function (runs on Core 1)
void Task2(void *pvParameters) {
  while (true) {
    // Code for task 2
    // Serial.println("Task 2 is running...");
    // Serial.print("Number of keypresses: ");
    // Serial.println(keypresses);
    vTaskDelay(pdMS_TO_TICKS(1500)); // Simulating work in task 2
  }
}

void setup() {
  // Start Serial communication
  Serial.begin(115200);
  
  // Allow some time for serial to initialize
  delay(1000);
  
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

  // Create Task 2 (this will run on Core 1)
  xTaskCreatePinnedToCore(
    Task2,               // Task function
    "Task 2",            // Task name
    10000,               // Stack size (bytes)
    NULL,                // Parameters passed to the task
    1,                   // Task priority (higher is higher priority)
    &Task2Handle,        // Task handle
    0                    // Core 0
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


// //Task notification

// #include <Arduino.h>

// TaskHandle_t taskHandle;

// void notifyTask(void *parameter) {
//   while (true) {
//     // Wait indefinitely for a notification
//     ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

//     // Once notified, perform some action
//     Serial.println("Task notified!");
//   }
// }

// void setup() {
//   Serial.begin(115200);

//   // Create a task
//   xTaskCreate(notifyTask, "NotifyTask", 1000, NULL, 1, &taskHandle);

//   // Simulate a delay before notifying the task
//   delay(2000);
//   Serial.println("Notifying task...");
//   xTaskNotifyGive(taskHandle);
// }

// void loop() {
//   // The loop can perform other tasks or remain empty
// }

