#include <Arduino.h>

#define WIFI_SSID        "your_ssid"
#define WIFI_PASSWORD    "your_password"

#include <APtime.h>
#define LED_ONBOARD 5

Config config;
APtime aptime;

void setup() {
  Serial.begin(115200);
  pinMode(LED_ONBOARD, OUTPUT); // Initialize the LED_ONBOARD pin as an output

  config.ssid = WIFI_SSID;   //The SSID of the AP you want to sync your clock to
  config.favoriteChannel = 1;  //Starts sniffing on this channel (for quicker init)
  config.tryAllChannels = true; //Set to false if you do not want to try other channels

  aptime.setConfig(config);
  aptime.synchronize();
}

void loop() {
  Serial.println("---------------------");
  aptime.sampleLinearSlope(10 * 1000); //10 seconds in milliseconds
  //aptime.sampleLinearSlope(30 * 60 * 1000); //30 minutes in milliseconds
  //aptime.sampleLinearSlope(12 * 60 * 60 * 1000); //12 hours in milliseconds
}

