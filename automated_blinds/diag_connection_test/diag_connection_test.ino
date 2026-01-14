#include <Arduino.h>
#include <TMCStepper.h>

// --- PIN CONFIGURATION ---
#define TX_PIN           22
#define RX_PIN           23
#define DIAG_PIN         25  // The pin we are testing
#define EN_PIN           13
#define DIR_PIN          14
#define STEP_PIN         27

#define DRIVER_ADDRESS   0b00
#define R_SENSE          0.11f

TMC2209Stepper driver(&Serial2, R_SENSE, DRIVER_ADDRESS);

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  pinMode(EN_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(DIAG_PIN, INPUT_PULLDOWN); // Use Pulldown to see the HIGH pulse clearly

  digitalWrite(EN_PIN, LOW); // Enable driver

  delay(2000);
  Serial.println("\n--- StallGuard DIAG Signal Debugger ---");

  if (driver.version() != 0x21) {
    Serial.println("ERROR: TMC2209 not found via UART. Check wiring!");
    while(1);
  }

  // --- STALLGUARD CONFIGURATION ---
  driver.begin();
  driver.toff(4);
  driver.blank_time(24);
  driver.rms_current(600);
  driver.microsteps(16);

  // StealthChop is required for StallGuard on TMC2209
  driver.en_spreadCycle(false);
  driver.pwm_autoscale(true);

  // SGTHRS: 0-255. Start high (sensitive) to force a trigger
  driver.SGTHRS(100);

  // VELOCITY THRESHOLDS (Your Documentation Finding)
  // TCOOLTHRS: StallGuard is only active when TSTEP < TCOOLTHRS (Speed > Threshold)
  // Set to max (0xFFFFF) to enable StallGuard at even the slowest speeds.
  driver.TCOOLTHRS(0xFFFFF);

  // TPWMTHRS: StealthChop threshold. Set to 0 to keep StealthChop always active.
  driver.TPWMTHRS(0);

  Serial.println("Config Applied. TCOOLTHRS set to MAX.");
  Serial.println("Observe the 'DIAG' column below. It should flip to 1 when you block the motor.");
}

void loop() {
  static uint32_t last_move = 0;
  static bool direction = HIGH;

  // Move the motor continuously
  digitalWrite(DIR_PIN, direction);

  Serial.println("Testing Motion... [Press Motor Shaft to Test Stall]");

  for(int i = 0; i < 3200; i++) { // 1 full rotation
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(600); // ~300 steps/s
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(600);

    // Monitor DIAG pin during every single step
    bool diag_state = digitalRead(DIAG_PIN);
    uint16_t sg_result = driver.SG_RESULT();

    if (diag_state) {
      Serial.print(">>> STALL DETECTED! DIAG: HIGH | SG_RESULT: ");
      Serial.println(sg_result);
      // Optional: Stop moving on stall for safety
      // break;
    }

    if (i % 400 == 0) { // Periodic log
      Serial.print("DIAG Pin: "); Serial.print(diag_state);
      Serial.print(" | SG_RESULT: "); Serial.println(sg_result);
    }
  }

  direction = !direction;
  delay(1000);
}
