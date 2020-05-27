################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.      #
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

import os
import bbctrl
import glob
import html
from tornado import gen
from tornado.web import HTTPError


def safe_remove(path):
    try:
        os.unlink(path)
    except OSError: pass


class FileHandler(bbctrl.APIHandler):
    def prepare(self): pass


    def delete_ok(self, filename):
        if not filename:
            # Delete everything
            for path in glob.glob(self.get_upload('*')): safe_remove(path)
            self.get_ctrl().preplanner.delete_all_plans()
            self.get_ctrl().state.clear_files()

        else:
            # Delete a single file
            filename = os.path.basename(filename)
            safe_remove(self.get_upload(filename))
            self.get_ctrl().preplanner.delete_plans(filename)
            self.get_ctrl().state.remove_file(filename)


    def put_ok(self, *args):
        gcode = self.request.files['gcode'][0]
        filename = os.path.basename(gcode['filename'].replace('\\', '/'))
        filename = filename.replace('#', '-').replace('?', '-')

        if not os.path.exists(self.get_upload()): os.mkdir(self.get_upload())

        with open(self.get_upload(filename).encode('utf8'), 'wb') as f:
            f.write(gcode['body'])
        os.sync()

        self.get_ctrl().preplanner.invalidate(filename)
        self.get_ctrl().state.add_file(filename)
        self.get_log('FileHandler').info('GCode received: ' + filename)


    @gen.coroutine
    def get(self, filename):
        if not filename: raise HTTPError(400, 'Missing filename')
        filename = os.path.basename(filename)

        with open(self.get_upload(filename).encode('utf8'), 'r') as f:
            self.write(f.read())

        self.get_ctrl().state.select_file(filename)
