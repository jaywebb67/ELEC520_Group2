  #include "MQTT.hpp"


  #define RedPin_1         9
  #define RedPin_2         10
  #define YellowPin        11
  #define PinkPin          12
  #define BluePin          13
  #define GreenPin         14
  #define resolution       8
  #define debounceDelay    250
  #define pwmFrequency     5000
  #define Pduty            54
  #define Bduty            180
  #define Gduty            40
  #define pwmOff           0
  #define speakerPin       4

  volatile unsigned char buffer[MESSAGE_LENGTH];
  volatile unsigned char bufferIndex = 0;
  bool I_am_Forwarder = true;
  bool Forward_To_MQTT = false;


  char Reset_Cmd[6] = "RESET";
  struct TX_Payload Reset = {5, "RESET"};
  bool Heat_Sensor_Error = false;

  void RX_Message_Process(void *pvParameters);
  void IRAM_ATTR onUartRx();
  void Set_Alarm(uint8_t pin, uint32_t duty);

  QueueHandle_t RX_Queue;

  TaskHandle_t RX_Message_Handle;
  TaskHandle_t LED_Flash = NULL;

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
    digitalWrite(RedPin_1, LOW);
    digitalWrite(RedPin_2, LOW);
    digitalWrite(YellowPin, HIGH);
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
      Task_LED_Flash, // Function to implement the task 
      "LED Flash Task", // Name of the task 
      1000, // Stack size in words 
      NULL, // Task input parameter 
      1, // Priority of the task 
      &LED_Flash, // Task handle 
      0
      );

    // Attach UART interrupt 
    RS485Serial.onReceive(onUartRx); // Attach the interrupt handler
    
    xTaskCreatePinnedToCore(MQTT_task, "MQTT_task", 5000, NULL, 1, &mqttTaskHandle, 1);
    if (mqttTaskHandle == NULL) {
        Serial.println("Failed to create mqtt thread.");
    } else {
        Serial.println("MQTT thread task created successfully.");
    }
  }


  void loop() {
    // put your main code here, to run repeatedly:
  xTaskNotifyGive(LED_Flash);

  }

  void Task_LED_Flash(void *pvParameters) { 
    const int ledInterval = 250; // Interval in milliseconds (4 Hz = 250ms on, 250ms off) 
    bool ledState1 = false; 
    bool ledState2 = true; 
    digitalWrite(RedPin_1, LOW); // Ensure the LED starts off 
    digitalWrite(RedPin_2, LOW);
    while (true) { 
      // Wait until woken up by a notification 
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
      // Toggle LED states 
      // Serial.println("LED task");
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

  // RS485 serial port task
  void RX_Message_Process(void *pvParameters) {
    unsigned char receivedMessage[MESSAGE_LENGTH];
    while (1) {
      if (xQueueReceive(RX_Queue, &receivedMessage, portMAX_DELAY)) {
        memset(RX_Message_Payload, 0, sizeof(RX_Message_Payload));
        Addressee = Decode_Message(receivedMessage, &Sender_Address, &Sender_Node_Type, RX_Message_Payload);
        // Process received message

        // Process the received message
        if ((Addressee == MQTT_Address) && (I_am_Forwarder)) {
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
          if(Sender_Node_Type == Fire_Node){
            if(strcmp((const char*)RX_Message_Payload, "Fire Call") == 0){
              Set_Alarm(PinkPin, Pduty);
            }
            else if (strcmp((const char*)RX_Message_Payload, "Heat Alarm") == 0){
              Set_Alarm(BluePin, Bduty);
            }
            else if(strcmp((const char*)RX_Message_Payload, "Sensor Error") == 0){
              Heat_Sensor_Error = true;
              //wake mqtt message thread
            }
          }
          else if(Sender_Node_Type == Intrusion_Node) {
              if(strcmp((const char*)RX_Message_Payload, "Intruder alarm") == 0){
                  Set_Alarm(GreenPin, Gduty);   
                  tone(speakerPin,400,500);
              }         
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


  void Set_Alarm(uint8_t pin, uint32_t duty){
    xTaskNotifyGive(LED_Flash);
    ledcWrite(pin, duty);
    digitalWrite(YellowPin, LOW);
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
