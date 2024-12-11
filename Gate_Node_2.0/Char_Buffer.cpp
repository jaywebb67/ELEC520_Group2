/*
 *ELEC_520 
 *authored by Alex Meredith
*/ 

#include "Char_Buffer.h"
#include <stdio.h>
#include <cstring> // Ensure cstring is included for string functions
#include <Arduino.h>

// Constructor to initialize the buffer with given dimensions 
CharBuffer::CharBuffer(int entries, int size) : Max_Buff_Size(entries), Buff_Width(size+1), Index(0) { 
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
    strncpy(buffer[Index], entry, Buff_Width - 1); // Copy the string 
    buffer[Index][Buff_Width - 1] = '\0'; // Ensure null termination 
    Index++; 
  }
}

// Function to search for an entry in the buffer
int CharBuffer::searchEntry(const char* entry) {
  for (int i = 0; i < Index; i++) {
    // Find the colon (:) in the buffer entry to locate the password
    const char* colonPosition = strchr(buffer[i], ':');
    if (colonPosition != nullptr) {
      // Move the pointer to the character after the colon
      const char* passwordPart = colonPosition + 1;

      // Compare only the password part with the input entry
      if (strncmp(passwordPart, entry, 6) == 0) {
        return i; // Return the index if the password matches
      }
    }
  }
  return -1; // Return -1 if the entry is not found
}

char* CharBuffer::findUsername(int idx) {
  if (idx < 0 || idx >= Index) {
    return nullptr; // Return nullptr if index is out of bounds
  }
  // Find the position of the colon in the entry
  const char* colonPosition = strchr(buffer[idx], ':');
  if (colonPosition == nullptr) {
    return nullptr; // Return nullptr if no colon is found (malformed entry)
  }
  // Calculate the length of the username
  int usernameLength = colonPosition - buffer[idx];

  // Allocate memory for the username (add 1 for the null terminator)
  char* username = new char[usernameLength + 1];

  // Copy the username part into the allocated memory
  strncpy(username, buffer[idx], usernameLength);
  
  // Null-terminate the username string
  username[usernameLength] = '\0';
  
  return username; // Return the username
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
  if (Index == 0) { 
    Serial.println("The buffer is empty."); 
    } 
  else { 
    int i;
    for ( i = 0; i < Index; i++) { 
      Serial.print("Entry ");
        Serial.print(i); 
        Serial.print(": "); 
        Serial.println(buffer[i]); 
      } 
      buffno = i;
    } 
  }
