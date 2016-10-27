/*
  APtime.h - 802.11 beacon-based clock synchronization library
  Intended for ESP8266 platform
  by DoNcK (Nov. 2016)
  License: MIT
  Repository: https://github.com/DoNck/esp8266-aptime
*/

#ifndef APtime_h
#define APtime_h

#include <Arduino.h>

///

#ifndef ETH_MAC_LEN
#define ETH_MAC_LEN 6
#endif

#ifndef MAX_APS_TRACKED
#define MAX_APS_TRACKED 100
#endif

// Expose Espressif SDK functionality - wrapped in ifdef so that it still
// compiles on other platforms
#ifdef ESP8266
extern "C" {
#include "user_interface.h" //to gain access to promiscuous mode
}
#endif

#include <ESP8266WiFi.h>

struct beaconinfo
{
  uint8_t bssid[ETH_MAC_LEN];
  uint8_t ssid[33];
  int ssid_len;
  int channel;
  int err;
  signed rssi;
  uint8_t capa[2];
  long long timestamp; //microseconds. this is what we are going to use for clock synchronization ! Overflows after ~584'942 years :-)
};

/* ==============================================
   Promiscous callback structures, see ESP manual
   ============================================== */

struct RxControl {
  signed rssi: 8;
  unsigned rate: 4;
  unsigned is_group: 1;
  unsigned: 1;
  unsigned sig_mode: 2;
  unsigned legacy_length: 12;
  unsigned damatch0: 1;
  unsigned damatch1: 1;
  unsigned bssidmatch0: 1;
  unsigned bssidmatch1: 1;
  unsigned MCS: 7;
  unsigned CWB: 1;
  unsigned HT_length: 16;
  unsigned Smoothing: 1;
  unsigned Not_Sounding: 1;
  unsigned: 1;
  unsigned Aggregation: 1;
  unsigned STBC: 2;
  unsigned FEC_CODING: 1;
  unsigned SGI: 1;
  unsigned rxend_state: 8;
  unsigned ampdu_cnt: 8;
  unsigned channel: 4;
  unsigned: 12;
};

struct sniffer_buf2 {
  struct RxControl rx_ctrl;
  uint8_t buf[112];
  uint16_t cnt;
  uint16_t len;
};

struct Config {
    String ssid;
    int favoriteChannel;
    boolean tryAllChannels;
    float linearSlope;
};

class APtime
{
  private:
  
    //fields config
    Config _config;
    
    //fields (clock):
    unsigned long _lastInternalTs; // = millis(), milliseconds
    unsigned long _lastApTs; //milliseconds

    unsigned long _linearYIntercept = 0;
    
    //fields(radio):
    beaconinfo _aps_known[MAX_APS_TRACKED]; // Array to save MACs of known APs
    int _aps_known_count;                   // Number of known APs
    int _nothing_new;

    //methods:
    struct beaconinfo parse_beacon(uint8_t *frame, uint16_t framelen, signed rssi);
    int register_beacon(beaconinfo beacon);
    void print_beacon(beaconinfo beacon);
    void try_sync_from_beacon(beaconinfo beacon, unsigned long internalTs);    
    void longLongPrint(long long v);
    
  public:
    APtime();
    //~APtime();

    static APtime* pInstance; //singleton
    
    //methods
  void setConfig(Config config);
    //to match Arduino library style:
    unsigned long synchronize();
    unsigned long synchronizeForced();
    /**
     * We model local time as being a linear function of AP time and try to find the slope coefficient
     */
    double sampleLinearSlope(unsigned long deltaMillis);
    //    
    /**
    * Returns milliseconds (32 bits) synced on Access Point beacons (64 bits microseconds).
    * It might be the milliseconds since the Acess Point has been running but that is not specified as mandatory by the 802.11 standard.
    * This number will overflow (go back to zero), after approximately 50 days.
    * Perfect millisecond accuracy should not be expected.
    */
    unsigned long getMillis();

    void promisc_cb(uint8_t *buf, uint16_t len);
};

#endif

