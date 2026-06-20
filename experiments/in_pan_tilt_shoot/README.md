# Pan-Tilt Shoot

Arduino Nano project: KY-023 joystick controlling a 2-axis 9g servo pan/tilt assembly. Planned addition: relay-driven micro water pump (the "shoot").

## Wiring

```
                         Arduino Nano
                    ┌──────────────────────┐
                    │                 5V ──┼──┬── Joystick VCC
                    │                GND ──┼──┼── Joystick GND
                    │                 A0 ──┼──┼── Joystick VRX
                    │                 A1 ──┼──┼── Joystick VRY
                    │                  2 ──┼──┘   Joystick SW
                    │                      │
                    │                  9 ──┼──── Pan servo  (signal, orange/yellow)
                    │                 10 ──┼──── Tilt servo (signal, orange/yellow)
                    └──────────────────────┘

Servo power (important)
  ┌───────────────────────────────────────────────────────┐
  │  External 5V supply ──── Pan servo  VCC (red)         │
  │                     ──── Tilt servo VCC (red)         │
  │  External GND       ──── Pan servo  GND (brown/black) │
  │                     ──── Tilt servo GND (brown/black) │
  │  External GND       ──── Nano GND  (shared ground)    │
  └───────────────────────────────────────────────────────┘

  ⚠  Do NOT power servos from the Nano's 5V pin during sustained use.
     Two SG90s under load can draw 500 mA+ which will brown-out or
     damage the USB regulator. Use a separate 5 V / 1 A supply (e.g.
     phone charger + barrel adapter) and tie the grounds together.
```

### KY-023 Joystick

| KY-023 | Nano |
|--------|------|
| VCC    | 5V   |
| GND    | GND  |
| VRX    | A0   |
| VRY    | A1   |
| SW     | D2   |

### Servo connectors (SG90 or equivalent 9g servo)

| Servo wire      | Colour (typical) | Nano / supply |
|-----------------|------------------|---------------|
| Signal          | Orange / yellow  | D9 (pan), D10 (tilt) |
| VCC             | Red              | External 5V   |
| GND             | Brown / black    | External GND (shared with Nano GND) |

## Servo limits

Limits are defined in `src/main.cpp` and can be tightened if your assembly binds:

```cpp
#define PAN_MIN   20   // degrees
#define PAN_MAX  160
#define TILT_MIN  30
#define TILT_MAX 150
```

`constrain()` is called on every write, so the firmware can never command the servo outside these bounds regardless of joystick input.

## Serial output

115200 baud. Tab-separated:

```
pan   tilt   btn
95    87     -
96    86     -
96    85     FIRE
```

`FIRE` = joystick button pressed (SW pulled LOW).

## Build & flash

```sh
cd experiments/in_pan_tilt_shoot
pio run -t upload
pio device monitor
```

## Planned additions

- [ ] Relay module for micro water pump (triggered by joystick button)
- [ ] Acceleration curve on servo speed (slow near centre, fast at extremes)
