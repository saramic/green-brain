#include <Arduino.h>

// KY-023 dual-axis joystick
//   VRX  -> A0   (X axis, 0-1023)
//   VRY  -> A1   (Y axis, 0-1023)
//   SW   -> D2   (button, active LOW — pin has internal pull-up)
//   +5V  -> 5V
//   GND  -> GND

#define JOY_X_PIN   A0
#define JOY_Y_PIN   A1
#define JOY_SW_PIN  2

// Dead-zone around centre (512) to suppress noise when at rest.
#define DEADZONE 20

void setup() {
    Serial.begin(115200);
    pinMode(JOY_SW_PIN, INPUT_PULLUP);
    Serial.println(F("GreenBrain | Pan-Tilt Shoot | Joystick"));
    Serial.println(F("x\ty\tbtn"));
}

void loop() {
    int x   = analogRead(JOY_X_PIN);
    int y   = analogRead(JOY_Y_PIN);
    bool btn = !digitalRead(JOY_SW_PIN); // LOW when pressed

    // Centre-offset with dead-zone applied (-512..+512, 0 in dead-zone)
    int dx = x - 512;
    int dy = y - 512;
    if (abs(dx) < DEADZONE) dx = 0;
    if (abs(dy) < DEADZONE) dy = 0;

    Serial.print(dx);
    Serial.print(F("\t"));
    Serial.print(dy);
    Serial.print(F("\t"));
    Serial.println(btn ? F("FIRE") : F("-"));

    delay(100);
}
