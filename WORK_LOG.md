# Work Log

## TODO Links

- [ ] https://docs.arduino.cc/tutorials/uno-q/ssh/
- [ ] https://docs.arduino.cc/software/app-lab/tutorials/getting-started/#install--set-up-arduino-app-lab
- [ ] https://docs.arduino.cc/tutorials/uno-q/power-specification/

- What is mains earth leakage current? - build your own detector - ANT&TEC
    - https://www.youtube.com/watch?v=kI_9usgyoGY

- [ ] Ligt desing https://community.element14.com/challenges-projects/design-challenges/light-up-your-life/b/news/posts/winners-announcement---light-up-your-life-design-challenge?ICID=DCHmain-feature-widget

- [ ] UNO Q enclosure
    - https://community.element14.com/products/roadtest/b/blog/posts/arduino-uno-q---getting-started

- https://core-electronics.com.au/projects/

- [ ] recommended by skrugliewicz
  - Two Months with the Arduino Uno Q: Hackster PRO Jeremy Cook // Getting
    Started - Hackster.io, an Avnet community
    - https://www.youtube.com/watch?v=5jbS4puIlUs
    - https://www.hackster.io/news/hands-on-with-the-arduino-uno-q-74eabc1bd962

- [ ] probably want this on my mac as well
  - https://docs.arduino.cc/tutorials/uno-q/adb/
  ```sh
  brew install android-platform-tools
  adb version
  ```

- https://www.hackster.io/mferuscomelo/somnosphere-the-smart-bedside-lamp-4aea48
  - https://github.com/mferuscomelo/somnosphere-firmware

---

## Fri 8 May 2026

What is NI LabView

