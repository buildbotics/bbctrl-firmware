################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.      #
#                                                                              #
#         This Source describes Open Hardware and is licensed under the        #
#                                 CERN-OHL-S v2.                               #
#                                                                              #
#         You may redistribute and modify this Source and make products        #
#    using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).  #
#           This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED          #
#    WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS  #
#     FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable    #
#                                  conditions.                                 #
#                                                                              #
#                Source location: https://github.com/buildbotics               #
#                                                                              #
#      As per CERN-OHL-S v2 section 4, should You produce hardware based on    #
#    these sources, You must maintain the Source Location clearly visible on   #
#    the external case of the CNC Controller or other product you make using   #
#                                  this Source.                                #
#                                                                              #
#                For more information, email info@buildbotics.com              #
#                                                                              #
################################################################################

from datetime import datetime
import pkg_resources
from pkg_resources import Requirement, resource_filename


_version = pkg_resources.require('bbctrl')[0].version

try:
  with open('/sys/firmware/devicetree/base/model', 'r') as f:
    _model = f.read()
except: _model = 'unknown'


# 16-bit less with wrap around
def id16_less(a, b): return (1 << 15) < (a - b) & ((1 << 16) - 1)


def get_resource(path):
  return resource_filename(Requirement.parse('bbctrl'), 'bbctrl/' + path)


def get_version(): return _version
def get_model(): return _model
def parse_version(s): return tuple([int(x) for x in s.split('.')])
def version_less(a, b): return parse_version(a) < parse_version(b)
def timestamp(): return datetime.now().strftime('%Y%m%d-%H%M%S')


def timestamp_to_iso8601(ts):
  return datetime.fromtimestamp(ts).replace(microsecond = 0).isoformat() + 'Z'
