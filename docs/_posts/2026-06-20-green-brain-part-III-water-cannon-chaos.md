---
layout: post
title:  "Green Brain - Part III - Water Cannon Chaos"
date:   2026-06-20 23:00:00 +1000
categories: element14 design-challenge green-brain
---

I promised a water cannon 💦🔫 for my plants, so I had to deliver, if only for a
forum post.

# Recap

Green Brain 🌿🧠 is the idea of tracking plant 🪴 nodes via an industrial **CAN
bus** to ultimately see which plant needs more water cannon 🔫.

- [Green Brain - Part I - CAN bus introduction][green-brain-part-I-can-bus-introduction]
- [Green Brain - Part II - Dev setup][green-brain-part-II-dev-setup]

## the idea and the gear

I had the gear for a water cannon. My wife wanted an indoor plant watering
system for Christmas so I got her one. But what present is complete without some
extra DIY in case I could build one myself. As such I had a few small pumps
around. I also had a servo driven X-Y pan and tilt module. How hard can it be to
hook that up and make a water cannon.

As an aside, I am also super grateful for a bunch of gear that Element 14 sent
to me as a selected challenger.

![](/green-brain/assets/20260620_on_the_line_challengers_kit.jpg)

The pressure is on to use as much of this gear as I can.

## Water and electronics

Well there was a lot of learning to be had:

1. no matter how well you think you seal things - water will still leak.
   Although I didn't go all out with silicon, I thought that tight fitting vinyl
   tubing, electrical tape and relatively low water pressure would be OK — it
   was not
2. Water brings in fluid dynamics and things like head, flow, nozzle shapes and
   the like. Clearly my pump could not initially pump up 300mm of head so I
   moved the motor closer to the nozzle. There would also be time required to
   refill the tubing up to the nozzle, as once the pump stops the tube empties.
   Gardening nozzles from a local hardware are only so good at firing a stream
   of water
3. Even small hoses can be stiff enough to move the servos
4. Pan and Tilt movement requires some planning in how everything interacts. My
   platform got tangled in the hose, and the wiring also got stuck every so
   often for the servos as everything moved around
5. Water is heavy and the motion of water in a hose can change the weight
   balance of everything and move things around
6. With all this chaos, I was not bringing the UNO Q anywhere near this water
   cannon
7. Noise, pump motors can produce noise, and that noise can be picked up by the
   servos and move them haywire around the place

It was this last point that really killed this feature. I had removed the servo
library entirely, moved the pump power onto a completely separate circuit
isolated by a relay — and still the servos would pan to the right and tilt to
the top. Stripping down to a pure diagnostic sketch (no servo code at all)
revealed the culprit: the servo signal pins were floating high-impedance inputs,
acting as antennas and picking up the pump motor's EMI directly? maybe?
¯\_(ツ)_/¯. Even with no code driving them, the servo controllers saw enough
noise to chase a position. I ran out of steam getting this to behave reliably
with the full mechanical assembly.

![](/green-brain/assets/20260620_pan_and_tilt_cannon.gif)

![](/green-brain/assets/20260620_water_cannon_shoot_right_and_up.gif)

<video width="740" controls>
  <source src="/green-brain/assets/20260620_palying_around_with_firing_the_cannon.mp4" type="video/mp4">
  Your browser does not support the video tag.
</video>

I was hoping to re-use the water cannon as a dog deterrent from scratching a
door. I think neither the dogs nor the plants will be seeing any water cannon
any time soon.

![](/green-brain/assets/20260620_water_cannon.jpg)

The only clever things I did was write to EEProm to see if a reboot was
happening when the pump kicked in and use the Omega temperature probe to hold
the tube that fed water to the pump.

## Next

- I have some MAX6675 which would be cool to connect to the K-Type Thermocouples
- there is the Omega Temperature probe
- and I am hoping to at least visualise a little bit of this on NI LabView

oh and there is like 24 hours left 🤞

## Source

[https://github.com/saramic/green-brain][github-green-brain]

[on-the-line]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/
[green-brain-part-i-can-bus-introduction]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/f/forum/56921/green-brain---part-i---can-bus-introduction
[green-brain-part-II-dev-setup]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/f/forum/57001/green-brain---part-ii---dev-setup

[sentinel-box-part-i-the-plan]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56869/sentinel-box---part-i---the-plan
[sentinel-box-part-ii-back-to-c]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56894/sentinel-box---part-ii---back-to-c

[github-green-brain]: https://github.com/saramic/green-brain

[zephyrproject-uno-q]: https://docs.zephyrproject.org/latest/boards/arduino/uno_q/doc/index.html
