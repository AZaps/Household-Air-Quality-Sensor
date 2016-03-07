#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library

#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

// When using the BREAKOUT BOARD only, use these 8 data lines to the LCD:
// For the Arduino Uno, Duemilanove, Diecimila, etc.:
//   D0 connects to digital pin 22 
//   D1 connects to digital pin 23
//   D2 connects to digital pin 24
//   D3 connects to digital pin 25
//   D4 connects to digital pin 26
//   D5 connects to digital pin 27
//   D6 connects to digital pin 28
//   D7 connects to digital pin 29

//16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

bool ran = true;

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);


void setup(void) {
 
  Serial.begin(9600);
  Serial.println(F("TFT LCD test"));
  pinMode(39, OUTPUT);

#ifdef USE_ADAFRUIT_SHIELD_PINOUT
  Serial.println(F("Using Adafruit 2.8\" TFT Arduino Shield Pinout"));
#else
  Serial.println(F("Using Adafruit 2.8\" TFT Breakout Board Pinout"));
#endif

  Serial.print("TFT size is "); Serial.print(tft.width()); Serial.print("x"); Serial.println(tft.height());

  tft.reset();

  uint16_t identifier = tft.readID();

  if(identifier == 0x9325) {
    Serial.println(F("Found ILI9325 LCD driver"));
  } else if(identifier == 0x9328) {
    Serial.println(F("Found ILI9328 LCD driver"));
  } else if(identifier == 0x7575) {
    Serial.println(F("Found HX8347G LCD driver"));
  } else if(identifier == 0x9341) {
    Serial.println(F("Found ILI9341 LCD driver"));
  } else if(identifier == 0x8357) {
    Serial.println(F("Found HX8357D LCD driver"));
  } else {
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    Serial.println(F("If using the Adafruit 2.8\" TFT Arduino shield, the line:"));
    Serial.println(F("  #define USE_ADAFRUIT_SHIELD_PINOUT"));
    Serial.println(F("should appear in the library header (Adafruit_TFT.h)."));
    Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    Serial.println(F("Also if using the breakout, double-check that all wiring"));
    Serial.println(F("matches the tutorial."));
    return;
  }


  tft.begin(identifier);

  Serial.println(F("Benchmark                Time (microseconds)"));


  Serial.println(F("Done!"));

  tft.setRotation(1);
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);    tft.setTextSize(5.5);
  tft.setCursor(180,0);
  tft.println("Home");
  tft.setCursor(200,60);
  tft.println("Air");
  tft.setCursor(135,120);
  tft.println("Quality");
  tft.setCursor(155,180);
  tft.println("Sensor");
  tft.setCursor(155,240);
  tft.println("'HAQS'");
  delay(5000);
  tft.setTextColor(BLACK);
  tft.setCursor(180,0);
  tft.println("Home");
  tft.setCursor(200,60);
  tft.println("Air");
  tft.setCursor(135,120);
  tft.println("Quality");
  tft.setCursor(155,180);
  tft.println("Sensor");
  tft.setCursor(155,240);
  tft.println("'HAQS'");
}


void loop(void) {
  if (ran){
    tft.setRotation(1);
   // welcomeScreen();
  //  delay(10000);
    dataLayout();
    tft.setTextSize(10);
    tft.setCursor(90,100);
    tft.println("10:43");
    tft.fillRoundRect(10, 240, 150, 70, 20, GREEN);
    tft.fillTriangle(85, 300, 48, 250, 121, 250, 0xF800);
    tft.fillRoundRect(320, 240, 150, 70, 20, MAGENTA);
    tft.fillTriangle(395, 250, 358, 300, 431, 300, RED);
  //  delay(10000);
    ran = false;
  }
}


void dataLayout(){
  //tft.fillScreen(BLACK);
  unsigned long start = micros();
  tft.setCursor(0,0);
  tft.setTextColor(WHITE);    tft.setTextSize(3);
  tft.println("Temperature");
  tft.setCursor(310,0);
  tft.println("Humidity");
//  tft.setCursor(0,210);
//  tft.println("Meas1");
//  tft.setCursor(310,210);
//  tft.println("Meas2");
  
}

void buttonLayout(){
  tft.fillRoundRect(100, 300, 10, 10, 1/8,BLUE);
}


