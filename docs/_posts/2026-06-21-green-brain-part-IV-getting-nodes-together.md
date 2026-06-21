---
layout: post
title: "Green Brain - Part IV - Nodes Well That Ends Well"
date: 2026-06-21 09:00:00 +1000
categories: element14 design-challenge green-brain
---

I had built up some proto board nodes for the distributed greenhouse control
platform — it was time to wire them all up together and get them talking. It did
not go smoothly 😅.

# Recap

Green Brain 🌿🧠 is the idea of tracking plant 🪴 nodes via an industrial **CAN
bus** to ultimately see which plant needs ~~more water cannon 🔫.~~ well
watering.

- [Green Brain - Part I - CAN bus introduction][green-brain-part-I-can-bus-introduction]
- [Green Brain - Part II - Dev setup][green-brain-part-II-dev-setup]
- [Green Brain - Part III - Water Cannon Chaos][green-brain-part-III-water-cannon-chaos]

## Three nodes, one bus

So far I had tested two nodes talking to each other on a work bench. Adding
a bunch of twisted pair wiring and a third node to form a proper little network
seemed straightforward — it was far from it.

The symptoms were baffling. The first two nodes would work, then one of them would
drop off and I would have only one node, and sometimes no nodes would appear at
all after some time. This went on in circles for a while, not to
mention some circuit board issues early on that prevented two boards working
altogether.

![](/green-brain/assets/20260621_all_nodes_in_greenhouse_and_a_plang.jpg)

_All the nodes set up in a greenhouse. You can see they are all connected with
twisted pair. Clearly plywood is not the best for a humid greenhouse but I have
a future use for the stands_

## Problem 1 — bad wiring and cold solder joints

The first and most humbling issue: bad physical connections.

Node 1 would print this at startup and hang:

```
ERR: setBitrate failed — check SPI wiring
```

I had checked the wiring visually — it looked fine. But to get a more precise
diagnosis I added a register read _before_ calling `setBitrate`, directly
checking the `CANSTAT` register on the MCP2515 chip over SPI:

```c
Serial.print(F("CANSTAT after reset=0x")); Serial.println(readCanstat(), HEX);
// expect 0x80 (config mode) — 0x00/0xFF means SPI not working
if (mcp2515.setBitrate(CAN_500KBPS, CAN_CLOCK) != MCP2515::ERROR_OK) {
    Serial.println(F("ERR: setBitrate failed — check SPI wiring"));
    while (true);
}
```

Where `readCanstat()` does a raw SPI read of register `0x0E`:

```c
uint8_t readCanstat() {
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    digitalWrite(MCP_CS_PIN, LOW);
    SPI.transfer(0x03); // READ instruction
    SPI.transfer(0x0E); // CANSTAT register address
    uint8_t val = SPI.transfer(0x00);
    digitalWrite(MCP_CS_PIN, HIGH);
    SPI.endTransaction();
    return val;
}
```

What came back:

```
CANSTAT after reset=0x0
```

The three possible values tell you exactly what is broken:

| CANSTAT | Meaning                                                               |
| ------- | --------------------------------------------------------------------- |
| `0x80`  | Chip alive, in config mode — SPI working ✅                           |
| `0xFF`  | MISO floating high — chip not connected or MISO wire missing          |
| `0x00`  | MISO pulled low — CS not going low, or SCK missing, or a short to GND |

Getting `0x00` meant the chip was never being selected. After going back with a
multimeter and checking continuity on every pin, I found a bad joint on the CS
(D10) header pin — it looked soldered but wasn't making contact. A quick reflow
fixed that joint, but `0x00` persisted. I tried loopback mode (which needs no
CAN bus wiring at all — the chip loops internally) and still got the same
result. That ruled out everything external. The MCP2515 chip itself, or its
crystal, was dead. As I didn't have a spare module to swap out, I swapped with
another node, to confirm the module was working, which it was - so there is an
issue in some of my wiring on that node - will fix that before I start watering
any plants with this system, for sure 🤩.

