/*
   SmartEverything Lion LoRa development board

   Workshop example to demonstrate OTA Join, reading GPS and sending data.

   As the shield blocks the headers, the temperature sensor has been replaced
   with a counter. The temperature sensor can be added by soldering
   it to the appropriate breakout pads on the TFT shield.

   Based on library example sendDataOTA_console

   Modified by Andrew Lindsay, Thing Innovations, May 2017.

   Original header below:

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
#include <Arduino.h>
#include <rn2483.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <sl868a.h>


/* * INPUT DATA (for OTA)

    1) device EUI,
    2) application EUI
    3) and application key

    then OTAA procedure can start.
*/

#define APP_EUI "70B3D57EF0004C61"
#define APP_KEY "C16AFE1239E1276CC00AEC701CAF106A"

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

//char c;
//char buff[100]  = {};
//int  len        = 0;
//unsigned char i = 0;

//unsigned char buff_size = 100;

// 30000 is about 5 minutes;
static long LOOP_PERIOD = 3000;
static long loop_cnt = LOOP_PERIOD - 300;

int led_status = HIGH;

typedef struct {
  float latitude;
  float longitude;
  uint16_t altitude;
  uint8_t  n_satellites;
  uint8_t utc_hour;
  uint8_t utc_min;
  uint8_t utc_sec;
  uint16_t utc_year;
  uint8_t utc_month;
  uint8_t utc_dayOfMonth;
} payloadDataStruct;

payloadDataStruct payloadData;
char *payloadPtr;

// TFT helper functions
/** setScreen - Initialise the screen for displaying a new page
    @param title - The screen title
*/
void setScreen( char *title ) {
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
  formatText(0, 0, 2, TEXT_CENTRED, title );
}

/** formatText - Basic formatted text output
    @param x - X pixel position, starts from 0 on left hand side
    @param y - Y pixel position, starts from 0 at top
    @param textSize - 1-4 with 1 being smallest, 4 being largest
    @param format - None, Centered, Left or Right justified. See defines for values
    @param text - Char pointer to text to display. Must be 0 terminated
*/
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
  errE err;
  loraDbg = false;         // Set to 'true' to enable RN2483 TX/RX traces
  bool storeConfig = true; // Set to 'false' if persistend config already in place

  resetBaseComponent();
  SerialUSB.begin(115200);
  smeGps.begin();
  gpsWakeup();

  lora.begin();
  // Waiting for the USB serial connection
  while (!Serial) {
    ;
  }

  // Use this initializer if you're using a 1.8" TFT
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab

  // Use this initializer (uncomment) if you're using a 1.44" TFT
  //tft.initR(INITR_144GREENTAB);   // initialize a ST7735S chip, black tab

  tft.fillScreen(ST7735_BLACK);
  // tft.setRotation(3);

  setScreen((char*)"Locator");

  tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0, 16 );

  /* NOTICE: Current Keys configuration can be skipped if already stored
              with the store config Example
  */
  if (storeConfig) {
    // Write HwEUI
    Serial.println("Writing Hw EUI in DEV EUI ...");
    lora.macSetDevEUICmd(lora.sysGetHwEUI());

    if (err != RN_OK) {
      Serial.println("\nFailed writing Dev EUI");
    }

    Serial.println("Writing APP EUI ...");
    err = lora.macSetAppEUICmd( APP_EUI );
    if (err != RN_OK) {
      Serial.println("\nFailed writing APP EUI");
    }

    Serial.println("Writing Application Key ...");
    lora.macSetAppKeyCmd( APP_KEY );
    if (err != RN_OK) {
      Serial.println("\nFailed writing raw APP Key");
    }

    Serial.println("Writing Device Address ...");
    err = lora.macSetDevAddrCmd(lora.sendRawCmdAndAnswer("mac get devaddr"));
    if (err != RN_OK) {
      Serial.println("\nFailed writing Dev Address");
    }

    Serial.println("Setting ADR ON ...");
    err = lora.macSetAdrOn();
    if (err != RN_OK) {
      Serial.println("\nFailed setting ADR");
    }
  }
  /* NOTICE End: Key Configuration */

  Serial.println("Setting Automatic Reply ON ...");
  err = lora.macSetArOn();
  if (err != RN_OK) {
    Serial.println("\nFailed setting automatic reply");
  }

  Serial.println("Setting Transmission Power to Max ...");
  lora.macPause();
  err = lora.radioSetPwr(14);
  if (err != RN_OK) {
    Serial.println("\nFailed Setting the power to max power");
  }
  lora.macResume();
}


