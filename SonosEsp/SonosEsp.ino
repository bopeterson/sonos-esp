#include "SonosEsp.h"

const char compiletime[]=__TIME__;
const char compiledate[]=__DATE__;

SonosEsp sonos;



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
  Serial.println("");
  Serial.print("compiletime: ");
  Serial.print(compiletime);
  Serial.print(" ");
  Serial.println(compiledate);

  //encoder setup
  pinMode (pinA, INPUT);
  pinMode (pinB, INPUT);
  pinALast = digitalRead(pinA);

  //connectWiFi();
  delay(7000); //wait for connection to wifi 
  
  sonos.discoverSonos();
  
  Serial.print("Found devices: ");
  Serial.println(sonos.getNumberOfDevices());


}

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
    int oldVolume=sonos.getVolume(0);
    Serial.print(oldVolume);
    //xxx handle timeout in getVolume
    int newVolume=oldVolume+2*encoderRelativeCount;
    newVolume=constrain(newVolume,0,100);
    Serial.print(" ");
    Serial.print(newVolume);
    Serial.print(" ");
    Serial.println(encoderRelativeCount);
    sonos.setVolume(newVolume,0);
    moveTime=0; 
    encoderRelativeCount=0;
  }


}