The lesson: **when SPI doesn't work, measure before you guess, and work inward**.
The `CANSTAT` read costs two lines of code and points at which layer is broken —
wiring, connection, or chip — without having to guess.

![](/green-brain/assets/20260621_node_in_greenhouse.jpg)

_The node is a simple build centered around an Arduino Nano and connected to a
number of sensors and the CAN transceiver. I have a "USB C" charger/power driver
— unfortunately I went cheap from China and they are not true USB C but just a
cheap power cable with a USB C connector - you get what you pay for_

## Problem 2 — duplicate node IDs

Setting node 1 aside with its wiring still unresolved, I focused on getting
nodes 2 and 3 reliably onto the bus. But the dashboard still only showed one node.

The cause was embarrassingly simple. The `NODE_ID` in the firmware is a
compile-time `#define`:

```c
#define NODE_ID    2 // 1 // 3 // change for appropriate node ID
```

I had been flashing both nodes from the same file without updating the
constant each time. Both nodes were broadcasting as node 2 on CAN ID
`0x102`.

CAN bus arbitration is based on the message ID. When two nodes try to transmit
the **same CAN ID at the same time**, both think they are winning arbitration
(the ID bits are identical so neither detects a collision in the arbitration
phase) — but their payload bytes differ. The result is a corrupted frame, which
every node on the bus detects as an error. Error counters start climbing on all
nodes, and the bus degrades.

The fix is obvious in hindsight: flash each node with the correct ID.

```
Node 1 → NODE_ID 1 → CAN ID 0x101
Node 2 → NODE_ID 2 → CAN ID 0x102
Node 3 → NODE_ID 3 → CAN ID 0x103
```

## Problem 3 — all nodes transmitting simultaneously

With unique IDs, both nodes came up and were visible on the dashboard.
But after a minute or two, node 3 would drop off. Then reappear. Then drop off
again.

CAN bus handles collisions via **arbitration** — when two nodes try to transmit
at the same moment, the node with the lower ID wins and the other backs off and
retries. This is elegant and normally fine. The problem is that my original code
used a flat `delay(2000)` in every node's loop:

```c
void loop() {
    delay(2000);
    // ... read sensor, transmit
}
```

If both nodes power up at roughly the same time (same power rail), they
stay locked in sync and collide on every single cycle. CAN arbitration resolves
each collision, but node 3 (higher CAN ID = `0x103`) _always loses_ — it backs
off every single time. Under repeated arbitration losses the MCP2515 starts
accumulating transmit errors in its internal **TEC (Transmit Error Counter)**.

- TEC > 96: warning threshold
- TEC > 127: error-passive (node still transmits but with passive error flags)
- TEC = 255: **bus-off** — node stops transmitting entirely

The fix: stagger the transmission timing per node so they are naturally out of
phase with each other:

```c
void loop() {
    // Stagger per node so they don't all transmit simultaneously.
    // Base 2s interval + NODE_ID * 300ms offset keeps nodes out of phase.
    delay(2000 + NODE_ID * 300);
    // ...
}
```

Node 2 now transmits every 2.6 s, node 3 every 2.9 s — and when node 1 is
eventually fixed, it will slot in at 2.3 s. They drift in and out of alignment
over time but never stay locked together.

## Problem 4 — no recovery when a node goes bus-off

Even with staggered timing, occasional collisions can still accumulate errors,
especially at 500 kbps with the MCP2515's tight 8-time-quanta bit timing at
8 MHz. Once a node hits bus-off it stops transmitting and **the original code
had no recovery logic** — the node would stay silent forever until manually
reset.

The fix: read the `EFLG` (Error Flag) register after any failed send and reset
the chip if the bus-off bit is set:

