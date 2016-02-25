/*
 * Home Air Quality Sensor
 * Senior Design 2016
 * 
 * Code Version 0.1
 * 
 * Project Members:
 * Lucas Enright
 * Barry Ng
 * Tyler Parchem
 * Anthony Zaprzalka
 * 
 * Description:
 * 
 * 
 * 
 * 
 */

/* ---------- Libraries ---------- */
#include <SDCardLibraryFunctions.h> // Handles all SD related functions
/*
 * SDCardLibraryFunctions library also includes
 * Arduino.h
 * SPI.h
 * SD.h
 * HardwareSerial.h
 * MemoryFree.h -- Used to check for currently free memory
 * Time.h
 * TimeLib.h
 * 
 * Also sets Serial.begin(9600)
 */
#include <stdlib.h>

/* ---------- Variable Declarations ---------- */
// SD related variables
SD_Functions sdCardFunctions;         // Used to communicate with library
File myFile;                          // Used to save information to SD card                      
bool isSDInserted;                    // Check to see if the SD card is inserted

// SD related saving variables
char directoryPath[21] = "/sendata/";
char fullInput[32];                   // The full input of Date/Time/Data
char directoryName[8];

// Sensor variables
float sensorVoltage;
float sensorValue;
int sensorCounter = 0;                // Counter variable for selecting sensor
int totalAmountOfSensors = 4;         // CHANGE DEPENDING ON AMOUNT OF SENSORS
long averageRunCounter = 0;
int sensorAverageArray[4];            // Averages of all the sensors through the minute


// General purpose variables
bool clarity;                         // Return variable check

// Temporary variables
int analogPin = 0;


void setup() {
  // Disable Interrupts

  // For debugging
  showFreeMemory();
  
  // Check for SD state
  clarity = sdCardFunctions.initializeSD(53,49); // THIS MIGHT CHANGE DEPENDING ON DIFFERENCES WITH TOUCHSCREEN DISPLAY INIT
  sdInitCheck(clarity);

  // Checks to see if SD card is inserted and if so, creates the filesystem
  setupFilesystemSD();

  // Manually set time here, or get inputted information from screen
  /*      hr  min sec day mth yr      *24 Hour time*/
  setTime(10, 30, 00, 15, 2, 16);

  // Enable Interrupts

  // For debugging
  showFreeMemory();

} // END OF SETUP

void loop() {
  sensorCounter = getSensorData(sensorCounter);
  
  if (!isSDInserted) {
    // Disable Interrupts

    // Go to function to (re)enable the SD card
    sdInitCheck(false);

    // Enable Interrupts
  }

    // Bool value changed from interrupts to see if a minute has passed and need to save data to sensor file

    // Disable the interrupts
    // Gets date and time.
    // Then averages each of the sensors and keeps the same date and time. Need to convert from int to ascii
    // Then saves to the SD card
    // Enable the interrupts
    
  
  
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
  // Creates or verifies directory
  strcat(directoryName, "sendata");
  
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
    switch(i) {
      case 0:
        strcat(directoryPath, "sen0.txt");
        break;
      case 1:
        strcat(directoryPath, "sen1.txt");
        break;
      case 2:
        strcat(directoryPath, "sen2.txt");
        break;
      case 3:
        strcat(directoryPath, "sen3.txt");
        break;
      case 4:
        strcat(directoryPath, "sen4.txt");
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
    memset(directoryPath, 0, sizeof(directoryPath));
    strcat(directoryPath, "/sendata/");
    delay(2000);
    myFile.close();
    
    // For debugging
    showFreeMemory();
  }
}

/* Switches through the sensors to get the data */
int getSensorData(int sensorNumber) {
  switch (sensorNumber) {
    case 0:
      strcat(directoryPath, "sen0.txt");
      break;
    case 1:
      strcat(directoryPath, "sen1.txt");
      break;
    case 2:
      strcat(directoryPath, "sen2.txt");
      break;
    case 3:
      strcat(directoryPath, "sen3.txt");
      break;
    case 4:
      strcat(directoryPath, "sen4.txt");
      break;
  }

  // Get date and time when either the data needs to be saved after a minute or if the user wants to see the currently running info
  
//  // Get the date
//  strcat(fullInput, getDateTimeData());
//  Serial.print("Date and time:\t");
//  Serial.println(fullInput);

//  // Get the sensor data
//  strcat(fullInput, readSensor(sensorNumber));

  sensorAverageArray[sensorNumber] = sensorAverageArray[sensorNumber] + readSensor(sensorNumber);

  // Clear out the variables for next time
  clearCharPathnames();

  // Increment sensor number and return
  sensorNumber++;
  if (sensorNumber > totalAmountOfSensors) {
    sensorNumber = 0;

    // Every time the sensor number is maxed increase the average counter
    // This way this'll be an average total versus incrementing per sensor reading
    averageRunCounter++;
    
    return sensorNumber;
  }
  return sensorNumber;
}

/* Gets and returns the current time formatted as xx/xx/xx xx:xx (month)/(day)/(year) (hour):(minute) */
char* getDateTimeData() {
  int tempDateTimeInt;
  char tempDateTime[12];
  char fullDateTime[18];

  tempDateTimeInt = month();
  itoa(tempDateTimeInt, tempDateTime, 10);
  strcat(fullDateTime, tempDateTime);
  strcat(fullDateTime, "/");

  tempDateTimeInt = day();
  itoa(tempDateTimeInt, tempDateTime, 10);
  strcat(fullDateTime, tempDateTime);
  strcat(fullDateTime, "/");

  tempDateTimeInt = year();
  itoa(tempDateTimeInt, tempDateTime, 10);
  strcat(fullDateTime, tempDateTime);
  strcat(fullDateTime, "/");

  strcat(fullDateTime, " ");

  tempDateTimeInt = hour();
  if (hour() < 10) {strcat(fullDateTime, "0");}
  itoa(tempDateTimeInt, tempDateTime, 10);
  strcat(fullDateTime, tempDateTime);
  strcat(fullDateTime, "/");

  tempDateTimeInt = minute();
  if (minute() < 10) {strcat(fullDateTime, "0");}
  itoa(tempDateTimeInt, tempDateTime, 10);
  strcat(fullDateTime, tempDateTime);
  strcat(fullDateTime, "/");

  return fullDateTime;
}

///* Reads the corresponding sensor data */
//char* readSensor(int currentSensor) {
//  // Local variables
//  char sensorDataChar[12];
//
//  // Will need to redo based on sensor looking to capture
//  sensorValue = analogRead(currentSensor);
//
//  // Convert to string
//  itoa(sensorValue, sensorDataChar, 10);
//
//  return sensorDataChar;
//}

/* Reads the corresponding sensor data */
int readSensor(int currentSensor) {

  // Will need to redo based on sensor looking to capture
  // sensorValue is global variable
  sensorValue = analogRead(currentSensor);

  return sensorValue;
}


/* Saves the full input to the SD card */
void saveSensorReadings(char* saveTo, char* dataToSave) {
  myFile = SD.open(directoryPath, FILE_WRITE);
  clarity = sdCardFunctions.writeToSD(myFile, dataToSave, saveTo);
  if (clarity) {
    Serial.println("The data was correctly saved");
  } else {
    Serial.println("The data was NOT saved.");
  }
  myFile.close();
}

/* Clears the pathname of the input files */
void clearCharPathnames() {
  memset(directoryPath, 0, sizeof(directoryPath));
  memset(fullInput, 0, sizeof(fullInput));
  strcat(directoryPath, "/sendata/");
}







