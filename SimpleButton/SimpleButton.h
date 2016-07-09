#ifndef SimpleButton_h
#define SimpleButton_h

#include "Arduino.h"

class SimpleButton {
  public:
    SimpleButton(int pin);
    bool readButton(); 

  private:
    int _buttonState;
    int _lastButtonState;
    unsigned long _lastDebounceTime;
    int _pin;
};
#endif

