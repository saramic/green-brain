#include <Arduino.h>
#include <Servo.h>
#include <EEPROM.h>

#define JOY_X_PIN  A0
#define JOY_Y_PIN  A1
#define JOY_SW_PIN 2
#define PAN_PIN    9
#define TILT_PIN   10
#define FIRE_PIN   7

#define DEADZONE      50
#define SPEED_DIVISOR 100
#define PAN_MIN       45
#define PAN_MAX       135
#define TILT_MIN      45
#define TILT_MAX      135
#define FIRE_MS       2000

Servo pan, tilt;
int panAngle  = 90;
int tiltAngle = 90;
int centerX, centerY;
unsigned long fireStartMs = 0;

static void servoAttach() {
    pan.attach(PAN_PIN,   500, 2500);
    tilt.attach(TILT_PIN, 500, 2500);
    pan.write(panAngle);
    tilt.write(tiltAngle);
}

static void servoDetach() {
    pan.detach();
    tilt.detach();
    // Drive signal lines LOW so they don't float and pick up pump EMI
    pinMode(PAN_PIN,  OUTPUT); digitalWrite(PAN_PIN,  LOW);
    pinMode(TILT_PIN, OUTPUT); digitalWrite(TILT_PIN, LOW);
}

void setup() {
    Serial.begin(115200);
    pinMode(JOY_SW_PIN, INPUT_PULLUP);
    pinMode(FIRE_PIN, OUTPUT);
    digitalWrite(FIRE_PIN, LOW);

    uint8_t boots = EEPROM.read(0) + 1;
    EEPROM.write(0, boots);

    delay(200);
    centerX = analogRead(JOY_X_PIN);
    centerY = analogRead(JOY_Y_PIN);

    servoAttach();

    Serial.println(F("GreenBrain | Pan/Tilt"));
    Serial.print(F("Boot #")); Serial.print(boots);
    Serial.print(F(" Center X:")); Serial.print(centerX);
    Serial.print(F(" Y:"));        Serial.println(centerY);
}

void loop() {
    int rawX = analogRead(JOY_X_PIN);
    int rawY = analogRead(JOY_Y_PIN);
    bool btn = !digitalRead(JOY_SW_PIN);
    unsigned long now = millis();

    static bool armed = false;
    if (!btn) armed = true;
    bool btnPress = btn && armed;

    bool currentlyFiring = fireStartMs != 0;

    // IIR filter — freeze during firing to ignore pump motor rail noise
    static float filtX = 512, filtY = 512;
    if (!currentlyFiring) {
        filtX = (rawX + filtX) / 2.0f;
        filtY = (rawY + filtY) / 2.0f;
    }

    int dx = (int)filtX - centerX;
    int dy = (int)filtY - centerY;

    if (btnPress && !currentlyFiring) {
        armed = false;
        if (!pan.attached()) servoAttach();  // re-attach if idle, then hold position
        digitalWrite(FIRE_PIN, HIGH);
        fireStartMs = now ? now : 1;
    } else if (currentlyFiring && now - fireStartMs >= FIRE_MS) {
        digitalWrite(FIRE_PIN, LOW);
        fireStartMs = 0;
        servoDetach();
    }
    bool firing = fireStartMs != 0;

    // Move only when not firing and joystick outside deadzone
    if (!firing && !btn) {
        if (abs(dx) > DEADZONE) {
            int newPan = constrain(panAngle - dx / SPEED_DIVISOR, PAN_MIN, PAN_MAX);
            if (newPan != panAngle) {
                panAngle = newPan;
                if (!pan.attached()) servoAttach();
                pan.write(panAngle);
            }
        }
        if (abs(dy) > DEADZONE) {
            int newTilt = constrain(tiltAngle + dy / SPEED_DIVISOR, TILT_MIN, TILT_MAX);
            if (newTilt != tiltAngle) {
                tiltAngle = newTilt;
                if (!tilt.attached()) servoAttach();
                tilt.write(tiltAngle);
            }
        }
    }

    static unsigned long lastPrintMs = 0;
    if (now - lastPrintMs >= 100) {
        lastPrintMs = now;
        Serial.print(F("btn="));    Serial.print(btn    ? F("1") : F("0"));
        Serial.print(F(" armed="));  Serial.print(armed  ? F("1") : F("0"));
        Serial.print(F(" firing=")); Serial.print(firing ? F("1") : F("0"));
        Serial.print(F(" pan="));   Serial.print(panAngle);
        Serial.print(F(" tilt="));  Serial.print(tiltAngle);
        Serial.print(F(" dx="));    Serial.print(dx);
        Serial.print(F(" dy="));    Serial.println(dy);
    }

    delay(20);
}
