/*
 *ELEC_520 
 *authored by Alex Meredith
*/ 

#ifndef LCD_MANAGER_H
#define LCD_MANAGER_H

#include "I2C_LCD.h"
#include <Arduino.h>
//#include <LiquidCrystal_I2C.h>

//extern LiquidCrystal_I2C lcd;
extern I2C_LCD lcd;
extern QueueHandle_t LCD_Queue;

typedef void (*FunctionPointer)(void);

typedef struct {
  FunctionPointer func;
} QueueItem;

void LCD_Innit();
void Enter_Mess();
void Access_G();
void Access_D();
void Bye();
void Invalid_Mess();
void Broken();
void T_Out();
void Send_To_LCD_Queue(FunctionPointer func);

#endif // LCD_MANAGER_H
