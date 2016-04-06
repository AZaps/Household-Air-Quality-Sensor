/*
   Home Air Quality Sensor
   Senior Design 2016

   Code Version 0.3

   Project Members:
   Lucas Enright
   Barry Ng
   Tyler Parchem
   Anthony Zaprzalka

   Description:
   This multi-gas sensing unit can detect more gases than the average homeowner accounts for. Typical household sensors 
   include smoke and carbon monoxide. This unit can detect variable concentrations of propane, CO, LPG, CH4, H2, and alcohol. 
   It also detects smoke, temperature, and humidity. 
   
   While detecting the specified gases above, it will alert the user to abnormal or high concentrations of the specified gas 
   by changing the color of the touchscreen LCD to a specific color, yellow for a warning and red for an alarm. If no alarm is 
   currently being shown, the user is free to ‘scroll’ through the gas list and get a current value of the selected gas. Under 
   normal conditions most of the values should be at or around 0.
   
   The device continuously monitors while powered-on and after every minute, it will save the average value of each gas to an 
   SD card. This data can later be viewed through a companion desktop application so device lifetime values can be easily seen 
   and understood.
*/

/* ---------- Libraries ---------- */
// Included
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <HardwareSerial.h>
#include <stdlib.h>
// Custom
#include <SDCardLibraryFunctions.h>           // Handles all SD related functions          
#include <DHT.h>
#include <Time.h>
#include <TimeLib.h>
#include <MemoryFree.h>

/* ---------- Definitions ---------- */
#define MQ2 8                                 // Analog pin for MQ2 sensor
#define MQ5 9                                 // Analog pin for MQ5 sensor
#define RL 5                                  // Sensor load resistance in k-Ohms
#define RO_CLEAN_AIR 9.83                     // clean air factor which is (sensor resistance in clean air/RO)

#define CALIBRATION_SAMPLE_SIZE 25
#define CALIBRATION_DELAY 250

#define DHTPIN 45                             // Digital pin for the temperature and humidity sensor
#define DHTTYPE DHT22

// Don't need multiple readings per loop cycle
// Loop throughs will take average after a minute

// MQ2
#define PROPANE 0                            // These values will also be used to link an array position to this specific gas
#define CO 1
#define SMOKE 2 

// MQ5
#define LPG 3
#define CH4 4
#define H2 5
#define ALCOHOL 6

// Temperature and Humidity sensor
#define TEMP 7
#define HUMIDITY 8 

/* ---------- Variable Declarations ---------- */
// SD related variables
SD_Functions sdCardFunctions;                 // Used to communicate with library
File myFile;                                  // Used to save information to SD card
bool isSDInserted;                            // Check to see if the SD card is inserted

// SD related saving variables
String directoryPath = "/sendata/";           // Pathname where sensor data will be saved
String fullInput;                             // The full input of Date/Time/Data
String directoryName = "sendata"; 
String currentSavingDateTime;                 // Full length of dateTime

// Sensor variables
float sensorValue;
int sensorCounter = 0;                        // Counter variable for selecting sensor
int totalAmountOfSensors = 9;                 // CHANGE DEPENDING ON AMOUNT OF SENSORS
long sensorAverageArray[9];                   // Holds accumulated value of all the sensor values
long sensorRuntimeCounter = 0;                // Averages of all the sensors through the minute

// Interrupt variables
unsigned int oneMinuteDelayCounter = 0;       // 60,000 milliseconds in a minute, resets once data has been saved
unsigned int twoSecondCounter = 0;            // Used for measuring the temperature and humidity sensor every two seconds
bool oneMinute = false;                       // Turns true once a minute has passed, resets once data has been saved

// General purpose variables
bool clarity;                                 // Return variable check

// MQ2 Curves
float COCurve[3] = {2.3, 0.72, -0.34};        // From the graph, (x, y, slope) ...
                                              // point1 (log(200), log(5.25)) --> (2.3, 0.72) 
                                              // point2 (log(10000), log(0.257)) -->(4, 0.15)
                                              // Slope = (y2-y1/x2-x1) 
float SmokeCurve[3] = {2.3, 0.53, -0.44};
float PropaneCurve[3] = {2.3, 0.23, -0.46};

