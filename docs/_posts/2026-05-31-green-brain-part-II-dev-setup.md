---
layout: post
title:  "Green Brain - Part II - Dev setup"
date:   2026-05-31 21:00:00 +1000
categories: element14 design-challenge green-brain
---

Finding a development toolchain for the **UNO Q** turned out to be more adventure than I bargained for.

# Recap

Green Brain 🌿🧠 is the idea of tracking plant 🪴 nodes via an industrial **CAN bus** to ultimately see which plant needs more water cannon 🔫.

- [Green Brain - Part I - CAN bus introduction][green-brain-part-I-can-bus-introduction]

## The wrong turn

Just like my recent [**Sentinel-box** project][sentinel-box-part-ii-back-to-c]
where I wanted to achieve "Hermetic Builds", totally isolated from external
systems and in particular any click-ops and IDE's, so I tried my luck with the
**UNO Q** environment. As I could install `arduino-cli` on my Mac OSX machine, I
thought that was a great start. After all it is only 4 characters off the
`arduino-app-cli` (Arduino App Lab CLI) command.

There seem to be a lot of videos, blogs and tutorials out there on how to
connect an "Arduino", even remotely, to a build system. Things like:

```shell
# DOES NOT WORK with UNO Q (for me)
arduino-cli upload \
  --fqbn arduino:zephyr:unoq \
  --port 192.168.68.128 \
  --protocol network \
  .
```

looked promising, connected, "succeeded," but nothing seemed to be actually
running on the **MCU** (Microcontroller Unit - `STM32U585`).

Given I really wanted to code on my Mac OSX, running my own specific editor
(VSCode) with version control setup and VIM key bindings, etc. I was keeping
away from the Arduino App Lab for the moment.

Although I couldn't get the **MCU** working all that quickly, I was aiming for a
website so I thought I would upload a Node JavaScript server. I worked out that
Unix `rsync` could "remote synchronize" my files to the remote **UNO Q**. The
name of the board - **pollyanna** in my case, is available on the network as
`pollyanna.local` with a generated SSH key shared between machines

```shell
# generate a key
ssh-keygen -o -a 100 -t ed25519

# with a specific name
find ~/.ssh/id_ed25519_UNO_Q*
/Users/michael/.ssh/id_ed25519_UNO_Q
/Users/michael/.ssh/id_ed25519_UNO_Q.pub

# upload it to the UNO Q
ssh pollyanna.local
mkdir .ssh
chmod 700 .ssh
vi .ssh/authorized_keys
chmod 600 .ssh/authorized_keys

# on my host machine Mac OSX update hosts to use the specific key
# append to file
cat << EOF >> ~/.ssh/config
Host pollyanna.local
    User arduino
    IdentityFile ~/.ssh/id_ed25519_UNO_Q
EOF
```

I could `rsync` my files across

```shell
# installing rsync on the UNO Q
sudo apt-get install -y rsync

# make sure directory exists
ssh pollyanna.local 'mkdir -p /home/arduino/ArduinoApps/green-brain'

# sync files up to the UNO Q
rsync -av \
  --exclude='.cache/' \
  experiments/in_app_new/ \
  pollyanna.local:~/ArduinoApps/green-brain-in_app_new/
```

Or with a quick `mise` script (handy runtime manager and task runner)

```shell
mise run upload:green-brain
```

So I could:

1. upload my Node files,
2. jump onto the machine
3. go to the directory
4. work out I don't have `node` installed nor `npm`
5. manually `apt-get` install those
6. `npm install` any libraries
7. start the web server
8. "success" I had a web page

Far from hermetic — and ironically far more painful than clicking a button in Arduino App Lab 🤔.

To make things worse, trying to work out how to communicate between the **MCU**
and the **MPU**, some blog posts suggested it was a good idea to try and
uninstall the `RouterBridge`. This is so deeply embedded in the platform that
none of the tricks seemed to work to stop it or read the interfaces directly.

## Always start with blink

