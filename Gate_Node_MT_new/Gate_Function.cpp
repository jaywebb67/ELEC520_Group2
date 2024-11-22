// #include "Gate_Function.h"
// #include "Char_Buffer.h"

// void Enter_Mess() {
//   lcd.setCursor(0,0);
//   lcd.print("   Enter Key:   ");
// }

// void Access_G() {
//   lcd.setCursor(0,0);
//   lcd.print("     Access     ");
//   lcd.setCursor(0,1);
//   lcd.print("     Granted    ");
// }

// void Access_D() {
//   lcd.setCursor(0,0);
//   lcd.print("     Access     ");
//   lcd.setCursor(0,1);
//   lcd.print("     Denied     ");
// }

// void Bye() {
//   lcd.setCursor(0,0);
//   lcd.print("     So Long    ");
//   lcd.setCursor(0,1);
//   lcd.print("    Farewell    ");
// }

// void Invalid_Mess(){
//   lcd.setCursor(0,0);
//   lcd.print("    Invalid     ");
//   lcd.setCursor(0,1);
//   lcd.print("     Input      ");
// }

// void Broken(){
//   lcd.setCursor(0,0);
//   lcd.print("    I Need A    ");
//   lcd.setCursor(0,1);
//   lcd.print("    COFFEE      ");
// }



// void Send_To_LCD_Queue(FunctionPointer func) {
//   QueueItem item = {func};
//   if (xQueueSend(queue, &item, portMAX_DELAY) != pdPASS) {
//     Serial.println("Failed to send to queue.");
//   }
// }


// //Function to check recieved keycodes
// int Test_Entry_Code(const char* code){
//   int x = Valid_Entrance_Codes.searchEntry(code);
//       if (x < 0) {

//         Serial.println("Access Denied");
//         return 1;
//       } 
//       else {
//         x = Current_Codes_In_Use.searchEntry(code);
//         if (x < 0) {
           
//           Serial.println("Access Granted");
//           Current_Codes_In_Use.addEntry(code);
//           return 2;
//         } else {
          
//           Serial.println("Exit Goodbye");
//           Current_Codes_In_Use.deleteEntry(x); 
//           return 3;
//         }
//       }
// }

// void notify_User(int x, int y) {
//   switch (x) {
//     case 1:     //Bluetooth thread call
//       switch (y) {
//         case 1:
//           SerialBT.write("Access Denied");
//           break;
//         case 2:
//           SerialBT.write("Access Granted");
//           break;
//         case 3:
//           SerialBT.write("Toodle Pip");
//           break;
//         default:
//           SerialBT.write("I Need A COFFEE");
//           break;
//       }
//       break;
//     case 2:     //Keypad thread call
//       switch (y) {
//         case 1:
//           Send_To_LCD_Queue(Access_D);
//           break;
//         case 2:
//           Send_To_LCD_Queue(Access_G);
//           break;
//         case 3:
//           Send_To_LCD_Queue(Bye);
//           break;
//         default:
//           Send_To_LCD_Queue(Broken);
//           break;
//       }
//       break;
//     default:
//       SerialBT.write("I Need A COFFEE");
//       Send_To_LCD_Queue(Broken);
//       break;
//   }
// }

