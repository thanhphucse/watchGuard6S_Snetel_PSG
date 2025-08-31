#include <Arduino.h>
#include "controller.h"
#include "model.h"
#include "view.h"

// Forward declared in controller.cpp
extern void initController();
extern void controllerLoop();

void setup() {
  initController();
}

void loop() {
  controllerLoop();
}