// the loop function runs over and over again forever
void loop() {
  static int tx_cnt = 0;
  static bool joined = false;
  static uint32_t state;
  static unsigned loop_cnt = 0;
  double latitude = 0;
  double longitude = 0;
  sl868aCachedDataT data;
  delay(5);

  if (smeGps.ready()) {
#ifdef ARDUINO_SAMD_SMARTEVERYTHING
    ledGreenLight(HIGH);
#endif

    char* rx = smeGps.getRx();
    SerialUSB.println(rx);
    
    tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
    tft.setTextSize(1);
    //    tft.setCursor(0, 26 );
    //    tft.print("                    ");
    tft.setCursor(0, 16 );
    tft.println("GPS Locked          ");
    //    altitude   = smeGps.getAltitude();
    latitude   = smeGps.getLatitude();
    longitude  = smeGps.getLongitude();
    // get raw cached data as a sl868aCachedDataT struct
    data = smeGps.getData();

    if ((loop_cnt % 200) == 0) {
      tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
      tft.print("Lat  = ");
      tft.println(latitude, 6);
      tft.print("Lon  = ");
      tft.println(longitude, 6);
      tft.print("Alt  = ");
      tft.print(data.altitude, DEC);
      //      tft.print(altitude, DEC);
      tft.println("m");
      tft.print("Sats = ");
      tft.println(data.n_satellites, DEC);
      //      tft.println(lockedSatellites, DEC);
      //print time
      tft.print("UTC  = ");
      if ( data.utc_hour < 10 ) tft.print("0");
      tft.print(data.utc_hour, DEC);
      tft.print(":");
      if ( data.utc_min < 10 ) tft.print("0");
      tft.print(data.utc_min, DEC);
      tft.print(":");
      if ( data.utc_sec < 10 ) tft.print("0");
      tft.print(data.utc_sec, DEC);
      tft.print(".");
      tft.print(data.utc_sec_decimals, DEC);
      tft.println(" ");
      //print date
      tft.print("Date = ");
      tft.print(data.utc_year, DEC);
      tft.print("/");
      tft.print(data.utc_month, DEC);
      tft.print("/");
      tft.println(data.utc_dayOfMonth, DEC);
      //tft.println();

      tft.print("Speed (knots) = ");
      tft.println((int)data.speed_knots, DEC);
      tft.print("Course = ");
      tft.println((int)data.course, DEC);

      SerialUSB.println(" ");
      SerialUSB.print("Latitude    = ");
      SerialUSB.println(latitude, 6);
      SerialUSB.print("Longitude   = ");
      SerialUSB.println(longitude, 6);
      SerialUSB.print("Altitude    = ");
      SerialUSB.print(data.altitude, DEC);
      SerialUSB.println("m");
      SerialUSB.print("Satellites  = ");
      SerialUSB.println(data.n_satellites, DEC);
      SerialUSB.print("Time (UTC)  = ");
      if ( data.utc_hour < 10 ) SerialUSB.print("0");
      SerialUSB.print(data.utc_hour, DEC);
      SerialUSB.print(":");
      if ( data.utc_min < 10 ) SerialUSB.print("0");
      SerialUSB.print(data.utc_min, DEC);
      SerialUSB.print(":");
      if ( data.utc_sec < 10 ) SerialUSB.print("0");
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
      SerialUSB.println(data.speed_knots, DEC);
      SerialUSB.print("Course = ");
      SerialUSB.println(data.course, DEC);

      // Build LoRa payload and send location
      state = lora.macGetStatus();

      // Check If network is still joined
      if (MAC_JOINED(state)) {
        if (!joined) {
          SerialUSB.println("\nOTA Network JOINED! ");
          joined = true;
        }
        // Sending String to the Lora Module towards the gateway

        payloadData.latitude = (float)latitude;
        payloadData.longitude = (float)longitude;
        payloadData.altitude = data.altitude;
        payloadData.n_satellites = data.n_satellites;
        payloadData.utc_hour = data.utc_hour;
        payloadData.utc_min = data.utc_min;
        payloadData.utc_sec = data.utc_sec;
        payloadData.utc_year = data.utc_year;
        payloadData.utc_month = data.utc_month;
        payloadData.utc_dayOfMonth = data.utc_dayOfMonth;

        payloadPtr = (char*)&payloadData;

        tx_cnt++;
        if ( tx_cnt > 999 )
          tx_cnt = 0;

        lora.macTxCmd(payloadPtr, sizeof(payloadData), 1, TX_NOACK);         // Unconfirmed tx String
      } else {
        joined = false;
        lora.macJoinCmd(OTAA);
        SerialUSB.println("Joining Network...");

        delay(100);
      }
    }
  } else {
    if ((loop_cnt % 500) == 0) {
      SerialUSB.println("Waiting for GPS Lock");
      if (led_status == LOW) {
        led_status = HIGH;
      } else {
        led_status = LOW;
      }
      tft.setTextColor(led_status == HIGH ? ST7735_RED : ST7735_YELLOW, ST7735_BLACK);
      tft.setTextSize(1);
      tft.setCursor(0, 16 );
      tft.print("Waiting for GPS Lock");
    }
  }

  loop_cnt++;

}

