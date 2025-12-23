int buttons[6];
int notes[] = {262,294,330,349};

void setup() {
  buttons[0] = 2;
  Serial.begin(9600);
}

// Mary had a little lamb (E D C D E E E, D D D E E E, E D C D E E E, D D E D C)
void loop() {
  int keyVal = analogRead(A0);
  Serial.println(keyVal);
  if(keyVal == 1023) {
    tone(8, notes[0]); // C
  } else if(keyVal >= 990 && keyVal <= 1010) {
    tone(8, notes[1]); // D
  } else if(keyVal >= 505 && keyVal <= 515) {
    tone(8, notes[2]); // E
  } else if(keyVal >= 5 && keyVal <= 10) {
    tone(8, notes[3]); // F
  } else {
    noTone(8);
  }
}
