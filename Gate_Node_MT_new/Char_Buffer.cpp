#include "Char_Buffer.h"
#include <stdio.h>
#include <cstring> // Ensure cstring is included for string functions
#include <Arduino.h>

// Constructor to initialize the buffer with given dimensions 
CharBuffer::CharBuffer(int entries, int size) : Max_Buff_Size(entries), Buff_Width(size), Index(0) { 
  buffer = new char*[Max_Buff_Size]; 
  for (int i = 0; i < Max_Buff_Size; i++) { 
    buffer[i] = new char[Buff_Width]; 
  } 
}

// Destructor to free dynamically allocated memory 
CharBuffer::~CharBuffer() { 
  for (int i = 0; i < Max_Buff_Size; i++) { 
    delete[] buffer[i]; 
  } 
  delete[] buffer; 
}

// Function to add an entry to the buffer
void CharBuffer::addEntry(const char* entry) {
  if (Index < Max_Buff_Size) {
    strncpy(buffer[Index], entry, Buff_Width);
    Index++;
  }
}

// Function to search for an entry in the buffer
int CharBuffer::searchEntry(const char* entry) {
  for (int i = 0; i < Index; i++) {
    if (strncmp(buffer[i], entry, Buff_Width) == 0) {
      return i; // Return the index if the entry is found
    }
  }
  return -1; // Return -1 if the entry is not found
}

// Function to delete an entry and realign the buffer
void CharBuffer::deleteEntry(int index) {
  if (index >= 0 && index < Index) {
    for (int i = index; i < Index - 1; i++) {
      strncpy(buffer[i], buffer[i + 1], Buff_Width);
    }
    Index--;
  }
}

// Function to check the current number of entries 
int CharBuffer::getCurrentIndex() { 
  return Index; 
}

// Function to print the buffer (for debugging purposes)
void CharBuffer::printBuffer() {
  for (int i = 0; i < Index; i++) {
    Serial.print("Entry ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(buffer[i]);
  }
}
