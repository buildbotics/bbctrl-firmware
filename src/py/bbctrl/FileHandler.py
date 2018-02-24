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


class FileHandler(bbctrl.APIHandler):
    def prepare(self): pass


    def delete_ok(self, path):
        if not path:
            if os.path.exists('upload'):
                for path in os.listdir('upload'):
                    if os.path.isfile('upload/' + path):
                        os.unlink('upload/' + path)
        else:
            path = 'upload' + path
            if os.path.exists(path): os.unlink(path)


    def put_ok(self, path):
        gcode = self.request.files['gcode'][0]

        if not os.path.exists('upload'): os.mkdir('upload')

        path ='upload/' + gcode['filename']

        with open(path, 'wb') as f:
            f.write(gcode['body'])

        self.ctrl.state.set('selected', gcode['filename'])


    def get(self, path):
        if path:
            path = path[1:]

            with open('upload/' + path, 'r') as f:
                self.write_json(f.read())

            self.ctrl.mach.select(path)
            return

        files = []

        if os.path.exists('upload'):
            for path in os.listdir('upload'):
                if os.path.isfile('upload/' + path):
                    files.append(path)

        selected = self.ctrl.state.get('selected', '')
        if not selected in files:
            if len(files): self.ctrl.state.set('selected', files[0])

        self.write_json(files)
