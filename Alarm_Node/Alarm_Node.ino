#include "MQTT.hpp"
#include "Communication_Protocol.h"

#define RedPin_1         9
#define RedPin_2         10
#define YellowPin        11
#define PinkPin          12
#define BluePin          13
#define GreenPin         14
#define PurplePin        20
#define resolution       8
#define debounceDelay    250
#define pwmFrequency     5000
#define Pduty            54
#define Bduty            180
#define Gduty            40
#define pwmOff           0
#define speakerPin       47

volatile unsigned char buffer[MESSAGE_LENGTH];
volatile unsigned char bufferIndex = 0;
bool I_am_Forwarder = false;
bool Forward_To_MQTT = false;

//ESP32-s3 WROOM pins
const int rowPins[4] = {42, 41, 40, 39};     // Row pins connected to the keypad
const int colPins[4] = {38, 37, 36, 35};     // Column pins connected to the keypad

//unsigned long debounceDelay = 250;        // Debounce time in milliseconds
volatile uint8_t Valid_Input_Presses = 0;
volatile unsigned long lastInterruptTime = 0;
volatile bool isPressed = false;

char Input_Key_Code[7];
char Admin[7] = {"012345"};
char Reset_Cmd[6] = "RESET";
struct TX_Payload Reset = {5, "RESET"};
bool Heat_Sensor_Error = false;

void RX_Message_Process(void *pvParameters);
void Keypad_Read(void *pvParameters);
void IRAM_ATTR onUartRx();
void IRAM_ATTR Key_Pressed_ISR();
void TimeoutCallback(TimerHandle_t xTimer);
void Set_Alarm(uint8_t pin, uint32_t duty);

QueueHandle_t RX_Queue;

// Timer handle 
TimerHandle_t Keypad_Timeout_Timer;

TaskHandle_t RX_Message_Handle;
TaskHandle_t Keypad_Reader = NULL;
TaskHandle_t LED_Flash;

