/*
   Home Air Quality Sensor
   Senior Design 2016

   Code Version 0.2

   Project Members:
   Lucas Enright
   Barry Ng
   Tyler Parchem
   Anthony Zaprzalka

   Description:




*/

/* ---------- Libraries ---------- */
//#include <SDCardLibraryFunctions.h> // Handles all SD related functions
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <HardwareSerial.h>
#include <MemoryFree.h>
#include <Time.h>
#include <TimeLib.h>
#include <stdlib.h>

/* ---------- Variable Declarations ---------- */
// SD related variables
SD_Functions sdCardFunctions;                 // Used to communicate with library
File myFile;                                  // Used to save information to SD card
bool isSDInserted;                            // Check to see if the SD card is inserted

// SD related saving variables
String directoryPath = "/sendata/";
String fullInput;                             // The full input of Date/Time/Data
String directoryName = "sendata";
String currentSavingDateTime;

// Sensor variables
float sensorVoltage;
float sensorValue;
int sensorCounter = 8;                        // Counter variable for selecting sensor
double totalAmountOfSensors = 4;              // CHANGE DEPENDING ON AMOUNT OF SENSORS
double sensorAverageArray[5];                 // Averages of all the sensors through the minute
double sensorRuntimeCounter = 0;

// Interrupt variables
unsigned int oneMinuteDelayCounter = 0;       // 60,000 milliseconds in a minute, resets once data has been saved
bool oneMinute = false;                       // Turns true once a minute has passed, resets once data has been saved

// General purpose variables
bool clarity;                                 // Return variable check

// Temporary variables
int analogPin = 0;

void setup() {
  Serial.begin(115200);

  // For debugging
  showFreeMemory();

  // Check for SD state
  clarity = sdCardFunctions.initializeSD(53, 49); // THIS MIGHT CHANGE DEPENDING ON DIFFERENCES WITH TOUCHSCREEN DISPLAY INIT
  sdInitCheck(clarity);

  // Checks to see if SD card is inserted and if so, creates the filesystem
  if (clarity) {
    setupFilesystemSD();
  }

  // Manually set time here, or get inputted information from screen
  /*      hr  min sec day mth yr      *24 Hour time*/
  setTime(10, 30, 00, 15, 2, 16);

  // Enable interrupts
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);

  // For debugging
  showFreeMemory();

} // END OF SETUP

SIGNAL(TIMER0_COMPA_vect) {
  // Increments once a millisecond
  oneMinuteDelayCounter++;
  if (oneMinuteDelayCounter == 20000) {
    oneMinute = true;
    Serial.println("One minute has passed.");
  }
} // END OF INTERRUPT

void loop() {
  sensorCounter = getSensorData(sensorCounter);

  // Check bool oneMinute value to see if true
  if (oneMinute) {
    // Check to see if SD is inserted
    if (SD.exists(directoryName)) {
      Serial.println("The SD card is inserted. Proceeding to save data.");
      // Go to function to save the data
      oneMinuteSave();
    } else {
      Serial.println("The SD card is not inserted. Not saving data. Will check again on next pass.");
      if (isSDInserted == false) {
        // Go to function to (re)enable the SD card
        clarity = sdCardFunctions.initializeSD(53, 49);
        sdInitCheck(clarity);
      }
    }
    clearInterruptVariables();
  }

  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == 'S') {
      Serial.println("STOPPING");
      while (true) {}
    }
  }


} // END OF LOOP

/* Function Declarations */

/* For debugging */
void showFreeMemory() {
  Serial.print("Free memory = ");
  Serial.println(freeMemory());
}

/* Initial check to see if the SD is inserted */
void sdInitCheck(bool SDInitBool) {
  if (SDInitBool) {
    Serial.println("SD card was found");
    isSDInserted = true;
  } else {
    Serial.println("SD card was not inserted. Need to check again later if inserted.");
    isSDInserted = false;
  }
}

/* Sets up the filesystem if the SD is inserted and initialized */
void setupFilesystemSD() {
  if (isSDInserted) {
    // The SD card was inserted
    // Need to check if /sendata/ directory and sensor files exist
    createFilesystem();
  } else {
    Serial.println("No SD card is inserted. Data is NOT being saved");
    // Alert the user or display static 'No SD found' message
  }
}

/* Creates the filesystem of the directory and files */
void createFilesystem() {

  // For debugging
  showFreeMemory();

  myFile = SD.open("/");
  clarity = sdCardFunctions.checkForDirectory(directoryName);
  if (clarity) {
    Serial.print("Directory ");
    Serial.print(directoryName);
    Serial.println(" was created or it already exists and nothing was done.");
  } else {
    Serial.print("Directory ");
    Serial.print(directoryName);
    Serial.println(" was not created.");

    // For debugging
    showFreeMemory();

    myFile.close();
    return;
  }

  // For debugging
  showFreeMemory();

  myFile.close();

  // Check for files associated with the sensors. Need to change depending on amount of sensors
  for (int i = 0; i < 5; i++) {
    switch (i) {
      case 0:
        directoryPath = String(directoryPath + "sen0.txt");
        break;
      case 1:
        directoryPath = String(directoryPath + "sen1.txt");
        break;
      case 2:
        directoryPath = String(directoryPath + "sen2.txt");
        break;
      case 3:
        directoryPath = String(directoryPath + "sen3.txt");
        break;
      case 4:
        directoryPath = String(directoryPath + "sen4.txt");
        break;
    }

    // For debugging
    showFreeMemory();

    myFile = SD.open("/sendata/");
    // Creates the files or verifies they already exist
    clarity = sdCardFunctions.checkForSensorFile(myFile, directoryPath);
    if (clarity) {
      Serial.print(directoryPath);
      Serial.println(" was created or already exists.");
    } else {
      Serial.print(directoryPath);
      Serial.println(" was NOT created.");
    }
    // Reset the directiry path filename
    directoryPath = String("/sendata/");
    myFile.close();

    // For debugging
    showFreeMemory();
  }
}

