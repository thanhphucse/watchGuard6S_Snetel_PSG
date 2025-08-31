#ifndef VIEW_H
#define VIEW_H

#include <Arduino.h>
#include "model.h"

class View {
public:
  View(int ledPin = 4, int buttonPin = 13);
  void begin();
  void update();            // reflect model -> hardware
  void showBootInfo();
  void blinkAlertOnce(uint16_t onMs = 80);
  bool readButton();        // optional local button read (debounced)

private:
  int _ledPin;
  int _buttonPin;
  unsigned long _lastDebounceMs = 0;
  bool _lastButtonState = HIGH;
};

#endif
