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
        filename = os.path.basename(gcode['filename'])

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

        with open(self.get_upload(filename), 'r') as f:
            self.write(f.read())

        self.get_ctrl().state.select_file(filename)
