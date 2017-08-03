

/*https://wiki.thingscalderdale.com/LoRaWAN_Workshop
 * 
 *  This sketch is based upon the SmarteEveryting Lion RN2483 Library - sendDataOTA_console
    This sketch also uses the code by Andrew Lindsay created for the things calderdale boost workshop
    created 25 Feb 2017
    by Seve (seve@ioteam.it)

    This example is designed as a periodic upload demonstrator and a compelling lora example https://twitter.com/ABOPThings
    The timing has been re-worked to use a tight timing loop to better control the time between transmit requests and other items.
    This code uses a button connected to Pin 2 (normally open) and will then upload the number of button pushes, Leds on pins 3,5,6 will then show the status.
    Pin 6 is the transmit pin and will be active during transmit
    Pin 5 is the button pin and will go active every time the button is pushed
    Pin 3 is the message waiting indicator and will be active if there is data to send in the next TX window


    Known issues:
    Some formatting issues for 100's of button pushes per minute
    The Button Led is not perfect and has some issues
    This example polls the button an interrupt driven method would be better

    
 */

#include <Arduino.h>
#include <rn2483.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
/* * INPUT DATA (for OTA)
 * 
 *  1) device EUI, 
 *  2) application EUI 
 *  3) and application key 
 *  
 *  then OTAA procedure can start.
 */
#define APP_EUI "70B3D57EF0005CDF" //"Place ThingsNetwork APP_EUI here"
#define APP_KEY "45250899DD297D1E4A9806E312FD90EE" //"Place ThingsNetwork APP_KEY here"


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
const int buttonPin = 2;
const int ledgPin = 5;
const int ledrPin = 6;
const int ledaPin = 3;

char c; 
char buff[100]  = {};
int  len        = 0;
unsigned char i = 0;
int Button_state =0;
int Button_pushes =0;

unsigned char buff_size = 100;

//30 seconds, default should be more than : 30000 is about 5 minutes;
static unsigned long Tx_interval = 60; // seconds 
static unsigned long loop_cnt = 0;
static unsigned long millisecondtimer =0;  


