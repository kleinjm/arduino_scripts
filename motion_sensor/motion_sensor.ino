const int pirPin = 13; // PIR sensor output pin

void setup() {
  pinMode(pirPin, INPUT);
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
    Serial.println("Motion Detected!");
  } else {
    Serial.println("quiet...");
  }
  delay(1000);
}