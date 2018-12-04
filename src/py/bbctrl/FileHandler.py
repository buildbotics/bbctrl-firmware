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
import logging
from tornado import gen


log = logging.getLogger('FileHandler')


def safe_remove(path):
    try:
        os.unlink(path)
    except OSError: pass


class FileHandler(bbctrl.APIHandler):
    def prepare(self): pass


    def delete_ok(self, filename):
        if not filename:
            # Delete everything
            for path in glob.glob('upload/*'): safe_remove(path)
            self.ctrl.preplanner.delete_all_plans()
            self.ctrl.state.clear_files()

        else:
            # Delete a single file
            safe_remove('upload' + filename)
            self.ctrl.preplanner.delete_plans(filename)
            self.ctrl.state.remove_file(filename)


    def put_ok(self, path):
        gcode = self.request.files['gcode'][0]
        filename = gcode['filename']

        if not os.path.exists('upload'): os.mkdir('upload')

        path ='upload/' + filename

        with open(path, 'wb') as f:
            f.write(gcode['body'])

        self.ctrl.preplanner.invalidate(filename)
        self.ctrl.state.add_file(filename)
        log.info('GCode updated: ' + filename)


    @gen.coroutine
    def get(self, filename):
        filename = filename[1:] # Remove /

        with open('upload/' + filename, 'r') as f:
            self.write(f.read())

        self.ctrl.state.select_file(filename)
