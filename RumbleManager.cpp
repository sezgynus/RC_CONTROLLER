#include "RumbleManager.h"

RumbleManager::RumbleManager() {
  lastRumbleMs = 0;
}

void RumbleManager::update(const RcCarController& car, ControllerPtr ctl) {
  if (!ctl || !ctl->isConnected())
    return;

  float accel = car.getVirtualAcceleration();
  //Serial.printf("Accel:%f",accel);
  //Serial.println();

  // ===== Launch / patinaj =====
  rumbleLaunchSlip(accel, ctl);

  // ===== Brake slip / ABS =====
  rumbleBrakeSlip(accel, ctl);

  // ===== Gear kick =====
  rumbleGearKick(accel, ctl);
}

// ================= EFFECTS =================

void RumbleManager::rumbleLaunchSlip(float accel, ControllerPtr ctl) {
  // High accel at low speed = wheel slip
  if (accel > 2.5f) {
    uint8_t strength = (uint8_t)(accel * 40.0f);
    if (strength > 120) strength = 120;

    Serial.println("rumbleLaunchSlip");
    play(ctl, 60, 0, strength);
  }
}

void RumbleManager::rumbleBrakeSlip(float accel, ControllerPtr ctl) {
  // Strong negative accel = braking slip
  if (accel < -2.5f) {
    uint8_t strength = (uint8_t)((-accel) * 45.0f);
    if (strength > 140) strength = 140;

    Serial.println("rumbleBrakeSlip");
    play(ctl, 80, 0,strength);
  }
}

void RumbleManager::rumbleGearKick(float accel, ControllerPtr ctl) {
  // Sudden torque change
  if (fabs(accel) > 4.9f) {
    Serial.println("rumbleGearKick");
    play(ctl, 40, 255, 0);
  }
}

// ================= UTILITY =================

void RumbleManager::play(ControllerPtr ctl, uint16_t durationMs, uint8_t weak, uint8_t strong) {
  uint32_t now = millis();

  // simple rate limit
  if (now - lastRumbleMs < 40)
    return;

  lastRumbleMs = now;

  ctl->playDualRumble(
    0,  // immediate
    durationMs,
    weak,
    strong);
}
