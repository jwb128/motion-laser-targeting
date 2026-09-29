#include <Arduino.h>
#include <Servo.h>

// ================= PINS =================
const int SIG_PIN   = 7;   // Parallax PING))) 28015 SIG pin
const int SERVO_PIN = 9;   // servo signal wire (orange)

// ============ SENSOR LIMITS =============
const int MIN_CM = 5;      // closer than this is unreliable
const int MAX_CM = 300;    // PING))) max range, about 3 m

// ========== SMOOTHING SETTINGS ==========
// Tune these while testing.
const float ALPHA          = 0.3;  // 0.0-1.0. Lower = smoother but slower to react.
                                   //          Higher = faster but jumpier.
const int MAX_MISSES       = 5;    // this many bad readings in a row = target lost
const int DEADBAND_DEG     = 2;    // ignore servo moves smaller than this (stops jitter)
const int PING_INTERVAL_MS = 50;   // time between pings (keep >= 30 so echoes don't overlap)

Servo servo;

// ============ FILTER STATE ==============
long  history[3];          // the last 3 good readings, for the median filter
int   historyIndex = 0;    // where the next reading goes in history[]
bool  haveTarget   = false;
float smoothedCm   = 0;    // final smoothed distance
int   missCount    = 0;    // bad readings in a row
int   lastAngle    = 90;   // last angle sent to the servo

// ---------------------------------------------------------------
// Sends one ping and returns the distance in cm, or -1 if no echo.
// ---------------------------------------------------------------
long readDistanceCm() {
  pinMode(SIG_PIN, OUTPUT);
  digitalWrite(SIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(SIG_PIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(SIG_PIN, LOW);

  pinMode(SIG_PIN, INPUT);
  long duration = pulseIn(SIG_PIN, HIGH, 30000);  // wait up to 30 ms for the echo
  if (duration == 0) return -1;
  return duration / 29 / 2;  // sound travels ~29 us per cm; divide by 2 for the round trip
}

// ---------------------------------------------------------------
// Returns the middle value of three numbers (the median).
// This throws out a single weird reading.
// ---------------------------------------------------------------
long median3(long a, long b, long c) {
  return max(min(a, b), min(max(a, b), c));
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
  long raw = readDistanceCm();
  bool valid = (raw >= MIN_CM && raw <= MAX_CM);

  if (valid) {
    missCount = 0;

    if (!haveTarget) {
      // First good reading (or just reacquired): fill the history
      // with it so old data doesn't drag the filter around.
      history[0] = history[1] = history[2] = raw;
      smoothedCm = raw;
      haveTarget = true;
    } else {
      // Replace the oldest reading with the new one
      history[historyIndex] = raw;
      historyIndex = (historyIndex + 1) % 3;
    }

    // Stage 1: median of the last 3 readings removes single spikes
    long med = median3(history[0], history[1], history[2]);

    // Stage 2: exponential moving average smooths out small wobble
    smoothedCm = smoothedCm + ALPHA * (med - smoothedCm);

    // Print in "label:value" format so the Arduino IDE Serial Plotter
    // can graph raw vs. smoothed if you want to compare them.
    Serial.print("raw:");      Serial.print(raw);
    Serial.print(" median:");  Serial.print(med);
    Serial.print(" smooth:");  Serial.println(smoothedCm, 1);

  } else {
    // Bad reading: don't touch the filter, just count it
    missCount++;
    Serial.print("miss (");
    Serial.print(missCount);
    Serial.println(")");

    if (missCount >= MAX_MISSES && haveTarget) {
      haveTarget = false;
      Serial.println("Target lost - holding servo position.");
    }
  }

  // Move the servo only if we have a target and the change is
  // big enough to matter (the deadband stops constant twitching)
  if (haveTarget) {
    int angle = map(constrain((long)smoothedCm, MIN_CM, MAX_CM), MIN_CM, MAX_CM, 0, 180);
    if (abs(angle - lastAngle) >= DEADBAND_DEG) {
      servo.write(angle);
      lastAngle = angle;
    }
  }

  delay(PING_INTERVAL_MS);
}