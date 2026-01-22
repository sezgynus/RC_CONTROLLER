#ifndef RUMBLE_MANAGER_H
#define RUMBLE_MANAGER_H

#include "RcController.h"
#include "RcCarController.h"

class RumbleManager {
public:
  RumbleManager();

  void update(const RcCarController& car, ControllerPtr ctl);

private:
  // ===== Internal states =====
  uint32_t lastRumbleMs;

  // ===== Effects =====
  void rumbleLaunchSlip(float accel, ControllerPtr ctl);
  void rumbleBrakeSlip(float accel, ControllerPtr ctl);
  void rumbleGearKick(float accel, ControllerPtr ctl);

  // Utility
  void play(ControllerPtr ctl,
            uint16_t durationMs,
            uint8_t weak,
            uint8_t strong);
};

#endif