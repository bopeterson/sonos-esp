//Use webbrowser to configure and connect ESP8266 to wifi network
//It has only been tested with Wemos D1 board (www.wemos.cc)

/*
Instructions for connecting to access point

1. Connect the ESP8266 to power, and connect a pushbutton between ground and input pin D6. Internal pullup is used, no external resistor needed
2. The ESP tries to connect to last known network. If it works, the built in led will turn off, but give a short blink every 15th second to indicate it is alive
3. If it does not succed the ESP goes to configure state for two minutes after failed connection attempt. The ESP can also be taken to configure state at any time by pressing the button
4. When the ESP is configure state: The built in led will start blinking 5 blinks per soecond and the ESP is now a wifi access point with SSID=configure
5. Connect a computer to the wifi network "configure"
6. Open a browser and go to http://192.168.4.1
7. Enter ssid and password in webform and connect. The led blinks a  little slower
8. A messege should appear if you succeeded or not.
9. If you succeeded, the ESP leaves configure state and is now connected
10. If you did not succeed you can try again
11. If you pressed the button accidentaly, you can press it again to leave configure mode
12. If it the ESP did not succeed in connecting to a network, the built in led will turn on, and give a short blink every  15th second

LED indication:
- OFF with short blink every 15th second: connected
- ON  with short blink every 15th second: not connected
- Blinking 5 blinks per second: ESP in configuration mode. Network ssid and password can be configured through web browser
- Blinking a little slower: ESP is trying to connect
*/

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

//should maybe be private variables or constants if possible
#define WFS_MAXSSIDLENGTH 33
#define WFS_MAXPASSKEYLENGTH 64
#define WFS_MAXNETWORKCHLENGTH 1024

#define WFS_MAXBODYLENGTH 1500
#define WFS_MAXHTMLLENGTH 1745
#define WFS_MAXONLOADLENGTH 128

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

class WiFiSetup {

  public:
    WiFiSetup(int led_pin);
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

    static void ledWrite(int status);


    static ESP8266WebServer server;

    static WiFiMode wifimode;

    static byte ledStatus; //LOW means ON, HIGH means OFF
    static int _led_pin;


    //some variables to keep track of current state
    static bool runAccessPoint;
    static bool showFailureOnWeb;
    static bool showSuccessOnWeb;
    static bool webServerRunning;
    static bool accessPointStarted;
    static bool tryingToConnect;

    const static char htmlstart[];
    const static char htmlmid[];
    const static char htmlend[];

    static String networks;

    static char WiFiSSID[WFS_MAXSSIDLENGTH];
    static char WiFiPSK[WFS_MAXPASSKEYLENGTH];

    static unsigned long timeToChangeToSTA; //when is it time time to change to STA (station) in ms. Zero means never

    static const byte OFF;
    static const byte ON;


  private:
    
    const unsigned long checkRate = 10*1000; //how often wifi status is checked and led blinked
    unsigned long lastCheck = 0;

    const unsigned long apBlinkRate = 100; //blink rate when in AP (access point) mode.
    unsigned long lastApBlink = 0;


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
