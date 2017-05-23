/**
   SmartEverything Lion LoRa development board

   Workshop example to demonstrate OTA Join. sending data and
   displaying the downlink message.
   To convert ASCII to hex for TTN Downling use https://www.branah.com/ascii-converter

   Use Port 1 for the message and Port 2 to change state of LED.

   As the shield blocks the headers, the temperature sensor has been replaced
   with a counter. The temperature sensor can be added by soldering
   it to the appropriate breakout pads on the TFT shield.

   Based on library example sendDataOTA_console

   Modified by Andrew Lindsay, Thing Innovations, May 2017.

   Original header below:

     SmarteEveryting Lion RN2483 Library - sendDataOTA_console

     This example shows how to configure and send messages after a OTA Join.
     Please consider the storeConfiguration example can be uset to store
     the required keys and skip the configuration part in curent example.

     created 25 Feb 2017
     by Seve (seve@ioteam.it)

     This example is in the public domain
     https://github.com/axelelettronica/sme-rn2483-library

     More information on RN2483 available here:
     http://www.microchip.com/wwwproducts/en/RN2483

 **/

#include <Arduino.h>
#include <rn2483.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

/* * INPUT DATA (for OTA)

    1) device EUI,
    2) application EUI
    3) and application key

    then OTAA procedure can start.
*/

#define APP_EUI "xxxxxxxxxxxxxxxx"
#define APP_KEY "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

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

char c;
char buff[100]  = {};
int  len        = 0;
unsigned char i = 0;

unsigned char buff_size = 100;

// 30000 is about 5 minutes;
static long LOOP_PERIOD = 3000;
static long loop_cnt = LOOP_PERIOD - 300;

// Data pin is plugged into D4 on the Arduino/SmartEverything LION
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

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

/** toDec - Simple 2 character hex digits to 8 bit binary function
   @param inBuf - Pointer to first hex character.
   @return Binary representation fo the 2 hext digits.
*/
uint8_t toDec( char *inBuf )
{
  char *buf = inBuf;
  uint8_t value = 0;
  if ( *buf >= '0' && *buf <= '9' )
    value = *buf - '0';
  else {
    value = (*buf & 0xDF) - 'A' + 10;
  }
  buf++;
  value = value << 4;
  if ( *buf >= '0' && *buf <= '9' )
    value += *buf - '0';
  else {
    value += (*buf & 0xDF) - 'A' + 10;
  }
  return value;
}

void hexToCharStr( char *hex, char *chrBuf ) {
  char *respBuf = chrBuf;

  // Convert to integers
  char *bufPtr = hex;
  // Calc buff len, need to take off 2 for line endings (a bit of a hack!)
  int chrBufLen = min((strlen(hex) - 2) / 2, 63);
  Serial.print("hex len ");
  Serial.println(strlen(hex), DEC);
  Serial.print("hexToCharStr ");
  Serial.println(chrBufLen, DEC);

  for ( int i = 0; i < chrBufLen; i++ ) {
    respBuf[i] = toDec(bufPtr);
    bufPtr += 2;
  }
  respBuf[chrBufLen] = '\0';
}

void setup() {
  errE err;
  loraDbg = false;         // Set to 'true' to enable RN2483 TX/RX traces
  bool storeConfig = true; // Set to 'false' if persistend config already in place

  ledYellowTwoLight(HIGH); // turn the LED on
  delay(500);
  Serial.begin(115200);

  lora.begin();
  // Remove this part if you are running on batteries as it will
  // hang here and not progress any further.
  // Waiting for the USB serial connection
//  while (!Serial) {
//    ;
//  }

  // Initialise the OneWire sensor
  sensors.begin();

  //  delay(1000);
  //  Serial.print("FW Version :");
  //  Serial.println(lora.sysGetVersion());

  // Use this initializer if you're using a 1.8" TFT
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab

  // Use this initializer (uncomment) if you're using a 1.44" TFT
  //tft.initR(INITR_144GREENTAB);   // initialize a ST7735S chip, black tab

  tft.fillScreen(ST7735_BLACK);
  // tft.setRotation(3);

  setScreen((char*)"Downlink");

  tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0, 16 );

  tft.println(lora.sysGetVersion());

  if ( !isOnBattery() )
    tft.print("Not ");
  tft.println("On Battery");

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

    Serial.println("Setting DR ...");
    err = lora.macSetDataRate(0);
    if (err != RN_OK) {
      Serial.println("\nFailed setting DR");
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

  Serial.println("Setting Trasmission Power to Max ...");
  lora.macPause();
  err = lora.radioSetPwr(14);
  if (err != RN_OK) {
    Serial.println("\nFailed Setting the power to max power");
  }
  lora.macResume();

  delay(5000);
  ledYellowTwoLight(LOW); // turn the LED off
}