- **Getting Started With the LabVIEW Interface for Arduino - VI Shots**

  [![Getting Started With the LabVIEW Interface for Arduino - VI Shots
     ](http://img.youtube.com/vi/RhdnmFJcFA0/0.jpg)
     ](https://youtu.be/RhdnmFJcFA0)

  - connect via USB
  - NI-VISA drivers
  - Arduino VI package and drivers

- **Using NI LabVIEW Case Structures - NI Apps**

  [![Using NI LabVIEW Case Structures - NI Apps
     ](http://img.youtube.com/vi/I6M57RCnl5I/0.jpg)
     ](https://youtu.be/I6M57RCnl5I)

  - data acquisition and control
  - how to connect ot arduion
  - what COM port is the arduino connected on - by UART?
  - add Hobbist -> Open.vi -> block diagram serial
    - sub VI for the serial to update it
  - DEPRECATED Aug 1 2020
    - [https://www.labviewmakerhub.com/doku.php?id=learn:tutorials:libraries:linx:misc:custom_command_example](
      https://www.labviewmakerhub.com/doku.php?id=learn:tutorials:libraries:linx:misc:custom_command_example)
  - a lot of click ops

- **NI LabVIEW 2025 | The Future of Test Software & AI - NI Apps**

  [![NI LabVIEW 2025 | The Future of Test Software & AI - NI Apps
     ](http://img.youtube.com/vi/dVFGMxfihOQ/0.jpg)
     ](https://youtu.be/dVFGMxfihOQ)

  - run in docker on linux
  - Community edition on Mac - free for makers
  - Nigel AI

- **NI LabVIEW Basics Part 1: Creating a VI - NI Apps**

  [![NI LabVIEW Basics Part 1: Creating a VI - NI Apps
     ](http://img.youtube.com/vi/OBcwsJb01F8/0.jpg)
     ](https://youtu.be/OBcwsJb01F8)

  - VI - Virtual Instrument
  - block diagram and instrument panel

- **What's new in NI LabVIEW 2026 Q1 - NI Apps**

  [![What's new in NI LabVIEW 2026 Q1 - NI Apps
     ](http://img.youtube.com/vi/3lqDOZL1wik/0.jpg)
     ](https://youtu.be/3lqDOZL1wik)

  - concept of controls, set a frequency or a wave form - can that be used to
    control some hardware?
  - Nigel AI - code completion across the whole system and LabView+ suite -
    LabView 2026 Q1
  - diff features - LabView compare
  - "mark of the web" - approve and accept externally downloaded code
  - Project wide debug settings
  - 3rd party languages - .Net
  - CI/CD workflows on docker hub
    - https://hub.docker.com/r/nationalinstruments/labview
    - windows docker
    - linux docker
  - web control
    - https://github.com/zeshanabdullah10/controlstack-web-bridge
    - use as a web browser from with LabView
    - can integrate web Plotly plots

- **Using NI LabVIEW Case Structures - NI Apps**

  [![Using NI LabVIEW Case Structures - NI Apps
     ](http://img.youtube.com/vi/I6M57RCnl5I/0.jpg)
     ](https://youtu.be/I6M57RCnl5I)
  - example of case statements - pretty simple but may be a way of getting
    started.

- **How to Connect Arduino to LabVIEW: Wiring, Setup, and Programming |
  Advanced Arduino Control Systems - Full Course**

  [![How to Connect Arduino to LabVIEW: Wiring, Setup, and Programming |
     Advanced Arduino Control Systems - Full Course
     ](http://img.youtube.com/vi/SUbU0FSK_nw/0.jpg)
     ](https://youtu.be/SUbU0FSK_nw)

  - some driver to get to COM port
  - 7 hours of poor audio

## Thu 7 May 2026

- wrapped up the 2 node comms using Arduino Nano
- had some issues generating the video and playing via Jekyll

> All three are now BT.709. Jekyll serves static files directly so the updated
> file should be live immediately — hard refresh the browser (Cmd+Shift+R) to
> bypass the cache and test again.

> The root cause: the iPhone 13 Pro records in Dolby Vision/HLG (BT.2020 HDR),
> and even though we re-encoded to H.264, ffmpeg was copying the HDR color
> metadata. Browsers that encounter HLG metadata on an H.264 stream can fail to
> decode beyond a certain point.

```sh
ffmpeg -y -ss 5 -i $USER/Downloads/IMG_XXXX.MOV \
    -an \
    -vf "setpts=PTS/1.5,setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709" \
    -c:v libx264 -crf 28 -preset fast \
    -maxrate 1500k -bufsize 3000k \
    -pix_fmt yuv420p \
    -movflags +faststart \
    green-brain/docs/assets/20260507_01_two_CAN_nodes.mp4
```

Key flags:
- -ss 5 — skip first 5 seconds
- -an — no audio
- setpts=PTS/1.5 — 1.5x speed
- setparams=...bt709 — force SDR color metadata (fixes the browser playback bug)
- -crf 28 -maxrate 1500k — keeps it under 10MB

## Thu 22 Apr 2026

added a Jekyll Github pages blog using the commands

```sh
mise use ruby@3.2.2
gem install jekyll bundler
jekyll new docs

cd docs

# downgrade jekyll to 3.9.6
# gem "jekyll", "~> 3.9.5" # to work with github-pages
bundle add github-pages webrick

# configure the _config.yml

# run
bundle exec jekyll serve --port 8888

# open
http://127.0.0.1:8888/green-brain/
```

## Thu 16 Apr 2026

- Very thorough overview of all the capabilities of the UNO Q
  [Element14 community: roadtest of UNO Q by skruglewicz](
  https://community.element14.com/products/roadtest/rv/roadtest_reviews/1894/)
- in particular a bunch of useful technical links, some linux command like
  `sudo halt` and the fact the board triggers a restart if you do a `sudo
  poweroff`.
- also all the connectors and the kinds of screens and cammeras you can connect.

## Sun 11 Apr 2026

### running headless

- Once working I ran blink and then a web cam person detector.
- the machine ground to a halt and even crashed.
- it was running docker, a web server, the model, Debian with a windows
  manager, etc
- I turned off the visual login
  ```sh
  sudo systemctl get-default
  > graphical.target

  sudo systemctl set-default multi-user.target

  # to turn visual back on
  systemctl set-default graphical.target
  ```
- I could now run an app headless
  ```sh
  arduion-app-cli
  arduino-app-cli
  arduino-app-cli brick
  arduino-app-cli brick list

  # see what I have had running
  docker ps --all

  # seems the apps is what I need to run
  arduino-app-cli  app
  arduino-app-cli  app list
  arduino-app-cli  app start examples:video-person-classification

  # and now running docker
  docker ps
  CONTAINER ID
  d6e77c80cd1e
    IMAGE       ghcr.io/arduino/app-bricks/python-apps-base:0.8.0
    COMMAND     "/run.sh"
    CREATED      33 minutes ago
    STATUS      Up 32 minutes
    PORTS        0.0.0.0:7000->7000/tcp, :::7000->7000/tcp
    NAMES       var-lib-arduino-app-cli-examples-video-person-classification-main-1

  9a8391ccb97e
    IMAGE       ghcr.io/arduino/app-bricks/ei-models-runner:0.8.0
    COMMAND     "/home/arduino/start…"
    CREATED     33 minutes ago
    STATUS      Up 33 minutes (unhealthy)
    PORTS       0.0.0.0:4912->4912/tcp
    NAMES       var-lib-arduino-app-cli-examples-video-person-classification-ei-video-classification-runner-1
  ```
- now just get the IP and check it out in the browser
  ```sh
  # via a shell session to the Arduino UNO Q
  hostname -I

  # on another computer on the network
  open http://192.168.68.104:7000/
  ```

![](./media/20260412_01_person_detected.jpg)

### Setup

- unpacked the Arduino UNO Q 2GB
- plugged it in
- connected to network
- started configuring it "on the device"
- it updates
- and once I try to enter the the Linux Credentials - the page keeps
  refreshing!!!
    - over and over again

      ![](media/uno_q_app_lab_desktop_setup_reload_loop.gif)

    - finally worked out how to look at the system log
      ```sh
      journalctl -f
      ```
    - seems like there were some issues
      - something to do with `/dev/ttyGS0`
      - and restarting SSH?
- assumed that I needed to restart and remove any half configured startup
  information
  ```sh
  rm -rf \
    .local/share/app-lab \
    .cache/app-lab/

  sudo apt-get remove app-lab
  sudo apt-get install app-lab --yes
  ```
  - still no luck, somehow it still remembered where in the setup process it
    was and hence would endlessly reload.
- Next I attempted to download the source off App Lab to work out where these
  secrets are stored
  ```sh
  git clone github.com:arduino/arduino-app-lab.git
  ```
  - this suggested `secret-tool` but after installing and not seeing anything I
    decided to give up on the path to fix the broken, looping install and to
    flash the UNO Q from scratch.

### Re flashing the UNO Q

following
- [https://core-electronics.com.au/guides/how-to-reinstall-linux-on-the-uno-q-arduino-reflashing-guide/](
  https://core-electronics.com.au/guides/how-to-reinstall-linux-on-the-uno-q-arduino-reflashing-guide/)
- and
- I downloaded the flasher [https://www.arduino.cc/en/software/#flasher-tool](
  https://www.arduino.cc/en/software/#flasher-tool)
  ```bash
  cd arduino-flasher-cli-0.5.0-darwin-arm64
  ./arduino-flasher-cli
  ./arduino-flasher-cli flash latest
  ./arduino-flasher-cli flash list
  ./arduino-flasher-cli flash 20251229-457
  ./arduino-flasher-cli flash 20251127-441
  ```
- but after running a really long download - the file is put in temp and
  disappears. This got frustrating. I took a look at the source:
  - [https://github.com/arduino/arduino-flasher-cli.git](
    https://github.com/arduino/arduino-flasher-cli.git)
  - from here it seems the images are found via
    [https://downloads.arduino.cc/debian-im/Stable/info.json](
    https://downloads.arduino.cc/debian-im/Stable/info.json)
- I attempted `latest` 20251229-457` which errored and `20251127-441` which did
  something different. Finally I put on `latest` again and decided to connect
  via my laptop.

### It Worked

Connecting via laptop to setup App Lab was the trick - it needs this to set
everything up for the first boot and it CANNOT HAPPEN ON the device - things
learned I guess.
