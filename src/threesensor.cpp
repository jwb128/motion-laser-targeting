#include <Servo.h>

// ---- Pins ----
const int SIG_PIN    = 7;    // 28015 SIG
const int SENSOR_PIN = 11;   // servo that turns the sensor left/right
const int PAN_PIN    = 9;    // pan/tilt bottom servo (base left/right)
const int TILT_PIN   = 10;   // pan/tilt top servo (up/down)
const int LASER_PIN  = 4;    // -> 1k resistor -> NPN transistor base

// ---- Settings ----
const int TRIGGER_CM = 100;  // detect objects closer than this (sensor max ~300)
const int TILT_AIM   = 90;   // tilt angle while aiming (ultrasonic can't measure height)
const int MISS_LIMIT = 3;    // misses in a row before the laser turns off
int stepSize = 3;            // degrees per scan step; bigger = faster scan

Servo sensorServo;
Servo pan;
Servo tilt;

int sensorAngle = 0;
int misses = MISS_LIMIT;     // start as "nothing detected"

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
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, LOW);

  sensorServo.attach(SENSOR_PIN);
  pan.attach(PAN_PIN);
  tilt.attach(TILT_PIN);

  // Self-test: each servo sweeps once, then centers (laser stays off)
  sensorServo.write(0);   delay(600);
  sensorServo.write(180); delay(600);
  sensorServo.write(90);  delay(600);
  pan.write(0);           delay(600);
  pan.write(180);         delay(600);
  pan.write(90);          delay(600);
  tilt.write(45);         delay(600);
  tilt.write(135);        delay(600);
  tilt.write(90);         delay(600);
  Serial.println("Self-test done. Scanning.");
}

void loop() {
  delay(40);                          // let the sensor servo settle
  long cm = readDistanceCm();
  bool detected = (cm > 0 && cm <= TRIGGER_CM);

  Serial.print(sensorAngle);
  Serial.print(" deg, ");
  Serial.print(cm);
  Serial.println(" cm");

  if (detected) {
    misses = 0;
  } else if (misses < MISS_LIMIT) {
    misses++;
  }

  if (misses < MISS_LIMIT) {
    // Locked on: hold the sensor, aim the pan/tilt, laser on
    pan.write(sensorAngle);
    tilt.write(TILT_AIM);
    digitalWrite(LASER_PIN, HIGH);
  } else {
    // Nothing in range: laser off, keep scanning
    digitalWrite(LASER_PIN, LOW);
    sensorAngle += stepSize;
    if (sensorAngle >= 180) { sensorAngle = 180; stepSize = -stepSize; }
    else if (sensorAngle <= 0) { sensorAngle = 0; stepSize = -stepSize; }
    sensorServo.write(sensorAngle);
  }
}