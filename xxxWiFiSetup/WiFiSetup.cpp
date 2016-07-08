#include "Arduino.h"
#include "WiFiSetup.h"

ESP8266WebServer WiFiSetup::server=ESP8266WebServer(80);

bool WiFiSetup::runAccessPoint=false;
bool WiFiSetup::showFailureOnWeb=false;
bool WiFiSetup::showSuccessOnWeb=false;
bool WiFiSetup::webServerRunning=false;
bool WiFiSetup::accessPointStarted=false;
bool WiFiSetup::tryingToConnect=false;

char WiFiSetup::html[]="";
char WiFiSetup::htmlstart[] = "<!doctype html>\r\n<html><head><meta charset='UTF-8'><title>Connect</title></head><body onload=\"";
char WiFiSetup::htmlmid[] = "\">";
char WiFiSetup::htmlend[] = "</body></html>";

String WiFiSetup::networks;

char WiFiSetup::WiFiSSID[MAXSSIDLENGTH];
char WiFiSetup::WiFiPSK[MAXPASSKEYLENGTH];

WiFiMode WiFiSetup::wifimode;
byte WiFiSetup::ledStatus;

unsigned long WiFiSetup::timeToChangeToSTA = 0; //when is it time time to change to STA (station) in ms. Zero means never


WiFiSetup::WiFiSetup() {
    //do som init stuff
    if (WFS_DEBUG) {
      Serial.begin(9600);
    }
   
    tempssid[MAXSSIDLENGTH] = '\0';

    strcpy(htmlstart,"<!doctype html>\r\n<html><head><meta charset='UTF-8'><title>Connect</title></head><body onload=\"");
    strcpy(htmlmid,"\">");
    strcpy(htmlend,"</body></html>");

    pinMode(LED_PIN, OUTPUT);
    ledStatus = ON;
    digitalWrite(LED_PIN, ledStatus);

}

void WiFiSetup::start() {
  //start in station mode and try to connect to last known ssid
  wifimode = WIFI_STA;
  WiFi.mode(wifimode);
  delay(7000);
  if (WiFi.status() == WL_CONNECTED) {
    wfs_debugprint("WL_CONNECTED to ");
    wfs_debugprintln(WiFi.SSID());
    ledStatus = OFF;
    digitalWrite(LED_PIN, ledStatus);
  } else {
    wfs_debugprintln("NOT WL_CONNECTED to ");
    wfs_debugprintln(WiFi.SSID());
 
    ledStatus = ON;
    digitalWrite(LED_PIN, ledStatus);
    startAccessPoint(2*60000); //start accesspoint for two minutes if it could not connect
  }

}

void WiFiSetup::handleStatus() {
  wfs_debugprintln("handleStatus");
  //we have the following possibilites:
  //1) this page is called after successful connection
  //2) this page is called after failed connection
  //3) this page is called during connection-but this is usually not handled
  //4) this page is called before connection attemp-redirect to form

  char onload[MAXONLOADLENGTH];
  char body[MAXBODYLENGTH];

  sprintf(onload,""); //not used now, keep for potential future use
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



void WiFiSetup::handleRoot() {
  wfs_debugprintln("handleroot");
  char onload[MAXONLOADLENGTH];
  char networkch[MAXNETWORKCHLENGTH]; 
  char body[MAXBODYLENGTH];

  sprintf(onload,"");

  bool initiateConnection=false;
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
    wfs_debugprintln("has arg ssid");
    if (server.hasArg("pass")) {
      server.arg("pass").toCharArray(WiFiPSK, MAXPASSKEYLENGTH);
    } else {
      sprintf(WiFiPSK,"");
    }
    sprintf(onload,"setInterval(function() {document.getElementById('status').src='status?Math.random()'},15000)");
    sprintf(body,"<p>Connecting to %s...</p><iframe frameborder='0'  id='status' width='480' height='320' src=''></iframe>",WiFiSSID);
  }

  //Don't change the content on any of these variables without checking their size limits!
  sprintf(html, "%s%s%s%s%s", htmlstart, onload, htmlmid, body, htmlend);
  
  server.send(200, "text/html", html);

  if (initiateConnection) {
    if (connectWiFi()) {
      wfs_debugprintln("Connection successful");
      showSuccessOnWeb = true;
      ledStatus = OFF;
      digitalWrite(LED_PIN, ledStatus);
    } else { 
      wfs_debugprintln("Connection failed");
      ledStatus = ON;
      digitalWrite(LED_PIN, ledStatus);
      startAccessPoint(0);
      showFailureOnWeb = true;
    }
  }
}

