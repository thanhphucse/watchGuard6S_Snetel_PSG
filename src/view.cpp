#include "view.h"

View::View(int ledPin, int buttonPin)
: _ledPin(ledPin), _buttonPin(buttonPin) {}

void View::begin() {
  Serial.begin(115200);
  delay(200);
  pinMode(_ledPin, OUTPUT);
  digitalWrite(_ledPin, LOW);

  pinMode(_buttonPin, INPUT_PULLUP); // active LOW
  showBootInfo();
}

void View::showBootInfo() {
  Serial.println(F("\n=== ESP32-CAM MVC (Internship) ==="));
  Serial.printf("Server: %s\n", appConfig.SERVER_URL);
  Serial.println(F("Boot OK\n"));
}

void View::update() {
  // Drive LED according to deviceState
  digitalWrite(_ledPin, deviceState.ledState ? HIGH : LOW);
}

void View::blinkAlertOnce(uint16_t onMs) {
  digitalWrite(_ledPin, HIGH);
  delay(onMs);
  digitalWrite(_ledPin, LOW);
}

bool View::readButton() {
  bool raw = digitalRead(_buttonPin);
  if (raw != _lastButtonState) {
    _lastDebounceMs = millis();
    _lastButtonState = raw;
  }
  if ((millis() - _lastDebounceMs) > 50) {
    // stable
    return (raw == LOW); // pressed = LOW
  }
  return false;
}
