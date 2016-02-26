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
#include <stdlib.h>
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

/* ---------- Variable Declarations ---------- */
// SD related variables
SD_Functions sdCardFunctions;         // Used to communicate with library
File myFile;                          // Used to save information to SD card                      
bool isSDInserted;                    // Check to see if the SD card is inserted

// SD related saving variables
char directoryPath[21] = "/sendata/";
char fullInput[32];                   // The full input of Date/Time/Data
char directoryName[8];
char currentSavingDateTime[16];
char intBuffer[16];

// Sensor variables
float sensorVoltage;
float sensorValue;
int sensorCounter = 0;                // Counter variable for selecting sensor
int totalAmountOfSensors = 4;         // CHANGE DEPENDING ON AMOUNT OF SENSORS
long averageRunCounter = 0;
int sensorAverageArray[4];            // Averages of all the sensors through the minute

// Interrupt variables
long oneMinuteDelayCounter = 0;       // 60,000 milliseconds in a minute, resets once data has been saved
bool oneMinute = false;               // Turns true once a minute has passed, resets once data has been saved

// General purpose variables
bool clarity;                         // Return variable check

// Temporary variables
int analogPin = 0;


void setup() {
  Serial.begin(115200);
  // For debugging
  showFreeMemory();

  // Check for SD state
  clarity = sdCardFunctions.initializeSD(53,49); // THIS MIGHT CHANGE DEPENDING ON DIFFERENCES WITH TOUCHSCREEN DISPLAY INIT
  Serial.println(clarity);
  sdInitCheck(clarity);

  // Checks to see if SD card is inserted and if so, creates the filesystem
  setupFilesystemSD();

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
  if (oneMinuteDelayCounter == 60000) {
    oneMinute = true;
  }
} // END OF INTERRUPT

void loop() {
  sensorCounter = getSensorData(sensorCounter);
  
  // Only re-initializes the SD card if it was removed or not inserted from the start
  if (!isSDInserted) {
    // Go to function to (re)enable the SD card
    clarity = sdCardFunctions.initializeSD(53,49);
    sdInitCheck(clarity);
  }

    // Bool value changed from interrupts to see if a minute has passed and need to save data to sensor file
  if (oneMinute) {
    // Can't save any data if there isn't a SD card...
    if (!isSDInserted) {
      Serial.println("SD card currently not found...");
      Serial.print("Checking for SD card...");
      clarity = sdCardFunctions.initializeSD(53,49);
      sdInitCheck(clarity);
      if (!clarity) {
         Serial.println("SD card not found, NOT saving data.");
         clearInterruptVariables();
      } else {
        Serial.println("SD card was found, saving sensor data");
        }
    } else {
    // For debugging
    showFreeMemory();
      
      // Get the current dateTime so each sensor saves at the same time
      strcat(currentSavingDateTime, getDateTimeData());
      Serial.print("Current dateTime to be saved: ");
      Serial.println(currentSavingDateTime);
      // Loop through to save each sensor to the SD card
      for (int i = 0; i == totalAmountOfSensors; i++) {
        // Concatenate the date to the full input
        strcat(fullInput, currentSavingDateTime);
        // Get the average value
        sensorAverageArray[i] = sensorAverageArray[i]/averageRunCounter;
        // Convert to an ASCII character
        itoa(sensorAverageArray[i], intBuffer, 10);
        // Concatenate the data to the full input
        strcat(fullInput, intBuffer);               // Full input set for current sensor
        memset(intBuffer, 0, sizeof(intBuffer));
        // Convert sensor postion to ASCII and concatenate onto directory filename i.e. "/sendata/sen(insert sensor number).txt"
        itoa(i, intBuffer, 10);
        strcat(directoryPath, "sen");
        strcat(directoryPath, intBuffer);
        strcat(directoryPath, ".txt");
        // Save current sensor data to the SD card
        Serial.print("For sensor ");
        Serial.print(i);
        Serial.print(" we are saving: ");
        Serial.print(fullInput);
        Serial.print(". To: ");
        Serial.println(directoryPath);
        saveSensorReadings(directoryPath, fullInput);
        // Reset pathname and data input
        clearCharPathnames();
        // Set the current sensor data to 0
        sensorAverageArray[i] = 0;
      }
    }

  // For debugging
  showFreeMemory();
  
  clearInterruptVariables();
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
  /* Might not need this commented section since it is not saving anymore in this section, will leave in temporarly
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
  } */

  // Get date and time when either the data needs to be saved after a minute or if the user wants to see the currently running info

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
  // Local Variables
  int tempDateTimeInt;
  char tempDateTime[12];
  char fullDateTime[18];

  tempDateTimeInt = month();
  itoa(tempDateTimeInt, tempDateTime, 10);
  strcat(fullDateTime, tempDateTime);
  strcat(fullDateTime, "/");                            // xx/

  tempDateTimeInt = day();
  itoa(tempDateTimeInt, tempDateTime, 10);
  strcat(fullDateTime, tempDateTime);
  strcat(fullDateTime, "/");                            // xx/xx/

  tempDateTimeInt = year();
  itoa(tempDateTimeInt, tempDateTime, 10);
  strcat(fullDateTime, tempDateTime);
  strcat(fullDateTime, "/");                            // xx/xx/xx

  strcat(fullDateTime, " ");

  tempDateTimeInt = hour();
  if (hour() < 10) {strcat(fullDateTime, "0");}
  itoa(tempDateTimeInt, tempDateTime, 10);
  strcat(fullDateTime, tempDateTime);
  strcat(fullDateTime, ":");                            // xx/xx/xx xx:

  tempDateTimeInt = minute();
  if (minute() < 10) {strcat(fullDateTime, "0");}
  itoa(tempDateTimeInt, tempDateTime, 10);
  strcat(fullDateTime, tempDateTime);
  strcat(fullDateTime, " ");                            // xx/xx/xx xx:xx(space)

  return fullDateTime;
}

/* Reads the corresponding sensor data */
float readSensor(int currentSensor) {

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
    isSDInserted = false;
  }
  myFile.close();
}

/* Clears the pathname of the input files */
void clearCharPathnames() {
  memset(directoryPath, 0, sizeof(directoryPath));
  memset(fullInput, 0, sizeof(fullInput));
  strcat(directoryPath, "/sendata/");
}

void clearInterruptVariables() {
  // Reset the averageRunCounter;
  averageRunCounter = 0;
  // Reset the oneMinuteDelayCounter
  oneMinuteDelayCounter = 0;
  // Reset the oneMinute bool
  oneMinute = false;
}





