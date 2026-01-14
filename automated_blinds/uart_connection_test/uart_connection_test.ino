// To run this script from the automated_blinds dir using arduino cli
// arduino-cli compile --fqbn esp32:esp32:esp32 uart_connection_test/uart_connection_test.ino
// arduino-cli upload -p /dev/cu.usbserial-0001 --fqbn esp32:esp32:esp32 uart_connection_test/uart_connection_test.ino
// arduino-cli monitor -p /dev/cu.usbserial-0001 --config baudrate=115200

#include <Arduino.h>
#include <TMCStepper.h>

#define TX_PIN           22
#define RX_PIN           23
#define DRIVER_ADDRESS   0b00
#define R_SENSE          0.11f

// Use Hardware Serial 2
TMC2209Stepper driver(&Serial2, R_SENSE, DRIVER_ADDRESS);

void setup() {
  Serial.begin(115200);
  // The RX/TX order here is crucial for the Serial2.begin function
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  delay(2000);
  Serial.println("--- TMC2209 Final UART Test ---");
  Serial.println("Logic: 1k resistor between GPIO 22 and Pin 5.");
  Serial.println("Logic: Direct wire between GPIO 23 and Pin 5.");
}

void loop() {
  uint8_t version = driver.version();

  if (version == 0x21) {
    Serial.println("!!! SUCCESS !!! TMC2209 Found!");
    uint32_t status = driver.DRV_STATUS();
    Serial.print("Status: "); Serial.println(status, BIN);
  } else {
    Serial.print("Failed. Received: 0x");
    Serial.println(version, HEX);
    Serial.println("Retrying...");
  }
  delay(2000);
}
