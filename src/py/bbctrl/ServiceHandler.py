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

import json
import datetime
from .APIHandler import *

__all__ = ['ServiceHandler', 'ServiceItemHandler', 'ServiceCompleteHandler',
           'ServiceManualEntryHandler', 'ServiceNoteHandler', 'ServiceExportHandler',
           'ServiceDueHandler', 'ServiceHoursHandler']


class ServiceHandler(APIHandler):
    def get(self):
        """Get all service data"""
        self.write_json(self.get_ctrl().service.get_data())


class ServiceHoursHandler(APIHandler):
    def get(self):
        """Get just the hours data"""
        self.write_json(self.get_ctrl().service.get_all_hours())


class ServiceItemHandler(APIHandler):
    def post(self):
        """Create a new service item"""
        self.authorize()
        data = json.loads(self.request.body.decode('utf-8'))
        
        service = self.get_ctrl().service
        item = service.add_item(
            data.get('label', 'New Item'),
            data.get('interval', 100),
            data.get('color', '#e6e6e6'),
            data.get('hour_type')
        )
        
        self.write_json(item)


    def put(self, item_id):
        """Update a service item"""
        self.authorize()
        data = json.loads(self.request.body.decode('utf-8'))
        
        service = self.get_ctrl().service
        item = service.update_item(
            int(item_id),
            label=data.get('label'),
            interval=data.get('interval'),
            color=data.get('color'),
            hour_type=data.get('hour_type')
        )
        
        self.write_json(item)


    def delete(self, item_id):
        """Delete a service item"""
        self.authorize()
        service = self.get_ctrl().service
        service.delete_item(int(item_id))
        self.write_json({'success': True})


class ServiceCompleteHandler(APIHandler):
    def put(self, item_id):
        """Mark a service item as completed"""
        self.authorize()
        data = json.loads(self.request.body.decode('utf-8'))
        
        service = self.get_ctrl().service
        item = service.complete_item(
            int(item_id),
            notes=data.get('notes', '')
        )
        
        self.write_json(item)


class ServiceManualEntryHandler(APIHandler):
    def post(self, item_id):
        """Add a manual history entry"""
        self.authorize()
        data = json.loads(self.request.body.decode('utf-8'))
        
        service = self.get_ctrl().service
        item = service.add_manual_entry(
            int(item_id),
            data.get('hours'),
            data.get('date'),
            data.get('notes', '')
        )
        
        self.write_json(item)


class ServiceNoteHandler(APIHandler):
    def post(self):
        """Create a new note"""
        self.authorize()
        data = json.loads(self.request.body.decode('utf-8'))
        
        service = self.get_ctrl().service
        note = service.add_note(
            data.get('title', 'New Note'),
            data.get('content', '')
        )
        
        self.write_json(note)


    def put(self, note_id):
        """Update a note"""
        self.authorize()
        data = json.loads(self.request.body.decode('utf-8'))
        
        service = self.get_ctrl().service
        note = service.update_note(
            int(note_id),
            title=data.get('title'),
            content=data.get('content')
        )
        
        self.write_json(note)


    def delete(self, note_id):
        """Delete a note"""
        self.authorize()
        service = self.get_ctrl().service
        service.delete_note(int(note_id))
        self.write_json({'success': True})


class ServiceExportHandler(APIHandler):
    def get(self):
        """Export all service data as text file"""
        service = self.get_ctrl().service
        text = service.export_text()
        
        # Set headers for download
        filename = 'service-export-' + datetime.datetime.now().strftime('%Y-%m-%d-%H%M%S') + '.txt'
        self.set_header('Content-Type', 'text/plain')
        self.set_header('Content-Disposition', 'attachment; filename="%s"' % filename)
        
        self.write(text)


class ServiceDueHandler(APIHandler):
    def get(self):
        """Get list of items that are due"""
        service = self.get_ctrl().service
        due_items = service.get_due_items()
        self.write_json(due_items)
