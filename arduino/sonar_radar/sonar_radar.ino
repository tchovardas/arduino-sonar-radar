/*
  Ultrasonic Sonar / Radar
  Arduino Uno + HC-SR04 + SG90 servo + red LED + buzzer

  Wiring:
    Servo signal (orange/yellow) -> D9    | red -> 5V | brown/black -> GND
    HC-SR04 TRIG                 -> D10   | VCC -> 5V | GND -> GND
    HC-SR04 ECHO                 -> D11
    LED anode (long leg)         -> 220 Ohm resistor -> D6, cathode -> GND
    Buzzer (+)                   -> D5,   buzzer (-) -> GND

  Serial output (9600 baud), one line per step:  "angle,distance\n"
  distance = 0 means "no echo / out of range".
*/

#include <Servo.h>

// ---------- Pins ----------
const uint8_t PIN_SERVO  = 9;
const uint8_t PIN_TRIG   = 10;
const uint8_t PIN_ECHO   = 11;
const uint8_t PIN_LED    = 6;
const uint8_t PIN_BUZZER = 5;

// ---------- Settings ----------
#define BUZZER_PASSIVE 1          // 1 = passive buzzer (needs tone), 0 = active buzzer (beeps on its own with DC)

const int ANGLE_MIN       = 15;   // sweep limits in degrees
const int ANGLE_MAX       = 165;
const int STEP_DELAY_MS   = 30;   // time for the servo to move 1 degree
const int ALARM_DIST_CM   = 20;   // LED + buzzer trigger below this distance
const unsigned long ALARM_HOLD_MS = 600; // keep alarm on this long after the last close detection
const unsigned int  BUZZ_FREQ_HZ  = 2500;

// ---------- State ----------
Servo servo;
unsigned long alarmUntil = 0;
unsigned long lastToggle = 0;
int  alarmDist = ALARM_DIST_CM;
bool outputOn  = false;

// ---------------------------------------------------------------

void setup() {
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_TRIG, LOW);

  Serial.begin(9600);
  servo.attach(PIN_SERVO);
  servo.write(ANGLE_MIN);
  delay(500);
}

void loop() {
  for (int a = ANGLE_MIN; a <= ANGLE_MAX; a++) scanStep(a);
  for (int a = ANGLE_MAX; a >= ANGLE_MIN; a--) scanStep(a);
}

// One step of the sweep: move, measure, report, update alarm
void scanStep(int angle) {
  servo.write(angle);
  waitWithAlarm(STEP_DELAY_MS);

  int d = readDistanceCm();

  // Alarm only if 2 consecutive steps see something close
  static uint8_t closeCount = 0;
  if (d > 0 && d <= ALARM_DIST_CM) {
    if (closeCount < 255) closeCount++;
  } else {
    closeCount = 0;
  }
  if (closeCount >= 2) {
    alarmUntil = millis() + ALARM_HOLD_MS;
    alarmDist = d;
  }

  Serial.print(angle);
  Serial.print(',');
  Serial.println(d);

  updateAlarm();
}

// Filtered distance: median of 3 pings, rejects single random spikes.
// Returns cm, 0 if no valid echo.
int readDistanceCm() {
  int r[3];
  for (int i = 0; i < 3; i++) {
    r[i] = pingOnceCm();
    waitWithAlarm(10);              // let old echoes die out before the next ping
  }
  // sort 3 values, take the middle one
  if (r[0] > r[1]) { int t = r[0]; r[0] = r[1]; r[1] = t; }
  if (r[1] > r[2]) { int t = r[1]; r[1] = r[2]; r[2] = t; }
  if (r[0] > r[1]) { int t = r[0]; r[0] = r[1]; r[1] = t; }
  int d = r[1];
  if (d < 3 || d > 400) return 0;   // outside the HC-SR04's reliable range
  return d;
}

// Single raw ping, 0 if no echo (timeout ~25 ms => ~4 m)
int pingOnceCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long t = pulseIn(PIN_ECHO, HIGH, 25000UL);
  if (t == 0) return 0;
  return (int)(t / 58UL);          // t[us] * 0.0343 cm/us / 2
}

// Delay that keeps the beeping/blinking responsive
void waitWithAlarm(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) updateAlarm();
}

// Blink LED + beep, faster the closer the object is
void updateAlarm() {
  unsigned long now = millis();

  if ((long)(alarmUntil - now) > 0) {
    int dc = constrain(alarmDist, 2, ALARM_DIST_CM);
    unsigned long interval = map(dc, 2, ALARM_DIST_CM, 50, 300);
    if (now - lastToggle >= interval) {
      lastToggle = now;
      outputOn = !outputOn;
      setOutputs(outputOn);
    }
  } else if (outputOn) {
    outputOn = false;
    setOutputs(false);
  }
}

void setOutputs(bool on) {
  digitalWrite(PIN_LED, on ? HIGH : LOW);
#if BUZZER_PASSIVE
  if (on) tone(PIN_BUZZER, BUZZ_FREQ_HZ);
  else    noTone(PIN_BUZZER);
#else
  digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
#endif
}
