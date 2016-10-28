/*
  APtime.h - 802.11 beacon-based clock synchronization library
  Intended for ESP8266 platform
  by DoNcK (Nov. 2016)
  License: MIT
  Repository: https://github.com/DoNck/esp8266-aptime
*/

#include <Arduino.h>
#include <APtime.h>

APtime* APtime::pInstance = NULL; //singleton initial definition

/**
   Methods cannot be passed as promiscuous callback. So we will use this simple wrapper and the pInstance singleton
*/
void promisc_cb_wrapper(uint8_t *buf, uint16_t len) {
  APtime::pInstance->promisc_cb(buf, len);
}


void APtime::promisc_cb(uint8_t *buf, uint16_t len)
{
  unsigned long internalTs = millis(); //we store this one ASAP before running any computation
  //Serial.println("promisc_cb called");
  int i = 0;
  uint16_t seq_n_new = 0;
  if (len == 12) {
    struct RxControl *sniffer = (struct RxControl*) buf;
  } else if (len == 128) {
    struct sniffer_buf2 *sniffer = (struct sniffer_buf2*) buf;
    struct beaconinfo beacon = parse_beacon(sniffer->buf, 112, sniffer->rx_ctrl.rssi);
    if (register_beacon(beacon) == 0) {
      //print_beacon(beacon);
      try_sync_from_beacon(beacon, internalTs);
      _nothing_new = 0;
    }
  }
}

APtime::APtime()
{
  pInstance = this; //sniffing callback requires a static function and does NOT accept class methods, so we will use a static callback and this singleton
  //
  _lastInternalTs = 0;  // = millis()
  _lastApTs;            //milliseconds
  _aps_known_count = 0; // Number of known APs
  _nothing_new = 0;
}


void APtime::setConfig(Config config)
{
  _config = config;
}

/**
   returns: absolute local clock error since last update (ms)
*/
unsigned long APtime::synchronize()
{
  uint8 wifi_opmode = wifi_get_opmode ();
  
  uint8_t _channel = _config.favoriteChannel; //Channel to sniff

  if (_linearYIntercept == 0) { //we skip running this if the clock is already synchronized

    unsigned long begints = millis();

    Serial.printf("\n[APtime] Trying to get time synced with AP beacons of \"%s\" on _channel %d ...", _config.ssid.c_str(), _channel);

    // Promiscuous works only with station mode
    wifi_set_opmode(STATION_MODE);

    // Set up promiscuous callback
    wifi_set_channel(_channel);
    wifi_promiscuous_enable(0);
    wifi_set_promiscuous_rx_cb(promisc_cb_wrapper);
    wifi_promiscuous_enable(1);

    while (_linearYIntercept == 0) {
      _nothing_new++;
      if (_nothing_new > 200) {
        _nothing_new = 0;

        if (_config.tryAllChannels) {
          _channel = _channel % 15 + 1; //1 to 15 CHECK a quoi servait leur _nothing_new
        }

        Serial.printf("\n[APtime] switching to _channel %d", _channel);
        wifi_set_channel(_channel);

      }
      delay(1);
    }

    Serial.printf("\n[APtime] SYNCED ! (%d ms)\n", millis() - begints);
  }

  //unregister promiscuous callback and switch promiscuous mode off
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(0);
  wifi_promiscuous_enable(1);
  wifi_promiscuous_enable(0);

  wifi_set_opmode(wifi_opmode); //restore wifi_opmode
  
  return 0; //TODO
}

unsigned long APtime::synchronizeForced()
{
  _aps_known_count = 0;  //reset beacon registration
  _linearYIntercept = 0; //reset clock sync
  return synchronize();
}

double APtime::sampleLinearSlope(unsigned long deltaMillis)
{
  //TODO: WARN in case of overflow ? Who cares? They should overflow quite in sync !

  Serial.print("\n[APtime] Starting sampling linear slope...\n");

  double slope;
  unsigned long tLocal1, tLocal2, tAP1, tAP2;

  synchronizeForced();
  tAP1 = _lastApTs;
  tLocal1 = _lastInternalTs;

  Serial.printf("\n[APtime] Please wait for %d ms...\n", deltaMillis);
  //blocking style, not very nice...
  while (millis() - tLocal1 < deltaMillis) {
    delay(1);
  }

  synchronizeForced();

  tAP2 = _lastApTs;
  tLocal2 = _lastInternalTs;

  Serial.printf("\n[APtime] Sampled: tLocal1 = %d ms, tLocal2 = %d ms, tAP1 = %d ms, tAP2 = %d ms", tLocal1, tLocal2, tAP1, tAP2);

  slope = (tLocal2 - tLocal1) / (1.0 * (tAP2 - tAP1));

  Serial.print("\n[APtime] Computed linear slope: ");
  Serial.print(slope, 10);
  Serial.println();

  return slope;
}

unsigned long APtime::getMillis()
{
  if (_config.linearSlope == 1.0) { //avoid slow floating point math if possible
    return millis() + _linearYIntercept;
  } else {
    return millis() * _config.linearSlope + _linearYIntercept;
  }
}

