#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char htmlmid[] = "\">";

class Libtest {

  public:
    Libtest(int buttonpin);
    void startAccessPoint(unsigned long activeTime);
    bool connectWiFi();
    void shortBlink();
    void handleRoot();
    void handleStatus();
    void startWebServer();
    void testfunc();

    char test10[10];


  private:
    //xxxESP8266WebServer server(80);

    char htmlstart[];
    //const char htmlmid[] = {'"','>','"'};// "\">";
    //xxxconst char htmlend[] = "</body></html>";

    

    char html[999]; 
    char networkch[999]; 
    char body[999];
    char onload[999];
    String networks;

    const int LED_PIN = BUILTIN_LED; // Wemos blue led
    const int INPUT_PIN = D6; // Digital pin to be read
    const unsigned long checkRate = 15*1000; //how often main loop performs periodical task
    unsigned long lastPost = 0;

    const unsigned long apBlinkRate = 100; //blink rate when in AP (access point) mode.
    unsigned long lastApBlink = 0;

    unsigned long timeToChangeToSTA = 0; //when is it time time to change to STA (station) in ms. Zero means never

    char WiFiSSID[999];
    char WiFiPSK[999];
    char tempssid[999];

    //some variables to keep track of current state
    bool initiateConnection; 
    bool runAccessPoint;
    bool showFailureOnWeb;
    bool showSuccessOnWeb;
    bool webServerRunning;
    bool accessPointStarted;
    bool tryingToConnect;

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

    byte ledStatus; //LOW means ON, HIGH means OFF

};
