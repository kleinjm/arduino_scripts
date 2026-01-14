# Automated Blinds

ESP32-based motorized blind controller using a TMC2209 stepper driver with UART control. Runs on ESPHome for easy Home Assistant integration.

## Features

- **Wi-Fi Control**: Control blinds via web interface or Home Assistant
- **Position Presets**: Open 25%, 50%, 75%, or fully open/closed
- **Quiet Operation**: StealthChop mode for silent movement
- **Gentle Homing**: Reduced speed/torque during calibration to minimize mechanical wear
- **Time-Based Homing**: Reliable position calibration without requiring physical endstops

## Hardware

### Components

- ESP32 DevKit V1
- TMC2209 Stepper Driver (UART mode)
- NEMA 17 or similar bipolar stepper motor
- 12-24V power supply (for motor)

### Pin Connections

#### ESP32 Pinout

| ESP32 Pin | Function | Connected To |
|-----------|----------|--------------|
| GPIO22 | UART TX | TMC2209 PDN/UART |
| GPIO23 | UART RX | TMC2209 PDN/UART |
| GPIO27 | STEP | TMC2209 STEP |
| GPIO14 | DIR | TMC2209 DIR |
| GPIO13 | ENABLE | TMC2209 EN |
| GPIO25 | DIAG | TMC2209 DIAG (unused) |

#### TMC2209 Pinout

| TMC2209 Pin | Function | Connected To |
|-------------|----------|--------------|
| VM | Motor Power | 12-24V Supply |
| GND | Ground | Common Ground |
| VIO | Logic Power | 3.3V from ESP32 |
| EN | Enable | ESP32 GPIO13 |
| STEP | Step Input | ESP32 GPIO27 |
| DIR | Direction | ESP32 GPIO14 |
| PDN/UART | UART Comms | ESP32 GPIO22 (TX) & GPIO23 (RX) |
| DIAG | Diagnostic | ESP32 GPIO25 (optional) |
| M1A, M1B | Motor Coil 1 | Stepper Motor |
| M2A, M2B | Motor Coil 2 | Stepper Motor |
| MS1, MS2 | Address | Both GND for address 0 |

## Configuration

The following values in `automated_blinds_esphome.yaml` can be adjusted for your setup:

### Movement Range

| Parameter | Default | Min | Max | Description |
|-----------|---------|-----|-----|-------------|
| `blind_max_steps` | 20000 | 1000 | 100000 | Total steps for full blind travel |
| `homing target` | -25000 | -100000 | -1000 | Steps to overshoot during homing |
| `homing delay` | 90s | 10s | 300s | Wait time for homing to complete |

### Normal Operation (Fast)

| Parameter | Default | Min | Max | Description |
|-----------|---------|-----|-----|-------------|
| `run_current` | 1000mA | 100mA | 2000mA | Motor current during movement |
| `hold_current` | 400mA | 0mA | 1000mA | Motor current when stationary |
| `max_speed` | 800 steps/s | 100 | 2000 | Maximum motor speed |
| `acceleration` | 400 | 100 | 2000 | Acceleration rate |
| `deceleration` | 400 | 100 | 2000 | Deceleration rate |

### Homing Operation (Gentle)

| Parameter | Default | Min | Max | Description |
|-----------|---------|-----|-----|-------------|
| `run_current` | 400mA | 100mA | 1000mA | Reduced current for gentle homing |
| `hold_current` | 200mA | 0mA | 500mA | Hold current during homing |
| `speed` | 300 steps/s | 50 | 500 | Slower speed to reduce wear |

### Stepper Settings

| Parameter | Default | Options | Description |
|-----------|---------|---------|-------------|
| `microsteps` | 16 | 1, 2, 4, 8, 16, 32, 64, 128, 256 | Microstepping resolution |
| `enable_spreadcycle` | false | true/false | false = StealthChop (quiet), true = SpreadCycle (more torque) |

## Usage

1. **Initial Setup**: Press "Home (Close Fully)" to calibrate position
2. **Operation**: Use Open/Close buttons or position presets
3. **Emergency**: Press "Stop" to halt movement immediately

## Installation

