const int rowPins[4] = {2, 3, 4, 5};     // Row pins connected to the keypad
const int colPins[4] = {6, 7, 8, 9};     // Column pins connected to the keypad
volatile bool keyPressed = false;
volatile char key = ' ';

// Keypad layout (row-major order)
char keypad[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

unsigned long lastDebounceTime = 0;      // Timestamp of last key press
unsigned long debounceDelay = 50;        // Debounce time in milliseconds

volatile int pressedRow = -1;  // Row where the key press occurred
volatile int pressedCol = -1; // Column where the key press occurred
volatile bool multipleKeysPressed = false; // Flag for multiple key presses

void setup() {
  Serial.begin(9600);

  // Initialize row pins as OUTPUT
  for (int i = 0; i < 4; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH);  // Keep rows HIGH initially (inactive)
  }

  // Initialize column pins as INPUT with internal pull-ups
  for (int i = 0; i < 4; i++) {
    pinMode(colPins[i], INPUT_PULLUP);
  }

  // Set up interrupts on the column pins to detect a key press
  for (int i = 0; i < 4; i++) {
    attachInterrupt(digitalPinToInterrupt(colPins[i]), columnISR, FALLING);
  }
}

void loop() {
  // Process the key press if detected and handle it in the main loop
  if (keyPressed) {
    unsigned long currentTime = millis();
    // Only process the key press if enough time has passed since the last debounce
    if ((currentTime - lastDebounceTime) > debounceDelay) {
      if (!multipleKeysPressed) {
        // If only one key is pressed, process it
        key = keypad[pressedRow][pressedCol]; // Set the key
        Serial.print("Key pressed: ");
        Serial.println(key);
      } else {
        // If multiple keys are pressed simultaneously, reject input
        Serial.println("Multiple keys detected, rejecting.");
      }
      // Reset key press flag and update debounce time
      keyPressed = false;
      lastDebounceTime = currentTime;

      // Reattach interrupts after debounce delay to avoid immediate retriggering
      for (int i = 0; i < 4; i++) {
        attachInterrupt(digitalPinToInterrupt(colPins[i]), columnISR, FALLING);
      }
    }
  }
}

// Interrupt Service Routine for detecting key press
void columnISR() {
  // Disable further interrupts to avoid multiple triggers for the same key press
  for (int i = 0; i < 4; i++) {
    detachInterrupt(digitalPinToInterrupt(colPins[i]));  // Disable interrupts on all columns
  }

  // Scan for key press in all rows and columns
  int pressedCount = 0;
  int foundRow = -1;
  int foundCol = -1;

  // Scan all rows and columns to determine which key was pressed
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

  // If more than one key is pressed, mark it as invalid
  if (pressedCount > 1) {
    multipleKeysPressed = true;
  } else if (pressedCount == 1) {
    pressedRow = foundRow;
    pressedCol = foundCol;
    keyPressed = true;  // Flag that a key press has been detected
    multipleKeysPressed = false;  // Reset the flag if a single key is pressed
  }
}
