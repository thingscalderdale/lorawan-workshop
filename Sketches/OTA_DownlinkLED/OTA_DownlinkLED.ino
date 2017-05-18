/**
   SmartEverything Lion LoRa development board

   Workshop example to demonstrate OTA Join, sending temperature
   readings and handling downlink to change the state of the onboard LED.

   Use TTN Console Downlink option 
   Turn LED off: Send 00 to port 1
   Turn LED on:  Send 01 to port 1

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

/* * INPUT DATA (for OTA) from TTN Console

    1) application EUI
    1) and application key

    then OTAA procedure can start.
*/

#define APP_EUI "xxxxxxxxxxxxxxxx"
#define APP_KEY "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

char c;
char buff[100]  = {};
int  len        = 0;
unsigned char i = 0;

unsigned char buff_size = 100;

// 30000 is about 5 minutes;
static long LOOP_PERIOD = 30000;
static long loop_cnt = LOOP_PERIOD - 300;

// Data pin is plugged into D4 on the Arduino/SmartEverything LION
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);


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
  int chrBufLen = strlen(hex) / 2;
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

  ledYellowTwoLight(LOW); // turn the LED off

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

  delay(1000);
  Serial.print("FW Version :");
  Serial.println(lora.sysGetVersion());

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

  Serial.println("Setting Trasmission Power to Max ...");
  lora.macPause();
  err = lora.radioSetPwr(14);
  if (err != RN_OK) {
    Serial.println("\nFailed Setting the power to max power");
  }
  lora.macResume();
}


void loop() {

  static int loop_cnt = 0;
  static int tx_cnt = 0;
  static bool joined = false;
  static uint32_t state;

  if (Serial.available()) {
    c = Serial.read();
    Serial.write(c);
    buff[i++] = c;
    if (c == '\n') {
      Serial.print(lora.sendRawCmdAndAnswer(buff));
      i = 0;
      memset(buff, 0, sizeof(buff));
    }
  }

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
      char *downPtr = strchr(portPtr, ' ');
      *downPtr++ = '\0';
      int portNum = atoi(portPtr);
      Serial.print("Port: ");
      Serial.println(portNum, DEC);
      Serial.print("Data: ");
      Serial.println(downPtr);

      // Convert hex string to char array
      char downBuf[32];
      hexToCharStr(downPtr, downBuf);

      // If receive port is 1 then change set LED status
      if( portNum == 1 ) 
        ledYellowTwoLight(downBuf[0] ? HIGH : LOW);
    }
  }

  if (!(loop_cnt % LOOP_PERIOD)) { //5 minutes
    state = lora.macGetStatus();

    // Check If network is still joined
    if (MAC_JOINED(state)) {
      if (!joined) {
        Serial.println("\nOTA Network JOINED! ");
        joined = true;
      }
      // Sending String to the Lora Module towards the gateway
      tx_cnt++;

      // call sensors.requestTemperatures() to issue a global temperature
      // request to all devices on the bus
      Serial.print("Requesting temperatures...");
      sensors.requestTemperatures(); // Send the command to get temperatures
      Serial.println("DONE");
      // After we got the temperatures, we can print them here.
      // We use the function ByIndex, and as an example get the temperature from the first sensor only.
      Serial.print("Temperature for the device 1 (index 0) is: ");
      float temperature = sensors.getTempCByIndex(0);
      Serial.println(temperature);
      const char tx_size = 8;
      char tx[tx_size];
      int decimal = (temperature - (int)temperature) * 100;
      sprintf(tx, "%d.%2d", (int)temperature, decimal);

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


