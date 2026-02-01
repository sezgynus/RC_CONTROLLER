#ifndef RC_HARDWARE_DRIVER_H
#define RC_HARDWARE_DRIVER_H

#include "RcController.h"
#include "RcCarController.h"

class RcHardwareDriver {
public:
  void begin();

  void update(RcCarController& controller);

private:
  void driveMotor(float motorCmd, float brakeCmd, RcCarController::Gear currentGear, float current_speed, RcCarController& controller);
  void driveSteering(float steeringCmd, int32_t trimPulseUs, bool releaseAfterMove = false, uint32_t holdTimeMs = 200);
  void driveLights(const RcCarController::LightingState& lights);

  // ================= BRAKE PROFILE =================
  struct BrakeProfile {
    uint8_t pwmStart;  // frene ilk basış
    uint8_t pwmHold;   // tutma
  };

  static constexpr uint32_t BRAKE_RAMP_TIME = 3000;  // ms

  // 1–4 vites için fren karakteristiği
  BrakeProfile brakeTable[4] = {
    { 150, 110 },  // 1. vites
    { 165, 120 },  // 2. vites
    { 180, 130 },  // 3. vites
    { 200, 140 }   // 4. vites
  };

  bool signalBlinkState = false;
  uint32_t lastBlinkMs = 0;
  static constexpr uint32_t BLINK_PERIOD_MS = 400;

  // PWM channels
  static constexpr int MOTOR_PWM_CH = 2;
  static constexpr int SERVO_PWM_CH = 1;
};

#endif