The frustration reminded me of how I tried to re-write the `HAL` (hardware
abstraction layer) for the **MAX32630FTHR** using [rust 🦀
][sentinel-box-part-i-the-plan]. I went back to **App Lab**, clicked on blink,
and in no time I had an LED blinking. I investigated further and worked out
that with

```shell
arduino-app-cli app new green-brain
```

I would have a predetermined, sketch and python code as well as an `app.yml` to
wrap the whole thing up. It also meant these apps appeared in **App Lab** user
interface and I could, 1 click deploy and monitor. In no time I extended the
blink to be able to set the LED ON/OFF via a website or writing to a file on
disk, yes using the `RouterBridge`.

So in the end the default tool chain seems more than satisfactory for what I was
using, and by maintaining my `rsync`, `SSH` settings and a few scripts, I was set
(An alternative to SSH may be `adb`, the Android Platform Tools). VSCode and my
default environment for coding, and 3 tasks to `upload` ↗, `start` 🚀 and `stop`
🛑 the sketch on both the MCU and a docker based web server (no need to manually
install anything).

The end advice - follow the **App Lab** way and build out from that.

```cpp
// allow connectivity to the Bridge between MCU and MPU
#include "Arduino_RouterBridge.h"

// inbuilt CAN drivers
#include <zephyr/drivers/can.h>
...
// 500 kbps — must match sensor nodes (CAN_500KBPS)
#define CAN_BITRATE 500000
...
// assuming the CAN device is fully setup
...
static String handle_get_nodes() {
  // handle get nodes, and return JSON string with current temp and humidity
}

static String handle_get_status() {
  // handle get status and return a JSON string of generic status data
}

... // make sure the CAN frame matches up with the nodes

void setup() {
  Bridge.begin();
  Bridge.provide_safe("get_nodes",  handle_get_nodes);
  Bridge.provide_safe("get_status", handle_get_status);
}

void loop() {
  //
}
```

On the Python end

```python
from arduino.app_utils import App, Bridge

...

def loop():
    status = None
    try:
        raw  = Bridge.call("get_nodes")
        data = json.loads(raw)
        # do something with the data

        status = json.loads(Bridge.call("get_status"))
        # do something with the status

App.run(user_loop=loop)
```

and the result

![](/green-brain/assets/20260531_CAN_hub_and_nodes.gif)

<video width="740" controls>
  <source src="/green-brain/assets/20260531_CAN_hub_and_nodes.mp4" type="video/mp4">
  Your browser does not support the video tag.
</video>

## CAN Hub

Now I could connect my existing Arduino Nano nodes to the UNO Q acting as a hub.
Noting that the **UNO Q** `STM32` runs on 3V3, I switched to another CAN bus
transceiver, the `WCMCU-230` based on `SN65HVD230`. This proved a little tricky
as it seems that the ["zephyr system"][zephyrproject-uno-q] supports `CAN` on
prescribed pins `~D5` (FDCAN1_RX) and `D4` (FDCAN1_TX)

Type | Location | Description                   | Compatible
-----|----------|-------------------------------|----------------
CAN  | on-chip  | STM32 FDCAN CAN FD controller | st, stm32-fdcan

And sure enough, after some tweaks, I had connected to my previous 2 nodes and
could read their temperature and humidity.

## Next

Lots still to cover:
- Unboxing newly selected hardware
- Connecting K-Type Thermocouples (hopefully via a MAX6675, which are on the way)
- The water cannon 🔫

oh and given that my build is a jumble of cables, and from a previous design competition I had run out of time, I will need to set up some kind of hardware build to contain the nodes and Garden Hub

## Source

[https://github.com/saramic/green-brain][github-green-brain]

[on-the-line]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/
[green-brain-part-i-can-bus-introduction]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/f/forum/56921/green-brain---part-i---can-bus-introduction

[sentinel-box-part-i-the-plan]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56869/sentinel-box---part-i---the-plan
[sentinel-box-part-ii-back-to-c]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56894/sentinel-box---part-ii---back-to-c

[github-green-brain]: https://github.com/saramic/green-brain

[zephyrproject-uno-q]: https://docs.zephyrproject.org/latest/boards/arduino/uno_q/doc/index.html