void setup() {
  // // Start Serial communication
  Serial.begin(9600);
  //call function to set up correct communication pins and serial port for the board in use
  Comms_Set_Up();
  Serial.println("Hello");
  MQTT_SetUp();
  ledcAttach(PinkPin,  pwmFrequency, resolution);
  ledcAttach(BluePin,  pwmFrequency, resolution);
  ledcAttach(GreenPin, pwmFrequency, resolution);
  pinMode(RedPin_1, OUTPUT);
  pinMode(RedPin_2, OUTPUT);
  pinMode(YellowPin, OUTPUT);
  pinMode(PurplePin, OUTPUT);
  digitalWrite(RedPin_1, LOW);
  digitalWrite(RedPin_1, LOW);
  digitalWrite(YellowPin, HIGH);
  digitalWrite(PurplePin, LOW);
  ledcWrite(PinkPin, Pduty);
  ledcWrite(BluePin, Bduty);
  ledcWrite(GreenPin, Gduty);

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
  
   xTaskCreatePinnedToCore(
    Keypad_Read,               // Task function
    "Keypad_Read",            // Task name
    10000,               // Stack size (bytes)
    NULL,                // Parameters passed to the task
    3,                   // Task priority (higher is higher priority)
    &Keypad_Reader,        // Task handle
    0                    // Core 0
  );

  xTaskCreatePinnedToCore( 
    Task_LED_Flash, // Function to implement the task 
    "LED Flash Task", // Name of the task 
    1000, // Stack size in words 
    NULL, // Task input parameter 
    1, // Priority of the task 
    &LED_Flash, // Task handle 
    0
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
  // Attach UART interrupt 
  RS485Serial.onReceive(onUartRx); // Attach the interrupt handler
  //vTaskSuspend(LED_Flash);
  
  xTaskCreatePinnedToCore(MQTT_task, "MQTT_task", 5000, NULL, 1, &mqttTaskHandle, 1);
  if (mqttTaskHandle == NULL) {
      Serial.println("Failed to create mqtt thread.");
  } else {
      Serial.println("MQTT thread task created successfully.");
  }

}


void loop() {
  // put your main code here, to run repeatedly:
//xTaskNotifyGive(LED_Flash);

}

void Set_Alarm(uint8_t pin, uint32_t duty){
  // xTaskNotifyGive(LED_Flash);
  vTaskResume( LED_Flash );
  ledcWrite(pin, duty);
  digitalWrite(YellowPin, LOW);
}

void Task_LED_Flash(void *pvParameters) {
  const int ledInterval = 250; // Interval in milliseconds (4 Hz = 250ms on, 250ms off)
  bool ledState1 = false;
  bool ledState2 = true;

  pinMode(RedPin_1, OUTPUT); // Ensure the LED pins are set as outputs
  pinMode(RedPin_2, OUTPUT);
  digitalWrite(RedPin_1, LOW); // Ensure the LEDs start off
  digitalWrite(RedPin_2, LOW);

  while (true) {
    // Wait until woken up by a notification
    //Serial.println("Waiting for notification...");
    //ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    //Serial.println("Notification received, toggling LEDs");

    // Toggle LED states
    ledState1 = !ledState1;
    digitalWrite(RedPin_1, ledState1);
    ledState2 = !ledState2;
    digitalWrite(RedPin_2, ledState2);
    vTaskDelay(pdMS_TO_TICKS(ledInterval));

    // Toggle LED states again
    ledState1 = !ledState1;
    digitalWrite(RedPin_1, ledState1);
    ledState2 = !ledState2;
    digitalWrite(RedPin_2, ledState2);
    vTaskDelay(pdMS_TO_TICKS(ledInterval));
  }
}

// Task 1 function 
void Keypad_Read(void *pvParameters) {
  
  
  unsigned long Start_Time;

  // Create the timeout timer (10 seconds) 
  Keypad_Timeout_Timer = xTimerCreate("TimeoutTimer", pdMS_TO_TICKS(8000), pdFALSE, 0, TimeoutCallback);

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
    //vTaskDelay(pdMS_TO_TICKS(debounceDelay));
   
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
      digitalWrite(PurplePin, HIGH);
    }
    Valid_Input_Presses++; 
    Serial.print("Key pressed: "); 
    Serial.println(key);

      if(Valid_Input_Presses == 6){
        Valid_Input_Presses = 0;
        digitalWrite(PurplePin, LOW);
        xTimerStop(Keypad_Timeout_Timer, 0);
        if(strncmp((const char*)Input_Key_Code, Admin, 6) == 0){
          Serial.println("Right admin code");
          vTaskSuspend(LED_Flash);
          ledcWrite(PinkPin, 0);
          ledcWrite(BluePin, 0);
          ledcWrite(GreenPin, 0);
          digitalWrite(YellowPin, HIGH);
          digitalWrite(RedPin_1, LOW); 
          digitalWrite(RedPin_2, LOW);
          Transmit_To_Bus(&Reset);
          //inform dashboard via mqtt Set inform flag to send standard message?
        }
        else {
          Serial.println("Wrong code");
        }
        Serial.println(Input_Key_Code);
      
      }
    }
    // Reset row pins to low to reenable interrupts
    for (int i = 0; i < 4; i++) {
      digitalWrite(rowPins[i], LOW); // Reset rows to LOW for interrupt to work
    }
    // Reset pressed flag 
      isPressed = false;

      
  }
}
void IRAM_ATTR Key_Pressed_ISR() {
  unsigned long interruptTime = millis();
  // Debounce logic
  if ((interruptTime - lastInterruptTime > debounceDelay) && !isPressed) {
    isPressed = true;
    //keypresses++;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(Keypad_Reader, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // Perform a context switch if needed
  }
  lastInterruptTime = interruptTime;
}
// RS485 serial port task
void RX_Message_Process(void *pvParameters) {
  unsigned char receivedMessage[MESSAGE_LENGTH];
  while (1) {
    if (xQueueReceive(RX_Queue, &receivedMessage, portMAX_DELAY)) {
      memset(RX_Message_Payload, 0, sizeof(RX_Message_Payload));
      Addressee = Decode_Message(receivedMessage, &Sender_Address, &Sender_Node_Type, RX_Message_Payload);
      // Process received message
      Serial.print("Sent from node of type: ");
      Serial.println(Sender_Node_Type);
      if((Addressee == MQTT_Address) && (I_am_Forwarder)) {
        // parse message into correct mqtt message struct 
        // Create an MQTT message structure
        MqttMessage mqttMessage;

        // Assign topic based on the received message type (example logic, adjust as needed)
        if (Sender_Node_Type == Fire_Node) {  // Example: Fire sensor type
            if(strcmp((const char*)RX_Message_Payload, "Fire1 online") == 0){
                mqttMessage.topic = "ELEC520/devicePing";
            }
            else{
                mqttMessage.topic = "ELEC520/temperature";
            }
        } else if (Sender_Node_Type == Intrusion_Node) {  // Example: Temperature node type
            if(strcmp((const char*)RX_Message_Payload, "SMB302 online") == 0){
                mqttMessage.topic = "ELEC520/devicePing";
            }
            else{
                mqttMessage.topic = "ELEC520/imu";
            }
        } else {
            mqttMessage.topic = "Home/Unknown";  // Default topic for unclassified nodes
        }

        // Assign the payload from RX_Message_Payload
        mqttMessage.payload = String((char*)RX_Message_Payload);

        // Send the MQTT message to the queue
        if (xQueueSend(mqttPublishQueue, &mqttMessage, portMAX_DELAY) != pdPASS) {
            Serial.println("Failed to send message to MQTT queue");
        }
      }
      if(Addressee == Home_Address){
        uint8_t temp = Destination_Address;
        Destination_Address = Intrusion_Node;
        struct TX_Payload msg;
        if(strcmp((const char*)RX_Message_Payload, "Alarm Enabled") == 0){
          msg = {7,"NVuser"};
        }
        if(strcmp((const char*)RX_Message_Payload, "Alarm Disabled") == 0){
          msg = {6,"Vuser"};
        }
        Transmit_To_Bus(&msg);
        Destination_Address = temp;
      }
      
      if(Sender_Node_Type == Fire_Node){
        MqttMessage mqttMessage;
        Serial.println("It's a fire node");
        if(strcmp((const char*)RX_Message_Payload, "Fire Call") == 0){
          Serial.println("It's a call event");
          Set_Alarm(PinkPin, Pduty);
          mqttMessage.topic = "ELEC520/alarm";
          // Assign the payload from RX_Message_Payload
          mqttMessage.payload = String((char*)RX_Message_Payload);
          // Send the MQTT message to the queue
          if (xQueueSend(mqttPublishQueue, &mqttMessage, portMAX_DELAY) != pdPASS) {
              Serial.println("Failed to send message to MQTT queue");
          }
        }
        else if (strcmp((const char*)RX_Message_Payload, "Heat Alarm") == 0){
          Set_Alarm(BluePin, Bduty);
          mqttMessage.topic = "ELEC520/alarm";
          // Assign the payload from RX_Message_Payload
          mqttMessage.payload = String((char*)RX_Message_Payload);
          // Send the MQTT message to the queue
          if (xQueueSend(mqttPublishQueue, &mqttMessage, portMAX_DELAY) != pdPASS) {
              Serial.println("Failed to send message to MQTT queue");
          }
        }
        else if(strcmp((const char*)RX_Message_Payload, "Sensor Error") == 0){
          Heat_Sensor_Error = true;
          //wake mqtt message thread
        }
      }
      else if(Sender_Node_Type == Intrusion_Node) {
          Set_Alarm(GreenPin, Gduty);
          MqttMessage mqttMessage;
          mqttMessage.topic = "ELEC520/alarm";
          // Assign the payload from RX_Message_Payload
          mqttMessage.payload = String((char*)RX_Message_Payload);
          // Send the MQTT message to the queue
          if (xQueueSend(mqttPublishQueue, &mqttMessage, portMAX_DELAY) != pdPASS) {
              Serial.println("Failed to send message to MQTT queue");
          }
          //tone(SPEAKER_PIN, 400,500);
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

// Timer callback function 
void TimeoutCallback(TimerHandle_t xTimer) { 
  Serial.println("Timeout occurred. Resetting Valid_Input_Presses."); 
  Valid_Input_Presses = 0; 
  digitalWrite(PurplePin, LOW);
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
