/*
    SmeGPS Library - Localization Information

    This example shows how to retrieve the GPS localization information:
    Latitude, Longitude,  Altitude

    It shows also the number of the satellites  are visible by the GPS receiver
    This information is useful to well understand the accuracy of the localization information

    created 2 August 2016
    include additional UTC and move methods  provided by Gabriel de Fombelle - line-up.io - gabriel.de.fombelle@gmail.com http://www.line-up.io
    This example is in the public domain
    https://github.com/gdefombelle

    original example created 27 May 2015 by Seve (seve@axelelettronica.it)

    This example is in the public domain
    https://github.com/ameltech

    SE868  more information available here:
    http://www.telit.com/products/product-service-selector/product-service-selector/show/product/jupiter-se868-as/
*/

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

#include <Wire.h>

#include <sl868a.h>
#include <Arduino.h>

// For the breakout, you can use any 2 or 3 pins
// These pins will also work for the 1.8" TFT shield
#define TFT_CS     10
#define TFT_RST    9  // you can also connect this to the Arduino reset
// in which case, set this #define pin to 0!
#define TFT_DC     8


// Application defines
// Display info
#define LCD_SIZE_X 160
#define LCD_SIZE_Y 128

// text formatting
#define TEXT_UNFORMAT 0
#define TEXT_CENTRED 1
#define TEXT_LEFT  2
#define TEXT_RIGHT 3


// Option 1 (recommended): must use the hardware SPI pins
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);


int led_status = HIGH;

// TFT helper functions

void setScreen( char *title ) {
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
  formatText(0, 0, 2, TEXT_CENTRED, title );
}


void formatText( int x, int y, int textSize, int format, char *text ) {
  // format text on display
  tft.setTextSize( textSize);
  int pixelsChar = 6;
  switch ( textSize ) {
    case 1:
      pixelsChar = 6;
      break;
    case 2:
      pixelsChar = 12;
      break;
    case 3:
      pixelsChar = 15;
      break;
    case 4:
      pixelsChar = 18;
      break;
  }

  switch ( format ) {
    case TEXT_CENTRED:    // Centered
      x = (tft.width() / 2 - ((strlen(text) * pixelsChar) / 2));
      break;
    case TEXT_LEFT:      // Left justified
      x = 0;
      break;
    case TEXT_RIGHT:     // Right Justified
      x = tft.width() - (strlen(text) * pixelsChar);
      break;
    default:             // None
      break;
  }

  tft.setCursor(x, y);
  tft.println( text );
}


// the setup function runs once when you press reset or power the board
void setup() {
  //  ledGreenLight(HIGH);
  //  ledRedLight(HIGH);
  //  ledGreenLight(HIGH);
  SerialUSB.begin(115200);
  smeGps.begin();
  // Use this initializer if you're using a 1.8" TFT
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab

  // Use this initializer (uncomment) if you're using a 1.44" TFT
  //tft.initR(INITR_144GREENTAB);   // initialize a ST7735S chip, black tab

  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(3);

  setScreen((char*)"GPS Location");

  tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0, 16 );

  if ( !isOnBattery() )
    tft.print("Not ");
  tft.println("On Battery");
}



