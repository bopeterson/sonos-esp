#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUDP.h>

//general definitions
#define DEBUG true

#if DEBUG
    #define debugprint(...) Serial.print(__VA_ARGS__)
    #define debugprintln(...) Serial.println(__VA_ARGS__)
#else
    #define debugprint(...)
    #define debugprintln(...)
#endif

#define OFF HIGH
#define ON LOW
#define PRESSED LOW


//definitions for web configuration
//the name of the network you use for configuration
#define APSSID "configure"

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


//definitions for sonos
#define BUFSIZ 512

//function headers for web configuration
void startAccessPoint(unsigned long activeTime);
bool connectWiFi();
void shortBlink();
void handleRoot();
void handleStatus();
bool readConfigureButton();
void printInfo();
void startWebServer();

//function headers for sonos
void removeAllTracksFromQueue(); 
void pause(IPAddress device);
byte getVolume(IPAddress device);
void setVolume(byte vol,IPAddress device);
void sonosAction(const char *url, const char *service, const char *action, const char *arguments,IPAddress device);
void filter(const char *starttag,const char *endtag);
int string2int(const char *s);
int discoverSonos();

//general global variables and constants
const char compiletime[] = __TIME__;
const char compiledate[] = __DATE__;
const int LED_PIN = BUILTIN_LED; // Wemos blue led
const int INPUT_PIN = D6; // Digital pin to be read
byte ledStatus; //LOW means ON, HIGH means OFF

//global variables and constants for web configuration
ESP8266WebServer server(80);
const char htmlstart[] = "<!doctype html>\r\n<html><head><meta charset='UTF-8'><title>Connect</title></head><body onload=\"";
const char htmlmid[] = "\">";
const char htmlend[] = "</body></html>";

char html[MAXHTMLLENGTH]; 
char networkch[MAXNETWORKCHLENGTH]; 
char body[MAXBODYLENGTH];
char onload[MAXONLOADLENGTH];
String networks;

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


//global variables and constants for sonos
char response[BUFSIZ];
char filtered[256];

IPAddress deviceIPs[10];
int numberOfDevices;

WiFiClient client;

IPAddress foundDevice;

//xxx const int LED_PIN = BUILTIN_LED; // Wemos blue led
const int ANALOG_PIN = A0; // not used now
const int DIGITAL_PIN = 12; // Digital pin to be read not used yet

const unsigned long postRate = 10*1000;
//xxx unsigned long lastPost = 0;

//variables and constants for encoder
const int pinA = D3;  // Connected to CLK on KY-040
const int pinB = D7;  // Connected to DT on KY-040
int encoderPosCount = 0;
int pinALast;
int aVal;
int encoderRelativeCount;
unsigned long moveTime;

void setup() {

xxxx här måste jag tänka lite vad som ska göras i setup och vad i loop. ska den bara göra connect i setup? eller ska den bara göra allt i loopen utom första setup?
ja, måste nog flytta discover devices till loopen. eg ska man ju i webinterface kunna välja devices....
nu blir det lite svårt. 

alltså... måste nog göra de olika delarna till libraries, annars blir det för kladdigt. är det rimligt????

  
  Serial.begin(9600);
  Serial.println(compiletime);
  Serial.println(compiledate);
  pinMode(DIGITAL_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.println("Starting...");

  //connectWiFi();
  delay(7000); //wait for connection to wifi 
  
  discoverSonos();
  
  Serial.print("Found devices: ");
  Serial.println(numberOfDevices);

  if (numberOfDevices>0) {
    foundDevice=deviceIPs[0];
  } else {
    foundDevice.fromString("0.0.0.0");
  }
  Serial.println(foundDevice);
  digitalWrite(LED_PIN, LOW);
  Serial.println("ready to go"); 
  lastPost = millis() - postRate;


  //encoder setup
  pinMode (pinA, INPUT);
  pinMode (pinB, INPUT);
  pinALast = digitalRead(pinA);
}

byte ledStatus2 = LOW;

void loop() {
  //read knob
  aVal = digitalRead(pinA);
  if (aVal != pinALast) { // Means the knob is rotating
    moveTime=millis()+150;
    // if the knob is rotating, we need to determine direction
    // We do that by reading pin B.
    if (digitalRead(pinB) != aVal) {  // Means pin A Changed first - We're Rotating Clockwise
      encoderPosCount++;
      encoderRelativeCount++;
    } else {// Otherwise B changed first and we're moving CCW
      encoderPosCount--;
      encoderRelativeCount--;
    }
    
    Serial.print("Encoder Position: ");
    Serial.println(encoderPosCount);

  }
  pinALast = aVal;

  if (millis()>moveTime && moveTime!=0) {
    int oldVolume=getVolume(foundDevice);
    Serial.print(oldVolume);
    //xxx handle timeout in getVolume
    int newVolume=oldVolume+2*encoderRelativeCount;
    newVolume=constrain(newVolume,0,100);
    Serial.print(" ");
    Serial.print(newVolume);
    Serial.print(" ");
    Serial.println(encoderRelativeCount);
    setVolume(newVolume,foundDevice);
    moveTime=0; 
    encoderRelativeCount=0;
  }



  /*
  if (lastPost + postRate <= millis()) {    
     lastPost = millis();
     
     int readvol=getVolume(foundDevice);
     
     Serial.println("Here is the response ");
     Serial.print("Running for ");
     Serial.print(millis()/1000.0/3600.0);
     Serial.println(" hours");
     Serial.println(readvol);
     Serial.println("End of response");
     Serial.println("");

     //removeAllTracksFromQueue(deviceKitchen);
     //Serial.println(response);
  }
  */
  
  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    client.stop();
  }
}



