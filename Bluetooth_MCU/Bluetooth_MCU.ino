#include <Wire.h>
#include <BluetoothSerial.h>

#define Timeout   1000
#define I2C_SLAVE_ADDRESS 0x03 // Address of the slave device

BluetoothSerial SerialBT;

char BT_Key_Code[7];
volatile uint32_t bytes_Received = 0;
uint32_t counter = 0;

TaskHandle_t Bluetooth_Task_Handle = NULL; // Task handle for Bluetooth task

void onBluetoothDataReceived(const uint8_t *data, size_t len);
void Process_BT_Message(void *pvParameters);


void setup() {
  Serial.begin(115200);
  Wire.begin(); // Initialize the I2C bus as a master
  

  xTaskCreatePinnedToCore(
     Process_BT_Message,        // Function to implement the task
    "Process_BT_Message",     // Name of the task
    10000,                    // Stack size in words
    NULL,                     // Task input parameter
    3,                        // Priority of the task
    &Bluetooth_Task_Handle,   // Task handle
    0                         // Core where the task should run
  );

  SerialBT.onData(onBluetoothDataReceived);
  SerialBT.begin("ESP32_BT"); // Initialize Bluetooth
  Serial.println("The device started, now you can pair it with Bluetooth!");
}

void loop() {
  
}

void onBluetoothDataReceived(const uint8_t *data, size_t len) {
  
  uint32_t validBytesReceived = 0; // Counter for valid characters

  // Copy received data to global buffer while filtering out control characters
  for (size_t i = 0; i < len && validBytesReceived < sizeof(BT_Key_Code) - 1; i++) {
    if (data[i] != '\r' && data[i] != '\n') { // Ignore newline and carriage return characters
      BT_Key_Code[validBytesReceived++] = data[i];
    }
  }
  BT_Key_Code[validBytesReceived] = '\0'; // Null-terminate the string

  bytes_Received = (len-2); // Update bytes_Received with the count of valid characters

  // Check if total characters received, including control characters, exceed 6
  if (bytes_Received != 6) {
    SerialBT.println("Invalid Code");
  } else {
    // Notify the Bluetooth task
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(Bluetooth_Task_Handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // Perform a context switch if needed
  }
}

void Process_BT_Message(void *pvParameters) {
  while (true) {
    // Wait for the notification from ISR 
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    Serial.println("BT task entered");

    // Send the 6-byte code to the slave
    Wire.beginTransmission(I2C_SLAVE_ADDRESS);
    Wire.write((const uint8_t*)BT_Key_Code, 6);
    Wire.endTransmission();

    delay(20);
  
    // Request response from the slave
    Wire.requestFrom(I2C_SLAVE_ADDRESS, 1);
    uint32_t start_T = millis();
    //while(!Wire.available() || (millis()-start_T < Timeout)){}
    while(!Wire.available()) { 
      if (millis() - start_T >= Timeout) { 
        SerialBT.println("Timeout"); 
        return; // Exit if timeout occurs 
      } 
    }
    if (Wire.available()) {
      // char response = '\n';
      uint8_t response = 0;
      response = Wire.read();
      Wire.requestFrom(I2C_SLAVE_ADDRESS, 1);
      delay(10);
      response = Wire.read();
     
      switch (response) {
        // Bluetooth thread call
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
    bytes_Received = 0; // Reset bytes_Received for the next code
    }
    else {
      SerialBT.println("Timeout");
    }
  }
}