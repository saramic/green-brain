# GreenBrain

**An Industrial Distributed Control Platform for Precision Greenhouse
Automation**

## About Me

I am a Software Developer in the startup scene of Melbourne, Australia for over
25 years. I build scalable, fault tolerant distributed systems as part of web
applications around job processing, let's call it glorified CRON. Recently my
focus has shifted to Agentic AI systems. I am an amateur electronics enthusiast
with an interest over 35 years, but it has recently been reignited with the
explosion in the commodity microcontroller scene, driven in no small part by
Arduino. The Arduino UNO Q — with a computer alongside the microcontroller —
sounds like a match for my abilities and interests.

## The Problem

Modern commercial greenhouses are sophisticated industrial environments. They
manage microclimate zones, irrigation scheduling, plant health, lighting, and
human safety, often across large areas with many independent sensors and
actuators. Yet at the hobbyist and small-commercial scale, these systems remain
fragmented: a cheap WiFi thermometer here, a timer-based irrigation controller
there, no integration, no intelligence, and no safety awareness.

The result is wasted water, suboptimal growing conditions, and no data to learn
from.

The core question this project addresses is: can we build a genuinely
industrial-grade distributed control architecture — using the same
communication standards, safety principles, and edge AI used in real Industry
4.0 plants, and apply it to a real working greenhouse?

## The Solution: GreenBrain

GreenBrain is a multi-node distributed greenhouse control system built on an
industrial CAN bus backbone, with edge AI, computer vision, automated
irrigation, and a human safety interlock — all orchestrated by the Arduino UNO
Q.

It is not a toy sensor dashboard. It is a genuine demonstration of industrial
control system architecture applied to precision agriculture, with the same
layered design found in real SCADA/PLC deployments:

- **Field layer**: distributed CAN sensor nodes (temperature, humidity, soil
  moisture)
- **Safety layer**: mmWave presence detection with pump interlock
- **Control layer**: Arduino UNO Q as the intelligent edge controller
- **Vision layer**: webcam + UNO Q image classification for plant health and
  precision irrigation
- **Supervisory layer**: MQTT broker + live web dashboard hosted on the UNO Q
- **Stretch goal — Actuation layer**: computer vision-guided water cannon for
  pot-targeted irrigation
- **Stretch goal — Energy layer**: dual-axis heliostat to redirect natural
  sunlight into the grow tent

## Why This Is Industrial, Not Just a Hobby Greenhouse

The design decisions throughout this project are deliberately industrial:

**CAN Bus over WiFi for the sensor network.** Real automated greenhouses,
factories, and agricultural machinery use CAN (or CANopen) for exactly the
reasons this project does: deterministic timing, noise immunity near pumps and
motors, multi-node on two wires, and fault tolerance if one node drops. WiFi
sensor nodes would be simpler to build but would not demonstrate the industrial
communication principles the challenge is designed to explore.

**mmWave safety interlock.** In any automated industrial environment with
moving actuators (pumps, valves, conveyors), a human presence detection
interlock is mandatory — not optional. The mmWave sensor provides a zone-based
safety stop: if a person enters the irrigation operating area, all pumps halt
and a CAN safety message is broadcast to all nodes. This directly mirrors IEC
61508 functional safety thinking at a demonstrable scale.

**Distributed nodes, not centralised polling**. Each sensor node has local
intelligence (like an Arduino with an MCP2515 CAN controller). Nodes publish
data onto the CAN bus autonomously; the UNO Q subscribes. This is exactly how
real distributed control systems work — not a hub polling slaves, but nodes
broadcasting state. This architecture scales to any number of nodes without
rewiring the controller.

**Edge AI, not cloud AI**. All vision processing runs on the UNO Q locally
using App Lab Bricks — no cloud dependency, no latency, no connectivity
requirement. This is precisely the edge intelligence model driving Industry 4.0
investment globally.

## The Hardware

### Sponsored Kit Components:

- Arduino UNO Q (master controller, Linux + STM32 dual-brain)
- Analog Devices MAX33041E CAN Transceiver Shield (UNO Q CAN interface)
- OmegaDwyer K-type Thermocouples × 5 (zone temperature monitoring)
- OmegaDwyer Humidity Sensor (greenhouse humidity monitoring)
- Emerson NI LabVIEW (3-month trial for supervisory dashboard)
- Molex connectors (field wiring termination)

### Additional Components (self-sourced):

- Arduino Nano × 2 (CAN sensor node MCUs)
- MCP2515 + TJA1050 CAN transceiver modules × 2 (node CAN interface, ~$3 each)
- DHT22 or SHT31 temperature/humidity sensors (supplementary nodes)
- Capacitive soil moisture sensors × 4
- mmWave presence detection module (24GHz, human safety interlock)
- USB webcam (computer vision)
- Mini submersible pumps × 3 (zone irrigation)
- Relay modules (pump switching, controlled from STM32)
- Pan/tilt servo bracket (stretch: water cannon targeting)
- 12V power supply + DC-DC buck converters (distributed power)
- Twisted pair cable (CAN bus wiring)

