import json
import tornado.web


class APIHandler(tornado.web.RequestHandler):
    def prepare(self):
        self.json = {}

        if self.request.body:
            try:
                self.json = tornado.escape.json_decode(self.request.body)
            except ValueError:
                self.send_error(400, message = 'Unable to parse JSON.')


    def set_default_headers(self):
        self.set_header('Content-Type', 'application/json')


    def write_error(self, status_code, **kwargs):
        e = {}
        e['message'] = str(kwargs['exc_info'][1])
        e['code'] = status_code

        self.write_json(e)


    def write_json(self, data):
        self.write(json.dumps(data))
