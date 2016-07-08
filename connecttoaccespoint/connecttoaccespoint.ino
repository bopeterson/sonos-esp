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

#define DEBUG true

//the name of the network you use for configuration
#define APSSID "configure"

#if DEBUG
    #define debugprint(...) Serial.print(__VA_ARGS__)
    #define debugprintln(...) Serial.println(__VA_ARGS__)
#else
    #define debugprint(...)
    #define debugprintln(...)
#endif

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

//ssid: allow for 32 chars + null termination
//passkey: allow for 63 chars + null termination
//html: length of htmlstart+onload+htmlmid+body+htmlend (at the moment 98+128+4+1500+15 = 1745)
//networkc: truncate if there are more networks
//body: max length of networkch + 2 * max length of WiFiSSID + fixed part (at the moment 1024 + 2*32 +367 = 1455)
#define MAXSSIDLENGTH 33
#define MAXPASSKEYLENGTH 64
#define MAXNETWORKCHLENGTH 1024
#define MAXBODYLENGTH 1500
#define MAXHTMLLENGTH 1745
#define MAXONLOADLENGTH 128

#define OFF HIGH
#define ON LOW
#define PRESSED LOW

void startAccessPoint(unsigned long activeTime);
bool connectWiFi();
void shortBlink();
void handleRoot();
void handleStatus();
bool readConfigureButton();
void printInfo();
void startWebServer();

const char compiletime[] = __TIME__;
const char compiledate[] = __DATE__;

ESP8266WebServer server(80);

const char htmlstart[] = "<!doctype html>\r\n<html><head><meta charset='UTF-8'><title>Connect</title></head><body onload=\"";
const char htmlmid[] = "\">";
const char htmlend[] = "</body></html>";

char html[MAXHTMLLENGTH]; 
char networkch[MAXNETWORKCHLENGTH]; 
char body[MAXBODYLENGTH];
char onload[MAXONLOADLENGTH];
String networks;

const int LED_PIN = BUILTIN_LED; // Wemos blue led
const int INPUT_PIN = D6; // Digital pin to be read
const unsigned long checkRate = 15*1000; //how often main loop performs periodical task
unsigned long lastPost = 0;

const unsigned long apBlinkRate = 100; //blink rate when in AP (access point) mode.
unsigned long lastApBlink = 0;

unsigned long timeToChangeToSTA = 0; //when is it time time to change to STA (station) in ms. Zero means never

char WiFiSSID[MAXSSIDLENGTH] = "";
char WiFiPSK[MAXPASSKEYLENGTH] = "";
char tempssid[MAXSSIDLENGTH] = "";

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
WiFiMode wifimode;

byte ledStatus; //LOW means ON, HIGH means OFF