void loop() {

  static int loop_cnt = 0;
  static int tx_cnt = 0;
  static bool joined = false;
  static uint32_t state;

  // Downlink messages are received after sending data
  // They start mac_rx port hex data
  // Example mac_rx 2 3031322E64
  if (lora.available()) {
    //Unexpected data received from Lora Module;
    Serial.print("\nRx> ");
    const char *rBuf = lora.read();
    Serial.print(rBuf);
    if ( strncmp(rBuf, "mac_rx", 6) == 0 ) {
      // Downlink message detected
      Serial.println("Downlink");
      char *portPtr = (char*)rBuf;
      portPtr += 7;
      Serial.println(portPtr);
      char *downPtr = strchr(portPtr, ' ');
      *downPtr++ = '\0';
      int portNum = atoi(portPtr);
      Serial.print("Port: ");
      Serial.println(portNum, DEC);
      Serial.print("Data: ");
      Serial.println(downPtr);

      // Convert hex string to char array
      char downBuf[64];
      hexToCharStr(downPtr, downBuf);

      // If received on port is 2 then change set LED status
      if ( portNum == 2 )
        ledYellowTwoLight(downBuf[0] ? HIGH : LOW);

      // If received on port 1 then display message
      if ( portNum == 1 ) {
        // Display on LCD
        // Reset screen quickly
        tft.fillScreen(ST7735_BLACK);
        // tft.setRotation(3);

        setScreen((char*)"Downlink");

        tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
        tft.setTextSize(1);
        tft.setCursor(0, 16 );
        // Raw hex
        tft.println("Hex Data: ");
        tft.println(downPtr);
        // Ascii message
        tft.println("ASCII Msg: ");
        tft.println(downBuf);
      }
    }
  }

  if (!(loop_cnt % LOOP_PERIOD)) { //5 minutes
    state = lora.macGetStatus();
    //Serial.println(state, HEX);

    // Check If network is still joined
    if (MAC_JOINED(state)) {
      if (!joined) {
        Serial.println("\nOTA Network JOINED! ");
        joined = true;
      }
      // Sending String to the Lora Module towards the gateway
      tx_cnt++;
      if ( tx_cnt > 999 )
        tx_cnt = 0;

      // call sensors.requestTemperatures() to issue a global temperature
      // request to all devices on the bus
      //      Serial.print("Requesting temperatures...");
      //      sensors.requestTemperatures(); // Send the command to get temperatures
      //      Serial.println("DONE");
      // After we got the temperatures, we can print them here.
      // We use the function ByIndex, and as an example get the temperature from the first sensor only.
      //      Serial.print("Temperature for the device 1 (index 0) is: ");
      //      float temperature = sensors.getTempCByIndex(0);
      //      Serial.println(temperature);
      const char tx_size = 8;
      char tx[tx_size];
      //      int decimal = (temperature - (int)temperature) * 100;
      //      sprintf(tx, "%d.%2d", (int)temperature, decimal);
      sprintf(tx, "%d", tx_cnt);

      Serial.println(tx);
      lora.macTxCmd(tx, strlen(tx), 1, TX_NOACK);         // Unconfirmed tx String

    } else {
      joined = false;
      lora.macJoinCmd(OTAA);
      delay(100);
    }
    loop_cnt = 0;
  }
  loop_cnt++;
  delay(10);
}


