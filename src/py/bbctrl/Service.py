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
import json
import time
import datetime

from . import util

__all__ = ['Service']


class Service:
    # Hour types tracked
    HOUR_POWER = 'power_hours'      # Total power-on time
    HOUR_MOTION = 'motion_hours'    # Time actively running program (in motion)
    
    SAVE_INTERVAL = 60  # Save to disk every 60 seconds
    
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.log = ctrl.log.get('Service')
        self.path = os.path.join(ctrl.get_path(), 'service.json')
        
        # Load template for defaults and validation
        try:
            with open(util.get_resource('http/service-template.json'), 'r',
                      encoding='utf-8') as f:
                self.template = json.load(f)
        except Exception as e:
            self.log.warning('Failed to load service-template.json: %s' % e)
            self.template = {}
        
        self.data = self._load()
        
        # Time tracking state
        self._power_start = time.time()  # Always tracking power-on
        self._motion_start = None
        self._last_save = time.time()
        
        # Accumulated time since last save (in seconds)
        self._pending_power = 0
        self._pending_motion = 0
        
        self.log.info('Service tracking initialized. Hours: power=%.1f, motion=%.1f' % (
            self.get_hours(self.HOUR_POWER),
            self.get_hours(self.HOUR_MOTION)
        ))


    def _get_template_default(self, section, key):
        """Get default value from template"""
        try:
            if section in self.template:
                if key in self.template[section]:
                    return self.template[section][key].get('default')
        except Exception:
            pass
        return None


    def _load(self):
        """Load service data from file or create default structure"""
        if os.path.exists(self.path):
            try:
                with open(self.path, 'r') as f:
                    data = json.load(f)
                    # Migrate from old single run_hours format
                    if 'run_hours' in data and self.HOUR_MOTION not in data:
                        data[self.HOUR_MOTION] = data.pop('run_hours')
                    return self._ensure_defaults(data)
            except Exception as e:
                self.log.error('Failed to load service.json: %s' % e)
        
        return self._ensure_defaults({})


    def _ensure_defaults(self, data):
        """Ensure all required fields exist using template defaults"""
        # Get defaults from template or use fallbacks
        defaults = {
            'version': self._get_template_default('meta', 'version') or 2,
            self.HOUR_POWER: self._get_template_default('hours', 'power_hours') or 0.0,
            self.HOUR_MOTION: self._get_template_default('hours', 'motion_hours') or 0.0,
            'last_update': datetime.datetime.utcnow().isoformat() + 'Z',
            'items': [],
            'notes': []
        }
        
        for key, value in defaults.items():
            if key not in data:
                data[key] = value
        
        return data


    def _get_item_defaults(self):
        """Get default values for a new service item from template"""
        # Simplified - no color field
        defaults = {
            'label': '',
            'interval': 100,
            'hour_type': self.HOUR_MOTION,
            'last_completed': 0.0,
            'next_due': 0.0,
            'history': []
        }
        
        # Override with template defaults if available
        if 'items' in self.template and 'template' in self.template['items']:
            tmpl = self.template['items']['template']
            for key in defaults:
                if key in tmpl and 'default' in tmpl[key]:
                    defaults[key] = tmpl[key]['default']
        
        return defaults


    def _get_note_defaults(self):
        """Get default values for a new note from template"""
        defaults = {
            'title': '',
            'content': ''
        }
        
        # Override with template defaults if available
        if 'notes' in self.template and 'template' in self.template['notes']:
            tmpl = self.template['notes']['template']
            for key in defaults:
                if key in tmpl and 'default' in tmpl[key]:
                    defaults[key] = tmpl[key]['default']
        
        return defaults


    def get_template(self):
        """Return the service template for frontend use"""
        return self.template


    def _save(self):
        """Save service data to file"""
        try:
            self.data['last_update'] = datetime.datetime.utcnow().isoformat() + 'Z'
            with open(self.path, 'w') as f:
                json.dump(self.data, f, indent=2)
            self._last_save = time.time()
        except Exception as e:
            self.log.error('Failed to save service.json: %s' % e)
            raise


    # =========================================================================
    # Hour Tracking
    # =========================================================================
    
    def get_hours(self, hour_type):
        """Get hours for a specific type"""
        return self.data.get(hour_type, 0.0)
    
    
    def get_all_hours(self):
        """Get all hour types as dict"""
        return {
            self.HOUR_POWER: self.get_hours(self.HOUR_POWER),
            self.HOUR_MOTION: self.get_hours(self.HOUR_MOTION)
        }
    
    
    def motion_started(self):
        """Called when program starts running (in motion)"""
        if self._motion_start is None:
            self._motion_start = time.time()
            self.log.debug('Motion tracking started')
    
    
    def motion_stopped(self):
        """Called when program stops (complete, pause, error)"""
        if self._motion_start is not None:
            elapsed = time.time() - self._motion_start
            self._pending_motion += elapsed
            self._motion_start = None
            self.log.debug('Motion tracking stopped, +%.1f seconds' % elapsed)
    
    
    def update(self):
        """Called periodically to update hours and save if needed"""
        now = time.time()
        
        # Always accumulate power hours
        if self._power_start is not None:
            elapsed = now - self._power_start
            self._pending_power += elapsed
            self._power_start = now
        
        # Accumulate motion hours if running
        if self._motion_start is not None:
            elapsed = now - self._motion_start
            self._pending_motion += elapsed
            self._motion_start = now
        
        # Save periodically
        if now - self._last_save >= self.SAVE_INTERVAL:
            self._flush_pending()
    
    
    def _flush_pending(self):
        """Flush pending hours to data and save"""
        if self._pending_power > 0 or self._pending_motion > 0:
            # Convert seconds to hours
            self.data[self.HOUR_POWER] += self._pending_power / 3600.0
            self.data[self.HOUR_MOTION] += self._pending_motion / 3600.0
            
            self._pending_power = 0
            self._pending_motion = 0
            
            self._save()
    
    
    def shutdown(self):
        """Called on shutdown to save final hours"""
        self.motion_stopped()
        self._flush_pending()
        self.log.info('Service tracking shutdown. Final hours: power=%.1f, motion=%.1f' % (
            self.get_hours(self.HOUR_POWER),
            self.get_hours(self.HOUR_MOTION)
        ))


    # =========================================================================
    # Service Items
    # =========================================================================
    
    def get_data(self):
        """Get all service data for API"""
        # Include current (unsaved) hours in response
        data = dict(self.data)
        data[self.HOUR_POWER] += self._pending_power / 3600.0
        data[self.HOUR_MOTION] += self._pending_motion / 3600.0
        return data


    def get_items(self):
        """Get all service items"""
        return self.data.get('items', [])


    def add_item(self, label, interval=None, hour_type=None):
        """Add a new service item"""
        items = self.data.get('items', [])
        new_id = max([item['id'] for item in items], default=0) + 1
        
        # Get defaults from template
        defaults = self._get_item_defaults()
        
        # Use provided values or template defaults
        if interval is None:
            interval = defaults['interval']
        if hour_type is None:
            hour_type = defaults['hour_type']
        
        current_hours = self.get_hours(hour_type) + (
            self._pending_motion / 3600.0 if hour_type == self.HOUR_MOTION else
            self._pending_power / 3600.0
        )
        
        item = {
            'id': new_id,
            'label': label,
            'interval': interval,
            'hour_type': hour_type,
            'last_completed': current_hours,
            'next_due': current_hours + interval,
            'created': datetime.datetime.utcnow().isoformat() + 'Z',
            'history': []
        }
        
        items.append(item)
        self.data['items'] = items
        self._save()
        return item


    def update_item(self, item_id, label=None, interval=None, hour_type=None):
        """Update an existing service item"""
        items = self.data.get('items', [])
        
        for item in items:
            if item['id'] == item_id:
                if label is not None:
                    item['label'] = label
                if hour_type is not None:
                    item['hour_type'] = hour_type
                if interval is not None:
                    item['interval'] = interval
                    # Recalculate next_due based on new interval
                    item['next_due'] = item['last_completed'] + interval
                
                self._save()
                return item
        
        raise Exception('Service item %d not found' % item_id)


    def delete_item(self, item_id):
        """Delete a service item"""
        items = self.data.get('items', [])
        self.data['items'] = [item for item in items if item['id'] != item_id]
        self._save()


    def complete_item(self, item_id, notes=''):
        """Mark a service item as completed"""
        items = self.data.get('items', [])
        
        for item in items:
            if item['id'] == item_id:
                hour_type = item.get('hour_type', self.HOUR_MOTION)
                current_hours = self.get_hours(hour_type)
                
                # Add pending hours
                if hour_type == self.HOUR_MOTION:
                    current_hours += self._pending_motion / 3600.0
                else:
                    current_hours += self._pending_power / 3600.0
                
                # Add history entry
                history_entry = {
                    'type': 'completed',
                    'hours': current_hours,
                    'date': datetime.datetime.utcnow().isoformat() + 'Z',
                    'notes': notes
                }
                
                if 'history' not in item:
                    item['history'] = []
                item['history'].insert(0, history_entry)  # Newest first
                
                # Update last_completed and next_due
                item['last_completed'] = current_hours
                item['next_due'] = current_hours + item['interval']
                
                self._save()
                return item
        
        raise Exception('Service item %d not found' % item_id)


    def add_manual_entry(self, item_id, hours, date, notes):
        """Add a manual history entry to a service item"""
        items = self.data.get('items', [])
        
        for item in items:
            if item['id'] == item_id:
                history_entry = {
                    'type': 'manual',
                    'hours': hours,
                    'date': date,
                    'notes': notes
                }
                
                if 'history' not in item:
                    item['history'] = []
                item['history'].append(history_entry)
                
                # Sort history by date (newest first)
                item['history'].sort(key=lambda x: x['date'], reverse=True)
                
                self._save()
                return item
        
        raise Exception('Service item %d not found' % item_id)


    def get_due_items(self):
        """Get list of items that are due for service"""
        items = self.data.get('items', [])
        due_items = []
        
        for item in items:
            hour_type = item.get('hour_type', self.HOUR_MOTION)
            current_hours = self.get_hours(hour_type)
            
            # Add pending hours
            if hour_type == self.HOUR_MOTION:
                current_hours += self._pending_motion / 3600.0
            else:
                current_hours += self._pending_power / 3600.0
            
            if current_hours >= item['next_due']:
                due_items.append({
                    **item,
                    'current_hours': current_hours,
                    'overdue_by': current_hours - item['next_due']
                })
        
        return due_items


    # =========================================================================
    # Notes
    # =========================================================================
    
    def get_notes(self):
        """Get all notes"""
        return self.data.get('notes', [])


    def add_note(self, title, content):
        """Add a new note"""
        notes = self.data.get('notes', [])
        new_id = max([note['id'] for note in notes], default=0) + 1
        
        note = {
            'id': new_id,
            'title': title,
            'content': content,
            'created': datetime.datetime.utcnow().isoformat() + 'Z',
            'modified': datetime.datetime.utcnow().isoformat() + 'Z'
        }
        
        notes.append(note)
        self.data['notes'] = notes
        self._save()
        return note


    def update_note(self, note_id, title=None, content=None):
        """Update an existing note"""
        notes = self.data.get('notes', [])
        
        for note in notes:
            if note['id'] == note_id:
                if title is not None:
                    note['title'] = title
                if content is not None:
                    note['content'] = content
                note['modified'] = datetime.datetime.utcnow().isoformat() + 'Z'
                
                self._save()
                return note
        
        raise Exception('Note %d not found' % note_id)


    def delete_note(self, note_id):
        """Delete a note"""
        notes = self.data.get('notes', [])
        self.data['notes'] = [note for note in notes if note['id'] != note_id]
        self._save()


    # =========================================================================
    # Export
    # =========================================================================
    
    def export_text(self):
        """Export all service data as formatted text"""
        lines = []
        lines.append('=' * 80)
        lines.append('BUILDBOTICS CNC CONTROLLER - SERVICE DATA EXPORT')
        lines.append('=' * 80)
        lines.append('Export Date: ' + datetime.datetime.now().strftime('%B %d, %Y %H:%M:%S'))
        lines.append('')
        lines.append('MACHINE HOURS:')
        lines.append('  Power-On Hours:  %.1f hrs' % self.get_hours(self.HOUR_POWER))
        lines.append('  Motion Hours:    %.1f hrs' % self.get_hours(self.HOUR_MOTION))
        lines.append('')
        
        # Service Items
        lines.append('=' * 80)
        lines.append('SERVICE ITEMS')
        lines.append('=' * 80)
        lines.append('')
        
        for i, item in enumerate(self.get_items(), 1):
            hour_type = item.get('hour_type', self.HOUR_MOTION)
            current_hours = self.get_hours(hour_type)
            hour_label = {
                self.HOUR_POWER: 'Power',
                self.HOUR_MOTION: 'Motion'
            }.get(hour_type, 'Motion')
            
            lines.append('[%d] %s' % (i, item['label']))
            lines.append('    Tracks: %s Hours' % hour_label)
            lines.append('    Interval: %d hrs' % item['interval'])
            lines.append('    Last Completed: %.1f hrs' % item['last_completed'])
            lines.append('    Next Due: %.1f hrs' % item['next_due'])
            
            if current_hours >= item['next_due']:
                lines.append('    Status: ** DUE NOW ** (%.1f hrs overdue)' % (current_hours - item['next_due']))
            else:
                remaining = item['next_due'] - current_hours
                lines.append('    Status: %.1f hrs remaining' % remaining)
            lines.append('')
        
        # Service History
        lines.append('=' * 80)
        lines.append('SERVICE HISTORY')
        lines.append('=' * 80)
        lines.append('')
        
        for item in self.get_items():
            if item.get('history'):
                lines.append('--- %s ---' % item['label'])
                for entry in item.get('history', []):
                    date_str = entry['date'][:10] + ' ' + entry['date'][11:19]
                    lines.append('  %s | %.1f hrs | %s' % (date_str, entry['hours'], entry['type'].upper()))
                    if entry.get('notes'):
                        lines.append('    Notes: %s' % entry['notes'])
                lines.append('')
        
        # Notes
        lines.append('=' * 80)
        lines.append('MACHINE NOTES')
        lines.append('=' * 80)
        lines.append('')
        
        for note in self.get_notes():
            lines.append('--- %s ---' % note['title'])
            lines.append('Created: %s' % note['created'][:10])
            if note.get('modified') != note.get('created'):
                lines.append('Modified: %s' % note['modified'][:10])
            lines.append('')
            lines.append(note['content'])
            lines.append('')
        
        lines.append('=' * 80)
        lines.append('END OF EXPORT')
        lines.append('=' * 80)
        
        return '\n'.join(lines)
