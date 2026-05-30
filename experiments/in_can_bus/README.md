# GreenBrain — CAN Bus Experiment

Industrial distributed greenhouse sensor network over CAN bus, with a garden-themed
real-time dashboard deployed on the Arduino UNO Q.

---

## Architecture

```
┌──────────────────────────────────────────────────────────────────────────┐
│                       CAN Bus Network (200kbps)                          │
│                                                                          │
│  ┌──────────────┐    ┌──────────────┐         ┌──────────────┐           │
│  │ Arduino Nano │    │ Arduino Nano │   ...   │ Arduino Nano │           │
│  │   Node #1    │    │   Node #2    │         │   Node #N    │           │
│  │              │    │              │         │              │           │
│  │  ┌────────┐  │    │  ┌────────┐  │         │  ┌────────┐  │           │
│  │  │ DHT11  │  │    │  │ DHT11  │  │         │  │ DHT11  │  │           │
│  │  └────────┘  │    │  └────────┘  │         │  └────────┘  │           │
│  │  ┌─────────┐ │    │  ┌─────────┐ │         │  ┌─────────┐ │           │
│  │  │MCP2515  │ │    │  │MCP2515  │ │         │  │MCP2515  │ │           │
│  │  │ 5V SPI  │ │    │  │ 5V SPI  │ │         │  │ 5V SPI  │ │           │
│  │  └────┬────┘ │    │  └────┬────┘ │         │  └────┬────┘ │           │
│  └───────┼──────┘    └───────┼──────┘         └───────┼──────┘           │
│          │                   │                        │                  │
│   CAN_H ─┴───────────────────┴────────────────────────┴─ CAN_H           │
│   CAN_L ─┬───────────────────┬────────────────────────┬─ CAN_L           │
│          │                                            │                  │
│   120Ω terminator                              120Ω terminator           │
│                                                       │                  │
│                    ┌──────────────────────────────────┘                  │
│                    │ CAN_H / CAN_L                                       │
│                    ▼                                                     │
│          ┌──────────────────┐                                            │
│          │  MCP2515  (5V)   │  ← same module as sensor nodes             │
│          └────────┬─────────┘                                            │
│           5 wires │ SPI (5V side)                                        │
│          ┌────────▼──────────────────┐                                   │
│          │  Level shifter (5V↔3.3V)  │  ← TXS0108E or BSS138 module      │
│          └────────┬──────────────────┘                                   │
│           5 wires │ SPI (3.3V side)                                      │
│          ┌────────▼───────────────────────────────┐                      │
│          │           Arduino UNO Q                │                      │
│          │                                        │                      │
│          │  STM32 co-processor                    │                      │
│          │  ├─ MCP2515 via SPI (RouterBridge)     │                      │
│          │  └─ Bridge.provide_safe("get_nodes")   │                      │
│          │                                        │                      │
│          │  Linux (Qualcomm QCS6490)              │                      │
│          │  ├─ arduino-router (/dev/ttyHS1)       │                      │
│          │  ├─ arduino-app-cli Python app         │                      │
│          │  │    polls STM32 via RouterBridge     │                      │
│          │  └─ Flask dashboard → :8080            │                      │
│          └────────────────────────────────────────┘                      │
└──────────────────────────────────────────────────────────────────────────┘
```

> **Why RouterBridge, not SocketCAN or raw serial?**
> The UNO Q's `arduino-router` service owns `/dev/ttyHS1` (STM32↔Linux UART) as
> root with `Restart=always` — it cannot be stopped. The RouterBridge library is
> the designed IPC mechanism. The STM32 registers methods with
> `Bridge.provide_safe()` and the Linux Python app polls them via `Bridge.call()`.
>
> **Why Python, not Node.js?**
> The `arduino-app-cli` brick framework provides `arduino.app_utils.Bridge` for
> Python. A Node.js client would require implementing the MsgPack/RPClite socket
> protocol by hand against the unix socket at `/var/run/arduino-router.sock`.
> Python + Flask gives the same REST + web result with far less complexity.

---

## Wiring

### Phase 1 & 2 — Arduino Nano sensor node

#### DHT11 → Arduino Nano

```
DHT11 Pin   Arduino Nano Pin   Notes
─────────   ────────────────   ──────────────────────────
VCC         5V
GND         GND
DATA        D4                 + 10kΩ pull-up resistor to 5V
```

```
Arduino 5V ──── 10kΩ ──┬── DHT11 DATA
                       │
                      D4 (Arduino)
```

#### MCP2515 → Arduino Nano (SPI)

```
MCP2515 Pin   Arduino Nano Pin   Notes
───────────   ────────────────   ─────────────────────────────────────
VCC           5V                 (some modules are 3.3V — check yours)
GND           GND
CS            D10                Chip Select
SI / MOSI     D11                SPI data in
SO / MISO     D12                SPI data out
SCK           D13                SPI clock
INT           D2                 Interrupt (required for receive)
```

### Phase 3 — MCP2515 → Level shifter → UNO Q STM32

The STM32 GPIO is 3.3V. The cheap MCP2515 modules are 5V.
Use a bidirectional level shifter — a BSS138-based 4-channel module works well.

