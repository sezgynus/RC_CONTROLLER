#include "esp_bt.h"
#include "RcController.h"
#include "RcCarController.h"
#include "RcHardwareDriver.h"
#include "RumbleManager.h"
#include <uni.h>
RcCarController controller;
RcHardwareDriver hardware;
ControllerPtr myControllers[BP32_MAX_GAMEPADS];
RumbleManager rumble;

ButtonEdge leftSig;
ButtonEdge rightSig;
ButtonEdge hazard;
ButtonEdge headlights;
ButtonEdge headlightsFlash;
ButtonEdge gear;
ButtonEdge dpadup;
ButtonEdge dpaddn;
ButtonEdge dpadleft;
ButtonEdge dpadright;
ButtonEdge share;

OTA ota;
bool saved_high_beam;

uint8_t mac[6];

void set_max_bt_tx_power() {
  uni_bt_bredr_scan_stop();
  uni_bt_bredr_set_enabled(false);
  if (uni_bt_bredr_is_enabled()) Serial.println("Bt enabled");
  else Serial.println("Bt not enabled");
  esp_power_level_t min, max;
  esp_bredr_tx_power_get(&min, &max);
  Serial.print("Bluetooth TX Power: ");
  Serial.printf("min %d max %d", min, max);
  Serial.println(" dBm");
  esp_bt_controller_disable();
  esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
  esp_bredr_tx_power_set(ESP_PWR_LVL_P9, ESP_PWR_LVL_P9);

  esp_bredr_tx_power_get(&min, &max);
  Serial.print("New Bluetooth TX Power: ");
  Serial.printf("min %d max %d", min, max);
  Serial.println(" dBm");
}

void setup() {
  Serial.begin(115200);
  Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
  const uint8_t* addr = BP32.localBdAddress();
  Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

  memcpy(mac, addr, sizeof(mac));
  //ota.begin("RcController", "RcController", "12345678");
  uni_bt_allowlist_init();
  uni_bt_allowlist_set_enabled(true);
  //uni_bt_enable_new_connections_safe(true);
  BP32.setup(&onConnectedController, &onDisconnectedController);
  //BP32.forgetBluetoothKeys();
  BP32.enableVirtualDevice(false);

  controller.begin();
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
  // X -> Headlights
  if (headlights.rising(ctl->x())) {
    Serial.println("X Pressed");
    if (!controller.getLightingState().headlights & !controller.getLightingState().highBeam) {
      controller.setHeadlights(true);
    } else if (controller.getLightingState().headlights & !controller.getLightingState().highBeam) {
      controller.setHighBeam(true);
    } else if (controller.getLightingState().headlights & controller.getLightingState().highBeam) {
      controller.setHighBeam(false);
      controller.setHeadlights(false);
    }
  }
  if (headlights.falling(ctl->x())) {
  }

  if (headlightsFlash.rising(ctl->a())) {
    saved_high_beam = controller.getLightingState().highBeam;
    controller.setHighBeam(true);
    Serial.println("A Pressed");
  }

  if (headlightsFlash.falling(ctl->a())) {
    controller.setHighBeam(saved_high_beam);
    Serial.println("A Relased");
  }

  // L1 -> Left signal
  if (leftSig.rising(ctl->l1())) {
    controller.setLeftSignal(!controller.getLightingState().leftSignal);
  }
  if (leftSig.falling(ctl->l1())) {
  }

  // R1 -> Right signal
  if (rightSig.rising(ctl->r1())) {
    controller.setRightSignal(!controller.getLightingState().rightSignal);
  }
  if (rightSig.falling(ctl->r1())) {
  }

  // Triangle -> Hazard
  if (hazard.rising(ctl->y())) {
    controller.setHazard(!controller.isHazardActive());
  }
  if (hazard.falling(ctl->y())) {
  }

  // DPAD_UP -> Shift Up
  if (dpadup.rising(ctl->dpad() & DPAD_UP)) {
    if ((controller.getGear() >= 1)) {
      if (ctl->miscButtons() & 0x04) {
        controller.shiftUp();
      }
    } else {
      controller.shiftUp();
    }
  }
  if (dpadup.falling(ctl->dpad() & DPAD_UP)) {
    switch (controller.getGear()) {
      case 0:                         // Vites 1
        ctl->setColorLED(0, 255, 0);  // Yeşil
        break;
      case 1:                           // Vites 2
        ctl->setColorLED(255, 255, 0);  // Sarı
        break;
      case 2:                           // Vites 3
        ctl->setColorLED(255, 30, 0);  // Turuncu
        break;
      case 3:                         // Vites 4
        ctl->setColorLED(255, 0, 0);  // Kırmızı
        break;
      default:
        ctl->setColorLED(0, 0, 0);  // Kapalı / bilinmeyen vites
        break;
    }
  }

  // DPAD_DOWN -> Shift Down
  if (dpaddn.rising(ctl->dpad() & DPAD_DOWN)) {
    controller.shiftDown();
  }
  if (dpaddn.falling(ctl->dpad() & DPAD_DOWN)) {

    switch (controller.getGear()) {
      case 0:                         // Vites 1
        ctl->setColorLED(0, 255, 0);  // Yeşil
        break;
      case 1:                           // Vites 2
        ctl->setColorLED(255, 255, 0);  // Sarı
        break;
      case 2:                           // Vites 3
        ctl->setColorLED(255, 30, 0);  // Turuncu
        break;
      case 3:                         // Vites 4
        ctl->setColorLED(255, 0, 0);  // Kırmızı
        break;
      default:
        ctl->setColorLED(0, 0, 0);  // Kapalı / bilinmeyen vites
        break;
    }
  }

  // DPAD_RIGHT -> Trim++
  if (dpadright.rising(ctl->dpad() & DPAD_RIGHT)) {
    if (ctl->miscButtons() & 0x04) {
      controller.setSteeringTrim(controller.getSteeringTrim() + 5);
      Serial.printf("Trim=%d\n", controller.getSteeringTrim());
    }
  }
  if (dpadright.falling(ctl->dpad() & DPAD_RIGHT)) {
  }

  // DPAD_LEFT -> Trim--
  if (dpadleft.rising(ctl->dpad() & DPAD_LEFT)) {
    if (ctl->miscButtons() & 0x04) {
      controller.setSteeringTrim(controller.getSteeringTrim() - 5);
      Serial.printf("Trim=%d\n", controller.getSteeringTrim());
    }
  }
  if (dpadleft.falling(ctl->dpad() & DPAD_LEFT)) {
  }
  if (share.rising(ctl->miscButtons() & 0x02)) {
    if (ota.isActive()) {
      ota.stop();
    } else {
      ota.begin("RcController", "RcController", "12345678");
    }
  }
  if (share.falling(ctl->miscButtons() & 0x02)) {
  }

  rumble.update(controller, ctl);




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
  ctl->setColorLED(0, 255, 0);
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
      ControllerProperties properties = ctl->getProperties();
      Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id,
                    properties.product_id);
      uni_bt_allowlist_add_addr(properties.btaddr);
      uni_bt_allowlist_set_enabled(true);
      myControllers[i] = ctl;
      foundEmptySlot = true;
      break;
    }
  }
  if (!foundEmptySlot) {
    Serial.println("CALLBACK: Controller connected, but could not found empty slot");
  }
}