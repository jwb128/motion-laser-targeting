#include <Servo.h>
#include <Arduino.h>

const int SIG_PIN   = 7;   // 28015 SIG
const int SERVO_PIN = 9;   // servo signal (orange)

const int MIN_CM = 5;      // sensor's practical minimum is about 2-5 cm
const int MAX_CM = 300;    // sensor's max range, about 3 m

Servo servo;

// Returns distance in cm, or -1 if no echo
long readDistanceCm() {
  pinMode(SIG_PIN, OUTPUT);
  digitalWrite(SIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(SIG_PIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(SIG_PIN, LOW);

  pinMode(SIG_PIN, INPUT);
  long duration = pulseIn(SIG_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  return duration / 29 / 2;
}

void setup() {
  Serial.begin(9600);
  servo.attach(SERVO_PIN);

  // Servo self-test: sweep once, then center
  servo.write(0);   delay(600);
  servo.write(180); delay(600);
  servo.write(90);  delay(600);
  Serial.println("Servo test done. Move your hand in front of the sensor.");
}

void loop() {
  long cm = readDistanceCm();
  Serial.print(cm);
  Serial.println(" cm");

  if (cm > 0) {
    // 5 cm -> 0 degrees, 300 cm -> 180 degrees
    int angle = map(constrain(cm, MIN_CM, MAX_CM), MIN_CM, MAX_CM, 0, 180);
    servo.write(angle);
  }
  delay(60);
}