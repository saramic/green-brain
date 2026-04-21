---
layout: post
title:  "Green Brain - Part I - CAN bus introduction"
date:   2026-04-22 08:05:50 +1000
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

Although the selected challengers for the FREE hardware have not been announce,
looking at the list, it seems the main elements are:

1. [Arduino UNO Q][element-14-arduino-UNO-Q]

   Single Board Computer, **Arduino UNO Q**, QRB2210/STM32U585, 2GB, ARM
   Cortex-A53/M33F

2. [MAX33041ESHLD#][element-14-MAX33041ESHLD#]

   Shield Evaluation Kit, MAX33041E, Interface, **CAN Transceiver**

I was intruiged by the UNO Q by a review by @skruglewicz

- [Test out Arduino's Uno Q - The new Single-Board
  Computer][element-14-review-uno-q]

So I purchased one. The **CAN Transciever** board was a bit out of my price
bracket, but I worked out I can get cheap, less industrial, **CAN bus** modules
so I got a few of those to get my head around the communications standard.

## UNO Q and CAN bus

... TODO

- CAN bus communication
- Visual dashboard on web running in docker

## Next

Hopefully the winners will be announced soon. Then I can work out if I will
have some extra hardware going my way or not. Either way I think I need to get
deep into some computer vision and plant detection using a model like
[**YOLOv8**][YOLOV8-com] or similar running no the **UNO Q**.

## Source

[https://github.com/saramic/green-brain][github-green-brain]

[on-the-line]: https://community.element14.com/challenges-projects/design-challenges/on-the-line/
[github-green-brain]: https://github.com/saramic/green-brain
[element-14-arduino-UNO-Q]: https://au.element14.com/new-products/embedded-computers-education-maker-boards/arduino-uno-q
[element-14-MAX33041ESHLD#]: https://au.element14.com/analog-devices/max33041eshld/shileld-eval-kit-can-transceiver/dp/3807530
[element-14-review-uno-q]: https://community.element14.com/products/roadtest/rv/roadtest_reviews/1894/test
[YOLOV8-com]: https://yolov8.com/
