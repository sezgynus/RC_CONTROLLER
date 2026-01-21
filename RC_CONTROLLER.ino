#include "RcCarController.h"
#include "RcHardwareDriver.h"
RcCarController controller;
RcHardwareDriver hardware;
void setup() {
  // put your setup code here, to run once:
  hardware.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  controller.update(millis());
  hardware.update(controller);
}
