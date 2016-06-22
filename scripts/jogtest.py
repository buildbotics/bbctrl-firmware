#!/usr/bin/env python3

from inevent import InEvent, JogHandler
from inevent.Constants import *


if __name__ == "__main__":
  # Load config
  config = {
    "deadband": 0.1,
    "axes": [ABS_X, ABS_Y, ABS_RZ],
    "arrows": [ABS_HAT0X, ABS_HAT0Y],
    "speed": [0x120, 0x121, 0x122, 0x123],
    "activate": [0x124, 0x126, 0x125]
    }

  # Listen for input events
  events = InEvent(types = "js kbd".split())
  handler = JogHandler(config)

  while not events.key_state("KEY_ESC"):
    events.process_events(handler)
