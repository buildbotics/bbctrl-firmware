#!/usr/bin/env python3

################################################################################
#                                                                              #
#                This file is part of the Buildbotics firmware.                #
#                                                                              #
#                  Copyright (c) 2015 - 2018, Buildbotics LLC                  #
#                             All rights reserved.                             #
#                                                                              #
#     This file ("the software") is free software: you can redistribute it     #
#     and/or modify it under the terms of the GNU General Public License,      #
#      version 2 as published by the Free Software Foundation. You should      #
#      have received a copy of the GNU General Public License, version 2       #
#     along with the software. If not, see <http://www.gnu.org/licenses/>.     #
#                                                                              #
#     The software is distributed in the hope that it will be useful, but      #
#          WITHOUT ANY WARRANTY; without even the implied warranty of          #
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       #
#               Lesser General Public License for more details.                #
#                                                                              #
#       You should have received a copy of the GNU Lesser General Public       #
#                License along with the software.  If not, see                 #
#                       <http://www.gnu.org/licenses/>.                        #
#                                                                              #
#                For information regarding this software email:                #
#                  "Joseph Coffland" <joseph@buildbotics.com>                  #
#                                                                              #
################################################################################

import sys
import csv
import matplotlib.pyplot as plt
import numpy as np
import math


def compute_velocity(rows):
    p = [0, 0, 0]

    for row in rows:
        t = float(row[0])
        d = 0.0

        for i in range(3):
            x = float(row[i + 1]) - p[i]
            d += x * x
            p[i] = float(row[i + 1])

        d = math.sqrt(d)

        yield d / t


time_step = 0.005

data = list(csv.reader(sys.stdin))

velocity = list(compute_velocity(data))
acceleration = np.diff(velocity)
jerk = np.diff(acceleration)

t = np.cumsum([float(row[0]) for row in data])
x = [float(row[1]) for row in data]
y = [float(row[2]) for row in data]
z = [float(row[3]) for row in data]

rows = 3
row = 1
if np.sum(x): rows += 1
if np.sum(y): rows += 1
if np.sum(z): rows += 1

def next_subplot():
    global row
    plt.subplot(rows * 100 + 10 + row)
    row += 1

if np.sum(x):
    next_subplot()
    plt.title('X')
    plt.plot(t, x, 'r')
    plt.ylabel(r'$mm$')

if np.sum(y):
    next_subplot()
    plt.title('X')
    plt.title('Y')
    plt.plot(t, [row[2] for row in data], 'g')
    plt.ylabel(r'$mm$')

if np.sum(z):
    next_subplot()
    plt.title('Z')
    plt.plot(t, [row[3] for row in data], 'b')
    plt.ylabel(r'$mm$')

next_subplot()
plt.title('Velocity')
plt.plot(t, velocity, 'g')
plt.ylabel(r'$\frac{mm}{min}$')

next_subplot()
plt.title('Acceleration')
plt.plot(t[1:], acceleration, 'b')
plt.ylabel(r'$\frac{mm}{min^2}$')

next_subplot()
plt.title('Jerk')
plt.plot(t[2:], jerk, 'r')
plt.ylabel(r'$\frac{mm}{min^3}$')
plt.xlabel('Seconds')

plt.tight_layout()
plt.subplots_adjust(hspace = 0.5)
mng = plt.get_current_fig_manager()
mng.resize(*mng.window.maxsize())
plt.show()
