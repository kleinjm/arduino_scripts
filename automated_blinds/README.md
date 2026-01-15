# Automated Blinds

ESP32-based motorized blind controller using a TMC2209 stepper driver with UART control. Runs on ESPHome for easy Home Assistant integration.

## Features

- **Wi-Fi Control**: Control blinds via web interface or Home Assistant
- **Position Presets**: Open 25%, 50%, 75%, or fully open/closed
- **Quiet Operation**: StealthChop mode for silent movement
- **StallGuard Homing**: Automatic endstop detection using TMC2209's sensorless stall detection
- **Gentle Homing**: Reduced speed/torque during calibration to minimize mechanical wear

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
| GPIO25 | DIAG | TMC2209 DIAG |

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
| DIAG | Diagnostic | ESP32 GPIO25 |
| M1A, M1B | Motor Coil 1 | Stepper Motor |
| M2A, M2B | Motor Coil 2 | Stepper Motor |
| MS1, MS2 | Address | Both GND for address 0 |

## Configuration

The following values in `automated_blinds_esphome.yaml` can be adjusted for your setup:

### Movement Range

| Parameter | Default | Description |
|-----------|---------|-------------|
| `blind_max_steps` | 20000 | Total steps for full blind travel |

### Normal Operation

| Parameter | Default | Description |
|-----------|---------|-------------|
| `run_current` | 1000mA | Motor current during movement |
| `hold_current` | 400mA | Motor current when stationary |
| `speed` | 800 steps/s | Motor speed |

### Homing Operation (Gentle)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `run_current` | 400mA | Reduced current for gentle homing |
| `hold_current` | 200mA | Hold current during homing |
| `speed` | 300 steps/s | Slower speed for reliable stall detection |
| `SGTHRS` | 1 | StallGuard threshold (stall triggers when SG_RESULT < 2) |

## Usage

1. **Initial Setup**: Press "Home (Close Fully)" to calibrate position using StallGuard
2. **Operation**: Use Open/Close buttons or position presets
3. **Emergency**: Press "Stop" to halt movement immediately

## Installation

1. Copy `automated_blinds_esphome.yaml` and `tmc2209_hw_stepper.h` to your ESPHome config directory
2. Create a `secrets.yaml` with your Wi-Fi credentials:
   ```yaml
   wifi_ssid: "YourNetworkName"
   wifi_password: "YourPassword"
   ```
3. Compile and upload: `esphome run automated_blinds_esphome.yaml`

## Files

| File | Description |
|------|-------------|
| `automated_blinds_esphome.yaml` | Main ESPHome configuration |
| `tmc2209_hw_stepper.h` | Custom C++ stepper driver with hardware timer and StallGuard |
| `hw_timer_test.yaml` | Test configuration for debugging StallGuard |

---

<details>
<summary><strong>Technical Details</strong></summary>

### Final Implementation: Hardware Timer StallGuard

The final implementation uses a **custom C++ stepper driver** with ESP32 hardware timers for reliable StallGuard-based stall detection.

#### Why Hardware Timers?

The TMC2209's StallGuard feature measures back-EMF between motor steps to detect stalls. This requires **consistent step timing** - any jitter corrupts the measurements. ESPHome's main-loop-based stepping has variable timing due to WiFi, logging, and other tasks running cooperatively.

The solution: Use ESP32's hardware timer interrupts for step generation, completely bypassing ESPHome's stepper component.

#### Implementation Details

**Custom Stepper Class (`tmc2209_hw_stepper.h`)**:
- Uses ESP32 hardware timer at 1MHz resolution
- Timer ISR generates step pulses with consistent timing (~800 steps/s = 625µs per half-step)
- DIAG pin checked in ISR after each step (after 500-step warmup period)
- TMCStepper library handles UART communication with TMC2209

**Key Configuration**:
```cpp
driver_.SGTHRS(1);           // Lowest threshold - DIAG triggers when SG_RESULT < 2
driver_.TCOOLTHRS(0xFFFFF);  // Enable StallGuard at all velocities
driver_.en_spreadCycle(false); // StealthChop mode (quieter)
driver_.pwm_autoscale(true);   // Auto-tune PWM
```

**Homing Sequence**:
1. Reduce motor current to 400mA and speed to 300 steps/s
2. Enable stall detection
3. Command motor to move -30000 steps (toward closed position)
4. Hardware timer ISR monitors DIAG pin after 500-step warmup
5. When DIAG goes HIGH (SG_RESULT < 2), motor stops immediately
6. Position reset to 0, normal current/speed restored
7. Stall detection disabled for normal operation

**Observed StallGuard Values**:
| Condition | SG_RESULT Range |
|-----------|-----------------|
| Normal running | 22-86 |
| Physical stall | 0-2 |

The clear separation between normal operation (SG > 20) and stall (SG < 2) ensures reliable detection with no false positives.

### Approaches That Did Not Work

#### 1. ESPHome TMC2209 Component with DIAG Interrupt

**Approach**: Use the external ESPHome TMC2209 component with its built-in DIAG pin interrupt handler.

**Why It Failed**:
- DIAG pin worked correctly in standalone Arduino sketch
- ESPHome's main-loop-based step generation has inconsistent timing
- StallGuard readings were too noisy (SG_RESULT oscillating 0-180)
- DIAG triggered immediately on motor start due to false readings

#### 2. GPIO Polling of DIAG Pin (ESPHome)

**Approach**: Bypass the component's interrupt handler by polling DIAG pin directly in ESPHome interval.

**Why It Failed**:
- DIAG pin was constantly HIGH during motor operation
- Even with SGTHRS=1, the noisy SG_RESULT values caused false triggers
- Timing jitter from main-loop stepping corrupted StallGuard measurements

#### 3. Software SG_RESULT Threshold Detection

**Approach**: Monitor SG_RESULT register directly via UART and trigger stall when value drops below threshold.

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

#### 6. Time-Based Homing (Fallback)

**Approach**: Skip stall detection entirely - run motor for fixed duration until it hits mechanical stop.

**Why It Was Replaced**:
- Required long timeout (90 seconds) to ensure motor reached stop
- No feedback if motor didn't reach stop (belt slip, obstruction)
- Hardware timer approach proved reliable, making this unnecessary

### Key Learnings

1. **StallGuard requires consistent step timing**: The TMC2209's StallGuard works by measuring back-EMF between steps. Any timing jitter corrupts these measurements. ESPHome's cooperative multitasking introduces too much jitter.

2. **Hardware timers solve the timing problem**: ESP32 hardware timer interrupts provide microsecond-accurate step generation, independent of main loop timing. This is the key to making StallGuard work in ESPHome.

3. **SGTHRS=1 is the sweet spot**: With consistent timing, normal operation produces SG_RESULT values of 22-86, while stalls produce 0-2. Setting SGTHRS=1 (threshold of 2) reliably distinguishes these states.

4. **Warmup period is essential**: StallGuard needs ~500 steps for the PWM autoscale to stabilize. Checking DIAG before this causes false triggers.

5. **Current sensing doesn't work**: The TMC2209 regulates current to the set value, so CS_ACTUAL doesn't increase during stall - the driver is already pushing maximum configured current.

6. **Arduino success was due to tight loop**: The Arduino sketch worked because `delayMicroseconds()` in a tight loop provides consistent timing. Replicating this with hardware timers in ESPHome achieves the same result.

</details>
