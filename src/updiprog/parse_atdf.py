#!/usr/bin/env python3

import sys
from xml.etree import ElementTree as ET


for filename in sys.argv[1:]:
  r = ET.parse(filename).getroot()

  for i in r.findall('.//interface'):
    if i.attrib['type'] == 'updi':
      name = r.find('./devices/device').attrib['name'].lower()[2:]

      for seg in r.findall('.//memory-segment'):
        sname = seg.attrib['name']

        if sname == 'FUSES': num_fuses = int(seg.attrib['size'], 0) - 1

        if sname == 'MAPPED_PROGMEM':
          flash_start = int(seg.attrib['start'], 0)
          flash_size = int(seg.attrib['size'], 0)
          page_size = int(seg.attrib['pagesize'], 0)

      id = 0
      for prop in r.findall('.//property-group[@name="SIGNATURES"]/property'):
        i = int(prop.attrib['name'][9:])
        id += int(prop.attrib['value'], 0) << (8 * (2 - i))

      print('  {')
      print('    "%s",' % name)
      print('    0x%06x,' % id)
      print('    0x%04x,' % flash_start)
      print('    %d,'     % flash_size)
      print('    %d,'     % page_size)
      print('    %d'      % num_fuses)
      print('  },')
