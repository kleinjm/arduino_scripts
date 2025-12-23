# Arduino Scripts

A collection of Arduino sketches for various hardware projects.

## Projects

| Project | Description |
|---------|-------------|
| `digital_hourglass` | LED-based timer that lights up LEDs sequentially |
| `hbridge_motor_controller` | H-bridge DC motor control |
| `human_capacitance_sensor` | Touch-sensitive LED using capacitive sensing |
| `keyboard_instrument` | Musical keyboard using tone generation |
| `launch_leds` | Switch-controlled LED sequence |
| `LCD_control` | LCD display "crystal ball" fortune teller |
| `light_to_sound` | Converts light levels to audio output |
| `motorized_pinwheel` | Motor-controlled spinning display |
| `phototransitor_lamp` | Light-responsive lamp |
| `potentiometer_reading` | Reads potentiometer values with Processing visualization |
| `potentiometer_servo_control` | Controls servo position with potentiometer |
| `temp_sensor` | Temperature sensor with LED indicators |
| `vibration_controlled_servo` | Servo controlled by vibration input |

## Requirements

- Arduino board (tested with Arduino Uno)
- [Arduino IDE](https://www.arduino.cc/en/software) or [arduino-cli](https://arduino.github.io/arduino-cli/)

### Libraries

Some projects require additional libraries (install via Library Manager):
- LiquidCrystal
- Servo
- CapacitiveSensor

## Usage

```bash
# Compile
arduino-cli compile --fqbn arduino:avr:uno <project_directory>

# Upload
arduino-cli upload -p /dev/cu.usbmodem* --fqbn arduino:avr:uno <project_directory>

# Monitor serial output
arduino-cli monitor -p /dev/cu.usbmodem* -c baudrate=9600
```
