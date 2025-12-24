// Define the pins for each segment
const int segmentPins[] = {2, 3, 4, 5, 6, 7, 8}; // A, B, C, D, E, F, G
const int dotPin = 9;                           // Decimal Point pin

// A table representing the segments for numbers 0-9
// Format: {A, B, C, D, E, F, G}
// 1 = HIGH (On), 0 = LOW (Off)
byte numbers[10][7] = {
  {1, 1, 1, 1, 1, 1, 0}, // 0
  {0, 1, 1, 0, 0, 0, 0}, // 1
  {1, 1, 0, 1, 1, 0, 1}, // 2
  {1, 1, 1, 1, 0, 0, 1}, // 3
  {0, 1, 1, 0, 0, 1, 1}, // 4
  {1, 0, 1, 1, 0, 1, 1}, // 5
  {1, 0, 1, 1, 1, 1, 1}, // 6
  {1, 1, 1, 0, 0, 0, 0}, // 7
  {1, 1, 1, 1, 1, 1, 1}, // 8
  {1, 1, 1, 1, 0, 1, 1}  // 9
};

void setup() {
  // Initialize all segment pins as OUTPUT
  for (int i = 0; i < 7; i++) {
    pinMode(segmentPins[i], OUTPUT);
  }
  pinMode(dotPin, OUTPUT);
}

void displayNumber(int digit) {
  // Loop through the 7 segments and set them based on our table
  for (int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], numbers[digit][i]);
  }
}

void loop() {
  for (int digit = 0; digit <= 9; digit++) {
    // 1. Show the number and turn the Dot ON
    displayNumber(digit);
    digitalWrite(dotPin, HIGH); 
    delay(500); // Wait 1/2 second

    // 2. Keep the number but turn the Dot OFF
    digitalWrite(dotPin, LOW);
    delay(300); // Wait 1/3 second
  }
}