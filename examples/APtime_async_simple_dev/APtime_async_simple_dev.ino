#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define LED_ONBOARD 5
#define NEOPIXEL_PIN 14

#define WIFI_SSID        "......" //SSID must not be hidden by the AP!
#define WIFI_PASSWORD    "......" //actually not necessary for clock sync

//*****
// HTTPSERVER
//*****
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
//
#define HTTP_SERVER true
#if HTTP_SERVER
ESP8266WebServer server(80);


// Expose Espressif SDK functionality - wrapped in ifdef so that it still
// compiles on other platforms
#ifdef ESP8266
extern "C" {
#include "user_interface.h" //to gain access to promiscuous mode
}
#endif

//WiFi station config backup
static struct station_config sta_conf_bkp;

//
void handleRoot() {
  uint8 bkp_wifi_channel = wifi_get_channel();
  //
  digitalWrite(LED_ONBOARD, !false);
  String message = "ESP8266 Connected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  //
  message += "\nConnected on SSID X\n";
  message += "\nConnected on SSID X\n";
  message += "Channel ";
  message += bkp_wifi_channel;
  message += "\nIP: ";
  message += WiFi.localIP();
  message += "\n";
  //
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(200, "text/plain", message);
  digitalWrite(LED_ONBOARD, !true);
}
//
void handleNotFound() {
  digitalWrite(LED_ONBOARD, !false);
  server.send(404, "text/plain", "not found!");
  digitalWrite(LED_ONBOARD, !true);
}
#endif

//clock sync stuffs:
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
    //Serial.println("\nApp connection: Disconnecting!");
    app_cnx_status = CNX_DISCONNECTED;
    Serial.println("\nApp connection: Disconnected!");
  }
}

void backupWiFiConfig() {
  //uint8* p_lastBSSID = WiFi.BSSID(); //reference : https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/station-class.md#bssid
  static struct station_config sta_conf_bkp;
  wifi_station_get_config(&sta_conf_bkp);

  static struct station_config conf;
  //TODO: la sauvegarder avant le disconnect, puis utiliser
  //uint8 bssid; //[6] normalement
  //memcpy(conf.bssid,);
  //
}

void restoreWiFiConfig() {
  //uint8* p_lastBSSID = WiFi.BSSID(); //reference : https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/station-class.md#bssid
  static struct station_config sta_conf_bkp;
  wifi_station_get_config(&sta_conf_bkp);


  wifi_station_disconnect();

  wifi_station_connect();

  static struct station_config conf;
  //TODO: la sauvegarder avant le disconnect, puis utiliser
  //uint8 bssid; //[6] normalement
  //memcpy(conf.bssid,);
  //
}

void connectStuffsAsync() {
  //TODO: wifi_status = wifi.connect() ? CNX_CONNECTING : CNX_CONNECTED;
  //if OK connect websocket

  bool bWiFi_connected = WiFi.status() == WL_CONNECTED;

  if (app_cnx_status != CNX_CONNECTED) {
    //Serial.println("\nApp connection: connecting!");

    if (app_cnx_status == CNX_DISCONNECTED) {
      Serial.print("\nTrying to connect to WiFi SSID ");
      Serial.println(WIFI_SSID);
      Serial.print("\nCurrently on channel ");
      Serial.println(wifi_get_channel());

      //WiFi.begin(WIFI_SSID, WIFI_PASSWORD); //set the config ONCE
      //TODO: test pour faster connection: WiFi.begin(ssid,pass,ch,bssid); //TODO try that instead for faster connections!
      //bssid_set = 1;

      //unsigned char ap_mac[18] = { 0x2C, 0x30, 0x33, 0x34, 0x08, 0x68 }; //jeff-2.4-G AP BSSID
      //unsigned char ap_mac[18] = { 0x2A, 0x30, 0x33, 0x34, 0x08, 0x68 }; //wrong AP BSSID
      //WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 13, ap_mac, true); //reference: https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/station-class.md#begin
      restoreWiFiConfig();

      Serial.println("\nWiFi begin");
      Serial.print("\nCurrently on channel");
      Serial.println(wifi_get_channel());
      app_cnx_status = CNX_CONNECTING;
    }

    bWiFi_connected = WiFi.status() == WL_CONNECTED;
    if (bWiFi_connected) {

      //WiFi.printDiag(Serial); //pratique !
      Serial.print("\nConnected! IP address: ");
      Serial.println(WiFi.localIP());
      Serial.print("\nCurrently on channel");
      Serial.println(wifi_get_channel());
      //
      //uint8* p_lastBSSID = WiFi.BSSID(); //reference : https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/station-class.md#bssid

      //display BSSID (MAC address of curent AP)
      wifi_station_get_config(&conf);
      char mac[18] = { 0 };
      sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", conf.bssid[0], conf.bssid[1], conf.bssid[2], conf.bssid[3], conf.bssid[4], conf.bssid[5]);
      Serial.println(mac);



      serve();
      app_cnx_status = CNX_CONNECTED;
    }
  }

  digitalWrite(LED_ONBOARD, !bWiFi_connected);
}

/**
   It seems OK to trigger this more than once
 * */
void serve(void) {
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
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
  config.autoUpdatePeriod = 30 * 1000; //30 seconds, for testing purposes

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

  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }

}

