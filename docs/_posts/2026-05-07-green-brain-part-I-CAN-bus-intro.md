---
layout: post
title:  "Green Brain - Part I - CAN bus introduction"
date:   2026-05-07 11:00:00 +1000
categories: element14 design-challenge green-brain
---

My first forum post for the element14 [On the line][on-the-line] smart industry
themed Design Challenge.

# The Idea

Plants 🪴🪴 and kids 🧒👧 are nice addition to the homeliness of a home 🏡.
Sometimes you need to take the kids on holidays 🏝️ and how do you keep the
plants at home watered? Enter the **Green Brain** a smart watering system.
Although my premise is to have a smart pot plant watering system for when no
one is around, to keep in tune with the "industry theme" I will dive into some
industry standards like **CAN bus** - Controller Area Network (CAN bus). Using
this I hope to connect a multi-node system to an **Arduino UNO Q** as the
central host computer. Throw in some **computer vision** and **edge AI** to
help orchestrate an industrial level green house, maybe even a water cannon 🔫.

## Pre hardware

Although I was lucky enough to have been announced as one of [10
challengers](element-14-10-challengers) to receive some free hardware, As I
live on the other side of the world 🦘🌏 my kit has not arrived yet, so I
thought it would be timely to get started anyway.

The main elements of the On-the-line challenge seem to be:

1. [Arduino UNO Q][element-14-arduino-UNO-Q]

   Single Board Computer, **Arduino UNO Q**, QRB2210/STM32U585, 2GB, ARM
   Cortex-A53/M33F