void pause(IPAddress device) { //xxx hellre pause(device)
  const char url[] = "/AVTransport/Control";
  const char service[] = "AVTransport:1";
  const char action[] = "Pause";
  const char arguments[] = "<InstanceID>0</InstanceID>";
  sonosAction(url,service,action,arguments,device);
}

byte getVolume(IPAddress device) {
  const char url[] = "/RenderingControl/Control";
  const char service[] = "RenderingControl:1";
  const char action[] = "GetVolume";
  const char arguments[] = "<InstanceID>0</InstanceID><Channel>Master</Channel>";
  sonosAction(url,service,action,arguments,device);
  filter("<CurrentVolume>","</CurrentVolume>");
  return string2int(filtered);
}

void removeAllTracksFromQueue(IPAddress device) { 
  const char url[] = "/AVTransport/Control";
  const char service[] = "AVTransport:1";
  const char action[] = "RemoveAllTracksFromQueue";
  const char arguments[] = "<InstanceID>0</InstanceID>";
  sonosAction(url,service,action,arguments,device);
}

int string2int(const char *s) {
  char *ptr; //not used but needed in strtol
  int ret = strtol(s, &ptr, 10);      
  return ret;
}

void filter(const char *starttag,const char *endtag) {
  //looks in global var response for content between starttag and endtag and "returns" it in global var filtered
  char * startpart=strstr(response,starttag);
  char * endpart=strstr(response,endtag);

  if (startpart!=0 && endpart!=0) {
    int startindex=startpart-response+strlen(starttag);
    int endindex=endpart-response;
 
    strncpy(filtered,response+startindex,endindex-startindex);
    filtered[endindex-startindex]='\0';
  } else {
    filtered[0]='\0'; //empty string
  }
}


void setVolume(byte vol,IPAddress device) {
  const char url[] = "/RenderingControl/Control";
  const char service[] = "RenderingControl:1";
  const char action[] = "SetVolume";
  char arguments[512];
  sprintf(arguments, "<InstanceID>0</InstanceID><Channel>Master</Channel><DesiredVolume>%d</DesiredVolume>", vol);  
  sonosAction(url,service,action,arguments,device);
}

