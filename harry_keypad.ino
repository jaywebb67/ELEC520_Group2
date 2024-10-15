////importing libraries for interfacing with the LCD and the Keypad:
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

////keypad setup
//keypad dimensions
const byte rows = 4;
const byte cols = 4;
//keypad button values
 char keys[rows][cols] = {
   {'1', '2', '3', 'A'},
   {'4', '5', '6', 'B'},
   {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
//setting the arduino pins that are used for the columns and rows
byte colPins[cols] = {26, 25, 33, 32};
byte rowPins[rows] = {13, 12, 14, 27};

//keypad initialisation
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);

////LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

////password management
int passwordLength = 6;
String setPassword = "123456";
String currentInput = "";
char key;
bool accessGranted = false;


void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.setBacklight(255);
  lcd.backlight();
  
}

void loop() {
  if (currentInput.length() < passwordLength){
      getInput();
      keyPrompt();
  } else {
    checkPassword();
  }


}

void getInput(){
  key = keypad.getKey();
  if (key){
    currentInput += key;
    Serial.println(currentInput);
  }
}


void keyPrompt(){
  lcd.setCursor(0,0);
  lcd.print("   Enter Key:   ");
   // Calculate how many underscores to display
  int remainingSpaces = passwordLength - currentInput.length();

  lcd.setCursor(4,1);
  lcd.print("[");
  lcd.setCursor(11,1);
  lcd.print("]");
  lcd.setCursor(5,1);
  lcd.print(currentInput);
    // Display remaining underscores
  for (int i = 0; i < remainingSpaces; i++) {
    lcd.print("_");}
}

void checkPassword(){
    lcd.setCursor(0,0);
    lcd.print("     Access     ");
    lcd.setCursor(0,1);
    if (currentInput == setPassword) {
      lcd.print("     Granted    ");
      delay(2);
      loop();
    } else 
    {
      lcd.print("     Denied     ");
      delay(2);
      loop();

    }
    return;
}