2. [MAX33041ESHLD#][element-14-MAX33041ESHLD#]

   Shield Evaluation Kit, MAX33041E, Interface, **CAN Transceiver**

I was intruiged by the UNO Q by a review by @skruglewicz

- [Test out Arduino's Uno Q - The new Single-Board
  Computer][element-14-review-uno-q]

So I purchased one a few weeks back. The **CAN Transciever** board was a bit
out of my price bracket, but I worked out I can get cheap, less industrial,
**CAN bus** modules based on `MCP2515`so I got a few of those to get my head
around the communications standard.

## CAN bus edge nodes

Given my idea was a green house, I got some `DHT11` temperature/humidity
sensors, and `YL-69` soil hygrometer sensors. Being a CAN "bus" I decided to
start by creating some simple edge nodes powered by an Arduino Nano. The idea
being that each node would take a measurement and send it across the CAN bus to
the central host computer (UNO Q)

### Simple CAN loopback

Using the mcp2515 Arduino library, I got the following "loopback mode" test
code.

```c
// NOTE abirdged code to only show important parts
#include <mcp2515.h>

#define MCP_CS_PIN 10
#define NODE_ID    1
#define CAN_CLOCK  MCP_8MHZ

// Setup the mcp2515 device
MCP2515 mcp2515(MCP_CS_PIN);

void setup() {
    mcp2515.reset();

    ...

    // Set the speed to 500Kbps and the clock to 8MHz as per crystal
    if (mcp2515.setBitrate(CAN_500KBPS, CAN_CLOCK) != MCP2515::ERROR_OK) {
        Serial.println(F("ERR: setBitrate failed — check SPI wiring"));
        while (true);
    }

    // set it in loopback mode for testing
    mcp2515.setLoopbackMode();
}

...

void loop() {
    delay(2000);

    ...

    can_frame tx = buildFrame(temp, hum, ok);
    // Send a message
    MCP2515::ERROR sendErr = mcp2515.sendMessage(&tx);
    if (sendErr != MCP2515::ERROR_OK) {
        // code 2=ALLTXBUSY: TX buffers all stuck — chip probably in CONFIG mode
        // code 4=FAILTX: frame sent but no ACK (expected in normal mode, no bus)
        Serial.print(F("ERR: send failed code=")); Serial.println(sendErr);
        return;
    }
    printFrame("TX ", tx);

    delay(10);
    can_frame rx;
    // Read the message, will come from loopback, so same one we sent above
    if (mcp2515.readMessage(&rx) == MCP2515::ERROR_OK) {
        printFrame("RX ", rx);
        Serial.println(F("  [loopback OK]"));
    } else {
        Serial.println(F("ERR: loopback RX failed"));
    }
}
```

And here I got stuck for a long time, trusting my wiring and not trusting the
code. In the end I worked out I had been doing too much UART recently where you
connect RX from one side to TX on the other, in this case I had misconnected by
MOSI (Master Out Slave In) and MISO (Master In Slave Out) SPI (Serial
Peripheral Interface), MOSI → MOSI, same same. Once the wiring got fixed, I was
getting the expected loopback message repeating what I was sending.

Missing from the above sketch are some initial verificatin and the frames for
the CAN bus communications.

### CAN verification

At startup, you can also verify the setup of the MCP2515 CAN bus device

```c
// acquires the SPI bus and configures it: 10 MHz clock, most-significant bit
// first, clock polarity/phase mode 0 (idle low, sample on rising edge). This
// matches MCP2515's requirements.
SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0))

// asserts chip select (pulls CS low), which tells the MCP2515 "the next
// bytes on the bus are for you." Without this, the chip ignores SPI traffic.
digitalWrite(MCP_CS_PIN, LOW)

// sends the MCP2515 READ command byte (0x03). This tells the chip: "I want
// to read a register."
SPI.transfer(0x03)

// sends the register address 0x0E, which is CANSTAT (CAN Status Register).
// The chip now knows which register to return.
SPI.transfer(0x0E)

// sends a dummy byte (0x00) to clock out the register value. SPI is
// full-duplex, so you must send something to receive something; the chip
// responds with the CANSTAT byte while you transmit the dummy.
uint8_t canstat = SPI.transfer(0x00)

// de-asserts chip select, ending the SPI transaction with this chip.
digitalWrite(MCP_CS_PIN, HIGH)

// releases the SPI bus so other devices can use it.
SPI.endTransaction()
```

CANSTAT tells us:

The bits [7:5] of CANSTAT encode the current operating mode:

- `0x00` → Normal mode (actively on the CAN bus)
- `0x40` → Loopback mode (internal loopback for testing)
- `0x80` → Configuration mode (used during init to set baud rate, masks, etc.)

This acts as a verification block for the MCP2515 setup.

### 8 byte CAN frame payload

To send data on CAN bus it needs to be organised in 8 bytes of payload. This is
baked into the protocol at the hardware level and the MCP2515 and every other
CAN controller envorces it.

The reasons CAN was designed this way is:

- **Real-time determinism** — CAN was designed for automotive/industrial
  control systems where latency guarantees matter more than throughput. Short
  fixed-size messages mean worst-case bus time is predictable.
- **Bus arbitration** — multiple nodes share one wire and arbitrate
  per-message. Long messages hold the bus and block higher-priority nodes for
  longer.
- **Error detection** — the built-in CRC and bit-stuffing scheme works on
  bounded-length frames. Longer frames would need a different (heavier) error
  model.

For the sake of starting, I came up with the following frame layout

Byte  | Field        | Detail
------|--------------|---------------------------------------------------------
0     | Node ID      | Which sensor node sent this (e.g. 1, 2)
1–2   | Temperature  | Big-endian uint16, scaled ×10 (e.g. 23.4°C → 0x00EA)
3–4   | Humidity     | Big-endian uint16, scaled ×10 (e.g. 65.1% → 0x028B)
5     | Status flags | bit 0 = sensor OK, bit 1 always set (protocol version marker?)
6     | Sequence     | Rolling counter, wraps 0–255, detects missed messages
7     | Checksum     | XOR of bytes 0–6, detects corruption

The CAN ID is `0x100` + `NODE_ID`, so node 1 sends on `0x101`, node 2 on
`0x102`, etc. — letting receivers filter by ID without parsing the payload.

For the time being I was not yet using the `YL-69` soil hygrometer, only the
DHT11 temperature/humidity sensor. The temperature and humidity is ×10 scaled
to pack a float into 2 bytes, big-endian uint16. I have an XOR checksum in the
8th byte) and everything for the time being fits into an 8-byte box. Going
forward I could split readings across multiple data frames or I might be able
to use CAN FD, a newer standard that allows up to 64 bytes.

The code to create the frame

```c
can_frame buildFrame(float temp, float hum, bool sensorOk) {
    can_frame frame;
    frame.can_id  = 0x100 + NODE_ID;
    frame.can_dlc = 8;

    uint16_t tempRaw = sensorOk ? (uint16_t)(temp * 10) : 0;
    uint16_t humRaw  = sensorOk ? (uint16_t)(hum  * 10) : 0;

    frame.data[0] = NODE_ID;
    frame.data[1] = (tempRaw >> 8) & 0xFF;
    frame.data[2] =  tempRaw       & 0xFF;
    frame.data[3] = (humRaw  >> 8) & 0xFF;
    frame.data[4] =  humRaw        & 0xFF;
    frame.data[5] = (sensorOk ? 0x01 : 0x00) | 0x02;
    frame.data[6] = sequence++;

    uint8_t chk = 0;
    for (int i = 0; i < 7; i++) chk ^= frame.data[i];
    frame.data[7] = chk;

    return frame;
}
```

