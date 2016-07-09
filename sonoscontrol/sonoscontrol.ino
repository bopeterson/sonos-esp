// Include the ESP8266 WiFi library. (Works a lot like the
// Arduino WiFi library.)
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#define BUFSIZ 512

const char compiletime[]=__TIME__;
const char compiledate[]=__DATE__;

char response[BUFSIZ];
char filtered[256];

void removeAllTracksFromQueue(); 
void pause(IPAddress device);
byte getVolume(IPAddress device);
void setVolume(byte vol,IPAddress device);
void sonosAction(const char *url, const char *service, const char *action, const char *arguments,IPAddress device);
void filter(const char *starttag,const char *endtag);
int string2int(const char *s);
int discoverSonos();

IPAddress deviceIPs[10];
int numberOfDevices;

WiFiClient client;

IPAddress foundDevice;

const int LED_PIN = BUILTIN_LED; // Wemos blue led
const int ANALOG_PIN = A0; //
const int DIGITAL_PIN = 12; // Digital pin to be read

const unsigned long postRate = 10*1000;
unsigned long lastPost = 0;

//encoder variables
const int pinA = D3;  // Connected to CLK on KY-040
const int pinB = D7;  // Connected to DT on KY-040
int encoderPosCount = 0;
int pinALast;
int aVal;
int encoderRelativeCount;
unsigned long moveTime;

void setup() {
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



void pause(IPAddress device) {
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



