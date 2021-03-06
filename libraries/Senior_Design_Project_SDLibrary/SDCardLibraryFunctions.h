/*
 SDCardLibraryFunctions.h
 Core header file for interfacing with the SD card
 Anthony Zaprzalka for Senior Design Project 2015-2016
 December 19, 2015
 
 Header file for interfacing with the SD card reader and saving the sensor data.
 This file contains all the variable and function declarations.
 */

/*
 **** Pin Assignments ****
 Sensor Pins             Arduino Pins (Uno)     Arduino Pins (Mega)
 D3 (Chip Select)        8                      49
 CMO (MOSI)              11                     51
 CLK (Clock)             13                     52
 D0 (MISO)               12                     50
 VCC                     3.3 V                  3.3 V
 GND                     GND                    GND
 SS Pin(If not used      10                     53
 still needs to be
 open)
 */

#ifndef SDCardLibraryFunctions_h
#define SDCardLibraryFunctions_h

#include "Arduino.h"
#include <Arduino.h>                  // Include the wiring API
#include <SPI.h>                      // Include the Serial Parallel Interface
#include <SD.h>                       // Include the SD library



    // library interface description
class SD_Functions {
        // User accessible public interface
public:
    bool initializeSD(int, int);
    bool checkForDirectory(String);
    bool checkForSensorFile(File, String);
    void printDirectory(File, int);
    bool writeToSD(File, String, String);
    bool readFromSD(File, String);
        // Library accessible private interface
private:
    
};

#endif