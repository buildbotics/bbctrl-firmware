import os
import bbctrl


class FileHandler(bbctrl.APIHandler):
    def prepare(self): pass


    def delete(self, path):
        path = 'upload' + path
        if os.path.exists(path): os.unlink(path)
        self.write_json('ok')


    def put(self, path):
        path = 'upload' + path
        if not os.path.exists(path): return

        with open(path, 'r') as f:
            for line in f:
                self.application.input_queue.put(line)


    def get(self, path):
        if path:
            with open('upload/' + path, 'r') as f:
                self.write_json(f.read())
            return

        files = []

        if os.path.exists('upload'):
            for path in os.listdir('upload'):
                if os.path.isfile('upload/' + path):
                    files.append(path)

        self.write_json(files)


    def post(self, path):
        gcode = self.request.files['gcode'][0]

        if not os.path.exists('upload'): os.mkdir('upload')

        with open('upload/' + gcode['filename'], 'wb') as f:
            f.write(gcode['body'])

        self.write_json('ok')
