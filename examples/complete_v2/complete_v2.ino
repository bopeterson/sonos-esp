/*
 * connections
 * 
 * Wemos   Rotary encoder (Keyes KY-040)
 * GND     GND
 * 5V      +
 * D3      CLK - pin A
 * D6      SW - push button
 * D7      DT - pin B
 * 
 */


#include <SimpleButton.h>
#include <SonosEsp.h>
#include <WiFiSetup.h>

const char compiletime[]=__TIME__;
const char compiledate[]=__DATE__;

WiFiSetup wifisetup(BUILTIN_LED); // Wemos blue led; -1 if not using led

SonosEsp sonos;

SimpleButton knobButton;

const unsigned long checkRate = 15*1000; //how often main loop performs periodical task
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
  knobButton.begin(D6);
  Serial.begin(9600);
  Serial.println("");
  Serial.print("compiletime: ");
  Serial.print(compiletime);
  Serial.print(" ");
  Serial.println(compiledate);

  //encoder setup
  pinMode (pinA, INPUT);
  pinMode (pinB, INPUT);
  pinALast = digitalRead(pinA);

  //wifisetup.start();
  wifisetup.startAccessPoint(10);

  //connectWiFi();
  //delay(7000); //wait for connection to wifi 
  if (wifisetup.connected()) {
    sonos.discoverSonos();
    Serial.print("Found devices: ");
    Serial.println(sonos.getNumberOfDevices());
  }

}

void loop() {
  unsigned long loopStart = millis();

  wifisetup.periodic();

  if (lastPost + checkRate <= loopStart) {
    lastPost = loopStart;
    //put any code here that should run periodically
    if (sonos.getNumberOfDevices()==0 && wifisetup.connected()) {
      sonos.discoverSonos();
      Serial.print("Found devices: ");
      Serial.println(sonos.getNumberOfDevices());
    }
  }  

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
    int oldVolume=sonos.getVolume(0);
    Serial.print("old volume; ");
    Serial.print(oldVolume);
    //xxx handle timeout in getVolume
    int newVolume=oldVolume+floor(0.5*(encoderRelativeCount+0.5));
    newVolume=constrain(newVolume,0,100);
    Serial.print(", new volume: ");
    Serial.print(newVolume);
    Serial.print(" , encoder position: ");
    Serial.print(encoderPosCount);
    Serial.print(" , encoder move: ");
    Serial.println(encoderRelativeCount);
    sonos.setVolume(newVolume,0);
    moveTime=0; 
    encoderRelativeCount=0;
  }

  if (knobButton.readButton()) {
    Serial.println("button pressed");
    if (sonos.getTransportInfo(0)=="PLAYING") {
      sonos.pause(0);
    } else {
      sonos.play(0);
    }
  }


}

