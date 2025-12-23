const int switchPin = 2;
const int motorPin = 9;
int switchState = 0;

void setup() {
  pinMode(motorPin, OUTPUT);
  pinMode(switchPin, INPUT);
  digitalWrite(motorPin, LOW);
  Serial.begin(9600);
}

void loop() {
  switchState = digitalRead(switchPin);

  Serial.print("Switch state: ");
  Serial.print(switchState);
  digitalWrite(motorPin, LOW);

  if(switchState == HIGH) {
    Serial.print(", Switch: ON ");
    digitalWrite(motorPin, HIGH);
  } else {
    Serial.print(", Switch: OFF ");
    digitalWrite(motorPin, LOW);
  }
  int motorSignal = digitalRead(motorPin);
  Serial.print(", Motor pin: ");
  Serial.println(motorSignal);
}
