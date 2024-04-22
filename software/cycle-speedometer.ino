/*
A simple sketch for a cycle speedometer, which senses the rotation of
the wheel, and displays your current speed/distance.

Written by Adbhut Patil
DIY Labs 2024, youtube.com/@onelabtorulethemall, github.com/DIYLabsED
*/

#define WAIT_FOR_SERIAL // Comment out to disable waiting for serial port, used for debugging

// Include libraries used
#include <SPI.h>               //Dependency of microSD library
#include <RP2040_SD.h>         // Library for microSD card
#include <Adafruit_SSD1306.h>  // Adafruit's library for OLED display
#include <RTClib.h>            // Adafruit's library for RTCs
#include <Adafruit_NeoPixel.h> // Adafruit's library for NeoPixels


// Pin definitions
const int PIN_SD_MISO = 0;  // SPI pins for microSD card
const int PIN_SD_MOSI = 3;
const int PIN_SD_SCK = 2;
const int PIN_SD_CS = 1;

const int PIN_RTC_SCL = 13; // I2C pins for DS3231 RTC
const int PIN_RTC_SDA = 12;

const int PIN_OLED_SCL = 27;  // I2C pins for SSD1306 OLED display
const int PIN_OLED_SDA = 26;
const int OLED_WIDTH = 128;   // OLED specs
const int OLED_HEIGHT = 64;
const int OLED_ADDR = 0x3C;
const int OLED_RESET = -1;

const int PIN_SENSOR_IN = 28;  // Digital input from 

const int PIN_BUILTIN_RGB = 16; // Pin connected to WS2812B LED


// Variables used for display data
bool noDataLogging = false;

uint16_t year;
uint8_t month;
uint8_t day;
uint8_t hour;
uint8_t minute;

float speed;
float currentRideDistance; // Distance ridden since device has been powered on
float totalDistance;       // Total distance this device has been recording, saved to EEPROM

int page = 1;
const int numPages = 3;

// Cross bitmap
static const unsigned char PROGMEM bitmapCross[] = 
{

  0x81,
  0x42,
  0x24,
  0x18,
  0x18,
  0x24,
  0x42,
  0x81

};
const int bitmapCrossWidth = 8;
const int bitmapCrossHeight = 8;

// Check mark bitmap
static const unsigned char PROGMEM bitmapCheck[] = 
{

  0x00,
  0x01,
  0x02,
  0x04,
  0x88,
  0x50,
  0x20,
  0x00

};
const int bitmapCheckWidth = 8;
const int bitmapCheckHeight = 8;


// Objects used
Sd2Card card; 
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire1, -1);
Adafruit_NeoPixel pixel(1, PIN_BUILTIN_RGB, NEO_RGB + NEO_KHZ800);
RTC_DS3231 rtc;



void setup(){

  Serial.begin();

  #ifdef WAIT_FOR_SERIAL
    while(!Serial);  // Waits until serial port has connected
    Serial.println("Sketch started, compiled for: " + String(BOARD_NAME) + " at: " + __DATE__ + " | " + __TIME__);
  #endif


  setCommunicationPins();
  initNeoPixel();
  initOLED();
  initRTC();
  initMicroSD();

  finishSetup();

}

void loop(){

  updateTime();

  displayInfo();

}


void setCommunicationPins(){

  // Set SPI0 pins, used for microSD card
  SPI.setCS(1);
  SPI.setSCK(2);
  SPI.setRX(0);
  SPI.setTX(3);

  // Set I2C0 pins, used for RTC
  Wire.setSCL(PIN_RTC_SCL);
  Wire.setSDA(PIN_RTC_SDA);

  // Set I2C1 pins, used for OLED
  Wire1.setSCL(PIN_OLED_SCL);
  Wire1.setSDA(PIN_OLED_SDA);  

}

void initNeoPixel(){

  Serial.println("initializing NeoPixel");
  
  pixel.begin();
  pixel.setPixelColor(0, pixel.Color(20, 0, 0));  // After init, sets color to red, color is set to green if everything inits 
  pixel.show();  

}

