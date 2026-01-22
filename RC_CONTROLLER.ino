#include "esp_bt.h"
#include "RcController.h"
#include "RcCarController.h"
#include "RcHardwareDriver.h"
#include "RumbleManager.h"
RcCarController controller;
RcHardwareDriver hardware;
ControllerPtr myControllers[BP32_MAX_GAMEPADS];
RumbleManager rumble;

ButtonEdge leftSig;
ButtonEdge rightSig;
ButtonEdge hazard;
ButtonEdge headlights;
ButtonEdge gear;
ButtonEdge dpadup;
ButtonEdge dpaddn;

void setup() {
  Serial.begin(115200);
  Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
  const uint8_t* addr = BP32.localBdAddress();
  Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
  esp_power_level_t min, max;
  esp_bredr_tx_power_get(&min, &max);
  Serial.print("Bluetooth TX Power: ");
  Serial.printf("min %d max %d", min, max);
  Serial.println(" dBm");
  esp_bredr_tx_power_set(ESP_PWR_LVL_P9, ESP_PWR_LVL_P9);

  esp_bredr_tx_power_get(&min, &max);
  Serial.print("New Bluetooth TX Power: ");
  Serial.printf("min %d max %d", min, max);
  Serial.println(" dBm");
  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys();
  BP32.enableVirtualDevice(false);

  hardware.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  bool dataUpdated = BP32.update();
  if (dataUpdated) {
    processControllers();
  }
  controller.update(millis());
  hardware.update(controller);
  //vTaskDelay(5);
  //delay(150);
}
void processControllers() {
  for (auto myController : myControllers) {
    if (myController && myController->isConnected() && myController->hasData()) {
      if (myController->isGamepad()) {
        processGamepad(myController);
      } else {
        Serial.println("Unsupported controller");
      }
    }
  }
}
void processGamepad(ControllerPtr ctl) {
  // There are different ways to query whether a button is pressed.
  // By query each button individually:
  //  a(), b(), x(), y(), l1(), etc...
  controller.setThrottle((float)(ctl->throttle()) / 1023);
  controller.setBrake((float)(ctl->brake()) / 1023);
  controller.setSteering((float)(ctl->axisX() - 4) / 512);
  //controller.setHazard(true);

  // L1 -> Left signal
  if (leftSig.rising(ctl->l1())) {
    controller.setLeftSignal(!controller.getLightingState().leftSignal);
  }

  // R1 -> Right signal
  if (rightSig.rising(ctl->r1())) {
    controller.setRightSignal(!controller.getLightingState().rightSignal);
  }

  // Triangle -> Hazard
  if (hazard.rising(ctl->y())) {
    controller.setHazard(!controller.isHazardActive());
  }

  // DPAD_UP -> Shift Up
  if (dpadup.rising(ctl->dpad() & DPAD_UP)) {
    controller.shiftUp();
  }

  // DPAD_DOWN -> Shift Down
  if (dpaddn.rising(ctl->dpad() & DPAD_DOWN)) {
    controller.shiftDown();
  }

  rumble.update(controller, ctl);


  if (ctl->b()) {
    // Turn on the 4 LED. Each bit represents one LED.
    static int led = 0;
    led++;
    // Some gamepads like the DS3, DualSense, Nintendo Wii, Nintendo Switch
    // support changing the "Player LEDs": those 4 LEDs that usually indicate
    // the "gamepad seat".
    // It is possible to change them by calling:
    ctl->setPlayerLEDs(led & 0x0f);
  }

  if (ctl->x()) {
    // Some gamepads like DS3, DS4, DualSense, Switch, Xbox One S, Stadia support rumble.
    // It is possible to set it by calling:
    // Some controllers have two motors: "strong motor", "weak motor".
    // It is possible to control them independently.
    ctl->playDualRumble(0 /* delayedStartMs */, 250 /* durationMs */, 0x80 /* weakMagnitude */,
                        0x40 /* strongMagnitude */);
  }

  // Another way to query controller data is by getting the buttons() function.
  // See how the different "dump*" functions dump the Controller info.
  //dumpGamepad(ctl);
}
void dumpGamepad(ControllerPtr ctl) {
  Serial.printf(
    "idx=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: %4d, %4d, brake: %4d, throttle: %4d, "
    "misc: 0x%02x, gyro x:%6d y:%6d z:%6d, accel x:%6d y:%6d z:%6d\n",
    ctl->index(),        // Controller Index
    ctl->dpad(),         // D-pad
    ctl->buttons(),      // bitmask of pressed buttons
    ctl->axisX(),        // (-511 - 512) left X Axis
    ctl->axisY(),        // (-511 - 512) left Y axis
    ctl->axisRX(),       // (-511 - 512) right X axis
    ctl->axisRY(),       // (-511 - 512) right Y axis
    ctl->brake(),        // (0 - 1023): brake button
    ctl->throttle(),     // (0 - 1023): throttle (AKA gas) button
    ctl->miscButtons(),  // bitmask of pressed "misc" buttons
    ctl->gyroX(),        // Gyro X
    ctl->gyroY(),        // Gyro Y
    ctl->gyroZ(),        // Gyro Z
    ctl->accelX(),       // Accelerometer X
    ctl->accelY(),       // Accelerometer Y
    ctl->accelZ()        // Accelerometer Z
  );
}
void onDisconnectedController(ControllerPtr ctl) {
  bool foundController = false;

  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
      myControllers[i] = nullptr;
      foundController = true;
      break;
    }
  }

  if (!foundController) {
    Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
  }
}
void onConnectedController(ControllerPtr ctl) {
  bool foundEmptySlot = false;
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
      // Additionally, you can get certain gamepad properties like:
      // Model, VID, PID, BTAddr, flags, etc.
      ControllerProperties properties = ctl->getProperties();
      Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id,
                    properties.product_id);
      myControllers[i] = ctl;
      foundEmptySlot = true;
      break;
    }
  }
  if (!foundEmptySlot) {
    Serial.println("CALLBACK: Controller connected, but could not found empty slot");
  }
}