void setup() {
  if (DEBUG) {
    Serial.begin(9600);
  }
  debugprintln("");
  debugprint("Compile time and date: ");
  debugprint(compiletime);
  debugprint(" ");
  debugprintln(compiledate);

  //initialize inputs and outputs
  pinMode(INPUT_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  ledStatus = ON;
  digitalWrite(LED_PIN, ledStatus);

  //start in station mode and try to connect to last known ssid
  wifimode = WIFI_STA;
  WiFi.mode(wifimode);
  delay(7000);
  if (WiFi.status() == WL_CONNECTED) {
    debugprint("WL_CONNECTED to ");
    debugprintln(WiFi.SSID());
    ledStatus = OFF;
    digitalWrite(LED_PIN, ledStatus);

  } else {
    debugprintln("NOT WL_CONNECTED to ");
    debugprintln(WiFi.SSID());
 
    ledStatus = ON;
    digitalWrite(LED_PIN, ledStatus);
    startAccessPoint(2*60000); //start accesspoint for two minutes if it could not connect
  }

  runAccessPoint = false;
  initiateConnection = false;
  showFailureOnWeb = false;
  showSuccessOnWeb = false;
  accessPointStarted = false;
  webServerRunning = false;
  webServerRunning = false;
  tryingToConnect = false;

  lastPost = millis() - checkRate;
}

void loop() {
  unsigned long loopStart = millis();

  //check if it is time to change from configure mode (access point) to normal mode (station)
  if (timeToChangeToSTA != 0 && timeToChangeToSTA < loopStart && wifimode != WIFI_STA) {
    WiFiMode prevMode = wifimode;
    wifimode = WIFI_STA;
    WiFi.mode(wifimode);
    if (prevMode == WIFI_AP) {
      WiFi.begin(); //reconnectes to previous SSID and PASSKEY if it has been in AP-mode
    }
    debugprintln("changing to STA");
    timeToChangeToSTA = 0; // Zero means don't change to STA
  }

  //button toggles between station and accesspoint
  if (readConfigureButton()) {
    if (wifimode == WIFI_STA) {
      runAccessPoint = true;
      initiateConnection = false;
      debugprintln("BUTTON PRESSED, RUN ACCESS POINT");
    } else {
      timeToChangeToSTA = loopStart;
      debugprintln("BUTTON PRESSED, CHANGE TO STA");
    }
  }
  
  if (runAccessPoint) {
    startAccessPoint(0);
    runAccessPoint = false;
  };

  if (wifimode != WIFI_STA) {
    //only handle web requests in access point mode
    server.handleClient();
  }

  if (initiateConnection) {
    initiateConnection = false;
    WiFi.disconnect(); //it sometimes crashes if you don't disconnect before new connection
    delay(100);
    if (connectWiFi()) {
      debugprintln("Connection successful");
      showSuccessOnWeb = true;
      ledStatus = OFF;
      digitalWrite(LED_PIN, ledStatus);
    } else { 
      debugprintln("Connection failed");
      runAccessPoint = true;
      showFailureOnWeb = true;
      ledStatus = ON;
      digitalWrite(LED_PIN, ledStatus);
    }
  }

  if ((wifimode == WIFI_AP) && (lastApBlink + apBlinkRate <= loopStart)) {
    //blink led to indicate access point mode
    lastApBlink = loopStart;
    ledStatus = !ledStatus;
    digitalWrite(LED_PIN, ledStatus);
  }

  if (lastPost + checkRate <= loopStart) {
    lastPost = loopStart;
    //put any code here that should run periodically


    //show wifi-status. OFF = connected, ON = disconnected
    if (WiFi.status() == WL_CONNECTED) {
      ledStatus = OFF;
      digitalWrite(LED_PIN, ledStatus);
    } else {
      ledStatus = ON;
      digitalWrite(LED_PIN, ledStatus);
    }

    //make a short blink periodically to signal that the device is alive
    shortBlink();

    printInfo();
  }
}


void printInfo() {
  //this function is only needed for debugging/testing and can be deleted
  debugprintln(F("CURRENT STATUS"));
  debugprint(F("Running for:         "));
  debugprint(millis()/1000.0/3600.0);
  debugprintln(F(" hours"));
  debugprint(F("initiate connection: "));
  debugprintln(initiateConnection);
  debugprint(F("runAccessPoint:      "));
  debugprintln(runAccessPoint);
  debugprint(F("showFailureOnWeb:    "));
  debugprintln(showFailureOnWeb);
  debugprint(F("webServerRunning:    "));
  debugprintln(webServerRunning);
  debugprint(F("accessPointStarted:  "));
  debugprintln(accessPointStarted);
  debugprint(F("showSuccessOnWeb:    "));
  debugprintln(showSuccessOnWeb);
  debugprint(F("WiFi Status:         "));
  switch(WiFi.status()){
    case WL_NO_SHIELD: 
      debugprintln(F("WL_NO_SHIELD"));
      break;
    case WL_IDLE_STATUS:
      debugprintln(F("WL_IDLE_STATUS"));
      break;
    case WL_NO_SSID_AVAIL:
      debugprintln(F("WL_NO_SSID_AVAIL"));
      break;
    case WL_SCAN_COMPLETED:
      debugprintln(F("WL_SCAN_COMPLETED"));
      break;
    case WL_CONNECTED:
      debugprintln(F("WL_CONNECTED"));
      break;
    case WL_CONNECT_FAILED:
      debugprintln(F("WL_CONNECT_FAILED"));
      break;
    case WL_CONNECTION_LOST:
      debugprintln(F("WL_CONNECTION_LOST"));
      break;
    case WL_DISCONNECTED:
      debugprintln(F("WL_DISCONNECTED"));
      break;
    default:
      debugprintln(WiFi.status());
  }
  long rssi = WiFi.RSSI();
  debugprint(F("RSSI:                "));
  debugprint(rssi);
  debugprintln(F(" dBm"));
  debugprint(F("SSID:                "));
  debugprintln(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  debugprint(F("Station IP Address:  "));
  debugprintln(ip);
  ip = WiFi.softAPIP();
  debugprint(F("AP IP Address:       "));
  debugprintln(ip);
  debugprint(F("INPUT_PIN:           "));
  debugprintln(digitalRead(INPUT_PIN)==PRESSED ? "pressed" : "not pressed");

  debugprint("WIFI MODE:           ");
  if (wifimode==WIFI_STA) {
    debugprintln(F("WIFI_STA"));
  } else if (wifimode==WIFI_AP) {
    debugprintln(F("WIFI_AP"));
  } else if (wifimode==WIFI_AP_STA) {
    debugprintln(F("WIFI_AP_STA"));
  } else {
    debugprintln(wifimode);
  }
  byte mac[6];
  WiFi.macAddress(mac);
  debugprint(F("MAC address:         "));
  debugprint(mac[0], HEX);
  debugprint(":");
  debugprint(mac[1], HEX);
  debugprint(":");
  debugprint(mac[2], HEX);
  debugprint(":");
  debugprint(mac[3], HEX);
  debugprint(":");
  debugprint(mac[4], HEX);
  debugprint(":");
  debugprintln(mac[5], HEX);
  debugprintln("");
}

void handleStatus() {
  //we have the following possibilites:
  //1) this page is called after successful connection
  //2) this page is called after failed connection
  //3) this page is called during connection-but this is usually not handled
  //4) this page is called before connection attemp-redirect to form
 
  sprintf(onload,"");
  if (showSuccessOnWeb) {
    sprintf(body, "Successfully connected to %s", WiFiSSID);
  } else if (showFailureOnWeb) {
    sprintf(body, "Could not connect to %s. Maybe wrong ssid or password.  <a target='_parent' href='/'>Try again</a>", WiFiSSID);
  } else if (tryingToConnect) 
    sprintf(body, "Connecting...");
  else {
    sprintf(body, "<script>window.parent.location.href='/';</script>");
  }
  sprintf(html, "%s%s%s%s%s", htmlstart, onload, htmlmid, body, htmlend);
  server.send(200, "text/html", html);
}

void handleRoot() {
  sprintf(onload,"");
  if (!server.hasArg("ssid") && WiFi.status() != WL_CONNECTED) {
    networks.toCharArray(networkch,MAXNETWORKCHLENGTH);
    //note that html element arguments may be double qouted, single quoted or unquoted. Unquoted is usually not recommended but used here to save memory
    sprintf(body, "<p>Not connected</p><form method=post action='/'><input type=text name=ssid value='%s' size=32 id=s1 onchange='document.getElementById(\"s2\").value=this.value'> SSID (network name)<br><select name=s2 id=s2 onchange='document.getElementById(\"s1\").value=this.value'>%s</select><br><input type=password name=pass size=32> PASSWORD<br><input type=submit value=connect></form>", WiFiSSID,networkch);
  } else if (!server.hasArg("ssid")) {
    sprintf(body,"<p>Connected to %s</p><form method=post action='/'><input type=text name=ssid value='%s' size=32 id=s1 onchange='document.getElementById(\"s2\").value=this.value'> SSID (network name)<br><select name=s2 id=s2 onchange='document.getElementById(\"s1\").value=this.value'>%s</select><br><input type=password name=pass size=32> PASSWORD<br><input type=submit value=connect></form>",WiFiSSID,WiFiSSID,networkch);
  } else { //server has arg ssid
    showFailureOnWeb = false;
    showSuccessOnWeb = false;
    initiateConnection = true;
    server.arg("ssid").toCharArray(WiFiSSID, MAXSSIDLENGTH);
    debugprintln("has arg ssid");
    if (server.hasArg("pass")) {
      server.arg("pass").toCharArray(WiFiPSK, MAXPASSKEYLENGTH);
    } else {
      sprintf(WiFiPSK,"");
    }
    sprintf(onload,"setInterval(function() {document.getElementById('status').src='status?Math.random()'},15000)");
    sprintf(body,"<p>Connecting to %s...</p><iframe frameborder='0'  id='status' width='480' height='320' src=''></iframe>",WiFiSSID);
  }
  sprintf(html, "%s%s%s%s%s", htmlstart, onload, htmlmid, body, htmlend);
  server.send(200, "text/html", html);
}

bool connectWiFi() {
  tryingToConnect = true;
  bool timeout = false;

  wifimode = WIFI_AP_STA;
  WiFi.mode(wifimode);
  //The connection sometimes fails if the previous ssid is the same as the new ssid.
  //A temporary connection attempt to a non existing network seems to fix this.
  WiFi.begin("hopefullynonexistingssid", "");
  delay(500);
  debugprint(F("Connecting to "));
  debugprintln(WiFiSSID);
  WiFi.begin(WiFiSSID, WiFiPSK);
  // Use the WiFi.status() function to check if the ESP8266
  // is connected to a WiFi network. Timeout after one minute.
  unsigned long timelimit = 60000;
  unsigned long starttimer = millis();
  int delayTime=200;
  int period = 600; //blink period and check if connected period. should be multiple of delayTime
  bool connected=false;
  unsigned long lastCheck=0;
  while (!connected && !timeout) {
    unsigned long loopStart = millis();
    if (lastCheck + period <= loopStart) {
      lastCheck = loopStart;
      connected = (WiFi.status() == WL_CONNECTED);
      debugprint(".");
      // Blink the LED
      ledStatus = !ledStatus;
      digitalWrite(LED_PIN, ledStatus); // Write LED high/low 
      //server.handleClient(); //99% sure this line makes it crash some times. uncomment at your own risk if you wan to handle web requests while ESP is trying to connect to network :)
    }
    delay(delayTime); //a delay is needed when connecting and doing other stuff, otherwise it will crash
    
    if ((loopStart - starttimer) > timelimit) {
      debugprintln("");
      debugprint("timed out");
      timeout = true;
    }
  }
  if (!timeout) {
    timeToChangeToSTA = millis() + 2*60000; //keep ap running for 120 s after connection
  }
  debugprintln("");
  tryingToConnect=false;
  return !timeout;
}

void startAccessPoint(unsigned long activeTime) {
  //activeTime is the the time in ms until it returns to WIFI_STA. 0 means stay until forced to leave for exampel when button pressed
  /* You can add password parameter if you don't want the AP to be open. */
  delay(500); //this delay seems to prevent occasional crashes when scanning an already running access point
  wifimode = WIFI_AP;
  WiFi.mode(wifimode); //this delay seems to prevent occasional crashes when scanning an already running access point
  delay(500); //this delay seems to prevent occasional crashes when changing to access point
 
  networks="<option value=''>enter ssid above or select here</option>";
  debugprintln("start scan");
  int n=WiFi.scanNetworks();
  debugprintln("scanning complete");
  if (n>0) {
    for (int i = 0; i < n; ++i) {
      if (networks.length()+17+WiFi.SSID(i).length()<MAXNETWORKCHLENGTH) {
        //if more networks than what fits in networksch char array are found, skip them
        networks=networks+"<option>"+WiFi.SSID(i)+"</option>";
      }
    }
  }
  if (activeTime==0) {
    timeToChangeToSTA = 0;
  } else {
    timeToChangeToSTA = millis()+activeTime;  
  }
  
  if (!accessPointStarted) {
    IPAddress stationip(192, 168, 4, 1);//this is the default but added manually just to be sure it doesn't change in the future
    IPAddress NMask(255, 255, 255, 0);
    WiFi.softAPConfig(stationip, stationip, NMask);    
    WiFi.softAP(APSSID);
    IPAddress apip = WiFi.softAPIP();
    debugprint("Starting access point mode with IP address ");
    debugprintln(apip);
    debugprint("Connect to ");
    debugprint(APSSID);
    debugprint(" network and browse to ");
    debugprintln(apip);
    accessPointStarted = true;
  }

  if (!webServerRunning) {
    startWebServer();
  }
}

void startWebServer() {
  debugprintln("Starting webserver");
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.begin();
  webServerRunning = true;  
}

void shortBlink() {
  ledStatus = !ledStatus;
  digitalWrite(LED_PIN, ledStatus);
  if (ledStatus == OFF) {
    delay(30); //longer pulse needed when going ON-OFF-ON
  } else {
    delay(5);
  }
  ledStatus = !ledStatus;
  digitalWrite(LED_PIN, ledStatus);
}

//global variables needed for readConfigureButton
//INPUT_PIN defined earlier
int buttonState = HIGH;             // the current reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
int lastButtonState = HIGH;   // the previous reading from the input pin
bool readConfigureButton() {
  //button connected to pullup and ground, ie LOW=PRESSED
  unsigned long debounceDelay = 50;
  int reading = digitalRead(INPUT_PIN);
  int pressDetected=false;
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        pressDetected=true;
      }
    }
  }
  lastButtonState = reading;  
  return pressDetected;
}

