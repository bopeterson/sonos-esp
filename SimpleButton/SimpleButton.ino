#include "SimpleButton.h"

SimpleButton b1(D6);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (b1.readButton()) {
    Serial.println("press detected");
  }
  delay(10);

}
