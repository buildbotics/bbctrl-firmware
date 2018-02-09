import os
import logging
import bbctrl


log = logging.getLogger('Msgs')


class Messages(logging.Handler):
    def __init__(self, ctrl):
        logging.Handler.__init__(self, logging.WARNING)

        self.ctrl = ctrl
        self.listeners = []

        logging.getLogger().addHandler(self)


    def add_listener(self, listener): self.listeners.append(listener)
    def remove_listener(self, listener): self.listeners.remove(listener)


    # From logging.Handler
    def emit(self, record):
        msg = dict(
            level = record.levelname.lower(),
            source = record.name,
            msg = record.getMessage())

        if hasattr(record, 'where'): msg['where'] = record.where
        else: msg['where'] = '%s:%d' % (record.filename, record.lineno)

        for listener in self.listeners:
            listener(msg)
