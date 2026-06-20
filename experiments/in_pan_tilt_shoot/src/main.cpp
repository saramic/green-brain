#include <Arduino.h>
#include <Servo.h>

// KY-023 dual-axis joystick
//   VRX  -> A0   (X axis, 0-1023)
//   VRY  -> A1   (Y axis, 0-1023)
//   SW   -> D2   (button, active LOW — pin has internal pull-up)
//   +5V  -> 5V
//   GND  -> GND

// 9g servos (SG90 or equivalent)
//   Pan  signal -> D9   (Timer1, safe for Servo library)
//   Tilt signal -> D10
//   Power: 5V from external supply recommended; GND shared with Nano

// Relay for water pump
//   FIRE_PIN -> D7 -> relay IN pin (active HIGH; swap to LOW if your relay is active-LOW)
//   Relay VCC -> 5V, GND -> GND (shared with Nano)

#define JOY_X_PIN   A0
#define JOY_Y_PIN   A1
#define JOY_SW_PIN  2
#define PAN_PIN     9
#define TILT_PIN    10
#define FIRE_PIN    7

// Dead-zone around the calibrated centre — sized to absorb KY-023 resting offset.
#define DEADZONE 20

// Mechanical travel limits in degrees — tighten if your assembly binds.
// Typical SG90 can do 0-180 but the pan/tilt frame usually runs 20-160.
#define PAN_MIN   0
#define PAN_MAX  180
#define TILT_MIN  0  // ~20° lower now accessible via wider pulse range
#define TILT_MAX 180

// Degrees per 20 ms tick at full joystick deflection.
// Lower = faster. 50 ≈ SG90 physical limit (~500 deg/s). 100 = half speed.
#define SPEED_DIVISOR 100

// ms after last movement before PWM is cut. Gives servo time to reach position.
#define SETTLE_MS  400

// How long the pump runs per button press (ms).
#define FIRE_MS   2000

Servo pan;
Servo tilt;

int panAngle  = 90;
int tiltAngle = 90;
int centerX, centerY; // calibrated at startup — KY-023 rarely rests at exactly 512
unsigned long lastMoveMs  = 0;
unsigned long fireStartMs = 0;  // millis() when last fire started; 0 = idle
bool attached = true;

void setup() {
    Serial.begin(115200);
    pinMode(JOY_SW_PIN, INPUT_PULLUP);
    pinMode(FIRE_PIN, OUTPUT);
    digitalWrite(FIRE_PIN, LOW);

    // Wider pulse range (500–2500 µs) unlocks the ~10° of travel that the
    // default 544–2400 µs range hides at each end of SG90-style servos.
    pan.attach(PAN_PIN,   500, 2500);
    tilt.attach(TILT_PIN, 500, 2500);
    pan.write(panAngle);
    tilt.write(tiltAngle);

    // Sample the resting position — keep stick centred on power-on.
    delay(200); // let ADC settle after power-on
    centerX = analogRead(JOY_X_PIN);
    centerY = analogRead(JOY_Y_PIN);

    lastMoveMs = millis(); // start settle timer from here, not from 0

    Serial.println(F("GreenBrain | Pan-Tilt Shoot"));
    Serial.print(F("Center X:")); Serial.print(centerX);
    Serial.print(F(" Y:"));       Serial.println(centerY);
    Serial.println(F("pan\ttilt\tbtn"));
}

void loop() {
    int x    = analogRead(JOY_X_PIN);
    int y    = analogRead(JOY_Y_PIN);
    bool btn = !digitalRead(JOY_SW_PIN); // LOW when pressed

    // Offset from calibrated centre; dead-zone suppresses residual noise.
    int dx = x - centerX;
    int dy = y - centerY;
    if (abs(dx) < DEADZONE) dx = 0;
    if (abs(dy) < DEADZONE) dy = 0;

    // Joystick deflection drives rate, not position.
    // Negate dy so pushing forward tilts up.
    int newPan  = constrain(panAngle  - dx / SPEED_DIVISOR, PAN_MIN,  PAN_MAX);
    int newTilt = constrain(tiltAngle - dy / SPEED_DIVISOR, TILT_MIN, TILT_MAX);

    unsigned long now = millis();

    // Arm on release: button must be seen released before it can fire.
    // Avoids boot-state issues where SW reads LOW before user touches it.
    static bool armed = false;
    if (!btn) armed = true;
    bool btnPress = btn && armed;

    // Fire: one press = one timed burst. Elapsed-time check is rollover-safe.
    if (btnPress && !fireStartMs) {
        armed = false;
        if (!attached) {
            pan.attach(PAN_PIN,   500, 2500);
            tilt.attach(TILT_PIN, 500, 2500);
            attached = true;
        }
        // Always write last aimed position at fire start — holds against hose weight.
        pan.write(panAngle);
        tilt.write(tiltAngle);
        digitalWrite(FIRE_PIN, HIGH);
        fireStartMs = now ? now : 1; // avoid 0 (reserved for "idle")
    } else if (fireStartMs && now - fireStartMs >= FIRE_MS) {
        digitalWrite(FIRE_PIN, LOW);
        fireStartMs = 0;
    }
    bool firing = fireStartMs != 0;

    bool moved = !firing && ((newPan != panAngle) || (newTilt != tiltAngle));

    if (moved || firing) {
        if (!attached) {
            pan.attach(PAN_PIN,   500, 2500);
            tilt.attach(TILT_PIN, 500, 2500);
            attached = true;
        }
        if (moved) {
            panAngle  = newPan;
            tiltAngle = newTilt;
            pan.write(panAngle);
            tilt.write(tiltAngle);
        }
        lastMoveMs = now;
    } else if (attached && now - lastMoveMs > SETTLE_MS) {
        // Servo has had time to reach position — kill PWM to stop buzzing.
        pan.detach();
        tilt.detach();
        attached = false;
    }

    Serial.print(panAngle);
    Serial.print(F("\t"));
    Serial.print(tiltAngle);
    Serial.print(F("\t"));
    Serial.println(firing ? F("FIRE") : F("-"));

    delay(20); // 50 Hz
}
