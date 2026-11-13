/*
  Servo centering helper
  Upload this BEFORE attaching the sensor mount to the servo horn.
  It moves the servo to 90° (middle of the sweep) and holds it there,
  so the sensor can be mounted facing straight forward.

  Servo signal -> D9, V+ -> 5V, GND -> GND
*/

#include <Servo.h>

Servo servo;

void setup() {
  servo.attach(9);
  servo.write(90);
}

void loop() {
}
