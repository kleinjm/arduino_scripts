#include <Servo.h>
Servo panServo;
Servo tiltServo;

const int panPin = A0, tiltPin = A1;
float smoothPan = 512, smoothTilt = 512;
float alpha = 0.85; // Smoothing: 0.9 = very smooth, 0.1 = raw

unsigned long lastPrint = 0;

void setup() {
  // Calibrate pulse widths for full SG90 range (adjust these if it stalls)
  panServo.attach(2, 500, 2500); 
  tiltServo.attach(3, 500, 2500);
  Serial.begin(9600);
}

void loop() {
  // Smooth the noisy analog signal
  smoothPan = (alpha * smoothPan) + ((1 - alpha) * analogRead(panPin));
  smoothTilt = (alpha * smoothTilt) + ((1 - alpha) * analogRead(tiltPin));

  // Map to degrees. If it jitters at ends, change 0/180 to 10/170.
  int panAngle = map(smoothPan, 0, 1023, 0, 180);
  int tiltAngle = map(smoothTilt, 0, 1023, 0, 90);

  panServo.write(panAngle);
  tiltServo.write(tiltAngle);

  // Print debugging data every 200ms without slowing down the motor
  if (millis() - lastPrint > 200) {
    Serial.print("Pan: "); Serial.print(panAngle);
    Serial.print("\tTilt: "); Serial.println(tiltAngle);
    lastPrint = millis();
  }
}
