/**
   SmartEverything Lion LoRa development board

   Workshop example to demonstrate OTA Join and sending temperature
   readings.

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

void setup() {
  errE err;
  loraDbg = false;         // Set to 'true' to enable RN2483 TX/RX traces
  bool storeConfig = true; // Set to 'false' if persistend config already in place

  ledYellowOneLight(LOW); // turn the LED off
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

  delay(5000);
  ledYellowTwoLight(LOW); // turn the LED off
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

  if (lora.available()) {
    //Unexpected data received from Lora Module;
    Serial.print("\nRx> ");
    Serial.print(lora.read());
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
      ledYellowTwoLight(HIGH); // turn the LED on
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
      sprintf(tx, "%d.%02d", (int)temperature, decimal);

      Serial.println(tx);
      lora.macTxCmd(tx, strlen(tx), 1, TX_NOACK);         // Unconfirmed tx String

      ledYellowTwoLight(LOW); // turn the LED off
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