void setup() {
    errE err;
    loraDbg = false;         // Set to 'true' to enable RN2483 TX/RX traces
    bool storeConfig = true; // Set to 'false' if persistent config already in place    
    pinMode(buttonPin, INPUT);
    digitalWrite(buttonPin, HIGH);

    pinMode(ledgPin, OUTPUT);
    digitalWrite(ledgPin, LOW);
    pinMode(ledrPin, OUTPUT);
    digitalWrite(ledrPin, LOW);
    pinMode(ledaPin, OUTPUT);
    digitalWrite(ledaPin, LOW);    
    
    ledYellowOneLight(LOW); // turn the LED off
    ledYellowTwoLight(HIGH); // turn the LED on
    delay(500);   
    Serial.begin(115200);

    lora.begin();
    // Waiting for the USB serial connection
//    while (!Serial) {
//        ;
//    }

    delay(1000);
    Serial.print("FW Version :");
    Serial.println(lora.sysGetVersion());

    /* NOTICE: Current Keys configuration can be skipped if already stored
     *          with the store config Example
     */
    if (storeConfig) {
         // Write HwEUI
        Serial.println("Writing Hw EUI in DEV EUI ...");
        lora.macSetDevEUICmd(lora.sysGetHwEUI());

        if (err != RN_OK) {
            Serial.println("\nFailed writing Dev EUI");
        }
        
        Serial.println("Writing APP EUI ...");
        err = lora.macSetAppEUICmd(APP_EUI);
        if (err != RN_OK) {
            Serial.println("\nFailed writing APP EUI");
        }
        
        Serial.println("Writing Application Key ...");
        lora.macSetAppKeyCmd(APP_KEY);
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
    
 /* KW Removing this as it is bad practice if we dont need it.  
  * Serial.println("Setting Trasmission Power to Max ...");
    lora.macPause();
    err = lora.radioSetPwr(14);
    if (err != RN_OK) {
        Serial.println("\nFailed Setting the power to max power");
    }
    lora.macResume();
*/
    delay(5000);
    ledYellowTwoLight(LOW); // turn the LED off 

    // Setup LCD
     // Use this initializer if you're using a 1.8" TFT
      tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
    
      // Use this initializer (uncomment) if you're using a 1.44" TFT
      //tft.initR(INITR_144GREENTAB);   // initialize a ST7735S chip, black tab
    
      tft.fillScreen(ST7735_BLACK);
      // If we want to rotate screen then use the setRotation function
      // tft.setRotation(3);
    
      setScreen((char*)"Button OS");
    
      tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
      tft.setTextSize(1);
      tft.setCursor(0, 16 );
      tft.println("LoRa firmware version");
      tft.println(lora.sysGetVersion());
      lora.macJoinCmd(OTAA); // Start a join event before we enter the loop (so we may have joined when we get our first TX window)

    
}


void loop() {
  
    static int loop_cnt = 0;
    static int tx_cnt = 0;
    static bool joined = false;
    static uint32_t state;
    int seconds =0;
    int timeatbp;


    // Loop though serial to the RN2483
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

    // Check button status polled (not a good way)
    if (digitalRead(buttonPin)==0)
    {
        Button_state =1;
        digitalWrite(ledrPin, HIGH);
    }
    if (Button_state ==1)
    { 
        //wait a sec
        //delay(10);
        if (digitalRead(buttonPin)==1)
        {  
           Button_pushes++; // Add a button push
           Button_state =0; // clear the flag
           //digitalWrite(ledrPin, LOW);  // update LEDs moved to 1 second tick
           digitalWrite(ledaPin, HIGH);
           tft.setCursor(0,60);         // update display
           tft.print("Button Pushes cached:         ");// Quick bodge to clear the LCD
           tft.setCursor(0,60);   // Quick fudge to clear the LCD
           tft.print("Button Pushes cached:");
           tft.println(Button_pushes);
           
        }
    }

    if (loop_cnt >=Tx_interval) 
    { 
       state = lora.macGetStatus();
       //Serial.println(state, HEX);
       
       // Check If network is still joined
       if (MAC_JOINED(state)) {
           if (!joined) {
                Serial.println("\nOTA Network JOINED! ");
                tft.println("OTAA Join ok");
                joined = true;
           }
               // Sending String to the Lora Module towards the gateway
               ledYellowTwoLight(HIGH); // turn the LED on
               tx_cnt++;
               tft.setCursor(0,100);  // update display with TX info
               tft.print("Tranmit data:");// update display with TX info
               tft.println(Button_pushes);// update display with TX info
               tft.print("Uptime mins:");// update display with TX info
               seconds = (millis()/1000)/60;// update up time
               tft.print(seconds);
                  
               digitalWrite(ledgPin, HIGH ); // Turn the green LED on
               Serial.println("Sending Confirmed String ...");
               lora.macTxCmd(String(Button_pushes), 1, TX_ACK); // Confirmed tx 
               //lora.macTxCmd(String(Button_pushes), 1);   // Unconfirmed tx buffer if required
               Button_pushes =0;
  
               tft.setCursor(0,60);                     // another fudge for display
               tft.print("Button Pushes cached:    ");  // another fudge for display
               
               tft.setCursor(0,60);                     // Display update
               tft.print("Button Pushes cached:");
               tft.println(Button_pushes);
               digitalWrite(ledgPin, LOW);
               digitalWrite(ledaPin, LOW);
               ledYellowTwoLight(LOW); // turn the LED off 
       } else {                   // Network is not joined so try to join
           lora.macJoinCmd(OTAA); // wait a while after joining
           delay(4000);
       }
      loop_cnt = 0; 
      
    }

    if ((millisecondtimer + 1000) <= millis()) // check to see if a second has elapsed if it has update the display and seconds counter
    {
     loop_cnt++;
     digitalWrite(ledrPin, LOW);  // update LEDs if red led active remove it
     // Serial.println(loop_cnt);
     millisecondtimer=millis(); // store current time
     tft.setCursor(0,140);
     tft.print("Time to next TX:");
     if((Tx_interval -loop_cnt)==9)  // fix for displaying single digits (another fudge)
     {
        tft.setCursor(100,140);
        tft.print("  ");
     }
     tft.setCursor(100,140);
     tft.println(Tx_interval -loop_cnt);
    }
    
}

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
  tft.setTextSize( textSize );
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
