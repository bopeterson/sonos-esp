#ifndef WiFiSetup_h
#define WiFiSetup_h

#define WFS_DEBUG true

#if WFS_DEBUG
    #define wfs_debugprint(...) Serial.print(__VA_ARGS__)
    #define wfs_debugprintln(...) Serial.println(__VA_ARGS__)
#else
    #define wfs_debugprint(...)
    #define wfs_debugprintln(...)
#endif

//ssid: allow for 32 chars + null termination
//passkey: allow for 63 chars + null termination
//html: length of htmlstart+onload+htmlmid+body+htmlend (at the moment 98+128+4+1500+15 = 1745)
//networkc: truncate if there are more networks
//body: max length of networkch + 2 * max length of WiFiSSID + fixed part (at the moment 1024 + 2*32 +367 = 1455)

//xxx should be private variables or constants if possible
#define MAXSSIDLENGTH 33
#define MAXPASSKEYLENGTH 64
#define MAXNETWORKCHLENGTH 1024
//#define MAXBODYLENGTH 1557
//#define MAXHTMLLENGTH 1802
//#define MAXONLOADLENGTH 175

#define MAXBODYLENGTH 1500
#define MAXHTMLLENGTH 1745
#define MAXONLOADLENGTH 128

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

//xxx should be private variables or constants
#define APSSID "configure"
#define OFF HIGH
#define ON LOW

class WiFiSetup {

  public:
    WiFiSetup();
    static void startAccessPoint(unsigned long activeTime);
    static bool connectWiFi();
    void shortBlink();
    static void startWebServer();
    void start();
    void handleClient();
    void printInfo();

    void periodic();

    void toggleAccessPoint();
    
    static void handleRoot();
    static void handleStatus();

    static ESP8266WebServer server;

    static WiFiMode wifimode;

    static byte ledStatus; //LOW means ON, HIGH means OFF
    static const int LED_PIN = BUILTIN_LED; // Wemos blue led


    //some variables to keep track of current state
    static bool runAccessPoint;
    static bool showFailureOnWeb;
    static bool showSuccessOnWeb;
    static bool webServerRunning;
    static bool accessPointStarted;
    static bool tryingToConnect;

    static char html[MAXHTMLLENGTH]; 
    static char htmlstart[];// = "<!doctype html>\r\n<html><head><meta charset='UTF-8'><title>Connect</title></head><body onload=\"";
    static char htmlmid[];// = "\">";
    static char htmlend[];// = "</body></html>";

    static String networks;

    static char WiFiSSID[MAXSSIDLENGTH];
    static char WiFiPSK[MAXPASSKEYLENGTH];

    static unsigned long timeToChangeToSTA; //when is it time time to change to STA (station) in ms. Zero means never



  private:
    

 

//    char html[MAXHTMLLENGTH]; 
 
    const unsigned long checkRate = 5*1000; //how often wifi status is checked
    unsigned long lastCheck = 0;

    const unsigned long apBlinkRate = 100; //blink rate when in AP (access point) mode.
    unsigned long lastApBlink = 0;


    char tempssid[MAXSSIDLENGTH];


    /*
    WL_NO_SHIELD = 255,
    WL_IDLE_STATUS = 0,
    WL_NO_SSID_AVAIL = 1
    WL_SCAN_COMPLETED = 2
    WL_CONNECTED = 3
    WL_CONNECT_FAILED = 4
    WL_CONNECTION_LOST = 5
    WL_DISCONNECTED = 6
    */


};

#endif
