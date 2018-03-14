#!/usr/bin/env python3

import sys, serial, argparse
import numpy as np
import math
import json
from time import sleep
from collections import deque
from datetime import datetime

import matplotlib.pyplot as plt
import matplotlib.animation as animation


MM_PER_STEP = 5 * 1.8 / 360 / 32
SAMPLES_PER_MIN = 6000


class Plot:
  def __init__(self, port, baud, max_len):
    # Open serial port
    self.sp = serial.Serial(port, baud)

    # Create data series
    self.series = []
    for i in range(5):
      self.series.append(deque([0.0] * max_len))

    # Init vars
    self.max_len = max_len
    self.incoming = ''
    self.last = [None] * 4

    # Open velocity log
    ts = datetime.now().strftime('%Y-%m-%d-%H:%M:%S')
    self.log = open('velocity-%s.log' % ts, 'w')


  # Add new series data
  def add(self, i, value):
    self.series[i].pop()
    self.series[i].appendleft(value)


  def update_text(self, text, vel, data):
      text[4].set_text('V {0:8,.2f}'.format(vel))

      for i in range(4):
        text[i].set_text('{} {:11,}'.format('XYZA'[i], int(data[i])))


  def update(self, frame, axes, text):
    # Read new data
    try:
      data = self.sp.read(self.sp.in_waiting)
      self.incoming += data.decode('utf-8')

    except Exception as e:
      print(e)
      return

    while True:
      # Parse lines
      i = self.incoming.find('\n')
      if i == -1: break
      line = self.incoming[0:i]
      self.incoming = self.incoming[i + 1:]

      # Handle reset
      if line.find('RESET') != -1:
        self.update_text(text, 0, [0] * 4)
        self.log.write(line + '\n')
        self.last = [None] * 4
        continue

      # Parse data
      try:
        data = [float(value) for value in line.split(',')]
      except ValueError: continue

      if len(data) != 4: continue

      # Compute axis velocities
      v = []     # Axis velocity
      totalV = 0 # Tool velocity

      for i in range(4):
        if self.last[i] is not None:
          delta = self.last[i] - data[i]
          v.append(delta * MM_PER_STEP * SAMPLES_PER_MIN) # mm/min
          totalV += math.pow(v[i], 2)

        self.last[i] = data[i]

      # Compute tool velocity
      totalV = math.sqrt(totalV)

      # Update position and velocity text
      self.update_text(text, totalV, data)

      # Don't update plots when not moving
      if totalV == 0: continue

      # Add new data
      for i in range(4): self.add(i, v[i])
      self.add(4, totalV)

      # Update plots
      for i in range(5):
        axes[i].set_data(range(self.max_len), self.series[i])

      self.log.write(line + '\n')


  def close(self):
    self.sp.flush()
    self.sp.close()


if __name__ == '__main__':
  # Parse command line arguments
  description = "Plot velocity data in real-time"
  parser = argparse.ArgumentParser(description = description)
  parser.add_argument('-p', '--port', default = '/dev/ttyUSB0')
  parser.add_argument('-b', '--baud', default = 115200, type = int)
  parser.add_argument('-m', '--max-width', default = 2000, type = int)
  args = parser.parse_args()

  # Create plot
  plot = Plot(args.port, args.baud, args.max_width)
  fig = plt.figure()
  ax = plt.axes(xlim = (0, args.max_width), ylim = (-10000, 10000))
  axes = []
  axes_text = []

  # Setup position and velocity text fields
  font = dict(fontsize = 14, family = 'monospace')

  axes_text.append(plt.text(0,    11700, '', **font))
  axes_text.append(plt.text(800,  11700, '', **font))
  axes_text.append(plt.text(0,    10500, '', **font))
  axes_text.append(plt.text(800,  10500, '', **font))
  axes_text.append(plt.text(1500, 11700, '', **font))

  # Create axes
  for i in range(5):
    axes.append(ax.plot([], [])[0])
    # Set text color to match axis color
    axes_text[i].set_color(axes[i].get_color())

  # Initial text views
  plot.update_text(axes_text, 0, [0] * 4)

  # Set up animation
  anim = animation.FuncAnimation(fig, plot.update, fargs = [axes, axes_text],
                                 interval = 100)

  # Run
  plt.show()
  plot.close()
