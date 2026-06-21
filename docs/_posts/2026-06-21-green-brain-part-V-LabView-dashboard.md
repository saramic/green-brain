---
layout: post
title: "Green Brain - Part V - LabView visualisation"
date: 2026-06-21 13:00:00 +1000
categories: element14 design-challenge green-brain
---

I had never heard of NI LabVIEW before this challenge but after watching some
overviews and tutorials it is clearly an industry standard, so I thought I would
give it a go.

# Recap

Green Brain 🌿🧠 is the idea of tracking plant 🪴 nodes via an industrial **CAN
bus**

- [Green Brain - Part I - CAN bus introduction][green-brain-part-I-can-bus-introduction]
- [Green Brain - Part II - Dev setup][green-brain-part-II-dev-setup]
- [Green Brain - Part III - Water Cannon Chaos][green-brain-part-III-water-cannon-chaos]
- [Green Brain - Part IV - Nodes Well That Ends Well][green-brain-part-IV-getting-the-nodes-together]

## Connecting to LabVIEW

The UNO Q already runs a Python Flask REST API that serves the live sensor
readings as JSON at `http://pollyanna.local:8080/api/nodes`:

```json
[
  { "node": 2, "temp": 24.5, "hum": 61.0, "seq": 42, "ok": true, "age": 3 },
  { "node": 3, "temp": 23.1, "hum": 58.0, "seq": 38, "ok": true, "age": 1 }
]
```

That means LabVIEW just needs to poll an HTTP endpoint — no new code on the
hardware side at all.

## What I tried — HTTP GET with JSON parsing

LabVIEW has a built-in **HTTP Client GET** VI under
`Data Communication → Protocols → HTTP Client`. Getting the raw JSON string
back from the API worked fine.

The stumbling block was parsing it. LabVIEW's `Unflatten From JSON` needs a
**type** input — an array-of-clusters constant whose structure exactly matches
the JSON. You build it outside-in: place an **Array Constant** shell first,
then drop a **Cluster Constant** inside it, then add typed constants for each
field with labels matching the JSON keys exactly.

![](/green-brain/assets/20260621_lab_view_temp_and_humidity.png)

I got as far as the HTTP response but hit a type mismatch error:

```
LabVIEW: (Hex 0xFFFA4723) Type mismatch between JSON and LabVIEW.
```

The debug approach suggested on the [NI forums][ni-json-forum] is to wire
the cluster-array constant to **Flatten To JSON** first and compare its output
to the real API response — whatever differs is the mismatch. I ran out of time
to track mine down fully, but it is close.

## Other ways to connect sensors to LabVIEW

For reference, here is how the options compare — from simplest to most capable:

| Approach        | How                                  | Good for                            |
| --------------- | ------------------------------------ | ----------------------------------- |
| **VISA Serial** | Arduino → USB → LabVIEW VISA Read VI | Quick bench work, no network needed |
| **TCP stream**  | UNO Q opens a socket, streams CSV    | Low latency, one-to-one             |
| **HTTP REST**   | LabVIEW polls a URL (what I used)    | Already have a server running       |
| **MQTT**        | UNO Q publishes, LabVIEW subscribes  | Multiple consumers, buffered        |
| **NI DAQmx**    | NI hardware plugs straight in        | Precision timing, industrial use    |

For temperature monitoring the timing requirements are relaxed, so all of these
work in practice. HTTP polling was the natural fit here since the REST API
already existed.

## Next

[Green Brain - distributed greenhouse control platform][green-brain-project-post]

## Source

[https://github.com/saramic/green-brain][github-green-brain]

[on-the-line]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/
[green-brain-part-i-can-bus-introduction]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/f/forum/56921/green-brain---part-i---can-bus-introduction
[green-brain-part-II-dev-setup]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/f/forum/57001/green-brain---part-ii---dev-setup
[sentinel-box-part-i-the-plan]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56869/sentinel-box---part-i---the-plan
[sentinel-box-part-ii-back-to-c]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56894/sentinel-box---part-ii---back-to-c
[green-brain-part-III-water-cannon-chaos]: http://127.0.0.1:8888/green-brain/element14/design-challenge/green-brain/2026/06/20/green-brain-part-III-water-cannon-chaos.html
[green-brain-part-IV-getting-the-nodes-together]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/f/forum/57049/green-brain---part-iv---nodes-well-that-ends-well
[ni-json-forum]: https://forums.ni.com/t5/G-Web-Development-Software/json-parsing-array-of-cluster/td-p/4188063
[green-brain-project-post]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/b/projects/posts/green-brain---distributed-greenhouse-control-platform
[github-green-brain]: https://github.com/saramic/green-brain
[zephyrproject-uno-q]: https://docs.zephyrproject.org/latest/boards/arduino/uno_q/doc/index.html
