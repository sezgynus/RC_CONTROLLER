#include "RcCarController.h"
static float clamp(float v, float minV, float maxV) {
  if (v < minV) return minV;
  if (v > maxV) return maxV;
  return v;
}

RcCarController::RcCarController() {
  Serial.begin(115200);
  throttle = 0.0f;
  brake = 0.0f;
  steering = 0.0f;
  direction = Direction::FORWARD;

  vehicleSpeed = 0.0f;

  motorCommand = 0.0f;
  brakeCommand = 0.0f;

  lights = { false, false, false, false, false, false, false };

  virtualSpeed = 0.0f;
  virtualAccel = 0.0f;
  lastDynMs = millis();

  lastUpdateMs = 0;
  failsafeTimeoutMs = 500;
  failsafeActive = false;
}
void RcCarController::begin() {
  prefs.begin("rc_cfg", false);
  trim = prefs.getInt("steer_trim", 0);
  Serial.printf("Saved Trim=%d\n", trim);
}

// ================= INPUT =================

void RcCarController::setThrottle(float value) {
  throttle = clamp(value, 0.0f, 1.0f);
  lastUpdateMs = millis();
  //Serial.printf("Throttle=%f\n", throttle);
}

void RcCarController::setBrake(float value) {
  brake = clamp(value, 0.0f, 1.0f);
  lastUpdateMs = millis();
  //Serial.printf("Brake=%f\n", brake);
}

void RcCarController::setSteering(float value) {
  steering = clamp(value, -1.0f, 1.0f);
  lastUpdateMs = millis();

  constexpr float STEER_ACTIVATE_TH = 0.35f;
  constexpr float STEER_CENTER_TH = 0.10f;

  // ===== RIGHT SIGNAL AUTO CANCEL =====
  if (lights.rightSignal) {
    // sağa yeterince kırıldıysa arm et
    if (steering > STEER_ACTIVATE_TH) {
      rightSignalArmed = true;
    }

    // arm edildikten sonra merkeze dönerse kapat
    if (rightSignalArmed && fabs(steering) < STEER_CENTER_TH) {
      lights.rightSignal = false;
      rightSignalArmed = false;
    }
  } else {
    rightSignalArmed = false;
  }

  // ===== LEFT SIGNAL AUTO CANCEL =====
  if (lights.leftSignal) {
    if (steering < -STEER_ACTIVATE_TH) {
      leftSignalArmed = true;
    }

    if (leftSignalArmed && fabs(steering) < STEER_CENTER_TH) {
      lights.leftSignal = false;
      leftSignalArmed = false;
    }
  } else {
    leftSignalArmed = false;
  }
}


void RcCarController::setSteeringTrim(int32_t value) {
  trim = value;
  if (prefs.getInt("steer_trim", 0) != trim) {
    bool ok = prefs.putInt("steer_trim", trim);
    (void)ok;
  }
  lastUpdateMs = millis();
  //Serial.printf("Steering=%f\n", steering);
}

void RcCarController::setDirection(Direction dir) {
  if (vehicleSpeed < 0.01f) {
    direction = dir;
  }
}

void RcCarController::setHeadlights(bool on) {
  lights.headlights = on;
}

void RcCarController::setHighBeam(bool on) {
  lights.highBeam = on;
}

void RcCarController::setBrakeLights(bool on) {
  lights.brakeLights = on;
}
void RcCarController::setReverseLight(bool on) {
  lights.reverseLight = on;
}

void RcCarController::setLeftSignal(bool on) {
  lights.leftSignal = on;
  if (on) lights.rightSignal = false;
}

void RcCarController::setRightSignal(bool on) {
  lights.rightSignal = on;
  if (on) lights.leftSignal = false;
}
void RcCarController::setHazard(bool on) {
  lights.hazard = on;
}

bool RcCarController::isHazardActive() const {
  return lights.hazard;
}

// ================= FEEDBACK =================

void RcCarController::setVehicleSpeed(float speed) {
  vehicleSpeed = speed;
}

// ================= SYSTEM =================

void RcCarController::setFailsafeTimeout(uint32_t timeoutMs) {
  failsafeTimeoutMs = timeoutMs;
}

