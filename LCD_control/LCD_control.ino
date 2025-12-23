// https://docs.arduino.cc/libraries/liquidcrystal/

#include <LiquidCrystal.h>
LiquidCrystal lcd(12,11,5,4,3,2);

const int switchPin = 6;
int switchState = 0;
int prevSwitchState = 0;
int reply;

byte smiley1[8] = {
  B00000,
  B10001,
  B00000,
  B00100,
  B10001,
  B01110,
  B00000,
};
byte smiley2[8] = {
  B00000,
  B10001,
  B00000,
  B00000,
  B00000,
  B01110,
  B10001,
};

void setup() {
  lcd.createChar(0, smiley1);
  lcd.createChar(1, smiley2);
  lcd.begin(16,2);
  pinMode(switchPin, INPUT);

  
  lcd.write(byte(0));
  delay(2000);
  lcd.write(byte(1));
  delay(2000);
  lcd.print("Ask the");
  lcd.setCursor(0, 1);
  lcd.print("Crystal Ball!");
}

void loop() {
  switchState = digitalRead(switchPin);

  if(switchState != prevSwitchState) {
    if(switchState == LOW) {
      reply = random(8);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("The ball says: ");
      lcd.setCursor(0,1);

      switch(reply) {
        case 0:
        lcd.print("Yes");
        break;
        case 1:
        lcd.print("Most likely");
        break;
        case 2:
        lcd.print("Certainly");
        break;
        case 3:
        lcd.print("Outlook good");
        break;
        case 4:
        lcd.print("Unsure");
        break;
        case 5:
        lcd.print("Ask again");
        break;
        case 6:
        lcd.print("Doubtful");
        break;
        case 7:
        lcd.print("No");
        break;
      }
    }
  }
  prevSwitchState = switchState;
}
