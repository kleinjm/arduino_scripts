void setup() {
  Serial.begin(9600);
}

void loop() {
  int val = analogRead(A0);
  Serial.write(val/4);

  delay(10);
}
