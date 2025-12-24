const int optoPin = 2;
const int optoRead = 3;
int setting = 0;

void setup() {
  pinMode(optoPin, OUTPUT);
  pinMode(optoRead, INPUT_PULLUP);
  Serial.begin(9600);
}

void loop() {
  digitalWrite(optoPin, HIGH);
  delay(10);
  setting = digitalRead(optoRead);
  Serial.print("On value: ");
  Serial.println(setting);

  delay(3000);
  digitalWrite(optoPin, LOW);
  delay(10);
  setting = digitalRead(optoRead);
  Serial.print("Off value: ");
  Serial.println(setting);

  delay(3000);
}
