#ifndef SonosEsp_h
#define SonosEsp_h

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>

#define SNSESP_BUFSIZ 512
#define SNSESP_MAXNROFDEVICES 10


class SonosEsp {
  public:
    SonosEsp();
    void removeAllTracksFromQueue(int device); 
    void pause(int device);
    void play(int device);
    byte getVolume(int device);
    void setVolume(byte vol,int device);
    String getTransportInfo(int device);
    int discoverSonos();
    int getNumberOfDevices();


  private:
    char _response[SNSESP_BUFSIZ];
    char _filtered[256];
    IPAddress _deviceIPs[SNSESP_MAXNROFDEVICES];
    WiFiClient _client;
    int _numberOfDevices;
    
    void sonosAction(const char *url, const char *service, const char *action, const char *arguments,int device);
    void filter(const char *starttag,const char *endtag);
    int string2int(const char *s);
    bool addIp(IPAddress ip);
};

#endif