// the loop function runs over and over again forever
void loop() {
  static unsigned loop_cnt = 0;
  unsigned int altitude = 0;
  unsigned char lockedSatellites = 0;
  double latitude = 0;
  double longitude = 0;
  // UTC, speed and course here below
  unsigned int utc_hour = 0;
  unsigned int utc_min = 0;
  unsigned int utc_sec = 0;
  unsigned char utc_secDecimals = 0;
  unsigned char utc_year = 0;
  unsigned char utc_month = 0;
  unsigned char utc_dayOfMonth = 0;
  double speed_knots = 0;
  double course = 0;
  sl868aCachedDataT data;
  delay(5);

  if (smeGps.ready()) {
#ifdef ARDUINO_SAMD_SMARTEVERYTHING
    ledGreenLight(HIGH);
#endif
    tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
    tft.setTextSize(1);
    tft.setCursor(0, 16 );
    tft.println("GPS Locked          ");

    altitude   = smeGps.getAltitude();
    latitude   = smeGps.getLatitude();
    longitude  = smeGps.getLongitude();
    lockedSatellites = smeGps.getLockedSatellites();
    // additional new getters
    utc_hour = smeGps.getUtcHour();
    utc_min = smeGps.getUtcMinute();
    utc_sec = smeGps.getUtcSecond();
    utc_secDecimals = smeGps.getUtcSecondDecimals();
    utc_year = smeGps.getUtcYear();
    utc_month = smeGps.getUtcMonth();
    utc_dayOfMonth = smeGps.getUtcDayOfMonth();
    speed_knots = smeGps.getSpeedKnots();
    course = smeGps.getCourse();
    // get raw cached data as a sl868aCachedDataT struct
    data = smeGps.getData();

    if ((loop_cnt % 200) == 0) {
      tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
      tft.print("Latitude   = ");
      tft.println(latitude, 6);
      tft.print("Longitude  = ");
      tft.println(longitude, 6);
      tft.print("Altitude   = ");
      tft.print(altitude, DEC);
      tft.println("m");
      tft.print("Satellites = ");
      tft.println(lockedSatellites, DEC);
      //print time
      tft.print("UTC Time   = ");
      if( data.utc_hour < 10 ) tft.print("0");
      tft.print(data.utc_hour, DEC);
      tft.print(":");
      if( data.utc_min < 10 ) tft.print("0");
      tft.print(data.utc_min, DEC);
      tft.print(":");
      if( data.utc_sec < 10 ) tft.print("0");
      tft.print(data.utc_sec, DEC);
      tft.print(".");
      tft.print(data.utc_sec_decimals, DEC);
      tft.println(" ");
      //print date
      tft.print("Date       = ");
      tft.print(data.utc_year, DEC);
      tft.print("/");
      tft.print(data.utc_month, DEC);
      tft.print("/");
      tft.println(data.utc_dayOfMonth, DEC);
      //tft.println();

      tft.print("Speed (knots) = ");
      tft.println((int)speed_knots, DEC);
      tft.print("Course = ");
      tft.println((int)course, DEC);

      SerialUSB.println(" ");
      SerialUSB.print("Latitude    = ");
      SerialUSB.println(latitude, 6);
      SerialUSB.print("Longitude   = ");
      SerialUSB.println(longitude, 6);
      SerialUSB.print("Altitude    = ");
      SerialUSB.print(altitude, DEC);
      SerialUSB.println("m");
      SerialUSB.print("Satellites  = ");
      SerialUSB.println(lockedSatellites, DEC);
      SerialUSB.print("Time (UTC)  = ");
      if( data.utc_hour < 10 ) SerialUSB.print("0");
      SerialUSB.print(data.utc_hour, DEC);
      SerialUSB.print(":");
      if( data.utc_min < 10 ) SerialUSB.print("0");
      SerialUSB.print(data.utc_min, DEC);
      SerialUSB.print(":");
      if( data.utc_sec < 10 ) SerialUSB.print("0");
      SerialUSB.print(data.utc_sec, DEC);
      SerialUSB.print(".");
      SerialUSB.print(data.utc_sec_decimals, DEC);
      SerialUSB.println("   ");
      //print date & time
      SerialUSB.print("Date  =  ");
      SerialUSB.print(data.utc_year, DEC);
      SerialUSB.print("/");
      SerialUSB.print(data.utc_month, DEC);
      SerialUSB.print("/");
      SerialUSB.print(data.utc_dayOfMonth, DEC);
      SerialUSB.println();

      SerialUSB.print("Speed (knots) = ");
      SerialUSB.println(speed_knots, DEC);
      SerialUSB.print("Course = ");
      SerialUSB.println(course, DEC);

    }
  } else {
    if ((loop_cnt % 200) == 0) {
      SerialUSB.println("Locking GPS position...");
      if (led_status == LOW) {
        led_status = HIGH;
      } else {
        led_status = LOW;
      }
      tft.setTextColor(led_status == HIGH ? ST7735_RED : ST7735_YELLOW, ST7735_BLACK);
      tft.setTextSize(1);
      tft.setCursor(0, 26 );
      tft.print("Waiting for GPS Lock");
    }
  }

  loop_cnt++;
#ifdef ARDUINO_SAMD_SMARTEVERYTHING
  ledGreenLight(led_status);
#endif
}
