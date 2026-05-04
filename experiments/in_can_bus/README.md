# GreenBrain — CAN Bus Experiment

Industrial distributed greenhouse sensor network over CAN bus, with a garden-themed
real-time dashboard deployed on the Arduino UNO Q.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        CAN Bus Network (500kbps)                        │
│                                                                         │
│  ┌──────────────┐    ┌──────────────┐         ┌──────────────┐          │
│  │  Arduino Uno │    │  Arduino Uno │   ...   │  Arduino Uno │          │
│  │   Node #1    │    │   Node #2    │         │   Node #N    │          │
│  │              │    │              │         │              │          │
│  │  ┌────────┐  │    │  ┌────────┐  │         │  ┌────────┐  │          │
│  │  │ DHT11  │  │    │  │ DHT11  │  │         │  │ DHT11  │  │          │
│  │  └────────┘  │    │  └────────┘  │         │  └────────┘  │          │
│  │  ┌─────────┐ │    │  ┌─────────┐ │         │  ┌─────────┐ │          │
│  │  │MCP2515  │ │    │  │MCP2515  │ │         │  │MCP2515  │ │          │
│  │  │ (SPI)   │ │    │  │ (SPI)   │ │         │  │ (SPI)   │ │          │
│  │  └────┬────┘ │    │  └────┬────┘ │         │  └────┬────┘ │          │
│  └───────┼──────┘    └───────┼──────┘         └───────┼──────┘          │
│          │                   │                         │                │
│    CAN_H ┴───────────────────┴─────────────────────────┘ CAN_H          │
│    CAN_L ┬───────────────────┬─────────────────────────┬ CAN_L          │
│          │                                             │                │
│   120Ω terminator                              120Ω terminator          │
│                                                                         │
│                    ┌─────────────────────────┐                          │
│                    │      Arduino UNO Q      │                          │
│                    │                         │                          │
│                    │  ┌────────────────────┐ │                          │
│                    │  │ STM32 co-processor │ │                          │
│                    │  │ MCP2515 (SPI)      │ │                          │
│                    │  │ CAN hub/receiver   │ │                          │
│                    │  └─────────┬──────────┘ │                          │
│                    │            │ serial/UART│                          │
│                    │  ┌─────────▼──────────┐ │                          │
│                    │  │  Linux (Qualcomm)  │ │                          │
│                    │  │  Node.js bridge    │ │                          │
│                    │  │  Next.js dashboard │ │                          │
│                    │  │  (Docker)          │ │                          │
│                    │  └────────────────────┘ │                          │
│                    └─────────────────────────┘                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Wiring

### Phase 1 & 2 — Arduino Uno sensor node

#### DHT11 → Arduino Uno

```

  +-----------+
  |   ▪ ▪ ▪   |
  |   ▪ ▪ ▪   |
  |   ▪ ▪ ▪   |
  |   ▪ ▪ ▪   |
  |   ▪ ▪ ▪   |
  |           |
  +-----------+
    |  |  |  |
    |  |  |  |
   VCC | NC GND
    Data
```

```
DHT11 Pin   Arduino Uno Pin   Notes
─────────   ───────────────   ──────────────────────────
VCC         5V
GND         GND
DATA        D4                + 10kΩ pull-up resistor to 5V
```

```
Arduino Uno 5V ──── 10kΩ ──┬── DHT11 DATA
                           │
                          D4 (Arduino)
```

#### MCP2515 → Arduino Uno (SPI)

```
MCP2515 Pin   Arduino Uno Pin   Notes
───────────   ───────────────   ─────────────────────────────────────
VCC           5V                (some modules are 3.3V — check yours)
GND           GND
CS            D10               Chip Select
SI / MOSI     D11               SPI data in
SO / MISO     D12               SPI data out
SCK           D13               SPI clock
INT           D2                Interrupt (required for receive)
```

```
          Arduino Uno
         ┌────────────┐
   5V ───┤5V          |
  GND ───┤GND         |                      MCP2515
         │            |               ┌───────────────────┐
         │         D2 ├───────────────| INT               │
         │        D13 ├───────────────| SCK               |
         │   MOSI/D11 ├───────────────| SI/MOSI    CAN_H ─┼── CAN bus high
         │   MISO/D12 ├───────────────| SO/MISO    CAN_L ─┼── CAN bus low
         │        D10 ├───────────────| CS                |
         └────────────┘               | GND               |
                                      | VCC               |
                                      └───────────────────┘
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

## Phases

| Phase | Target                  | Goal                                      | Status  |
|-------|-------------------------|-------------------------------------------|---------|
| 1     | Arduino Uno             | DHT11 serial output (no CAN)              | 🌱 next |
| 2     | Arduino Uno             | DHT11 + MCP2515 CAN sender                | 🌿      |
| 3     | UNO Q STM32             | MCP2515 CAN receiver → serial JSON        | 🌿      |
| 4     | UNO Q Linux             | Node.js serial bridge → WebSocket         | 🌿      |
| 5     | UNO Q Linux             | Next.js garden threat-map dashboard       | 🌿      |
| 6     | UNO Q Linux (Docker)    | Deploy bridge + web via arduino-app-cli   | 🌿      |

---

## Quick Commands

```sh
# Phase 1 — build and upload DHT11 sensor
cd experiments/in_can_bus/firmware/dht11_sensor
pio run --target upload
pio device monitor --baud 115200

# Phase 2 — CAN sender node
cd experiments/in_can_bus/firmware/can_garden_node
pio run --target upload
pio device monitor --baud 115200

# Phase 3 — UNO Q hub receiver
cd experiments/in_can_bus/firmware/can_garden_hub
pio run --target upload

# Phase 4 — bridge
cd experiments/in_can_bus/bridge
npm install && npm start

# Phase 5 — web dashboard (dev)
cd experiments/in_can_bus/web
npm install && npm run dev

# Phase 6 — deploy to UNO Q
cd experiments/in_can_bus/deploy
docker-compose up --build
```
