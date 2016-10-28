#define WIFI_SSID        "your_ssid"

#include <APtime.h>
Config config;
APtime aptime;

#define LED_ONBOARD 5

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
  //blink the led once every sec after synching on AP's clock
  digitalWrite(LED_ONBOARD, (aptime.getMillis() / 500 ) % 2);
}

