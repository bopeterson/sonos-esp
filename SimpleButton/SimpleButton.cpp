#include "Arduino.h"
#include "SimpleButton.h"

SimpleButton::SimpleButton(int pin) {
  _buttonState = HIGH;
  _lastButtonState = HIGH;
  _lastDebounceTime = 0;
  _pin = pin;
  pinMode(_pin, INPUT_PULLUP);
}

bool SimpleButton::readButton() {
  //button connected to pullup and ground, ie LOW=PRESSED
  unsigned long debounceDelay = 50;
  int reading = digitalRead(_pin);
  int pressDetected=false;
  if (reading != _lastButtonState) {
    // reset the debouncing timer
    _lastDebounceTime = millis();
  }

  if ((millis() - _lastDebounceTime) > debounceDelay) {
    if (reading != _buttonState) {
      _buttonState = reading;
      if (_buttonState == LOW) {
        pressDetected=true;
      }
    }
  }
  _lastButtonState = reading;  
  return pressDetected;
}

