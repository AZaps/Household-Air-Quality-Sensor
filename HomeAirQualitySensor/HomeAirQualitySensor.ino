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
char convertBuffer[16];

// Sensor variables
float sensorVoltage;
float sensorValue;
int sensorCounter = 0;                // Counter variable for selecting sensor
long totalAmountOfSensors = 4;        // CHANGE DEPENDING ON AMOUNT OF SENSORS
long averageRunCounter = 0;
long sensorAverageArray[4];           // Averages of all the sensors through the minute

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
    Serial.println("One minute has passed.");
  }
} // END OF INTERRUPT

void loop() {
  sensorCounter = getSensorData(sensorCounter);
  
  // Only re-initializes the SD card if it was removed or not inserted from the start
  if (isSDInserted == true) {
    // Need to talk real quick to see if there is an SD card inserted
  } else if (isSDInserted == false) {
    // Go to function to (re)enable the SD card
    clarity = sdCardFunctions.initializeSD(53,49);
    sdInitCheck(clarity);
  }

  // Need to talk real quick to see if there is an SD card inserted

  // Check bool oneMinute value to see if true
  if (oneMinute) {
    // Check to see if SD is inserted
    if (isSDInserted) {
      Serial.println("The SD card is inserted. Proceeding to save data.");
      // Go to function to save the data
      oneMinuteSave();
    } else {
      Serial.println("The SD card is not inserted. Not saving data. Will check again on next pass.");
    }
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
  // Get date and time when either the data needs to be saved after a minute or if the user wants to see the currently running info

  sensorAverageArray[sensorNumber] = sensorAverageArray[sensorNumber] + readSensor(sensorNumber);
  Serial.print("On sensor number: ");
  Serial.print(sensorNumber);
  Serial.print(". Has a value of: ");
  Serial.println(sensorAverageArray[sensorNumber]);

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
  // Get the date first so the data will all be saved at the same time
  strcat(currentSavingDateTime, getDateTimeData());
  Serial.print("The current dateTime to be saved to all sensors: ");
  Serial.println(currentSavingDateTime);

  // Save all sensor data
  for (int i = 0; i <= totalAmountOfSensors; i++) {
    // Concatenate the date to the fullInput
    strcat(fullInput, currentSavingDateTime);

    // Get average value for sensor
    sensorAverageArray[i] = sensorAverageArray[i] / averageRunCounter;
    // Convert to an ascii character
    itoa(sensorAverageArray[i], convertBuffer, 10);
    // Append to the fullInput
    strcat(fullInput, convertBuffer);

    // Convert current sensor number to ascii
    int tempPos = i;
    itoa(tempPos, convertBuffer, 10);
    // Add to directoryPath
    strcat(directoryPath, "sen");
    strcat(directoryPath, "0");
    strcat(directoryPath,".txt");

    // Send to be saved
    Serial.print("INPUT TO BE SAVED: ");
    Serial.print(fullInput);
    Serial.print(". AT DIRECTORYPATH: ");
    Serial.println(directoryPath);

    saveSensorReadings(directoryPath, fullInput);
    clearCharPathnames();
  }
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
  for (int i = 0; i <= totalAmountOfSensors; i++) {
    sensorAverageArray[i] = 0;
    Serial.print("Sensor number: ");
    Serial.print(i);
    Serial.print(". Value is: ");
    Serial.println(sensorAverageArray[i]);
  }
}





