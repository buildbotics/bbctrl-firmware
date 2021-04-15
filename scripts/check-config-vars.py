#!/usr/bin/env python3

'''
Check that the configuration variable template used on the RPi matches the
variables used in the AVR.
'''

import sys
import json

templ = json.load(open('src/resources/config-template.json', 'r'))
vars  = json.load(open('src/avr/build/vars.json', 'r'))


def get_codes(vars):
    for code in vars:
        if 'index' in vars[code]:
            for c in vars[code]['index']: yield c + code
        else: yield code


def check(section, codes, prefix = ''):
    errors = 0

    for name, entry in section.items():
        if 'type' in entry:
            if entry['type'] == 'list' and 'index' in entry:
                index = entry['index']

                for c in index:
                    errors += check(entry['template'], codes, c)

                return errors

            # TODO check that defaults are valid

            # Check that code exists
            if 'code' in entry:
                code = prefix + entry['code']
                if not code in codes:
                    errors += 1
                    print('"%s" with code "%s" not found' % (name, code))

                else:
                    # Check that types match
                    type1 = vars[entry['code']]['type']
                    type2 = entry['type']
                    if ((type1 == 'u8' and type2 not in ('int', 'enum')) or
                        (type1 == 'u16' and type2 != 'int') or
                        (type1 == 'f32' and type2 != 'float') or
                        (type1 == 'b8' and type2 != 'bool')):
                        errors += 1
                        print('%s, %s type mismatch %s != %s' % (
                            name, code, type2, type1))

        else: errors += check(entry, codes, prefix)

    return errors


codes = set(get_codes(vars))
errors = check(templ, codes)
if errors:
    print('\n%d errors' % errors)
    sys.exit(1)