## System Architecture

```
                    ┌──────────────────────────────────┐
                    │         Arduino UNO Q            │
                    │  ┌──────────────┐ ┌────────────┐ │
                    │  │  Linux MPU   │ │ STM32 MCU  │ │
                    │  │  (Python)    │ │  (C++/RT)  │ │
                    │  │ MQTT Broker  │ │ Pump Relay │ │
                    │  │ Vision Brick │ │ CAN Master │ │
                    │  │ Web Dashboard│ │ Safety Stop│ │
                    │  └──────────────┘ └────────────┘ │
                    │       MAX33041E CAN Shield       │
                    └──────────────┬───────────────────┘
                                   │ CAN Bus (2-wire)
               ┌───────────────────┼──────────────────┐
               │                   │                  │
    ┌──────────┴──────┐  ┌─────────┴───────┐  ┌───────┴───────────┐
    │   CAN Node 1    │  │   CAN Node 2    │  │   Safety Node     │
    │  Arduino Nano   │  │  Arduino Nano   │  │  mmWave Sensor    │
    │  MCP2515        │  │  MCP2515        │  │  (direct to UNO Q │
    │  Thermocouple   │  │  Thermocouple   │  │   GPIO)           │
    │  DHT22 Humidity │  │  DHT22 Humidity │  └───────────────────┘
    │  Soil Moisture  │  │  Soil Moisture  │
    │  Zone A         │  │  Zone B + Pump  │
    └─────────────────┘  └─────────────────┘

    Webcam → UNO Q USB → Vision Brick (plant health / pot targeting)
    LabVIEW ← MQTT ← UNO Q (supervisory dashboard on PC)
```

## The Project Plan

### Phase 1 — Core Infrastructure (Weeks 1–2)

* Assemble CAN sensor nodes (Arduino Nano + MCP2515 + sensors)
* Establish CAN bus communication between nodes and UNO Q
* Validate thermocouple and humidity data across all nodes
* Deploy MQTT Broker Brick on UNO Q; nodes publish to topics
* Build initial web dashboard (WebUI Brick)
* Blog post: CAN bus architecture and node design

### Phase 2 — Control & Safety (Weeks 3–4)

* Implement pump irrigation control from UNO Q STM32 side
* Implement threshold-based auto-irrigation (soil moisture → pump on)
* Integrate mmWave safety interlock:
  ```
  person detected → CAN safety broadcast → all pumps off
  ```
* Test and document safety response latency
* Blog post: safety interlock design and industrial relevance

### Phase 3 — Edge AI Vision (Weeks 5–6)

* Connect webcam to UNO Q
* Deploy image classification Brick for plant health assessment (wilting,
  discolouration)
* Integrate health alerts into MQTT dashboard
* Blog post: edge AI plant health monitoring

### Phase 4 — Vision-Guided Irrigation (Weeks 7–8)

* Build pan/tilt water cannon (servo bracket + pump)
* Implement pot/plant object detection using UNO Q detection Brick
* Calculate pan/tilt angles from detected pot positions
* Fire targeted irrigation burst to detected pots
* Blog post: computer vision precision irrigation

### Phase 5 — Stretch: Heliostat (If time permits)

* Build dual-axis mirror mount (3D printed or aluminium extrusion)
* Implement solar position algorithm (latitude/longitude/time) on Linux side
* STM32 drives stepper motors for azimuth/elevation
* Redirect natural light into grow tent; thermocouples validate heat delivery
* Blog post: heliostat design, solar math, and results

### Phase 6 — Integration & Documentation

* Full system integration test
* LabVIEW supervisory dashboard final build
* Final project blog with video demonstration
* Open-source code release (GitHub)

## Blog Post Plan (5 forum posts + 1 project blog required)

Post         | Topic
-------------|-----------------------------------------------
Forum Post 1 | Design decisions: why CAN bus over WiFi for greenhouse sensor networks
Forum Post 2 | CAN node build: Arduino Nano + MCP2515 + sensor integration
Forum Post 3 | mmWave safety interlock — industrial safety principles at hobby scale
Forum Post 4 | Edge AI plant health monitoring with the UNO Q Vision Brick
Forum Post 5 | Computer vision precision irrigation — targeting pot plants with a water cannon
Project Blog | Full system documentation, video demo, lessons learned, open-source release

## Credentials

I have taken part in many computer hackathons over the years and had never
really seen much in the electronics space. This and the community at Element 14
are things I have only recently discovered and am super happy to have a dip.
In my professional career I have also presented at a number of conferences, one
such time at RubyKaigi 2023 in Japan I even demonstrated an electronics
build. I am super excited to bring my energy, presentation style and hardware
hackery to this Design Challenge - let's go!

https://github.com/saramic/green-brain
