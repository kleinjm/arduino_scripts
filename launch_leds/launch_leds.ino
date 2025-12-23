int switchState = 0;
const int PIN_RED2 = 5;
const int PIN_RED1 = 4;
const int PIN_GREEN = 3;
const int PIN_SWITCH = 2;

void setup() {
  pinMode(PIN_RED1, OUTPUT);
  pinMode(PIN_RED2, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_SWITCH, INPUT);
}

void loop() {
  switchState = digitalRead(2);

  if(switchState == LOW) {
    digitalWrite(PIN_GREEN, HIGH);
    digitalWrite(PIN_RED1, LOW);
    digitalWrite(PIN_RED2, LOW);
  } else {
    digitalWrite(PIN_GREEN, LOW);
    digitalWrite(PIN_RED1, LOW);
    digitalWrite(PIN_RED2, HIGH);

    delay(250);
    digitalWrite(PIN_RED1, HIGH);
    digitalWrite(PIN_RED2, LOW);
    delay(250);
  }
}
