#!/usr/bin/env python3

from setuptools import setup
import json

pkg = json.load(open('package.json', 'r'))


setup(
    name = pkg['name'],
    version = pkg['version'],
    description = 'Buildbotics Machine Controller',
    long_description = open('README.md', 'rt').read(),
    author = 'Joseph Coffland',
    author_email = 'joseph@buildbotics.org',
    platforms = ['any'],
    license = pkg['license'],
    url = pkg['homepage'],
    package_dir = {'': 'src/py'},
    packages = ['bbctrl', 'inevent', 'lcd'],
    include_package_data = True,
    entry_points = {
        'console_scripts': [
            'bbctrl = bbctrl:run'
            ]
        },
    scripts = ['scripts/update-bbctrl', 'scripts/upgrade-bbctrl'],
    install_requires = 'tornado sockjs-tornado pyserial pyudev smbus2'.split(),
    zip_safe = False,
    )
