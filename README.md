# RC Controller

[English](README.md) | [Türkçe](README-tr.md)

**RC Controller is ESP32 firmware for driving an RC vehicle with a Bluetooth gamepad through Bluepad32.** It separates gamepad input, vehicle-state logic, hardware PWM/output control, feedback effects, and optional OTA updating into dedicated components.

The current firmware controls:

- forward and reverse motor drive;
- active braking when throttle and brake are pressed together;
- steering servo position and persistent steering trim;
- four software gear levels;
- headlights, high beam, flash-to-pass, brake light, reverse light, turn signals, and hazard lights;
- gamepad color LED feedback and rumble effects;
- a 500 ms command failsafe;
- an optional Wi-Fi access point for Arduino OTA updates.

## Architecture

```text
Bluetooth gamepad
       |
       v
    Bluepad32
       |
       v
RC_CONTROLLER.ino
  input mapping / connection handling
       |
       v
RcCarController
  vehicle state / gears / lighting / failsafe
       |
       +--------------------+
       |                    |
       v                    v
RcHardwareDriver       RumbleManager
motor / servo /        gamepad vibration
lighting outputs       from virtual dynamics
       |
       v
 RC vehicle hardware
```

`OTA.h` provides the optional Wi-Fi AP and ArduinoOTA path independently of the driving logic.

## Hardware

The firmware targets an ESP32 environment compatible with Bluetooth Classic / Bluepad32. A supported gamepad, motor driver with EN/IN1/IN2 control, steering servo, and suitable light-driver circuitry are required.

**Do not power the motor or steering servo directly from ESP32 GPIO pins.** Use appropriate power and driver hardware for the actual motor, servo, and lamps.

### Pin Assignment

| Function | GPIO |
|---|---:|
| Motor EN / PWM | 12 |
| Motor IN1 | 14 |
| Motor IN2 | 15 |
| Steering servo | 4 |
| Left turn signal | 5 |
| Right turn signal | 17 |
| Headlight | 33 |
| Brake light | 32 |
| High beam | 2 |
| Reverse light | 0 |

All lighting outputs are **active-low**: an active lamp is written LOW and an inactive lamp HIGH.

## PWM Configuration

### Motor

```text
Frequency: 20 kHz
Resolution: 8 bit
PWM channel: 2
```

The motor command is signed:

- positive: forward;
- negative: reverse;
- zero: no drive PWM.

For a non-zero drive command, the hardware driver remaps the PWM range so the lower output starts at 127 while the upper limit follows the selected gear scale.

### Steering Servo

```text
Frequency: 50 Hz
Resolution: 16 bit
PWM channel: 1
Base pulse range: 900–2000 us
```

The steering command is mapped from `-1.0 ... +1.0` to the servo pulse range, then the stored trim value is added.

When the requested duty stops changing, the current implementation keeps the servo PWM active for 500 ms and then writes zero duty to release it.

The pulse range and trim must be checked against the real mechanical steering limits before operation.

## Gamepad Controls

The names below are Bluepad32 logical inputs; physical button symbols can differ between controllers.

| Input | Action |
|---|---|
| `throttle()` | Forward throttle |
| `brake()` | Reverse when used alone; full-brake request when used with throttle |
| Left stick X / `axisX()` | Steering |
| X | Headlight → high beam → off cycle |
| A, held | Flash-to-pass / temporary high beam |
| L1 | Toggle left turn signal |
| R1 | Toggle right turn signal |
| Y | Toggle hazard lights |
| D-pad Up | Shift up |
| D-pad Down | Shift down |
| `miscButtons() & 0x04` + D-pad Right | Steering trim +5 us |
| `miscButtons() & 0x04` + D-pad Left | Steering trim -5 us |
| `miscButtons() & 0x02` | Toggle OTA access point |

The firmware starts in first gear. Shifting above second gear requires `miscButtons() & 0x04` while pressing D-pad Up.