void RcCarController::update(uint32_t nowMs) {
  if ((nowMs - lastUpdateMs) > failsafeTimeoutMs) {
    applyFailsafe();
  } else {
    failsafeActive = false;
    computeMotorOutput();
  }
}

// ================= OUTPUT =================

float RcCarController::getMotorCommand() const {
  return motorCommand;
}

float RcCarController::getBrakeCommand() const {
  return brakeCommand;
}

float RcCarController::getSteeringCommand() const {
  return steering;
}

int32_t RcCarController::getSteeringTrim() const {
  return trim;
}

RcCarController::Direction RcCarController::getDirection() const {
  return direction;
}

RcCarController::LightingState RcCarController::getLightingState() const {
  return lights;
}

bool RcCarController::isFailsafeActive() const {
  return failsafeActive;
}

// ================= INTERNAL =================

void RcCarController::applyFailsafe() {
  failsafeActive = true;

  throttle = 0.0f;
  brake = 0.0f;
  steering = 0.0f;
  motorCommand = 0.0f;
  brakeCommand = 0.0f;

  //Serial.println("Failsafe!!!");
  lights.brakeLights = true;
  lights.hazard = true;
}

void RcCarController::computeMotorOutput() {
  const bool throttleActive = throttle > 0.05f;
  const bool brakeActive = brake > 0.05f;

  float maxMotor = gearMaxMotor[gearLevel];

  // ===== TIME BASE =====
  uint32_t now = millis();
  float dt = (now - lastDynMs) * 0.001f;  // seconds
  if (dt <= 0.0f || dt > 0.1f) dt = 0.01f;
  lastDynMs = now;

  float prevSpeed = virtualSpeed;

  // ===== CASE 1: FULL BRAKE =====
  if (throttleActive && brakeActive) {
    motorCommand = 0.0f;
    brakeCommand = 1.0f;
    lights.brakeLights = true;

    // strong decel
    virtualSpeed -= 10.0f * dt;
  }
  // ===== CASE 2: FORWARD =====
  else if (throttleActive) {
    motorCommand = throttle * maxMotor;
    brakeCommand = 0.0f;
    lights.brakeLights = false;

    virtualSpeed += motorCommand * 5.0f * dt;
  }
  // ===== CASE 3: REVERSE =====
  else if (brakeActive) {
    motorCommand = -brake * maxMotor;
    brakeCommand = 0.0f;
    lights.brakeLights = false;

    virtualSpeed += (-motorCommand) * 5.0f * dt;
  }
  // ===== CASE 4: FREEWHEEL =====
  else {
    motorCommand = 0.0f;
    brakeCommand = 0.0f;
    lights.brakeLights = false;

    // rolling resistance
    virtualSpeed -= 1.6f * dt;
  }

  // ===== CLAMP SPEED =====
  if (virtualSpeed < 0.0f) virtualSpeed = 0.0f;
  if (virtualSpeed > maxMotor * 500) virtualSpeed = maxMotor * 500;

  // ===== ACCELERATION =====
  virtualAccel = (virtualSpeed - prevSpeed) / dt;
  if (virtualAccel > 5.0f) virtualAccel = 5.0f;
  if (virtualAccel < -5.0f) virtualAccel = -5.0f;
  // static uint32_t lastPrintMs = 0;
  // if (now - lastPrintMs > 100) {  // 10 Hz
  //   lastPrintMs = now;
  //   Serial.printf(
  //     "[PHY] dt=%.3f | thr=%.2f brk=%.2f | motor=%.2f | speed=%.3f | accel=%.3f | gear=%d\n",
  //     dt,
  //     throttle,
  //     brake,
  //     motorCommand,
  //     virtualSpeed,
  //     virtualAccel,
  //     gearLevel);
  // }
}
float RcCarController::getVirtualSpeed() const {
  return virtualSpeed;
}

float RcCarController::getVirtualAcceleration() const {
  return virtualAccel;
}
void RcCarController::shiftUp() {
  if (gearLevel < Gear::GEAR4) {
    gearLevel = static_cast<Gear>(static_cast<int>(gearLevel) + 1);
    //Serial.println("Shift up");
  }
}

void RcCarController::shiftDown() {
  if (gearLevel > Gear::GEAR1) {
    gearLevel = static_cast<Gear>(static_cast<int>(gearLevel) - 1);
    //Serial.println("Shift down");
  }
}