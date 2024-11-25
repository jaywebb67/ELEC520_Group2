#ifndef CHARBUFFER_H
#define CHARBUFFER_H

#include <Arduino.h>

class CharBuffer {
  private:
    int Max_Buff_Size;    // Maximum number of entries in the buffer
    int Buff_Width;       // Size of each entry
    char** buffer;        // Pointer to a dynamically allocated 2D array
    int Index;            // Current number of entries in the buffer

  public:
    // Constructor to initialize the buffer with given dimensions
    CharBuffer(int entries, int size);

    // Destructor to free dynamically allocated memory
    ~CharBuffer();

    // Function to add an entry to the buffer
    void addEntry(const char* entry);

    // Function to search for an entry in the buffer
    int searchEntry(const char* entry);

    // Function to delete an entry and realign the buffer
    void deleteEntry(int index);

    // Function to check the current number of entries 
    int getCurrentIndex();

    // Function to print the buffer (for debugging purposes)
    void printBuffer();
};


#endif // CHARBUFFER_H
