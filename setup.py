#!/usr/bin/env python3

from setuptools import setup

setup(
    name = 'bbctrl',
    version = '0.0.2',
    description = 'Buildbotics Machine Controller',
    long_description = open('README.md', 'rt').read(),
    author = 'Joseph Coffland',
    author_email = 'joseph@buildbotics.org',
    platforms = ['any'],
    license = 'GPL 3+',
    url = 'https://github.com/buildbotics/rpi-firmware',
    package_dir = {'': 'src/py'},
    packages = ['bbctrl', 'inevent', 'lcd'],
    include_package_data = True,
    eager_resources = ['bbctrl/http/*'],
    entry_points = {
        'console_scripts': [
            'bbctrl = bbctrl:run'
            ]
        },
    install_requires = 'tornado sockjs-tornado pyserial pyudev smbus2'.split(),
    zip_safe = False,
    )
