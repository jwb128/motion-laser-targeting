#include <Servo.h>

const int SIG_PIN  = 7;    // 28015 SIG
const int PAN_PIN  = 9;    // bottom servo (left/right)
const int TILT_PIN = 10;   // top servo (up/down)

const int MIN_CM = 5;
const int MAX_CM = 300;    // sensor max range, about 3 m

// Keep tilt inside a safe range so the bracket doesn't hit its stops
const int TILT_MIN = 45;
const int TILT_MAX = 135;

Servo pan;
Servo tilt;
int panAngle = 0;
int stepSize = 2;          // degrees per loop; bigger = faster sweep

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
  pan.attach(PAN_PIN);
  tilt.attach(TILT_PIN);

  // Self-test: pan left/right, then tilt up/down, then center both
  pan.write(0);    delay(600);
  pan.write(180);  delay(600);
  pan.write(90);   delay(600);
  tilt.write(TILT_MIN); delay(600);
  tilt.write(TILT_MAX); delay(600);
  tilt.write(90);       delay(600);
  Serial.println("Self-test done.");
}

void loop() {
  long cm = readDistanceCm();
  Serial.print(cm);
  Serial.println(" cm");

  // Bottom servo sweeps left/right continuously
  panAngle += stepSize;
  if (panAngle >= 180) { panAngle = 180; stepSize = -stepSize; }
  else if (panAngle <= 0) { panAngle = 0; stepSize = -stepSize; }
  pan.write(panAngle);

  // Top servo follows distance: near = down, far = up
  if (cm > 0) {
    int tiltAngle = map(constrain(cm, MIN_CM, MAX_CM), MIN_CM, MAX_CM, TILT_MIN, TILT_MAX);
    tilt.write(tiltAngle);
  }
  delay(30);
}
