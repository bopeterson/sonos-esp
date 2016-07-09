#include "Arduino.h"
#include "SonosEsp.h"

SonosEsp::SonosEsp() {
  _numberOfDevices=0;
}

void SonosEsp::pause(int device) {
  const char url[] = "/AVTransport/Control";
  const char service[] = "AVTransport:1";
  const char action[] = "Pause";
  const char arguments[] = "<InstanceID>0</InstanceID>";
  sonosAction(url,service,action,arguments,device);
}

void SonosEsp::play(int device) {
  const char url[] = "/AVTransport/Control";
  const char service[] = "AVTransport:1";
  const char action[] = "Play";
  const char arguments[] = "<InstanceID>0</InstanceID><Speed>1</Speed>";
  sonosAction(url,service,action,arguments,device);
}


byte SonosEsp::getVolume(int device) {
  const char url[] = "/RenderingControl/Control";
  const char service[] = "RenderingControl:1";
  const char action[] = "GetVolume";
  const char arguments[] = "<InstanceID>0</InstanceID><Channel>Master</Channel>";
  sonosAction(url,service,action,arguments,device);
  filter("<CurrentVolume>","</CurrentVolume>");
  return string2int(_filtered);
}

String SonosEsp::getTransportInfo(int device) {
 //returns STOPPED, PLAYING or PAUSED_PLAYBACK
  const char url[] = "/AVTransport/Control";
  const char service[] = "AVTransport:1";
  const char action[] = "GetTransportInfo";
  const char arguments[] = "<InstanceID>0</InstanceID>";
  sonosAction(url,service,action,arguments,device);
  filter("<CurrentTransportState>","</CurrentTransportState>");
  return String(_filtered);
}



void SonosEsp::removeAllTracksFromQueue(int device) { 
  const char url[] = "/AVTransport/Control";
  const char service[] = "AVTransport:1";
  const char action[] = "RemoveAllTracksFromQueue";
  const char arguments[] = "<InstanceID>0</InstanceID>";
  sonosAction(url,service,action,arguments,device);
}

int SonosEsp::string2int(const char *s) {
  char *ptr; //not used but needed in strtol
  int ret = strtol(s, &ptr, 10);      
  return ret;
}

void SonosEsp::filter(const char *starttag,const char *endtag) {
  //looks in global var _response for content between starttag and endtag and "returns" it in global var _filtered
  char * startpart=strstr(_response,starttag);
  char * endpart=strstr(_response,endtag);

  if (startpart!=0 && endpart!=0) {
    int startindex=startpart-_response+strlen(starttag);
    int endindex=endpart-_response;
 
    strncpy(_filtered,_response+startindex,endindex-startindex);
    _filtered[endindex-startindex]='\0';
  } else {
    _filtered[0]='\0'; //empty string
  }
}


void SonosEsp::setVolume(byte vol,int device) {
  const char url[] = "/RenderingControl/Control";
  const char service[] = "RenderingControl:1";
  const char action[] = "SetVolume";
  char arguments[512];
  sprintf(arguments, "<InstanceID>0</InstanceID><Channel>Master</Channel><DesiredVolume>%d</DesiredVolume>", vol);  
  sonosAction(url,service,action,arguments,device);
}

void SonosEsp::sonosAction(const char *url, const char *service, const char *action, const char *arguments,int device) {
  IPAddress deviceIP = _deviceIPs[device];
  if (_client.connect(deviceIP, 1400)) {
    _client.print("POST /MediaRenderer");
    _client.print(url); //skiljer
    _client.println(" HTTP/1.1");
    _client.print("Host: ");
    _client.println(deviceIP); //note: device is of type IPAddress which converts to int
    _client.print("Content-Length: "); //samma
    int contlen = 124+8+3+strlen(action)+39+strlen(service)+2+strlen(arguments)+4+strlen(action)+1+9+13;
    _client.println(contlen);
    _client.println("Content-Type: text/xml; charset=utf-8");
    _client.print("Soapaction: ");
    _client.print("urn:schemas-upnp-org:service:");
    _client.print(service);
    _client.print("#");
    _client.println(action);
    _client.println("Connection: close"); //samma
    _client.println(""); //samma
    _client.print("<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" ");
    _client.print("s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">");
    _client.print("<s:Body>");
    _client.print("<u:");
    _client.print(action);
    _client.print(" xmlns:u=\"urn:schemas-upnp-org:service:");
    _client.print(service);
    _client.print("\">");
    _client.print(arguments);
    _client.print("</u:");
    _client.print(action);
    _client.print(">");
    _client.print("</s:Body>");
    _client.println("</s:Envelope>");  

    //wait if _client not immediately available
    unsigned long starttimer=millis();
    unsigned long timelimit=10000;
    bool timeout = false;
    while (!_client.available() && !timeout) {
      //wait until _client available
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

    while (_client.available()  && !timeout) {
      char c = _client.read();
      _response[index]=c;
      index++;
      if (index >= SNSESP_BUFSIZ) { 
        index = SNSESP_BUFSIZ -1;
      }

      //Serial.write(c);
      if ((millis() - starttimer) > timelimit) {
        Serial.println("timed out part 2");
        timeout=true;
      }
    }
    if (timeout) {
      _client.stop();
    }
    _response[index]='\0'; //end of string
  }  
}

int SonosEsp::discoverSonos(){
    _numberOfDevices=0;
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
    return _numberOfDevices;
}

bool SonosEsp::addIp(IPAddress ip) {
  bool found=false;
  bool added=false;
  if (_numberOfDevices<SNSESP_MAXNROFDEVICES) { //must agree with definition of global array _deviceIPs
    for (int i=0;i<_numberOfDevices && !found;i++) {
      if (ip==_deviceIPs[i]) {
        found=true;     
      }
    }
    if (!found) {
      _deviceIPs[_numberOfDevices]=ip;
      _numberOfDevices++;
      added=true;
    }
  } 
 
  return added;
}

int SonosEsp::getNumberOfDevices() {
  return _numberOfDevices;
}