### Gear Feedback

| Gear | Motor scale | Gamepad LED |
|---|---:|---|
| 1 | 60% | Green |
| 2 | 70% | Yellow |
| 3 | 80% | Orange |
| 4 | 100% | Red |

The LED color is updated when a D-pad gear-change button is released.

## Steering Trim

Trim is stored with ESP32 `Preferences`:

```text
namespace: rc_cfg
key: steer_trim
```

Each trim command changes the stored value by 5 µs. The value is loaded during startup and added directly to the generated servo pulse width.

The source does not currently clamp the trim value, so excessive repeated adjustment can move the resulting pulse outside the nominal 900–2000 µs range.

## Lighting

Turn signals share a 400 ms toggle period. Hazard mode has priority over individual left/right signals.

When an individual signal is enabled, steering can cancel it automatically:

```text
activation threshold: |steering| > 0.35 in the matching direction
cancel threshold:     |steering| < 0.10 after activation
```

Enabling the left signal disables the right signal and vice versa.

The brake and reverse light states are also updated by the motor hardware driver according to the current drive/brake command.

## Drive and Brake Logic

`RcCarController::computeMotorOutput()` interprets the two analog trigger inputs as follows:

| Throttle | Brake | Result |
|---|---|---|
| Active | Inactive | Forward drive |
| Inactive | Active | Reverse drive |
| Active | Active | Full brake command |
| Inactive | Inactive | Freewheel / zero motor command |

The active threshold for each trigger is `0.05`.

### Brake Profile

When the full-brake command is active, the hardware driver drives the motor in the direction opposite the last movement direction.

The configured PWM profiles are:

| Gear | Start PWM | Hold PWM |
|---|---:|---:|
| 1 | 180 | 120 |
| 2 | 200 | 120 |
| 3 | 220 | 120 |
| 4 | 240 | 120 |

The ramp duration constant is 8000 ms. While the virtual speed is above 10 and the ramp time has not expired, PWM decreases from the gear-specific start value toward 120; otherwise it uses the hold value.

This is an active reverse-drive braking strategy, not a guarantee of a particular physical braking force. Compatibility must be evaluated with the actual motor driver and mechanics.

## Virtual Dynamics and Rumble

The firmware maintains software-only `virtualSpeed` and `virtualAccel` values. The main loop does not read a physical vehicle-speed sensor.

The model uses:

- acceleration proportional to motor command during drive;
- strong modeled deceleration during full braking;
- modeled rolling resistance while freewheeling;
- acceleration clamped to `-5.0 ... +5.0`.

These values feed `RumbleManager`.

Current rumble triggers are:

| Condition | Effect |
|---|---|
| Virtual acceleration > 2.5 | Launch/slip rumble |
| Virtual acceleration < -2.5 | Brake-slip rumble |
| Absolute virtual acceleration > 4.9 | Short gear-kick style rumble |

Rumble calls are rate-limited to at least 40 ms apart. These effects are derived from the virtual model, not measured wheel slip or acceleration.

## Turn-Signal Auto Cancel

A right signal becomes armed after steering exceeds `+0.35`; a left signal becomes armed below `-0.35`. Once armed, returning steering inside `±0.10` cancels that signal.

This reproduces a steering-return style cancellation without a physical steering-angle sensor.

## Failsafe

Each throttle, brake, or steering update refreshes the input timestamp. The default failsafe timeout is:

```text
500 ms
```

If the timeout expires, the controller:

- sets throttle and brake input values to zero;
- centers the software steering command;
- sets motor and brake commands to zero;
- marks failsafe active;
- enables the brake-light state;
- enables hazard lights.

The hardware motor command therefore goes to zero. This should not be interpreted as a guaranteed mechanical or active electrical brake.

## Bluetooth Behavior

Startup initializes and enables the Bluepad32 / BTstack allowlist before `BP32.setup()`.