bool WiFiSetup::connectWiFi() {
  WiFi.disconnect(); //it sometimes crashes if you don't disconnect before new connection
  delay(100);

  tryingToConnect = true;
  bool timeout = false;

  wifimode = WIFI_AP_STA;
  WiFi.mode(wifimode);
  //The connection sometimes fails if the previous ssid is the same as the new ssid.
  //A temporary connection attempt to a non existing network seems to fix this.
  WiFi.begin("hopefullynonexistingssid", "");
  delay(500);
  wfs_debugprint(F("Connecting to "));
  wfs_debugprintln(WiFiSSID);
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
      wfs_debugprint(".");
      // Blink the LED
      ledStatus = !ledStatus;
      digitalWrite(LED_PIN, ledStatus); // Write LED high/low 
      //server.handleClient(); //99% sure this line makes it crash some times. uncomment at your own risk if you wan to handle web requests while ESP is trying to connect to network :)
    }
    delay(delayTime); //a delay is needed when connecting and doing other stuff, otherwise it will crash
    
    if ((loopStart - starttimer) > timelimit) {
      wfs_debugprintln("");
      wfs_debugprint("timed out");
      timeout = true;
    }
  }
  if (!timeout) {
    timeToChangeToSTA = millis() + 2*60000; //keep ap running for 120 s after connection
  }
  wfs_debugprintln("");
  tryingToConnect=false;
  return !timeout;
}

void WiFiSetup::startAccessPoint(unsigned long activeTime) {
  //activeTime is the the time in ms until it returns to WIFI_STA. 0 means stay until forced to leave for exampel when button pressed
  /* You can add password parameter if you don't want the AP to be open. */
  delay(500); //this delay seems to prevent occasional crashes when scanning an already running access point
  wifimode = WIFI_AP;
  WiFi.mode(wifimode); //this delay seems to prevent occasional crashes when scanning an already running access point
  delay(500); //this delay seems to prevent occasional crashes when changing to access point
 
  networks="<option value=''>enter ssid above or select here</option>";
  wfs_debugprintln("start scan");
  int n=WiFi.scanNetworks();
  wfs_debugprintln("scanning complete");
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
    wfs_debugprint("Starting access point mode with IP address ");
    wfs_debugprintln(apip);
    wfs_debugprint("Connect to ");
    wfs_debugprint(APSSID);
    wfs_debugprint(" network and browse to ");
    wfs_debugprintln(apip);
    accessPointStarted = true;
  }

  if (!webServerRunning) {
    startWebServer();
  }
}


void WiFiSetup::startWebServer() {

  wfs_debugprintln("Starting webserver");
  //server.on("/", WiFiSetup::handleRoot);
  server.on("/", handleRoot);

  server.on("/status", handleStatus);
  server.begin();
  webServerRunning = true;  
}