```
MCP2515 (5V side)     Level shifter    UNO Q STM32 SPI (3.3V)
──────────────────    ─────────────    ──────────────────────
VCC  ────────────────────────────────  5V  (power for MCP2515)
GND  ────────────────────────────────  GND
SI/MOSI ──────────── HV1 ── LV1 ─────  MOSI  (SPI)
SO/MISO ──────────── HV2 ── LV2 ─────  MISO  (SPI)
SCK ──────────────── HV3 ── LV3 ─────  SCK   (SPI)
CS ───────────────── HV4 ── LV4 ─────  D10   (CS pin in sketch)
INT ──────────────── HV5 ── LV5 ─────  (not used — polling mode)
                     HV_VCC ← 5V
                     LV_VCC ← 3.3V
```

#### CAN Bus wiring (all nodes)

```
[Node 1 MCP2515]──CAN_H──[Node 2 MCP2515]──CAN_H──[Hub MCP2515]
[Node 1 MCP2515]──CAN_L──[Node 2 MCP2515]──CAN_L──[Hub MCP2515]
     │                                                    │
  120Ω (to CAN_L)                                  120Ω (to CAN_L)
  (terminator)                                     (terminator)
```

> Both ends of the CAN bus need a 120Ω terminator between CAN_H and CAN_L.
> Many MCP2515 modules have a solder-jumper for this — enable it on the two
> end nodes only.

---

## CAN Message Protocol

```
CAN ID: 0x100 + nodeId    (node 1 = 0x101, node 2 = 0x102, …)

Byte   Field           Encoding
────   ─────           ────────────────────────────────────────────
0      nodeId          uint8, 1–255
1      temp_hi         uint16 big-endian, temperature × 10
2      temp_lo           e.g. 253 = 25.3°C
3      hum_hi          uint16 big-endian, humidity × 10
4      hum_lo            e.g. 682 = 68.2%
5      status          bit0=sensor_ok, bit1=can_ok
6      sequence        uint8, wraps 0–255 (detect dropped frames)
7      checksum        XOR of bytes 0–6
```

---

## Repository Layout

```
experiments/
  in_can_bus/                     ← this directory — sensor node firmware
    firmware/
      dht11_sensor/               ← Phase 1: bare DHT11 (PlatformIO)
      can_garden_node/            ← Phase 2: CAN sender (PlatformIO)
      can_garden_hub/             ← archived — pre-RouterBridge attempts
    bridge/                       ← archived — Node.js bridge (reference only)

  in_app_new/                     ← RouterBridge proof-of-concept (LED + tick)
  in_can_garden_hub/              ← Phase 3+: CAN hub arduino-app-cli app
    sketch/sketch.ino             ← STM32: reads CAN, provides get_nodes RPC
    python/main.py                ← Linux: polls STM32, Flask REST + dashboard
    python/static/index.html      ← Node cards, auto-updating
```

---

## Phases

| Phase | Target               | Goal                                               | Status     |
|-------|----------------------|----------------------------------------------------|------------|
| 1     | Arduino Nano         | DHT11 serial output (no CAN)                       | ✅ done    |
| 2     | Arduino Nano         | DHT11 + MCP2515 CAN sender/receiver                | ✅ done    |
| 3     | UNO Q STM32 + Linux  | STM32 CAN hub → RouterBridge → Flask dashboard     | 🌱 next    |
| 4     | UNO Q Linux          | Expanded dashboard — graphs, history, alerts       | 🌿         |
| 5     | UNO Q Linux          | Additional sensors, actuators (pumps via STM32)    | 🌿         |

---

## Quick Commands

```sh
# Phase 1 — build and upload DHT11 sensor (USB to Nano)
cd experiments/in_can_bus/firmware/dht11_sensor
~/.platformio/penv/bin/pio run --target upload
~/.platformio/penv/bin/pio device monitor --baud 115200

# Phase 2 — CAN sensor node (USB to Nano, change NODE_ID per device)
cd experiments/in_can_bus/firmware/can_garden_node
~/.platformio/penv/bin/pio run --target upload
~/.platformio/penv/bin/pio device monitor --baud 115200

# Phase 3 — one-time install (on Mac)
mise run install

# Phase 3 — upload sketch to UNO Q STM32 (SSH to pollyanna, compile + upload there)
mise run upload-on-board   # syncs experiments/in_can_garden_hub/sketch → pollyanna

# Phase 3 — deploy and start the Python app on pollyanna
mise run upload:in_can_garden_hub
mise run start:in_can_garden_hub

# Open the dashboard
open http://pollyanna.local:8080/

# Stop the app
mise run stop:in_can_garden_hub
```

---

## Deploying to the UNO Q

The UNO Q runs Debian Linux. Connect from your Mac via:

**SSH:**
```sh
ssh arduino@pollyanna.local
# or by IP:
ssh arduino@192.168.68.128
```

**USB (adb):**
```sh
brew install android-platform-tools   # one-time
adb devices                           # confirm UNO Q is listed
adb shell                             # shell on UNO Q
```