When a controller connects, its Bluetooth address is added to the allowlist and the controller is stored in the first free `BP32_MAX_GAMEPADS` slot. Only gamepad-type controllers are processed by the driving code.

The source contains commented-out calls for forgetting Bluetooth keys and enabling new connections. Exact first-pairing/provisioning behavior therefore also depends on the Bluepad32 version and environment used to build the firmware.

`set_max_bt_tx_power()` exists in the source but is not called by `setup()`.

## OTA Update Mode

OTA is normally inactive at boot. The corresponding gamepad misc button toggles it.

When enabled, `OTA::begin()`:

1. constructs a hostname from `RcController-` plus the last three bytes of the ESP32 Bluetooth address;
2. switches Wi-Fi to AP mode;
3. starts a SoftAP;
4. configures ArduinoOTA;
5. starts a FreeRTOS task pinned to core 0 that continuously calls `ArduinoOTA.handle()`.

The current call from the main sketch starts the AP with:

```text
SSID: RcController
```

Authentication values are present directly in the current source. Review and change those credentials before deploying the firmware in an environment where OTA access matters.

Disabling OTA deletes the OTA task, stops ArduinoOTA, disconnects the SoftAP, and turns Wi-Fi off.

## Build

The repository does not pin exact ESP32 core or Bluepad32 versions.

To build it:

1. Use an Arduino-compatible ESP32 development environment with Bluepad32 and the `uni.h` API used by this source.
2. Keep all project files in the same Arduino sketch directory as `RC_CONTROLLER.ino`.
3. Use an ESP32 core compatible with the `ledcSetup()` and `ledcAttachPin()` APIs used by the hardware driver.
4. Select the actual ESP32 board and build/upload the sketch.
5. Serial diagnostics use **115200 baud**.

For first testing, keep the driven wheels clear of the ground and verify motor direction, steering center/endpoints, light polarity, braking behavior, and failsafe operation before driving the vehicle.

## Current Limitations

- No physical speed sensor is read by the current main application.
- Virtual speed/acceleration are model values, not measurements.
- Steering trim has no software clamp.
- The brake implementation actively drives the motor in the opposite direction.
- Lighting outputs assume active-low external circuitry.
- The firmware uses fixed GPIO assignments.
- Exact Bluepad32 / ESP32 dependency versions are not pinned.
- OTA credentials are embedded in source.
- `RcInputAdapter.cpp` and `RcInputAdapter.h` are currently empty placeholders.
- The firmware has storage for multiple controller slots, but driving inputs from connected gamepads are processed through the same single vehicle controller state.

## Source Map

| File | Responsibility |
|---|---|
| `RC_CONTROLLER.ino` | Startup, Bluepad32 callbacks, controller storage, button mapping, main loop and OTA toggle |
| `RcCarController.h/.cpp` | Vehicle state, gear scaling, steering trim, lighting state, virtual dynamics and failsafe |
| `RcHardwareDriver.h/.cpp` | GPIO assignments, motor PWM/direction, active braking, servo PWM and light outputs |
| `RumbleManager.h/.cpp` | Gamepad rumble effects generated from virtual acceleration |
| `OTA.h` | Optional SoftAP + ArduinoOTA service and task |
| `RcController.h` | Common includes and button-edge helper |
| `RcInputAdapter.h/.cpp` | Empty placeholders in the current repository |
| `.github/workflows/sign-commits.yml` | Repository commit-signing workflow |

## Repository Structure

```text
RC_CONTROLLER/
├── .github/
│   └── workflows/
│       └── sign-commits.yml
├── OTA.h
├── RC_CONTROLLER.ino
├── README.md
├── RcCarController.cpp
├── RcCarController.h
├── RcController.h
├── RcHardwareDriver.cpp
├── RcHardwareDriver.h
├── RcInputAdapter.cpp
├── RcInputAdapter.h
├── RumbleManager.cpp
└── RumbleManager.h
```