```c
MCP2515::ERROR sendErr = mcp2515.sendMessage(&tx);
if (sendErr != MCP2515::ERROR_OK) {
    Serial.print(F("ERR: send failed code=")); Serial.println(sendErr);
    // EFLG bit 5 = TXBO (bus-off). Reset and re-enter normal mode.
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    digitalWrite(MCP_CS_PIN, LOW);
    SPI.transfer(0x03); SPI.transfer(0x2D); // READ EFLG
    uint8_t eflg = SPI.transfer(0x00);
    digitalWrite(MCP_CS_PIN, HIGH);
    SPI.endTransaction();
    if (eflg & 0x20) {
        Serial.println(F("EFLG: bus-off — resetting MCP2515"));
        mcp2515.reset();
        delay(100);  // give oscillator time to stabilise
        mcp2515.setBitrate(CAN_500KBPS, CAN_CLOCK);
        delay(10);
        mcp2515.setNormalMode();
    }
    return;
}
```

Note the `delay(100)` after reset — the original code had only 10 ms here,
which is sometimes not enough for the MCP2515's oscillator to stabilise before
you start writing configuration registers.

## Seeing errors before they become a problem

To understand what is happening on the bus in real time, I added a helper that
reads and prints `EFLG` and `TEC` after every successful transmission, but only
if something is non-zero (so normal output stays clean):

```c
// EFLG 0x2D: bit5=TXBO(bus-off) bit4=TXEP(err-passive) bit2=TXWAR(warning)
// TEC  0x1C: transmit error counter  REC 0x1D: receive error counter
void printErrors() {
    uint8_t eflg = readReg(0x2D);
    uint8_t tec  = readReg(0x1C);
    if (eflg == 0 && tec == 0) return;
    Serial.print(F(" EFLG=0x")); Serial.print(eflg, HEX);
    Serial.print(F(" TEC=")); Serial.print(tec);
    if (eflg & 0x20) Serial.print(F(" [BUS-OFF]"));
    else if (eflg & 0x10) Serial.print(F(" [ERR-PASSIVE]"));
    else if (eflg & 0x01) Serial.print(F(" [WARNING]"));
    Serial.println();
}
```

With this in place you can watch TEC climbing before the node goes quiet — it
gives you a warning rather than a silent disappearance.

## What I learned

Getting even two CAN nodes reliably talking to a master required solving four
separate problems: a cold solder joint that looked fine visually, duplicate node
IDs from a careless flash, synchronised timing that caused constant arbitration
collisions, and no recovery from the bus-off state those collisions eventually
caused. None of them were obvious until I had the right instrumentation in place.
Node 1 remains a work in progress.

The core lesson is that **CAN bus is robust but unforgiving about setup**: it
handles arbitration and error detection in hardware, but the hardware needs clean
wiring, unique IDs, and a firmware that knows how to recover from error states.

![](/green-brain/assets/20260621_node_not_helping_plant_much.jpg)

Another thing I learned from my last Design Challenge is not to leave the build
till too late. I had a fair bit of time to get these stands and circuit boards
done. I was hoping to get more sensors on there, including the K-type
thermocouples but some late shipment and a busy few weeks means all I got is a
DHT11 temperature and humidity sensor.

## Next

- Fix node 1's wiring
- maybe take some time out to actually grow some plants 😆
- Hook it up to LabView

## Source

[https://github.com/saramic/green-brain][github-green-brain]

[on-the-line]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/
[green-brain-part-i-can-bus-introduction]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/f/forum/56921/green-brain---part-i---can-bus-introduction
[green-brain-part-II-dev-setup]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/f/forum/57001/green-brain---part-ii---dev-setup
[sentinel-box-part-i-the-plan]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56869/sentinel-box---part-i---the-plan
[sentinel-box-part-ii-back-to-c]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56894/sentinel-box---part-ii---back-to-c
[green-brain-part-III-water-cannon-chaos]: http://127.0.0.1:8888/green-brain/element14/design-challenge/green-brain/2026/06/20/green-brain-part-III-water-cannon-chaos.html
[github-green-brain]: https://github.com/saramic/green-brain
[zephyrproject-uno-q]: https://docs.zephyrproject.org/latest/boards/arduino/uno_q/doc/index.html
