#define WIFI_SSID        "JEFF-2.4-G"

/**
* Intended to deploy on multiple boards to get a visual effect looking like a wave
*/


#include <APtime.h>
Config config;
APtime aptime;

#define LED_ONBOARD 1

void setup() {
  Serial.begin(115200);
  pinMode(LED_ONBOARD, OUTPUT); // Initialize the LED_ONBOARD pin as an output

  config.ssid = WIFI_SSID;   //The SSID of the AP you want to sync your clock to
  config.favoriteChannel = 1;  //Starts sniffing on this channel (for quicker init)
  config.tryAllChannels = true; //Set to false if you do not want to try other channels
  //config.linearSlope = 0.999988264111653; //set proper value and uncoment to compensate clock drift (please see the README file)

  aptime.setConfig(config);
  aptime.synchronize();
}

void loop() {
  //blink the led once every sec after synching on AP's clock
  //digitalWrite(LED_ONBOARD, (aptime.getMillis() / 500 ) % 2);
  int v = round(512 * (1.0 + sin(2.0 * 3.14 * 3.0 / 8.0 + aptime.getMillis() * 3.14 / 500.0 )));
  if (v > 256) {
    analogWrite(LED_ONBOARD, 1024-v);
  } else {
    analogWrite(LED_ONBOARD, 1024);
  }
}

