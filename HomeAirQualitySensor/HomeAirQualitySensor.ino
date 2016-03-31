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

/* ---------- Definitions ---------- */
#define MQ2 8                                 // Analog pin for MQ2 sensor
#define MQ5 9                                 // Analog pin for MQ5 sensor
#define RL 5                                  // Sensor load resistance in k-Ohms
#define RO_CLEAN_AIR 9.83                     // clean air factor which is (sensor resistance in clean air/RO)

#define CALIBRATION_SAMPLE_SIZE 25
#define CALIBRATION_DELAY 250

// Don't need multiple readings per loop cycle
// Loop throughs will take average after a minute

// MQ2
#define LPG 0
#define CO 1
#define SMOKE 2
// MQ5

// Temperature and Humidity sensor
// Change these values to be last
#define TEMP 3
#define HUMIDITY 4

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
float sensorValue;
int sensorCounter = 0;                        // Counter variable for selecting sensor
int totalAmountOfSensors = 5;                 // CHANGE DEPENDING ON AMOUNT OF SENSORS
long sensorAverageArray[5];                   // Averages of all the sensors through the minute
long sensorRuntimeCounter = 0;

// Interrupt variables
unsigned int oneMinuteDelayCounter = 0;       // 60,000 milliseconds in a minute, resets once data has been saved
bool oneMinute = false;                       // Turns true once a minute has passed, resets once data has been saved

// General purpose variables
bool clarity;                                 // Return variable check

// Temporary variables
int analogPin = 0;

float LPGCurve[3] = {2.3, 0.21, -0.47};
float COCurve[3] = {2.3, 0.72, -0.34};
float SmokeCurve[3] = {2.3, 0.53, -0.44};

float RoMQ2 = 10;                             // 10 kOhms
float RoMQ5 = 10;