and to print an incoming frame

```c
void printFrame(const char* prefix, const can_frame& f) {
    Serial.print(prefix);
    Serial.print(F("ID:0x")); Serial.print(f.can_id, HEX);
    Serial.print(F(" node:")); Serial.print(f.data[0]);
    float t = ((uint16_t)(f.data[1] << 8) | f.data[2]) / 10.0;
    float h = ((uint16_t)(f.data[3] << 8) | f.data[4]) / 10.0;
    Serial.print(F(" T:")); Serial.print(t, 1); Serial.print(F("C"));
    Serial.print(F(" H:")); Serial.print(h, 1); Serial.print(F("%"));
    Serial.print(F(" seq:")); Serial.print(f.data[6]);
    Serial.print(F(" chk:0x")); Serial.println(f.data[7], HEX);
}
```

### Success

I had 2 Arduinos connected by 2 wire CAN bus with a 120Ω terminator resistor
(to prevent signal reflection) sending temperature and humidity readings and
via a serial monitor on one of them I could see the TX, transmitted values and
RX, recieved values from the other edge node.

TODO ... image

## Next

My gear is on it's way so there will no doubt be an unboxing. But I also need
to hook up my UNO Q to the above CAN bus. I did some investigation and it seems
the MCP2515 devices I got are 5V whilst the UNO Q is 3V3, so I have some logic
shifters coming as well as some some 3V3 logic CAN trasnceivers based around
`SN65HVD230`.

I was hoping to just connect the UNO Q MPU (Microprocessor Unit, the Linux box)
straight throug to the CAN transciever, but it seems that the MPU has no GPIO
connections.

```sh
# wishful thinking - WILL NOT WORK

# Install can-utils if not present
sudo apt-get install -y can-utils

# Load CAN kernel modules
sudo modprobe can
sudo modprobe can_raw
sudo modprobe can_dev
sudo modprobe mcp251x

export CAN_BITRATE=500000       # must match firmware (CAN_500KBPS)
export MCP_OSCILLATOR=8000000   # 8 MHz crystal on the MCP2515 module

# NOTE: this is what presumably CANNOT be done on an UNO Q as the MPU has no
#       access to GPIO pins
export MCP_INT_GPIO=25          # GPIO pin wired to MCP2515 INT

# check for available SPI devices
ls /dev/spi*

# add a device tree overaly
#   If dtoverlay is not available, create a .dtbo manually.
#   See: https://docs.kernel.org/devicetree/overlay-notes.html
sudo dtoverlay mcp2515-can0 \
  oscillator=${MCP_OSCILLATOR} \
  interrupt=${MCP_INT_GPIO}

# check device is present
ip link show can0 &>/dev/null

# Bring up the CAN interface
sudo ip link set can0 down 2>/dev/null || true
sudo ip link set can0 up type can bitrate ${CAN_BITRATE}

echo "can0 up at ${CAN_BITRATE} bps"

# Quick sanity check, listening for 5 seconds
timeout 5 candump can0 || true

echo ""
echo "=== Setup complete ==="
echo "Run the bridge:  cd bridge && npm start"
```

As presumably the above will not work as there are no GPIO pins to wire the
MCP2515 to have access to the Linux MPU, this means I will need the MCU
(Microcontroller Unit, STM32U585) to connect to the CAN transciver via JDIGITAL
pins and then have the MPU talk to the MCU via `/dev/spidev0.0` - I hope 🤞.

At some point I hope to get into some computer vision and plant detection using
a model like [**YOLOv8**][YOLOV8-com] or similar running no the **UNO Q**.

## Source

[https://github.com/saramic/green-brain][github-green-brain]

[on-the-line]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/
[github-green-brain]: https://github.com/saramic/green-brain
[element-14-arduino-UNO-Q]: https://au.element14.com/new-products/embedded-computers-education-maker-boards/arduino-uno-q
[element-14-MAX33041ESHLD#]: https://au.element14.com/analog-devices/max33041eshld/shileld-eval-kit-can-transceiver/dp/3807530
[element-14-review-uno-q]: https://community.element14.com/products/roadtest/rv/roadtest_reviews/1894/test
[element-14-10-challengers]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/b/news/posts/challenge-announcement-on-the-line-design-challenge
[YOLOV8-com]: https://yolov8.com/