void initOLED(){

  Serial.println("initializing OLED");

  if(!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)){

    Serial.println("oled init fail");
    while(true);
  
  }

  // After initializing, display some text on oled
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.setTextSize(3);
  oled.setTextColor(SSD1306_WHITE);
  oled.println("DIYLabs");
  oled.setTextSize(2);
  oled.setCursor(0, 32);
  oled.println("booting...");
  oled.display();

}

void initRTC(){

  Serial.println("initializing RTC");

  // If RTC doesnt't initialize
  if(!rtc.begin()){
    
    Serial.println("rtc init fail");

    oled.setCursor(0, 0);
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.println("RTC not detected!");
    oled.display();

    while(true);

  }

  // If RTC lost battery power, promt user to calibrate RTC
  if(rtc.lostPower()){

    Serial.println("rtc lost power, needs calibrating");

    oled.setCursor(0, 0);
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.println("RTC lost power!\nCalibrate it!");
    oled.display();

    while(true);

  }

}

void initMicroSD(){

  Serial.println("initializing microSD");

  // If card does not initialize, ask user if they want to proceed without datalogging
  if(!card.init(SPI_HALF_SPEED, 1)){
  
    Serial.println("microsd init failed");

    oled.setCursor(0, 0);
    oled.clearDisplay();
    oled.setTextSize(1);              
    oled.println("Card not detected!\n\nPress BOOTSEL to\nproceed\n(without datalogging)");
    oled.display();

    while(!BOOTSEL); // Waits until BOOTSEL button

    noDataLogging = true;

  } 

}

void finishSetup(){

  if(noDataLogging){
    pixel.setPixelColor(0, pixel.Color(0, 0, 20));
  }

  else{
    pixel.setPixelColor(0, pixel.Color(0, 20, 0));
  }

  pixel.show();
  Serial.println("init complete!");

}

void displayInfo(){

  if(BOOTSEL){

    page++;
    page = page % numPages;

    while(BOOTSEL); // Wait until button has been released

  }

  oled.clearDisplay();

  switch(page){

    case 0:
      page0();
      break;
    
    case 1:
      page1();
      break;

    case 2:
      page2();
      break;

    default:
      error();
      break;

  }

  oled.display();

}

void updateTime(){

  DateTime time = rtc.now();

  year   = time.year();
  month = time.month();
  day    = time.day();
  hour   = time.hour();
  minute = time.minute();

}

void error(){

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("Error detected\nTry rebooting");
  oled.display();

}

void page0(){

  // Left intentionally blank, in case you want the display turned off

}

void page1(){
  // If datalogging is disabled, draw a little cross, else, a little check mark
  if(noDataLogging){
    oled.drawBitmap(0, 56, bitmapCross, bitmapCrossWidth, bitmapCrossHeight, SSD1306_WHITE);
  }

  else{
    oled.drawBitmap(0, 56, bitmapCheck, bitmapCheckWidth, bitmapCheckHeight, SSD1306_WHITE);
  }    

  // Draw the current date and time
  oled.setCursor(18, 56);
  oled.setTextSize(1);

  oled.print(hour);
  oled.print(":");
  oled.print(minute);
  oled.print("  ");
  oled.print(year);
  oled.print("/");
  oled.print(month);
  oled.print("/");
  oled.print(day);

  // Draw speed and distance
  oled.setTextSize(3);
  oled.setCursor(0, 0);
  oled.print(speed);

  oled.setTextSize(2);
  oled.print(" m/s");

  oled.setTextSize(3);
  oled.setCursor(0, 25);  
  oled.print(currentRideDistance);
  
  oled.setTextSize(2);
  oled.println(" km");  

}

void page2(){

  oled.setCursor(0, 0);
  oled.setTextSize(2);
  oled.print(totalDistance);
  oled.print(" km");
  oled.setTextSize(1);
  oled.setCursor(0, 32);
  oled.print("Total distance\ntravelled with this\ndevice active");
  
  oled.display();

}