struct beaconinfo APtime::parse_beacon(uint8_t *frame, uint16_t framelen, signed rssi)
{
  struct beaconinfo bi;
  bi.ssid_len = 0;
  bi.channel = 0;
  bi.err = 0;
  bi.rssi = rssi;
  int pos = 36;

  if (frame[pos] == 0x00) {
    while (pos < framelen) {
      switch (frame[pos]) {
        case 0x00: //SSID
          bi.ssid_len = (int) frame[pos + 1];
          if (bi.ssid_len == 0) {
            memset(bi.ssid, '\x00', 33);
            break;
          }
          if (bi.ssid_len < 0) {
            bi.err = -1;
            break;
          }
          if (bi.ssid_len > 32) {
            bi.err = -2;
            break;
          }
          memset(bi.ssid, '\x00', 33);
          memcpy(bi.ssid, frame + pos + 2, bi.ssid_len);
          bi.err = 0;  // before was error??
          break;
        case 0x03: //Channel
          bi.channel = (int) frame[pos + 2];
          pos = -1;
          break;
        default:
          break;
      }
      if (pos < 0) break;
      pos += (int) frame[pos + 1] + 2;
    }
  } else {
    bi.err = -3;
  }

  bi.capa[0] = frame[34];
  bi.capa[1] = frame[35];
  memcpy(bi.bssid, frame + 10, ETH_MAC_LEN);

  /**
     https://mrncciew.files.wordpress.com/2014/10/cwap-mgmt-beacon-01.png
    https://github.com/RandDruid/esp8266-deauth2/blob/master/esp8266-deauth2.ino
  */
  int ts_pos = 24; //starting from SSID position: 36-2Bytes CapaInfo-2Bytes Beacon Inteval -8Bytes timestamp
  //int bytes_to_copy = 4; //to fit in a unsingned int
  int bytes_to_copy = 8; //to fit in a long long

  memcpy(&bi.timestamp, frame + 24, bytes_to_copy);

  //Serial.print("\nTimestamp:");
  //Serial.println(bi.timestamp);

  return bi;
}



int APtime::register_beacon(beaconinfo beacon)
{
  int known = 0;   // Clear known flag
  for (int u = 0; u < _aps_known_count; u++)
  {
    if (! memcmp(_aps_known[u].bssid, beacon.bssid, ETH_MAC_LEN)) {
      known = 1;
      break;
    }   // AP known => Set known flag
  }
  if (! known)  // AP is NEW, copy MAC to array and return it
  {
    memcpy(&_aps_known[_aps_known_count], &beacon, sizeof(beacon));
    _aps_known_count++;

    if ((unsigned int) _aps_known_count >=
        sizeof (_aps_known) / sizeof (_aps_known[0]) ) {
      Serial.printf("exceeded max _aps_known\n");
      _aps_known_count = 0;
    }
  }
  return known;
}

void APtime::print_beacon(beaconinfo beacon)
{
  if (beacon.err != 0) {
    //Serial.printf("BEACON ERR: (%d)  ", beacon.err);
  } else {
    Serial.printf("BEACON: [%s]  ", beacon.ssid);

    for (int i = 0; i < 6; i++) Serial.printf("%02x", beacon.bssid[i]);
    Serial.printf("   %2d", beacon.channel);
    Serial.printf("   %4d\r\n", beacon.rssi);
  }
}

void APtime::try_sync_from_beacon(beaconinfo beacon, unsigned long internalTs)
{
  if (beacon.err == 0) {

    //String _config.ssid = WIFI_SSID;
    char test[_config.ssid.length()];
    _config.ssid.toCharArray(test, _config.ssid.length() + 1);
    //Serial.printf("OUR BEACON: [%s]  | ", test); //prints OK

    //String currentSSID= String(beacon.ssid);

    //Serial.printf("MEMCP: [%d] | ", memcmp(&test, &beacon.ssid, sizeof(test)));
    if (memcmp(&test, &beacon.ssid, sizeof(test)) == 0 ) {

      _lastApTs = beacon.timestamp / 1000;
      //Serial.printf("AP  Timestamp:   %d ms | ", _lastApTs);

      _lastInternalTs = internalTs; //~millis()

      if (_config.linearSlope == 1.0) {
        _linearYIntercept = _lastApTs - _lastInternalTs;
      } else {
        //TODO handle drift
      }

      /*
        Serial.printf("%d , ", _lastInternalTs);
        Serial.printf("%d , ", _lastApTs);
        Serial.printf("_linearYIntercept %d\r\n", _linearYIntercept);
        Serial.printf("_config.linearSlope %d\r\n", _config.linearSlope);
      */

    }
  }
}

void APtime::longLongPrint(long long v){
    char buffer[100];
    sprintf(buffer, "%0ld", v/1000000L);
    Serial.print(buffer);  
    sprintf(buffer, "%0ld", v%1000000L);
    Serial.println(buffer);  
}
