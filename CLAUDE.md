# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This repository contains a collection of standalone Arduino sketches (.ino files) for various hardware projects. Each project is self-contained in its own directory.

## Build and Upload

Arduino sketches are compiled and uploaded using the Arduino IDE or `arduino-cli`:

```bash
# Compile a sketch
arduino-cli compile --fqbn arduino:avr:uno <project_directory>

# Upload to connected board
arduino-cli upload -p /dev/cu.usbmodem* --fqbn arduino:avr:uno <project_directory>

# Open serial monitor (9600 baud is typical for these projects)
arduino-cli monitor -p /dev/cu.usbmodem* -c baudrate=9600
```

## Code Conventions

- Pin numbers are defined as constants at the top of each sketch (e.g., `const int PIN_GREEN = 3`)
- Analog pins use `A0`, `A1`, etc. notation
- Serial communication typically uses 9600 baud rate
- Custom characters for LCD displays are defined as byte arrays

## External Libraries Used

Some sketches require Arduino libraries:
- `LiquidCrystal` - LCD display control
- `Servo` - Servo motor control
- `CapacitiveSensor` - Human touch/capacitance sensing

## Processing Integration

The `potentiometer_reading/logo_processing/` directory contains a Processing sketch (.pde) that reads serial data from an Arduino to control a visual display.
