#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define WIFI_SSID        "**********"
#define WIFI_PASSWORD    "**********"

#include <APtime.h>
Config config;
APtime aptime;



//will describe a websocket connection as an example (not the wifi status)
enum connection_status {
  CNX_CONNECTING,
  CNX_CONNECTED,
  CNX_DISCONNECTING,
  CNX_DISCONNECTED
};

connection_status app_cnx_status;

#define LED_ONBOARD 5
#define NEOPIXEL_PIN 14
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, NEOPIXEL_PIN, NEO_RGB + NEO_KHZ800);
int colorMaskR = 255, colorMaskG = 255, colorMaskB = 255;
void setLedColor(int R, int G, int B)
{
  colorMaskR = R;
  colorMaskG = G;
  colorMaskB = B;
}
void setLedBrightness(int brightness)
{
  strip.setPixelColor(0, strip.Color(brightness * colorMaskR / 255, brightness * colorMaskG / 255, brightness * colorMaskB / 255)); //from 0 to 255
  strip.show();
}

void show_app_status() {
  switch (app_cnx_status) {
    case CNX_CONNECTING:
      //digitalWrite(LED_ONBOARD, (millis() / 100 ) % 2); //blink quickly
      digitalWrite(LED_ONBOARD, !false); //OFF
      break;
    case CNX_CONNECTED:
      digitalWrite(LED_ONBOARD, !true); //ON
      break;
    case CNX_DISCONNECTING:
      //digitalWrite(LED_ONBOARD, (millis() / 100 ) % 2); //blink quickly
      digitalWrite(LED_ONBOARD, !false); //OFF
      break;
    case CNX_DISCONNECTED:
      digitalWrite(LED_ONBOARD, !false); //OFF
      break;
    default: //ERROR
      digitalWrite(LED_ONBOARD, !false); //OFF
      break;
  }
}

void show_APtime_status() {
  //use the proper RGB led color to show time stuff status:
  switch (aptime.getStatus()) {
    case APTIME_INIT:
      setLedColor(0, 0, 255); //BLUE
      break;
    case APTIME_SYNCHRONIZING:
      setLedColor(255, 0, 255); //PURPLE
      break;
    case APTIME_SYNCHRONIZED:
      setLedColor(0, 255, 0); //GREEEN
      break;
    case APTIME_RESYNCHRONIZING:
      setLedColor(255, 127, 0); //ORANGE
      break;
    default: //ERROR
      setLedColor(255, 0, 0); //RED
      break;
  }
}

void disconnectStuffsNonBlocking() {
  //this is a simple example so we change the status on the first call
  //we could wait instead for an async process to complete, as an example, to make sure a "goodbye" message was sent
  //TODO add async delay for demo purpose
  if (app_cnx_status != CNX_DISCONNECTED) {
    Serial.println("\nWeb socket: Goodbye!");
    app_cnx_status = CNX_DISCONNECTED;
  }
}

void connectStuffsAsync() {
  //TODO: wifi_status = wifi.connect() ? CNX_CONNECTING : CNX_CONNECTED;
  //if OK connect websocket
  if (app_cnx_status != CNX_CONNECTED) {
    Serial.println("\nWeb socket: connected!");
    app_cnx_status = CNX_CONNECTED;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n--------------------------------------------------------------------\n");
  Serial.println("APtime advanced demo getting started...");

  pinMode(LED_ONBOARD, OUTPUT); // Initialize the LED_ONBOARD pin as an output

  connection_status app_cnx_status = CNX_DISCONNECTING;
  show_app_status(); //sets the onboard led on/off/blink

  config.ssid = WIFI_SSID;   //The SSID of the AP you want to sync your clock to
  config.favoriteChannel = 6;  //Starts sniffing on this channel (for quicker init)
  config.tryAllChannels = true; //Set to false if you do not want to try other channels
  //config.linearSlope = 0.999988264111653; //set proper value and uncoment to compensate clock drift (please see the README file)
  config.autoUpdatePeriod = 5 * 1000; //5 seconds, for testing purposes

  //
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  aptime.setConfig(config);
  //aptime.blockingSync(); //blocking prefered here -> pour cet exemple on fera du non blocking partout -> on attendra la fin de l'état init pour allumer les LEDs
}

void loop() {
  /*
    //simple blocking-style manual resync without connections precautions
    if(aptime.isLastSyncTooOld()){
      aptime.blockingSync();
    }
  */

  /*
    //simple non-blocking style auto resync without connections precautions
    if(aptime.isLastSyncTooOld()){
      aptime.asyncSync();
    }
  */

  /*
     Let's say we want to run a time-synchronized animation, continuously, from boot to poweroff, without freezing during periodical resynchronization

     So, when clock gets a bit old, we want to:
     -send a goodbye message and close connections
     -switch radio mode
     -resync clock
     -restore radio mode
     -restore connections (so a user can enable/disable the animation through the HTTP server)

     All of this without blocking the loop for too long ! So we will use this non-blocking-style API:
  */

  //display statuses
  show_app_status(); //sets the onboard led on/off/blink
  show_APtime_status(); //sets the color of the RGB led

  //LIBRARY <-> USER PROGRAM INTERACTIONS for auto synchronizsation in a non-blocking style:
  if (aptime.isLastSyncTooOld()) {
    disconnectStuffsNonBlocking(); //should also be either quick either async (and finally set app_cnx_status to CNX_DISCONNECTED)
    if ( app_cnx_status == CNX_DISCONNECTED) {
      aptime.asyncSync(); //keep running this as often as possible (it should return quickly) until aptime.isLastSyncTooOld() returns false
      //TODO: this is required to perform channel hopping, could we trigger that from a timer interrupt so we would not have to call it more than once like that ?
    }
  } else {
    connectStuffsAsync(); // (does nothing if already connected)
  }

  //USER PROGRAM:

  //create the brightness animation based on time (RGB led)
  int brightness = round(127 * (1.0 + sin(2.0 * 3.14 * 3.0 / 8.0 + aptime.getMillis() * 3.14 / 500.0 ))); //(0-255)
  setLedBrightness(brightness);

  if ( app_cnx_status == CNX_CONNECTED) {
    //do something interesting like sending sensor value changes
  }

}

