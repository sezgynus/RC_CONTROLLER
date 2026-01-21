#include "RcCarController.h"

static float clamp(float v, float minV, float maxV)
{
    if (v < minV) return minV;
    if (v > maxV) return maxV;
    return v;
}

RcCarController::RcCarController()
{
    throttle = 0.0f;
    brake = 0.0f;
    steering = 0.0f;
    direction = Direction::FORWARD;

    vehicleSpeed = 0.0f;

    motorCommand = 0.0f;
    brakeCommand = 0.0f;

    lights = {false, false, false, false};

    lastUpdateMs = 0;
    failsafeTimeoutMs = 500;
    failsafeActive = false;
}

// ================= INPUT =================

void RcCarController::setThrottle(float value)
{
    throttle = clamp(value, 0.0f, 1.0f);
    lastUpdateMs = millis();
}

void RcCarController::setBrake(float value)
{
    brake = clamp(value, 0.0f, 1.0f);
    lastUpdateMs = millis();
}

void RcCarController::setSteering(float value)
{
    steering = clamp(value, -1.0f, 1.0f);
    lastUpdateMs = millis();
}

void RcCarController::setDirection(Direction dir)
{
    if (vehicleSpeed < 0.01f) {
        direction = dir;
    }
}

void RcCarController::setHeadlights(bool on)
{
    lights.headlights = on;
}

void RcCarController::setLeftSignal(bool on)
{
    lights.leftSignal = on;
    if (on) lights.rightSignal = false;
}

void RcCarController::setRightSignal(bool on)
{
    lights.rightSignal = on;
    if (on) lights.leftSignal = false;
}

// ================= FEEDBACK =================

void RcCarController::setVehicleSpeed(float speed)
{
    vehicleSpeed = speed;
}

// ================= SYSTEM =================

void RcCarController::setFailsafeTimeout(uint32_t timeoutMs)
{
    failsafeTimeoutMs = timeoutMs;
}

void RcCarController::update(uint32_t nowMs)
{
    if ((nowMs - lastUpdateMs) > failsafeTimeoutMs) {
        applyFailsafe();
    } else {
        failsafeActive = false;
        computeMotorOutput();
    }
}

// ================= OUTPUT =================

float RcCarController::getMotorCommand() const
{
    return motorCommand;
}

float RcCarController::getBrakeCommand() const
{
    return brakeCommand;
}

float RcCarController::getSteeringCommand() const
{
    return steering;
}

RcCarController::Direction RcCarController::getDirection() const
{
    return direction;
}

RcCarController::LightingState RcCarController::getLightingState() const
{
    return lights;
}

bool RcCarController::isFailsafeActive() const
{
    return failsafeActive;
}

// ================= INTERNAL =================

void RcCarController::applyFailsafe()
{
    failsafeActive = true;

    throttle = 0.0f;
    brake = 1.0f;
    steering = 0.0f;

    motorCommand = 0.0f;
    brakeCommand = 1.0f;

    lights.brakeLights = true;
}

void RcCarController::computeMotorOutput()
{
    brakeCommand = brake;
    lights.brakeLights = (brake > 0.05f);

    float effectiveThrottle = throttle * (1.0f - brake);
    motorCommand = effectiveThrottle;

    if (direction == Direction::REVERSE) {
        motorCommand = -motorCommand;
    }
}