void setup() {
  Serial.begin(115200);

  // For debugging
  showFreeMemory();

  // Check for SD state
  clarity = sdCardFunctions.initializeSD(53, 49);
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

  // Calibrate the senor
  RoMQ2 = calibrateMQSensor(MQ2);
  RoMQ5 = calibrateMQSensor(MQ5);
  
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

/*
 * Initial check to see if an SD card is inserted into the system
 */
void sdInitCheck(bool SDInitBool) {
  if (SDInitBool) {
    Serial.println("SD card was found");
    isSDInserted = true;
  } else {
    Serial.println("SD card was not inserted. Need to check again later if inserted.");
    isSDInserted = false;
  }
}

/*
 * This checks to see if the SD card is inserted and will setup the filesystem.
 * This saftey check is needed because if the Arduino tries create a filesystem on a non-existent SD
 * card the program will crash.
 */
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

/*
 * This function will first check to see if the correct directoy and files have been created on the SD card
 * If the directory and files are already created then nothing is needed.
 * Otherwise, the function will create the directory and files and relies on the custom SD library
 */
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

/*
 * Initial function to determine the currently selected sensor which will go to another function to 
 * read the currently selected sensor and will pass the value back to this function which will hold a total
 * added value for each gas type until a minute has passed.
 * It also keeps a running total for when the average is needed after a minute
 */
int getSensorData(int sensorNumber) {
  float tempSensorValue = 0;
  
  // Get pure analog or digital value for sensors
  tempSensorValue = readSensor(sensorNumber);

  // Convert value according to datasheet
  // Saves in ppm 
  tempSensorValue = getSensorPercentages(tempSensorValue, sensorNumber);
  
  sensorAverageArray[sensorNumber] = sensorAverageArray[sensorNumber] + tempSensorValue;
  Serial.print("On sensor number: ");
  Serial.print(sensorNumber);
  Serial.print(". Has a value of: ");
  Serial.println(sensorAverageArray[sensorNumber]);

  // Increment sensor number and return
  sensorNumber++;
  if (sensorNumber > totalAmountOfSensors) {
    sensorNumber = 0;
    sensorRuntimeCounter++;
    return sensorNumber;
  }
  return sensorNumber;
}

/*
 * Reads the pure analog resistance value from the sensors or
 * digital value from the temperature and humidity sensor
 */
float readSensor(int currentSensor) {
  float rs = 0;

  // Reads the analog or digital value of the sensors
  // First checks MQ2 sensor
  if (currentSensor == 0 || currentSensor <= 2) {
    rs = sensorResistanceCalculation(analogRead(MQ2));
    rs = rs / RoMQ2;
  }
  // MQ5 
  //else if (currentSensor ==) {
    // rs = sensorResistanceCalculation(analogRead(MQ5));
    // rs = rs / RoMQ5;
  //}
  // Temperature and humidity
  else if (currentSensor == 3 || currentSensor == 4) {
    
  }
  return rs;
}

/*
 * Passes the gas curves to getPercentages if MQ sensor otherwise
 * will go to different function to get the temperature or humidity
 */
long getSensorPercentages (float rsRoRatio, int gasID) {
  switch (gasID) {
    case 0:
      return getPercentages(rsRoRatio,LPGCurve);
      break;
    case 1:
      return getPercentages(rsRoRatio,COCurve);
      break;
    case 2:
      return getPercentages(rsRoRatio,SmokeCurve);
      break;
    case 3:
      return getTemperatureOrHumidity(rsRoRatio, TEMP);
      break;
    case 4:
      return getTemperatureOrHumidity(rsRoRatio, HUMIDITY);
      break;
  }
  return 0;
}

/*
 * Using the slopes from the datasheet determines the ppm value of the current gas
 */
long getPercentages(float rsRoRatio, float *pCurve) {
  return (pow(10, (((log(rsRoRatio) - pCurve[1]) / pCurve[2]) + pCurve[0])));
}

/*
 * Will determine if it is checking the temperature or humidity value, might change if the sensor
 * is only polled once a minute
 */
long getTemperatureOrHumidity(float tempOrHumidityValue, int tempOrHumidityID) {
  return 0;
}
/*
 * This function is called when the current date and time is needed when saving the sensor information
 * It returns the dateTime formatted as xx/xx/xx xx:xx (month)/(day)/(year) (hour):(minute)
 */
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

/*
 * After a minute has passed, this function will be callled.
 * It will iterate through each array position that has been cumulatively adding the sensor data.
 * Each array position holds a specific sensor type which is declared at the top
 * All sensors will be saved at the save time so, the dateTime will only be determined once.
 * It will concate all the relevant information( time, date, sensor value) into one string and set the pathname
 * and pass the data to another function to save the data
 */
void oneMinuteSave() {
  Serial.println("IN ONE MINUTE SAVE");

  // Get the date first so the data will all be saved at the same time
  currentSavingDateTime = getDateTimeData();

  // Save all sensor data
  for (int i = 0; i <= totalAmountOfSensors; i++) {
    // Concatenate the date to the fullInput
    fullInput = String(fullInput + currentSavingDateTime);
    // Get average value for sensor
    long tempAverageLong = sensorAverageArray[i];
    String tempAverageString;
    tempAverageLong = tempAverageLong / sensorRuntimeCounter;
    // Convert to an ascii character
    tempAverageString = String(tempAverageLong);

    // Append to the fullInput
    fullInput = String(fullInput + tempAverageString);

    // Convert current sensor number to ascii
    directoryPath = String(directoryPath + "sen" + i + ".txt");
    Serial.println(directoryPath);
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

/*
 * Passes the save data and pathname to the custom SD library to be saved to the SD card
 */
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

/*
 * Clears the pathnames of the input files to be saved to the SD card
 */
void clearSavedStrings() {
  directoryPath = String("/sendata/");
  fullInput = String("");
}

/*
 * Clears all the variables that are used in the interrupt routine and during saving
 * so the next time the data needs to be saved the variables will be cleared
 */
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

/*
 * Calibrates the MQ sensors upon startup to get the correct resistance values
 */
float calibrateMQSensor(int mqSensorPin) {
  float value = 0;

  // Get multiple samples for the calibration report
  for (int i = 0; i < CALIBRATION_SAMPLE_SIZE; i++) {                          
    value += sensorResistanceCalculation(analogRead(mqSensorPin));
    delay(CALIBRATION_DELAY);
  }
  // Average
  value = value / CALIBRATION_SAMPLE_SIZE;

  // Yields Ro value according to chart on datasheet
  value = value / RO_CLEAN_AIR;

  return value;
}

/*
 * Calculates the voltage across the load resistor to determine the current sensor value
 */
float sensorResistanceCalculation (int adcValue) {
  return (((float)RL * (1023 - adcValue) / adcValue));
}




