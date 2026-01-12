// Basic Stepper Test for TMC2209 and ESP32
const int stepPin = 12; 
const int dirPin = 14; 
const int stepsPerRev = 2000; // Standard for NEMA 17
const int lightSensorPin = 34; // GPIO 34 (Analog ADC1_CH6)

void setup() {
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  Serial.begin(115200);
  Serial.println("--- Blind Light Sensor Calibration ---");
}

void loop() {
  // Set direction counter-clockwise
  digitalWrite(dirPin, LOW);

  // Spin one revolution slowly
  for(int x = 0; x < stepsPerRev; x++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(500); // Speed: lower is faster
    digitalWrite(stepPin, LOW);
    delayMicroseconds(500);
  }
  
  // // Read the raw analog value (0 to 4095 on ESP32)
  // int rawValue = analogRead(lightSensorPin);
  
  // // Convert to voltage (0V to 3.3V)
  // float voltage = rawValue * (3.3 / 4095.0);
  
  // // Print results
  // Serial.print("Raw Value: ");
  // Serial.print(rawValue);
  // Serial.print(" | Voltage: ");
  // Serial.print(voltage);
  // Serial.println("V");

  // delay(500); // Wait half a second between readings
}