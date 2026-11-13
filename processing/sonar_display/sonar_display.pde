/*
  Sonar / Radar display for Processing (processing.org)
  Reads "angle,distance\n" lines from the Arduino.

  Close the Arduino Serial Monitor before running this!
  If nothing shows up, check the port list printed in the console
  and change PORT_INDEX.
*/

import processing.serial.*;

// ---------- Settings ----------
final int PORT_INDEX = 0;    // index into Serial.list()
final int BAUD       = 9600;
final int MAX_RANGE  = 40;   // cm shown on the radar
final int ALARM_DIST = 20;   // cm, same as on the Arduino

Serial port;
volatile int angle = 0;
volatile int distance = 0;

float cx, cy, R;   // radar centre and radius
float barH;        // info bar height

final color GREEN  = color(98, 245, 31);
final color SWEEP  = color(30, 250, 60);
final color RED    = color(255, 20, 20);

void setup() {
  size(1200, 700);
  smooth();
  barH = height * 0.08;
  cx = width / 2.0;
  cy = height - barH - 10;
  R  = min(width / 2.0, cy) * 0.92;

  printArray(Serial.list());
  port = new Serial(this, Serial.list()[PORT_INDEX], BAUD);
  port.bufferUntil('\n');
  background(0);
}

void draw() {
  // Fade the radar area slightly each frame -> trailing effect
  noStroke();
  fill(0, 6);
  rect(0, 0, width, height - barH);

  drawGrid();
  drawSweep();
  drawObject();
  drawInfoBar();
}

void serialEvent(Serial p) {
  String line = p.readStringUntil('\n');
  if (line == null) return;
  String[] parts = split(trim(line), ',');
  if (parts.length != 2) return;
  angle    = int(parts[0]);
  distance = int(parts[1]);
}

// Polar (degrees, pixels) -> screen coordinates. 0° = right, 180° = left
PVector polar(float deg, float r) {
  float a = radians(deg);
  return new PVector(cx + r * cos(a), cy - r * sin(a));
}

void drawGrid() {
  noFill();
  stroke(GREEN);
  strokeWeight(2);

  // Range arcs
  for (int i = 1; i <= 4; i++) {
    float d = 2 * R * i / 4.0;
    arc(cx, cy, d, d, PI, TWO_PI);
  }
  // Angle lines every 30°
  for (int a = 0; a <= 180; a += 30) {
    PVector e = polar(a, R);
    line(cx, cy, e.x, e.y);
  }

  // Labels
  noStroke();
  fill(GREEN);
  textSize(14);
  textAlign(RIGHT, BOTTOM);
  for (int i = 1; i <= 4; i++) {
    text((MAX_RANGE * i / 4) + " cm", cx + R * i / 4.0 - 4, cy - 4);
  }
  textAlign(CENTER, CENTER);
  for (int a = 0; a <= 180; a += 30) {
    PVector t = polar(a, R + 20);
    text(a + "°", t.x, t.y);
  }
}

void drawSweep() {
  stroke(SWEEP);
  strokeWeight(6);
  PVector e = polar(angle, R);
  line(cx, cy, e.x, e.y);
}

void drawObject() {
  if (distance <= 0 || distance > MAX_RANGE) return;
  float r = distance * R / MAX_RANGE;
  PVector a = polar(angle, r);
  PVector b = polar(angle, R);
  stroke(RED);
  strokeWeight(6);
  line(a.x, a.y, b.x, b.y);
}

void drawInfoBar() {
  noStroke();
  fill(0);
  rect(0, height - barH, width, barH);

  boolean inRange = distance > 0 && distance <= MAX_RANGE;
  boolean alarm   = distance > 0 && distance <= ALARM_DIST;

  float y = height - barH / 2;
  textSize(24);
  textAlign(LEFT, CENTER);

  fill(GREEN);
  text("Angle: " + angle + "°", width * 0.04, y);
  text("Distance: " + (inRange ? distance + " cm" : "--"), width * 0.24, y);
  text(inRange ? "Object: in range" : "Object: out of range", width * 0.48, y);

  if (alarm) {
    fill(RED);
    text("OBJECT CLOSE!", width * 0.78, y);
  }
}
