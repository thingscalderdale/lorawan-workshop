/*
 Simple serial pasthrough sketch to talk to either the
 LoRa RN2483 or BLE RN4871 module. Useful for testing the modules.
 Depending on option enabled, the correct baud rate is selected.

 Doesn't work correctly with the Arduino IDE Serial monitor, please use
 a terminal emulator, e.g. TeraTerm on Windows, Serial on OS X.
 
 Andrew Lindsay, May 2017. Thing Innovations @ThingInnovation

 */

// For RN4871 have this define uncommented
// For RN2483 comment this line.
#define USE_BLE

#define usbSerial SerialUSB
#define usbSerialBaud 115200

#ifdef USE_BLE
#define rnSerial BLE
#define rnSerialBaud 115200
#else
#define rnSerial iotAntenna
#define rnSerialBaud 57600
#endif


void setup() {
  // Open serial communications and wait for port to open:
  usbSerial.begin(usbSerialBaud);
  while (!usbSerial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  rnSerial.begin(rnSerialBaud);

  usbSerial.println("Passthrough to RN device");
}

void loop() {
  if (rnSerial.available()) {
    usbSerial.write(rnSerial.read());
  }
  if (usbSerial.available()) {
    rnSerial.write(usbSerial.read());
  }
}