1. Copy `automated_blinds_esphome.yaml` to your ESPHome config directory
2. Create a `secrets.yaml` with your Wi-Fi credentials:
   ```yaml
   wifi_ssid: "YourNetworkName"
   wifi_password: "YourPassword"
   ```
3. Compile and upload: `esphome run automated_blinds_esphome.yaml`

---

<details>
<summary><strong>Technical Details</strong></summary>

### Final Implementation: Time-Based Homing

The final implementation uses **time-based homing** rather than sensorless stall detection:

1. **Homing Sequence**:
   - Reduce motor current to 400mA and speed to 300 steps/s
   - Command motor to move -25000 steps (toward closed position)
   - Wait 90 seconds for movement to complete
   - Motor hits mechanical stop and stalls harmlessly (low torque)
   - Reset position counter to 0
   - Restore normal current (1000mA) and speed (800 steps/s)

2. **Normal Operation**:
   - Motor moves to absolute step positions
   - Position 0 = fully closed, `blind_max_steps` = fully open
   - Percentage positions calculated from `blind_max_steps`

3. **Why This Works**:
   - No sensor hardware required
   - Reliable across all conditions
   - Low torque during homing prevents mechanical damage
   - Simple implementation with predictable behavior

### Approaches That Did Not Work

#### 1. TMC2209 StallGuard via DIAG Pin (Hardware Interrupt)

**Approach**: Use the TMC2209's built-in StallGuard feature which asserts the DIAG pin when motor stalls.

**Why It Failed**:
- DIAG pin worked correctly in standalone Arduino sketch (`diag_connection_test.ino`)
- ESPHome's external TMC2209 component uses GPIO interrupts for DIAG detection
- The interrupt-based detection failed to trigger despite identical register configuration
- Root cause: ESPHome's main-loop-based step generation has inconsistent timing compared to Arduino's tight polling loop

#### 2. GPIO Polling of DIAG Pin

**Approach**: Bypass the component's interrupt handler by polling DIAG pin directly with `digitalRead()`.

**Why It Failed**:
- DIAG pin did go HIGH, but was extremely sensitive
- Triggered immediately on motor start, even without physical stall
- SGTHRS threshold couldn't be tuned - values that prevented false triggers also missed real stalls
- SG_RESULT values were too noisy (oscillating 0-180) to distinguish running vs stalled

#### 3. Software SG_RESULT Threshold Detection

**Approach**: Monitor SG_RESULT register directly and trigger stall when value drops below threshold.

**Why It Failed**:
- SG_RESULT values highly inconsistent (2-180 range during normal operation)
- Rolling average smoothing helped but couldn't reliably distinguish states
- Minimum SG during normal running overlapped with stall values
- Even with warmup period and consecutive-reading requirements, false positives persisted

#### 4. CS_ACTUAL Current-Based Detection

**Approach**: Monitor CS_ACTUAL (actual motor current) which should spike during stall.

**Why It Failed**:
- CS_ACTUAL stayed constant at ~27 (near max) during both normal operation and stall
- TMC2209's current regulation maintains set current regardless of load
- No measurable difference between running freely and stalled condition

#### 5. SpreadCycle Mode (vs StealthChop)

**Approach**: Switch from StealthChop to SpreadCycle mode hoping for more stable StallGuard readings.

**Why It Failed**:
- SG_RESULT values were actually worse (lower baseline, more noise)
- False stall triggers occurred within seconds of motor start
- SpreadCycle is noisier and didn't improve detection reliability

### Key Learnings

1. **StallGuard requires consistent step timing**: The TMC2209's StallGuard works by measuring back-EMF between steps. ESPHome's main-loop stepping introduces timing jitter that corrupts these measurements.

2. **Arduino success vs ESPHome failure**: The Arduino sketch worked because it used `delayMicroseconds()` in a tight loop for consistent ~833 steps/s. ESPHome's cooperative multitasking limits reliable step rates to ~400-800 steps/s with variable timing.

3. **Current sensing doesn't help**: The TMC2209 regulates current to the set value, so CS_ACTUAL doesn't increase during stall - the driver is already pushing maximum configured current.

4. **Time-based homing is industry standard**: Many commercial motorized blinds use this approach - run until hitting mechanical stop, then set home position. It's simple and reliable.

</details>
