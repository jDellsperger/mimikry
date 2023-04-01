#!/usr/bin/python
# Demo code showing main and subclass and various methods invocation

from brightpi.brightpilib import *
import time

brightPi = BrightPi()
brightSpecial = BrightPiSpecialEffects()

brightSpecial.reset()

brightSpecial.set_led_on_off(LED_WHITE, OFF)
brightSpecial.set_led_on_off(LED_IR, ON)
brightSpecial.set_gain(9)
#brightSpecial.set_led_dim(LED_IR, 100)
print brightSpecial
time.sleep(2)
