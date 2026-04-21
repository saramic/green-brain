# Work Log

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
