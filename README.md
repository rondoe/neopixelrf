# NeoPixelRF

## Hardware

For longer strips use a mcu with plenty of ram. in my case i'm using an arduino mega.

## Upload via ISP

platformio run -t program

## V_DIMMER

0..100

## V_RGB

every rgb color

## V_VAR1

number of leds, causes reboot upon set

## Connecting the strip

use a 5v 15A power supply and connect the supply on both ends.