void sonosAction(const char *url, const char *service, const char *action, const char *arguments,IPAddress device) {
  if (client.connect(device, 1400)) {
    client.print("POST /MediaRenderer");
    client.print(url); //skiljer
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(device); //note: device is of type IPAddress which converts to int
    client.print("Content-Length: "); //samma
    int contlen = 124+8+3+strlen(action)+39+strlen(service)+2+strlen(arguments)+4+strlen(action)+1+9+13;
    client.println(contlen);
    client.println("Content-Type: text/xml; charset=utf-8");
    client.print("Soapaction: ");
    client.print("urn:schemas-upnp-org:service:");
    client.print(service);
    client.print("#");
    client.println(action);
    client.println("Connection: close"); //samma
    client.println(""); //samma
    client.print("<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" ");
    client.print("s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">");
    client.print("<s:Body>");
    client.print("<u:");
    client.print(action);
    client.print(" xmlns:u=\"urn:schemas-upnp-org:service:");
    client.print(service);
    client.print("\">");
    client.print(arguments);
    client.print("</u:");
    client.print(action);
    client.print(">");
    client.print("</s:Body>");
    client.println("</s:Envelope>");  

    //wait if client not immediately available
    unsigned long starttimer=millis();
    unsigned long timelimit=10000;
    bool timeout = false;
    while (!client.available() && !timeout) {
      //wait until client available
      //Serial.println("waiting for client");
      delay(100);
      if ((millis() - starttimer) > timelimit) {
        Serial.println("timed out part 1 ");
        timeout=true;
      }
    }
    //timeout shall keep current state for the coming while-loop. Only starttimer should be updated

    starttimer=millis();
    int index=0;

    while (client.available()  && !timeout) {
      char c = client.read();
      response[index]=c;
      index++;
      if (index >= BUFSIZ) { 
        index = BUFSIZ -1;
      }

      //Serial.write(c);
      if ((millis() - starttimer) > timelimit) {
        Serial.println("timed out part 2");
        timeout=true;
      }
    }
    if (timeout) {
      client.stop();
    }
    response[index]='\0'; //end of string
  }  
}

int discoverSonos(){
    numberOfDevices=0;
    WiFiUDP Udp;
    Udp.begin(1900);
    IPAddress sonosIP;
    bool timedOut = false;
    unsigned long timeLimit = 15000;
    unsigned long firstSearch = millis();
    do {
        Serial.println("Sending M-SEARCH multicast");
        Udp.beginPacketMulticast(IPAddress(239, 255, 255, 250), 1900, WiFi.localIP());
        Udp.write("M-SEARCH * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "MAN: \"ssdp:discover\"\r\n"
        "MX: 1\r\n"
        "ST: urn:schemas-upnp-org:device:ZonePlayer:1\r\n");
        Udp.endPacket();
        unsigned long lastSearch = millis();

        while((millis() - lastSearch) < 5000){
            int packetSize = Udp.parsePacket();
            if(packetSize){
                char packetBuffer[255];
                //Serial.print("Received packet of size ");
                //Serial.println(packetSize);
                //Serial.print("From ");
                sonosIP = Udp.remoteIP();

                //xxx if new IP, it should be put in an array

                addIp(sonosIP);
                //found = true; 
                Serial.print(sonosIP);
                Serial.print(", port ");
                Serial.println(Udp.remotePort());
                
                // read the packet into packetBufffer
                int len = Udp.read(packetBuffer, 255);
                if (len > 0) {
                    packetBuffer[len] = 0;
                }
                //Serial.println("Contents:");
                //Serial.println(packetBuffer);
            }
            delay(50);
        }
    } while((millis()-firstSearch)<timeLimit);
    //if (!found) {
      //sonosIP.fromString("0.0.0.0"); xxx 
    //}
    return numberOfDevices;
}

//uses global variables:
//IPAddress deviceIPs[10];
//int numberOfDevices;
bool addIp(IPAddress ip) {
  bool found=false;
  bool added=false;
  if (numberOfDevices<10) { //must agree with definition of global array deviceIPs
    for (int i=0;i<numberOfDevices && !found;i++) {
      if (ip==deviceIPs[i]) {
        found=true;     
      }
    }
    if (!found) {
      deviceIPs[numberOfDevices]=ip;
      numberOfDevices++;
      added=true;
    }
  } 
 
  return added;
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
  
  timeToChangeToSTA = activeTime;
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


/*
IPAddress discoverSonosOld(){
    WiFiUDP Udp;
    Udp.begin(1900);
    IPAddress sonosIP;
    bool found = false;
    bool timedOut = false;
    unsigned long timeLimit = 60000;
    unsigned long firstSearch = millis();
    do {
        Serial.println("Sending M-SEARCH multicast");
        Udp.beginPacketMulticast(IPAddress(239, 255, 255, 250), 1900, WiFi.localIP());
        Udp.write("M-SEARCH * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "MAN: \"ssdp:discover\"\r\n"
        "MX: 1\r\n"
        "ST: urn:schemas-upnp-org:device:ZonePlayer:1\r\n");
        Udp.endPacket();
        unsigned long lastSearch = millis();

        while(!found && (millis() - lastSearch) < 5000){
            int packetSize = Udp.parsePacket();
            if(packetSize){
                char packetBuffer[255];
                //Serial.print("Received packet of size ");
                //Serial.println(packetSize);
                //Serial.print("From ");
                sonosIP = Udp.remoteIP();
                found = true;
                Serial.print(sonosIP);
                //Serial.print(", port ");
                //Serial.println(Udp.remotePort());
                
                // read the packet into packetBufffer
                int len = Udp.read(packetBuffer, 255);
                if (len > 0) {
                    packetBuffer[len] = 0;
                }
                //Serial.println("Contents:");
                //Serial.println(packetBuffer);
            }
            delay(50);
        }
    } while(!found && (millis()-firstSearch)<timeLimit);
    if (!found) {
      sonosIP.fromString("0.0.0.0");
    }
    return sonosIP;
}
*/



