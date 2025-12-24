const int pirPin = 11; // PIR sensor output pin
const int ledPin = 13; // Built-in Arduino LED

void setup() {
  pinMode(pirPin, INPUT);
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);

  // Calibration phase
  Serial.print("Calibrating sensor... please wait");
  for(int i = 0; i < 20; i++){
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" ACTIVE");
}

void loop() {
  int motion = digitalRead(pirPin);

  if (motion == HIGH) {
    digitalWrite(ledPin, HIGH); // Turn on LED if motion is detected
    Serial.println("Motion Detected!");
  } else {
    digitalWrite(ledPin, LOW);  // Turn off LED if no motion
  }
}