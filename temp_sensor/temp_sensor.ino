const int PIN_GREEN1 = 5;
const int PIN_GREEN2 = 4;
const int PIN_GREEN3 = 3;
const int TMP_PIN = A0;
const float BASELINE_TEMP = 68.0; // 20 C = 68 F
const float VOLTAGE = 5.0;
const float SENSOR_RANGE = 1024.0;

void setup() {
  Serial.begin(9600); // open serial port for reading output

  for(int pinNumber = PIN_GREEN3; pinNumber <= PIN_GREEN1; pinNumber++) {
    pinMode(pinNumber, OUTPUT);
    digitalWrite(pinNumber, LOW);
  }
}

void loop() {
  int sensorVal = analogRead(TMP_PIN);
  Serial.print("Sensor Value: ");
  Serial.print(sensorVal);

  float voltage = (sensorVal / SENSOR_RANGE) * VOLTAGE;
  Serial.print(", Volts: ");
  Serial.print(voltage);

  Serial.print(", degrees F: ");
  float temp = (voltage - .5) * 100;
  temp = celsiusToFahrenheit(temp);
  Serial.print(temp);
  Serial.print("\n");

  if(temp < BASELINE_TEMP) {
    digitalWrite(PIN_GREEN3, LOW);
    digitalWrite(PIN_GREEN2, LOW);
    digitalWrite(PIN_GREEN1, LOW);
    delay(1);
    return;
  } else if(temp >= BASELINE_TEMP + 5) {
    digitalWrite(PIN_GREEN3, HIGH);
    digitalWrite(PIN_GREEN2, HIGH);
    digitalWrite(PIN_GREEN1, HIGH);
    delay(1);
    return;
  } else if(temp >= BASELINE_TEMP + 3) {
    digitalWrite(PIN_GREEN3, HIGH);
    digitalWrite(PIN_GREEN2, HIGH);
    digitalWrite(PIN_GREEN1, LOW);
    delay(1);
    return;
  } else if(temp >= BASELINE_TEMP + 1) {
    digitalWrite(PIN_GREEN3, HIGH);
    digitalWrite(PIN_GREEN2, LOW);
    digitalWrite(PIN_GREEN1, LOW);
    delay(1);
    return;
  }
}

float celsiusToFahrenheit(float celsius) {
  const float conversionFactor = 1.8; // 9/5
  const float baseF = 32.0;
  return (celsius * conversionFactor) + baseF;
}