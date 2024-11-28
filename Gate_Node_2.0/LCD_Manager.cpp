#include "LCD_Manager.h"

//create LCD object
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Define the queue 
QueueHandle_t LCD_Queue;

void LCD_Innit() {
  lcd.init();
  lcd.setBacklight(150);
  lcd.backlight();
  LCD_Queue = xQueueCreate(10, sizeof(QueueItem));
}

void Enter_Mess() {
  lcd.clear();
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

void T_Out() {
  lcd.setCursor(0,0);
  lcd.print("    TOO SLOW    ");
  lcd.setCursor(0,1);
  lcd.print("   ENTRY VOID   ");
}

void Send_To_LCD_Queue(FunctionPointer func) {
  QueueItem item = {func};
  if (xQueueSend(LCD_Queue, &item, portMAX_DELAY) != pdPASS) {
    Serial.println("Failed to send to queue.");
  }
}