/**
   This example is based on the "HelloServer" example of the ESP8266 platform and the APtime_simple example
   It serves a page displaying the timestamp computed at request time from local clock and AP beacons.
*/

#define WIFI_SSID        "your_ssid"
#define WIFI_PASSWORD    "your_password"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <APtime.h>
Config config;
APtime aptime;

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

ESP8266WebServer server(80);

#define LED_ONBOARD 5

void handleRoot() {

  String message = "<!DOCTYPE html>";
  {
    message += "<script src='https://ajax.googleapis.com/ajax/libs/jquery/1.8.2/jquery.min.js'></script>";
    message += "<script src='http://momentjs.com/static/js/global.js'></script>";

    message += "<body>";
    {
      message += "<h1>APtime demo</h1>";
      message += "<div>Local timestamp computed from local time and AP beacons: ";
      message += aptime.getMillis();
      message += " milliseconds";
      message += " <span id='humanized'></span>";
      message += " </div>";
    }
    message += "</body>";

    message += "<script>";
    {
      message += "$('#humanized').text('(approximately '+moment.duration({milliseconds: ";
      message += aptime.getMillis();
      message += +"}).humanize()+')');";
    }
    message += "</script>";

  }
  message += "</html>";

  server.send(200, "text/html", message);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void) {
  pinMode(LED_ONBOARD, OUTPUT);
  digitalWrite(LED_ONBOARD, 0);

  Serial.begin(115200);

  config.ssid = WIFI_SSID;   //The SSID of the AP you want to sync your clock to
  config.favoriteChannel = 1;  //Starts sniffing on this channel (for quicker init)
  config.tryAllChannels = true; //Set to false if you do not want to try other channels
  config.linearSlope = 1.0;     //AP clock drift correction relative to local clock
  aptime.setConfig(config);
  aptime.synchronize();

  Serial.println("Conecting to WiFi...");
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();

  //blink the led once every sec after synching on AP's clock
  digitalWrite(LED_ONBOARD, (aptime.getMillis() / 500 ) % 2);
}