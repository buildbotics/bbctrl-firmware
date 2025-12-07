################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.      #
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
import stat
import json
from tornado import gen
from tornado.web import HTTPError

from .RequestHandler import *
from . import util

__all__ = ['FileSystemHandler']


def clean_path(path):
    if path is None: return ''

    path = os.path.normpath(path)
    if path.startswith('..'): raise HTTPError(400, 'Invalid path')
    return path.lstrip('./').replace('#', '-').replace('?', '-')


class FileSystemHandler(RequestHandler):
    def get_fs(self): return self.get_ctrl().fs
    
    
    def delete(self, path):
        path = clean_path(path)
        
        # Check for protected paths before deletion
        if self.get_fs().is_protected_path(path):
            raise HTTPError(403, 
                'Cannot delete files in the macros folder. '
                'Remove the macro from Settings to delete its file.')
        
        # Invalidate any cached plan for deleted file
        self.get_ctrl().preplanner.invalidate(path)
        
        self.get_fs().delete(path)


    def put(self, path = None):
        if path is not None:
            path = clean_path(path)
            
            # Check for protected paths before write
            if self.get_fs().is_protected_path(path):
                raise HTTPError(403, 
                    'Cannot upload files to the macros folder. '
                    'Macro files are managed through Settings.')

            if 'file' in self.request.files:
                self.get_fs().mkdir(os.path.dirname(path))
                file = self.request.files['file'][0]
                self.get_fs().write(path, file['body'])
                
                # FIX: Invalidate cached plan when file is uploaded/overwritten
                # This ensures the preplanner recomputes the toolpath for new content
                self.get_ctrl().preplanner.invalidate(path)

            else: self.get_fs().mkdir(clean_path(path))

        elif 'gcode' in self.request.files: # Backwards compatibility
            file = self.request.files['gcode'][0]
            path = 'Home/' + clean_path(os.path.basename(file['filename']))
            self.get_fs().write(path, file['body'])
            
            # FIX: Invalidate cached plan for legacy upload path too
            self.get_ctrl().preplanner.invalidate(path)

        os.sync()


    @gen.coroutine
    def get(self, path):
        path = clean_path(path)
        if path == '': path = 'Home'
        realpath = self.get_fs().realpath(path)

        if not os.path.exists(realpath): raise HTTPError(404, 'File not found')
        elif os.path.isdir(realpath):
            files = []

            if os.path.exists(realpath):
                for name in os.listdir(realpath):
                    s = os.stat(realpath + '/' + name)

                    d = dict(name = name)
                    d['created']  = util.timestamp_to_iso8601(s.st_ctime)
                    d['modified'] = util.timestamp_to_iso8601(s.st_mtime)
                    d['size']     = s.st_size
                    d['dir']      = stat.S_ISDIR(s.st_mode)
                    
                    # Mark protected directories
                    if stat.S_ISDIR(s.st_mode) and name in self.get_fs().PROTECTED_DIRS:
                        d['protected'] = True

                    files.append(d)

            d = dict(path = path, files = files)

            self.set_header('Content-Type', 'application/json')
            self.write(json.dumps(d, separators = (',', ':')))

        else:
            # FIX: Add no-cache headers to prevent stale file content
            # This ensures re-uploaded files are fetched fresh
            self.set_header('Cache-Control', 
                'no-store, no-cache, must-revalidate, max-age=0')
            self.set_header('Pragma', 'no-cache')
            self.set_header('Expires', '0')
            
            with open(realpath.encode('utf8'), 'rb') as f:
                self.write(f.read())