/* Switches through the sensors to get the data */
int getSensorData(int sensorNumber) {
  // Get date and time when either the data needs to be saved after a minute or if the user wants to see the currently running info

  sensorAverageArray[sensorNumber] = sensorAverageArray[sensorNumber] + readSensor(sensorNumber);
  Serial.print("On sensor number: ");
  Serial.print(sensorNumber);
  Serial.print(". Has a value of: ");
  Serial.println(sensorAverageArray[sensorNumber]);

  // Increment sensor number and return
  sensorNumber++;
  if (sensorNumber > 15) {
    sensorNumber = 8;
    sensorRuntimeCounter++;
    return sensorNumber;
  }
  return sensorNumber;
}

/* Gets and returns the current time formatted as xx/xx/xx xx:xx (month)/(day)/(year) (hour):(minute) */
String getDateTimeData() {
  // Local Variables
  int tempDateTimeInt;
  char tempDateTime[16];
  String dateTimeString;
  String fullDateTime;

  tempDateTimeInt = month();
  itoa(tempDateTimeInt, tempDateTime, 10);
  dateTimeString = String(tempDateTime);
  fullDateTime = String(fullDateTime + dateTimeString + "/");      // xx/

  tempDateTimeInt = day();
  itoa(tempDateTimeInt, tempDateTime, 10);
  dateTimeString = String(tempDateTime);
  fullDateTime = String(fullDateTime + dateTimeString + "/");      // xx/xx/

  tempDateTimeInt = year();
  dateTimeString = String(tempDateTime);
  fullDateTime = String(fullDateTime + dateTimeString + " ");     // xx/xx/xx


  tempDateTimeInt = hour();
  if (hour() < 10) {
    fullDateTime = String(fullDateTime + "0");
  }
  itoa(tempDateTimeInt, tempDateTime, 10);
  dateTimeString = String(tempDateTime);
  fullDateTime = String(fullDateTime + dateTimeString + ":");    // xx/xx/xx xx:

  tempDateTimeInt = minute();
  if (minute() < 10) {
    fullDateTime = String(fullDateTime + "0");
  }
  itoa(tempDateTimeInt, tempDateTime, 10);
  dateTimeString = String(tempDateTime);
  fullDateTime = String(fullDateTime + dateTimeString + " ");    // xx/xx/xx xx:xx(space)

  Serial.print("The returning dateTime is: ");
  Serial.println(fullDateTime);

  return fullDateTime;
}

/* Reads the corresponding sensor data */
float readSensor(int currentSensor) {

  // Will need to redo based on sensor looking to capture
  // sensorValue is global variable
  sensorValue = analogRead(currentSensor);

  return sensorValue;
}

/* After a minute this function will be called to save the value of each sensor */
void oneMinuteSave() {
  Serial.println("IN ONE MINUTE SAVE");
  // Get the date first so the data will all be saved at the same time
  currentSavingDateTime = getDateTimeData();
  
  // Save all sensor data
  for (int j = 8; j <= 15; j++) {
    // Concatenate the date to the fullInput
    fullInput = String(fullInput + currentSavingDateTime);

    // Get average value for sensor
    long tempAverageLong = sensorAverageArray[j];
    String tempAverageString;
    tempAverageLong = tempAverageLong / sensorRuntimeCounter;
    // Convert to an ascii character
    tempAverageString = String(tempAverageLong);
    
    // Append to the fullInput
    fullInput = String(fullInput + tempAverageLong);

    // Convert current sensor number to ascii
    directoryPath = String(directoryPath + "sen" + j + ".txt");
    // Send to be saved
    Serial.print("INPUT TO BE SAVED: ");
    Serial.print(fullInput);
    Serial.print(". AT DIRECTORYPATH: ");
    Serial.println(directoryPath);
    saveSensorReadings(directoryPath, fullInput);
    
    clearSavedStrings();
    
    // For debugging
    showFreeMemory();
  }
}

/* Saves the full input to the SD card */
void saveSensorReadings(String saveTo, String dataToSave) {
  myFile = SD.open(directoryPath, FILE_WRITE);
  clarity = sdCardFunctions.writeToSD(myFile, dataToSave, saveTo);
  if (clarity) {
    Serial.println("The data was correctly saved");
  } else {
    Serial.println("The data was NOT saved.");
    isSDInserted = false;
  }
  myFile.close();
}

/* Clears the pathname of the input files */
void clearSavedStrings() {
  directoryPath = String("/sendata/");
  fullInput = String("");
}

void clearInterruptVariables() {
  // Reset the averageRunCounter;
  sensorRuntimeCounter = 0;
  // Reset the oneMinute bool
  oneMinute = false;
  // Reset the current dateTime saved
  currentSavingDateTime = String("");
  // Reset the values saved
  for (int i = 0; i <= totalAmountOfSensors; i++) {
    sensorAverageArray[i] = 0;
    Serial.print("Sensor number: ");
    Serial.print(i);
    Serial.print(". Value is: ");
    Serial.println(sensorAverageArray[i]);
  }
  // Reset the oneMinuteDelayCounter
  oneMinuteDelayCounter = 0;
}





