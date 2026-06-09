"""clocksmith — TMC2209 / RAMPS 1.4 stepper viability tester (host side).

A small Python application that talks to the Arduino Mega tester firmware over
USB serial, drives a single stepper through microstep / current / chopper
configurations, and visualises StallGuard load and achieved step rate so a
motor can be judged viable for a target speed.
"""

__version__ = "0.1.0"
