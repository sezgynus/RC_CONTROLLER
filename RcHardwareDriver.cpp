#include "RcHardwareDriver.h"

// ===== Pin definitions =====
constexpr int PIN_MOTOR_EN = 12;
constexpr int PIN_MOTOR_IN1 = 14;
constexpr int PIN_MOTOR_IN2 = 15;

constexpr int PIN_STEERING_SERVO = 4;

constexpr int PIN_LIGHT_LEFT = 17;
constexpr int PIN_LIGHT_RIGHT = 5;
constexpr int PIN_LIGHT_HEAD = 19;
constexpr int PIN_LIGHT_BRAKE = 20;

// ===== PWM parameters =====
constexpr int MOTOR_PWM_FREQ = 20000;
constexpr int MOTOR_PWM_RES = 8;  // 0–255

constexpr int SERVO_PWM_FREQ = 50;
constexpr int SERVO_PWM_RES = 16;  // high resolution

// SG90 pulse width
constexpr uint32_t SERVO_MIN = 900;
constexpr uint32_t SERVO_MAX = 2000;

void RcHardwareDriver::begin() {
  // Motor pins
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);

  ledcSetup(MOTOR_PWM_CH, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
  ledcAttachPin(PIN_MOTOR_EN, MOTOR_PWM_CH);

  // Servo
  ledcSetup(SERVO_PWM_CH, SERVO_PWM_FREQ, SERVO_PWM_RES);
  ledcAttachPin(PIN_STEERING_SERVO, SERVO_PWM_CH);

  // Lights
  pinMode(PIN_LIGHT_LEFT, OUTPUT);
  pinMode(PIN_LIGHT_RIGHT, OUTPUT);
  pinMode(PIN_LIGHT_HEAD, OUTPUT);
  pinMode(PIN_LIGHT_BRAKE, OUTPUT);
}

void RcHardwareDriver::update(const RcCarController& controller) {
  driveMotor(controller.getMotorCommand(), controller.getBrakeCommand(), controller.getGear(), controller.getVirtualSpeed(), controller);
  driveSteering(controller.getSteeringCommand(), controller.getSteeringTrim(), true, 250);
  driveLights(controller.getLightingState());
}

// ================= MOTOR =================

void RcHardwareDriver::driveMotor(float motorCmd, float brakeCmd, RcCarController::Gear currentGear, float current_speed, const RcCarController& controller) {
  static bool before_brake_dir = true;
  static uint32_t brakeStartMs = 0;
  static bool braking = false;

  uint32_t now = millis();

  // ===================== BRAKE =====================
  if (brakeCmd > 0.05f) {
    if (!braking) {
      braking = true;
      brakeStartMs = now;
    }

    // Mevcut vites (0–3)
    uint8_t gear = currentGear;
    if (gear > 3) gear = 3;

    BrakeProfile& bp = brakeTable[gear];
    // Yön: son hareket yönüne ters
    if (before_brake_dir) {
      digitalWrite(PIN_MOTOR_IN1, LOW);
      digitalWrite(PIN_MOTOR_IN2, HIGH);
    } else {
      digitalWrite(PIN_MOTOR_IN1, HIGH);
      digitalWrite(PIN_MOTOR_IN2, LOW);
    }

    // PWM ramp
    uint32_t t = now - brakeStartMs;
    uint8_t pwm;

    if ((t < BRAKE_RAMP_TIME) & (current_speed > 10)) {
      float k = 1.0f - ((float)t / (float)BRAKE_RAMP_TIME);
      pwm = bp.pwmHold + (uint8_t)((bp.pwmStart - bp.pwmHold) * k);
    } else {
      pwm = bp.pwmHold;
    }

    ledcWrite(MOTOR_PWM_CH, pwm);
    return;
  }

  braking = false;

  // ===================== DRIVE =====================
  if (motorCmd >= 0.0f) {
    digitalWrite(PIN_MOTOR_IN1, HIGH);
    digitalWrite(PIN_MOTOR_IN2, LOW);
    before_brake_dir = true;
  } else {
    digitalWrite(PIN_MOTOR_IN1, LOW);
    digitalWrite(PIN_MOTOR_IN2, HIGH);
    motorCmd = -motorCmd;
    before_brake_dir = false;
  }

  if (motorCmd != 0) {
    uint8_t pwm = (uint8_t)(motorCmd * 255.0f);
    //Serial.printf("PWM=%d", pwm);
    pwm = map(pwm, 0, (255 * (controller.gearMaxMotor[currentGear])), 127, (255 * (controller.gearMaxMotor[currentGear])));
    //Serial.printf("Mapped PWM=%d\n", pwm);
    ledcWrite(MOTOR_PWM_CH, pwm);
  } else ledcWrite(MOTOR_PWM_CH, 0);
}


// ================= STEERING =================

void RcHardwareDriver::driveSteering(float steeringCmd, int32_t trimPulseUs, bool releaseAfterMove, uint32_t holdTimeMs) {
  static uint32_t lastCommandMs = 0;
  static uint32_t lastDuty = 0;
  static bool holding = false;

  uint32_t now = millis();

  // Map -1..+1 → pulse width
  float norm = (steeringCmd + 1.0f) * 0.5f;
  uint32_t pulseUs = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * norm;
  pulseUs += trimPulseUs;

  uint32_t duty =
    (pulseUs * ((1 << SERVO_PWM_RES) - 1)) / 20000;

  // ===== PWM serbest bırakma kapalıysa =====
  if (!releaseAfterMove) {
    ledcWrite(SERVO_PWM_CH, duty);
    lastDuty = duty;
    holding = false;
    return;
  }

  // ===== Yeni açı geldiyse =====
  if (duty != lastDuty) {
    ledcWrite(SERVO_PWM_CH, duty);
    lastDuty = duty;
    lastCommandMs = now;
    holding = true;
    return;
  }

  // ===== Tutma süresi dolduysa =====
  if (holding && (now - lastCommandMs >= holdTimeMs)) {
    ledcWrite(SERVO_PWM_CH, 0);  // servo serbest
    holding = false;
  }
}

// ================= LIGHTS =================

void RcHardwareDriver::driveLights(const RcCarController::LightingState& lights) {
  uint32_t now = millis();

  // Blink timing (shared)
  if (now - lastBlinkMs >= BLINK_PERIOD_MS) {
    lastBlinkMs = now;
    signalBlinkState = !signalBlinkState;
  }

  bool leftOut = false;
  bool rightOut = false;

  // ===== PRIORITY LOGIC =====
  if (lights.hazard) {
    // Hazard has absolute priority
    leftOut = signalBlinkState;
    rightOut = signalBlinkState;
  } else {
    if (lights.leftSignal) {
      leftOut = signalBlinkState;
    }
    if (lights.rightSignal) {
      rightOut = signalBlinkState;
    }
  }

  digitalWrite(PIN_LIGHT_LEFT, leftOut ? HIGH : LOW);
  digitalWrite(PIN_LIGHT_RIGHT, rightOut ? HIGH : LOW);

  // Steady lights
  digitalWrite(PIN_LIGHT_HEAD, lights.headlights ? HIGH : LOW);
  digitalWrite(PIN_LIGHT_BRAKE, lights.brakeLights ? HIGH : LOW);
}
