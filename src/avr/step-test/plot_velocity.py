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


MM_PER_STEP = 6.35 * 1.8 / 360 / 32


class Plot:
  def __init__(self, port, baud, max_len):
    self.sp = serial.Serial(port, baud)

    self.series = []
    for i in range(5):
      self.series.append(deque([0.0] * max_len))

    self.max_len = max_len
    self.incoming = ''

    ts = datetime.now().strftime('%Y-%m-%d-%H:%M:%S')
    self.log = open('velocity-%s.log' % ts, 'w')

    self.last = [None] * 4


  def add(self, buf, value):
    if len(buf) < self.max_len:
      buf.append(value)
    else:
      buf.pop()
      buf.appendleft(value)


  def update_text(self, text, vel, data):
      text[4].set_text('V {0:8,.2f}'.format(vel))

      for i in range(4):
        text[i].set_text('{} {:11,}'.format('XYZA'[i], int(data[i])))


  def update(self, frame, axes, text):
    try:
      data = self.sp.read(self.sp.in_waiting)
      self.incoming += data.decode('utf-8')

    except Exception as e:
      print(e)
      return

    while True:
      i = self.incoming.find('\n')
      if i == -1: break
      line = self.incoming[0:i]
      self.incoming = self.incoming[i + 1:]

      if line.find('RESET') != -1:
        self.update_text(text, 0, [0] * 4)
        self.log.write(line + '\n')
        self.last = [None] * 4
        continue

      try:
        data = [float(value) for value in line.split(',')]
      except ValueError: continue

      if len(data) != 4: continue

      dist = 0
      v = [0] * 4

      for i in range(4):
        if self.last[i] is not None:
          delta = self.last[i] - data[i]
          dist += math.pow(delta, 2)
          v[i] = delta * MM_PER_STEP / 2.0 / 0.01 * 60 # Velocity in mm/min

        self.last[i] = data[i]

      # Compute total velocity
      vel = math.sqrt(dist) * MM_PER_STEP / 2.0 / 0.01 * 60
      self.update_text(text, vel, data)
      if vel == 0: continue

      self.log.write(line + '\n')

      for i in range(4): self.add(self.series[i], v[i])
      self.add(self.series[4], vel)

      for i in range(5):
        axes[i].set_data(range(self.max_len), self.series[i])


  def close(self):
    self.sp.flush()
    self.sp.close()


description = "Plot velocity data in real-time"

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description = description)
  parser.add_argument('-p', '--port', default = '/dev/ttyUSB0')
  parser.add_argument('-b', '--baud', default = 115200, type = int)
  parser.add_argument('-m', '--max-width', default = 2000, type = int)
  args = parser.parse_args()

  plot = Plot(args.port, args.baud, args.max_width)

  # Set up animation
  fig = plt.figure()
  ax = plt.axes(xlim = (0, args.max_width), ylim = (-10000, 10000))
  axes = []
  axes_text = []

  for i in range(5):
    axes.append(ax.plot([], [])[0])

  font = dict(fontsize = 14, family = 'monospace')

  axes_text.append(plt.text(0,    11700, '', **font))
  axes_text.append(plt.text(800,  11700, '', **font))
  axes_text.append(plt.text(0,    10500, '', **font))
  axes_text.append(plt.text(800,  10500, '', **font))
  axes_text.append(plt.text(1500, 11700, '', **font))

  plot.update_text(axes_text, 0, [0] * 4)

  anim = animation.FuncAnimation(fig, plot.update, fargs = [axes, axes_text],
                                 interval = 100)

  plt.show()
  plot.close()