void WiFiSetup::shortBlink() {
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

void WiFiSetup::handleClient() {
  server.handleClient(); 
}


void WiFiSetup::printInfo() {
  //this function is only needed for debugging/testing and can be deleted
  wfs_debugprintln(F("CURRENT STATUS"));
  wfs_debugprint(F("Running for:         "));
  wfs_debugprint(millis()/1000.0/3600.0);
  wfs_debugprintln(F(" hours"));
  //wfs_debugprint(F("initiate connection: "));
  //wfs_debugprintln(initiateConnection);
  wfs_debugprint(F("runAccessPoint:      "));
  wfs_debugprintln(runAccessPoint);
  wfs_debugprint(F("showFailureOnWeb:    "));
  wfs_debugprintln(showFailureOnWeb);
  wfs_debugprint(F("webServerRunning:    "));
  wfs_debugprintln(webServerRunning);
  wfs_debugprint(F("accessPointStarted:  "));
  wfs_debugprintln(accessPointStarted);
  wfs_debugprint(F("showSuccessOnWeb:    "));
  wfs_debugprintln(showSuccessOnWeb);
  wfs_debugprint(F("WiFi Status:         "));
  switch(WiFi.status()){
    case WL_NO_SHIELD: 
      wfs_debugprintln(F("WL_NO_SHIELD"));
      break;
    case WL_IDLE_STATUS:
      wfs_debugprintln(F("WL_IDLE_STATUS"));
      break;
    case WL_NO_SSID_AVAIL:
      wfs_debugprintln(F("WL_NO_SSID_AVAIL"));
      break;
    case WL_SCAN_COMPLETED:
      wfs_debugprintln(F("WL_SCAN_COMPLETED"));
      break;
    case WL_CONNECTED:
      wfs_debugprintln(F("WL_CONNECTED"));
      break;
    case WL_CONNECT_FAILED:
      wfs_debugprintln(F("WL_CONNECT_FAILED"));
      break;
    case WL_CONNECTION_LOST:
      wfs_debugprintln(F("WL_CONNECTION_LOST"));
      break;
    case WL_DISCONNECTED:
      wfs_debugprintln(F("WL_DISCONNECTED"));
      break;
    default:
      wfs_debugprintln(WiFi.status());
  }
  long rssi = WiFi.RSSI();
  wfs_debugprint(F("RSSI:                "));
  wfs_debugprint(rssi);
  wfs_debugprintln(F(" dBm"));
  wfs_debugprint(F("SSID:                "));
  wfs_debugprintln(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  wfs_debugprint(F("Station IP Address:  "));
  wfs_debugprintln(ip);
  ip = WiFi.softAPIP();
  wfs_debugprint(F("AP IP Address:       "));
  wfs_debugprintln(ip);
  wfs_debugprint("WIFI MODE:           ");
  if (wifimode==WIFI_STA) {
    wfs_debugprintln(F("WIFI_STA"));
  } else if (wifimode==WIFI_AP) {
    wfs_debugprintln(F("WIFI_AP"));
  } else if (wifimode==WIFI_AP_STA) {
    wfs_debugprintln(F("WIFI_AP_STA"));
  } else {
    wfs_debugprintln(wifimode);
  }
  byte mac[6];
  WiFi.macAddress(mac);
  wfs_debugprint(F("MAC address:         "));
  wfs_debugprint(mac[0], HEX);
  wfs_debugprint(":");
  wfs_debugprint(mac[1], HEX);
  wfs_debugprint(":");
  wfs_debugprint(mac[2], HEX);
  wfs_debugprint(":");
  wfs_debugprint(mac[3], HEX);
  wfs_debugprint(":");
  wfs_debugprint(mac[4], HEX);
  wfs_debugprint(":");
  wfs_debugprintln(mac[5], HEX);
  wfs_debugprintln("");
}

void WiFiSetup::periodic() {
  unsigned long loopStart=millis();
  //check if it is time to change from configure mode (access point) to normal mode (station)

  
  if (timeToChangeToSTA != 0 && timeToChangeToSTA < loopStart && wifimode != WIFI_STA) {
    WiFiMode prevMode = wifimode;
    wifimode = WIFI_STA;
    WiFi.mode(wifimode);
    if (prevMode == WIFI_AP) {
      WiFi.begin(); //reconnectes to previous SSID and PASSKEY if it has been in AP-mode
    }
    wfs_debugprintln("changing to STA");
    timeToChangeToSTA = 0; // Zero means don't change to STA
  }

  if (wifimode != WIFI_STA) {
    //only handle web requests in access point mode
    server.handleClient();
    delay(5);
  }

  if ((wifimode != WIFI_AP) && (lastCheck + checkRate <= loopStart)) {
    lastCheck=loopStart;
    if (WiFi.status() == WL_CONNECTED) {
      ledStatus = OFF;
      digitalWrite(LED_PIN, ledStatus);
    } else {
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
  
}

void WiFiSetup::toggleAccessPoint() {
  if (wifimode == WIFI_STA) {
      startAccessPoint(0);
    } else {
      timeToChangeToSTA = millis();
    }  
}

