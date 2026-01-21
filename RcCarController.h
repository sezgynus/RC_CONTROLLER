#ifndef RC_CAR_CONTROLLER_H
#define RC_CAR_CONTROLLER_H

#include "RcController.h"

class RcCarController {
public:
  enum class Direction {
    FORWARD,
    REVERSE
  };

  struct LightingState {
    bool headlights;
    bool brakeLights;
    bool leftSignal;
    bool rightSignal;
    bool hazard;
  };

  enum Gear {
    GEAR1 = 0,
    GEAR2,
    GEAR3,
    GEAR4,
    NUM_GEARS
  };

  RcCarController();

  // ===== Input interface =====
  void setThrottle(float value);  // 0.0 – 1.0
  void setBrake(float value);     // 0.0 – 1.0
  void setSteering(float value);  // -1.0 – +1.0
  void setDirection(Direction dir);

  void setHeadlights(bool on);
  void setLeftSignal(bool on);
  void setRightSignal(bool on);
  void setHazard(bool on);
  bool isHazardActive() const;

  // ===== Feedback =====
  void setVehicleSpeed(float speed);

  // ===== System =====
  void update(uint32_t nowMs);
  void setFailsafeTimeout(uint32_t timeoutMs);

  // ===== Outputs =====
  float getMotorCommand() const;     // -1.0 … +1.0
  float getBrakeCommand() const;     // 0.0 … 1.0
  float getSteeringCommand() const;  // -1.0 … +1.0
  Direction getDirection() const;

  LightingState getLightingState() const;
  bool isFailsafeActive() const;

  // ===== Gear / Vites =====
  void shiftUp();
  void shiftDown();
  Gear getGear() const {
    return gearLevel;
  }

private:
  // Inputs
  float throttle;
  float brake;
  float steering;
  Direction direction;

  // Feedback
  float vehicleSpeed;

  // Outputs
  float motorCommand;
  float brakeCommand;

  LightingState lights;

  // Failsafe
  uint32_t lastUpdateMs;
  uint32_t failsafeTimeoutMs;
  bool failsafeActive;

  // Gear
  Gear gearLevel;
  float gearMaxMotor[NUM_GEARS] = { 0.65f, 0.75f, 0.85f, 1.0f };  // vites başına max motor scaling

  // Internal
  void applyFailsafe();
  void computeMotorOutput();
};

#endif  // RC_CAR_CONTROLLER_H
