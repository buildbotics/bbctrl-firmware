#!/usr/bin/env python3

'''Check that the configuration variable template used on the RPi matches the
variables used in the AVR'''

import sys
import json

templ = json.load(open('src/resources/config-template.json', 'r'))
vars = json.load(open('avr/build/vars.json', 'r'))


def check(section):
    errors = 0

    for name, entry in section.items():
        if 'type' in entry:
            ok = False

            if 'code' in entry and not entry['code'] in vars:
                print('"%s" with code "%s" not found' % (name, entry['code']))

            else: ok = True

            if not ok: errors += 1

        else: errors += check(entry)

    return errors


errors = check(templ)
print('\n%d errors' % errors)
sys.exit(errors != 0)
