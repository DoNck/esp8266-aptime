esp8266-aptime
==============

 

Introduction
------------

This library and the PoC examples demonstrate how multiple ESP8266-based IoT can
get their clock set and synchronized using a 802.11 AP (WiFi Access Point).

**ESP8266 have no real-time clock, so I based this approach on using 802.11 AP
beacons** (each AP beacon contains a microsecond accurate timestamp).

**At boot time, the ESP8266 will first switch its 802.11 interface to
promiscuous mode and wait for a beacon to set its clock reference.** Once
synchronized, radio can be switched to whatever mode (even off). User
applications can rely on ESP8266 internal clock after synchronization..

An interesting point is that you don't even need to connect to the AP. Thus you
can use someone else's secured AP to get clock synchronization without requiring
any password.

 

Running the PoC example
-----------------------

Edit the "APtime\_simple" example to **set "WIFI\_SSID"**, **flash**  it to
multiple ESP8266 boards. Connect LEDs on ports 5 if there are none on your
boards. Then power the boards. Wait for a few seconds for the boards to sniff
beacons....

**LEDs should blink in sync !**

 

Limitations
-----------

### Connectivity

**All devices must be in range of the same AP** to get their clock synchronized
together.

There is **no security** enforcement on this project so it should be easy to
mess it up. Library is just checking the "SSID" field of beacons.

Warning: **synchronize() is blocking** until synchronization is over. It should
take \~50-150ms if you specified the correct preferred channel and if the AP is
in range !

**No WiFi communication is possible during synchronization** as the ESP8266 will
be turned to promiscuous mode.

### Precision and accuracy

AP timestamps have microsecond precision, **this library has millisecond
precision**. Working in microsecond precision would require storing 64 bits
timestamp to avoid frequent overflow and the synchronization is not actually
that accurate. I did not conduct any serious experiment yet, but **I believe the
accuracy to be close to 50ms right after synchronization**.

**Moreover, accuracy will slowly degrade after synchronization because of
relative clocks drifts** between local timebases and AP timebase. This turns visible with the simple led blinking example if you power a second
ESP8266 a few hours after the first one.

To avoid inaccuracy, you have two options:
* call ```synchronizedForced()``` **as frequently as you need** to force resynchronization periodically
* set ```config.linearSlope``` to the proper value for your AP/ESP8266 pair (provided by running the "APtime\_sample\_drift" example). 

#### Using the "APtime\_sample\_drift" example ####

This example file will measure the relative clock drift of your AP/ESP8266 pair and print the advised setting for config.linearSlope.

**Run the "APtime\_sample\_drift" as long as required by your accuracy expectancies.**
If you aim at millisecond accuracy over a period of one hour, it would not make sense to run the experiment for less than one hour.
If you aim at 0.5 second accuracy over a month, run it for at least ```30*60*60*1000/500 = 216000ms ~4minutes```.
If the expected accuracy is not very high, do not set config.linearSlope and linear computations will be completely disabled (**faster**); 

As an example, here are the results after running the experiment on one of my boards for approximately **12 hours**:

```
Sampled: tLocal1 = 825 ms, tLocal2=43201138 ms, tAP1=31304806 ms, tAP2=74505626 ms
Linear Slope = (tLocal2-tLocal1)/(tAP2-tAP1) ~= 0.9999882641
Absolute error = Delta_tLocal-Delta_tAP = -507 ms
Relative error = (Delta_tLocal-Delta_tAP)/(Delta_tAP) ~= -1,17358883465638E-05
```

**The resulting computed linear Slope is very specific to the AP/ESP8266 pair**. Sadly, experience shown that the same value cannot be used on a batch of boards.


License and credits
-------------------

This work was inspired by [RandDruid’s esp8266-deauth
project](https://github.com/RandDruid/esp8266-deauth) and widely uses code from
this project. A big thank to RandDruid for sharing his great work !

Thus license for this project is MIT.
