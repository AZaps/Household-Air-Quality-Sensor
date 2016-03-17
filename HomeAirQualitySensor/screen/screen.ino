#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
#include <Fonts/FreeSerif12pt7b.h>  // Include new fonts
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSerifBold24pt7b.h>
#include <Fonts/FreeSerif24pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>


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

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 23   // can be a digital pin
#define XP 22  // can be a digital pin

#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

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
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);


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

  tft.setFont(&FreeSerif24pt7b);  // Set font to Serif 24pt
  tft.setRotation(1);             // Rotate text on screen
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);  
  tft.setCursor(170,40);          // Set cursor location
  tft.println("Home");            // Print string
  tft.setCursor(190,100);
  tft.println("Air");
  tft.setCursor(155,160);
  tft.println("Quality");
  tft.setCursor(160,220);
  tft.println("Sensor");
  tft.setCursor(155,280);
  tft.println("'HAQS'");
  delay(1000);
  tft.setTextColor(BLACK);        // Set text to black and write over previous text strings
  tft.setCursor(170,40);
  tft.println("Home");
  tft.setCursor(190,100);
  tft.println("Air");
  tft.setCursor(155,160);
  tft.println("Quality");
  tft.setCursor(160,220);
  tft.println("Sensor");
  tft.setCursor(155,280);
  tft.println("'HAQS'");

   
  dataLayout();
  tft.setFont(&FreeSansBold24pt7b);
  
  tft.setCursor(180,300);
  tft.println("Time");
  
  tft.setFont();  // Disables previous font setting
  tft.fillRoundRect(10, 240, 150, 70, 20, BLUE);       // Print boxes for buttons
  tft.fillTriangle(85, 300, 48, 250, 121, 250, 0xF800); //
  
  tft.fillRoundRect(320, 240, 150, 70, 20, BLUE);    //
  tft.fillTriangle(395, 250, 358, 300, 431, 300, RED);  //
}


#define MINPRESSURE 1
#define MAXPRESSURE 1000

int screenCount = 1;

void loop(void) {
  if (ran){

    TSPoint p = ts.getPoint();
    
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);

    
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    
    p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
    p.y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);

    if (p.x >= 350 && p.x <= 480){
   
      if (p.y <100){
            screenCount++;
            
            if (screenCount==10){

             screenCount=1;
           }
            tft.fillRect(0, 45, 480, 190, BLACK);
            dataLayout();
            tft.setFont(&FreeSansBold24pt7b);

      }

    
     else if (p.y > 220){
            screenCount--;
            if (screenCount==0){
             
              screenCount=9;
              
              }
            tft.fillRect(0, 45, 480, 190, BLACK);
            dataLayout();
            tft.setFont(&FreeSansBold24pt7b);


    }
    }
    
  }
 
  }
}


void dataLayout(){  //Prints data in top right & left corners
  tft.setFont(&FreeSerif24pt7b);
  tft.setCursor(5,40);
  tft.setTextColor(WHITE);   
  tft.println("76 F");
  tft.setCursor(390,40);
  tft.println("80%");  

  if (screenCount==1){

    tft.setCursor(220,150);
    tft.println("GAS1");
    
    tft.setCursor(50,230);
    tft.println("GAS9");

    tft.setCursor(350,230);
    tft.println("GAS2");   
     
  }

  else if (screenCount==2){
    tft.setCursor(220,150);
    tft.println("GAS2");
    
    tft.setCursor(50,230);
    tft.println("GAS1");

    tft.setCursor(350,230);
    tft.println("GAS3");
      
  }

   else if (screenCount==3){
    tft.setCursor(220,150);
    tft.println("GAS3");
    
    tft.setCursor(50,230);
    tft.println("GAS2");

    tft.setCursor(350,230);
    tft.println("GAS4");
      
  }

   else if (screenCount==4){
    tft.setCursor(220,150);
    tft.println("GAS4");
    
    tft.setCursor(50,230);
    tft.println("GAS3");

    tft.setCursor(350,230);
    tft.println("GAS5");
      
  }

   else if (screenCount==5){
    tft.setCursor(220,150);
    tft.println("GAS5");
    
    tft.setCursor(50,230);
    tft.println("GAS4");

    tft.setCursor(350,230);
    tft.println("GAS6");
      
  }

   else if (screenCount==6){
    tft.setCursor(220,150);
    tft.println("GAS6");
    
    tft.setCursor(50,230);
    tft.println("GAS5");

    tft.setCursor(350,230);
    tft.println("GAS7");
      
  }

   else if (screenCount==7){
    tft.setCursor(220,150);
    tft.println("GAS7");
    
    tft.setCursor(50,230);
    tft.println("GAS6");

    tft.setCursor(350,230);
    tft.println("GAS8");
      
  }

   else if (screenCount==8){
    tft.setCursor(220,150);
    tft.println("GAS8");
    
    tft.setCursor(50,230);
    tft.println("GAS7");

    tft.setCursor(350,230);
    tft.println("GAS9");
      
  }

   else if (screenCount==9){
    tft.setCursor(220,150);
    tft.println("GAS9");
    
    tft.setCursor(50,230);
    tft.println("GAS8");

    tft.setCursor(350,230);
    tft.println("GAS1");
      
  }


}



