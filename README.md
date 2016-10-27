esp8266-aptime
==============


Introduction
------------

This is a PoC that demonstrates how multiple ESP8266-based IoT can get their
clock set and synchronized using a 802.11 AP (WiFi Access Point).

**ESP8266 have no real-time clock, so I based this approach on using 802.11 AP
beacons** (every beacon contains a timestamp).

**At boot time, the ESP8266 will first switch its 802.11 interface to
promiscuous mode and wait for a beacon to set its clock reference.** Once
synchronized, radio can be switched to whatever mode (even off) and user
applications can rely on ESP8266 internal clock.

An interesting point is that you don't even need to connect to the AP. Thus you
can use someone else's secured AP to get clock synchronization without requiring
any password.


Running the PoC
---------------

Edit this program to **set "WIFI\_SSID"**, **flash **this program to 2 ESP8266
boards, connect an LED on ports 5 if there are none on your boards. Then power
the first board, wait for a random time and power the second board. Wait for a
few seconds for the boards to sniff beacons....

**LEDs should blink in sync !**


Limitations
-----------

Device using different APs cannot get their clock synchronized this way.

There is not security enforcement on this project so it should be easy to mess
it up.

Timestamps are microsecond accurate, but precision might degrade to a second a
worse after running a few hours.  
Actually, everything should work well if you power all your devices within a
short period of time. However, if you wait for a few minutes/hours to power
a device after the first one, you may encounter a delay between the two blinking LEDs,
meaning the two boards somehow got out of sync. May be the 1st device has clock
drift, may be the AP has. To be investigated... Currently, I suspect the AP more
than the ESP8266 boards.


License and credits
------------------

This work was inspired by [RandDruid’s esp8266-deauth
project](https://github.com/RandDruid/esp8266-deauth) and widely uses code from this project. A big thank to RandDruid for 
sharing his great work !

Thus license for this project is MIT.