// MQ5 Curves
float LPGCurve[3] = {2.3, -0.15, -0.39};
float CH4Curve[3] = {2.3, -0.02, -0.39};
float H2Curve[3] = {2.3, 0.26, -0.25};
float AlcoholCurve[3] = {2.3, 0.56, -0.21};

// Resistances for the gas sensors
float RoMQ2 = 10;                             // 10 kOhms
float RoMQ5 = 10;

DHT dht(DHTPIN, DHTTYPE);                    // Initialize the temperature and humidity sensor

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

  dht.begin();
  
  // For debugging
  showFreeMemory();
} // END OF SETUP

SIGNAL(TIMER0_COMPA_vect) {
  // Increments once a millisecond
  oneMinuteDelayCounter++;
  twoSecondCounter++;
  if (oneMinuteDelayCounter == 60000) {
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
  // Limited to an 8.3 naming pattern when creating filenames on the SD card
  for (int i = 0; i < totalAmountOfSensors; i++) {
    switch (i) {
      case 0:
        directoryPath = String(directoryPath + "sen0.txt");         // Propane
        break;
      case 1:
        directoryPath = String(directoryPath + "sen1.txt");         // CO
        break;
      case 2:
        directoryPath = String(directoryPath + "sen2.txt");         // Smoke
        break;
      case 3:
        directoryPath = String(directoryPath + "sen3.txt");         // LPG
        break;
      case 4:
        directoryPath = String(directoryPath + "sen4.txt");         // CH4
        break;
      case 5:
        directoryPath = String(directoryPath + "sen5.txt");         // H2
        break;
      case 6:
        directoryPath = String(directoryPath + "sen6.txt");         // Alcohol
        break;
      case 7:
        directoryPath = String(directoryPath + "sen7.txt");         // Temperature
        break;
      case 8:
        directoryPath = String(directoryPath + "sen8.txt");         // Humidity
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
  if (currentSensor == PROPANE || currentSensor <= SMOKE) {              // Between #define values of 0 and 2 for MQ2 sensor readings
    rs = sensorResistanceCalculation(analogRead(MQ2));
    rs = rs / RoMQ2;
  }
  // MQ5 
  else if (currentSensor == LPG || currentSensor <= ALCOHOL) {           // Between #define values of 3 and 6 for MQ5 sensor readings
     rs = sensorResistanceCalculation(analogRead(MQ5)); 
     rs = rs / RoMQ5;
  }
  // Temperature 
  else if (currentSensor == TEMP && twoSecondCounter >= 2000 ) {         // The #define value of TEMP is 7
    // Read temperature as Fahrenheit (isFahrenheit = true)
    rs = dht.readTemperature(true);
  }
  // Humidity
  else if (currentSensor == HUMIDITY && twoSecondCounter >= 2000) {      // The #define value of HUMIDITY is 8
    rs = dht.readHumidity();
    // Reset the two second counter value
    twoSecondCounter = 0;
  }
  return rs;
}

/*
 * Passes the gas curves to getPercentages if MQ sensor otherwise
 * will go to different function to get the temperature or humidity
 */
long getSensorPercentages (float rsRoRatio, int gasID) {
  switch (gasID) {
    // MQ2
    case 0:
      return getPercentages(rsRoRatio, PropaneCurve);
      break;
    case 1:
      return getPercentages(rsRoRatio, COCurve);
      break;
    case 2:
      return getPercentages(rsRoRatio, SmokeCurve);
      break;
    // MQ5
    case 3:
      return getPercentages(rsRoRatio, LPGCurve);
      break;
    case 4:
      return getPercentages(rsRoRatio, CH4Curve);
      break;
    case 5:
      return getPercentages(rsRoRatio, H2Curve);
      break;
    case 6:
      return getPercentages(rsRoRatio, AlcoholCurve);
      break;
    // Temperature
    case 7:
      return rsRoRatio;       // Need to return value brought in since this will be temperature. Don't want to return 0!
      break;
    // Humidity
    case 8:
      return rsRoRatio;       // Need to return value brought in since this will be humidity
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
