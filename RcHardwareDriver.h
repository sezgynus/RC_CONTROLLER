#ifndef RC_HARDWARE_DRIVER_H
#define RC_HARDWARE_DRIVER_H

#include "RcController.h"
#include "RcCarController.h"

class RcHardwareDriver {
public:
  void begin();

  void update(const RcCarController& controller);

private:
  void driveMotor(float motorCmd, float brakeCmd);
  void driveSteering(float steeringCmd, int32_t trimPulseUs);
  void driveLights(const RcCarController::LightingState& lights);

  bool signalBlinkState = false;
  uint32_t lastBlinkMs = 0;
  static constexpr uint32_t BLINK_PERIOD_MS = 500;

  // PWM channels
  static constexpr int MOTOR_PWM_CH = 2;
  static constexpr int SERVO_PWM_CH = 1;
};

#endif
