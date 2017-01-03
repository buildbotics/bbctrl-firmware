import json
import tornado.web
import tornado.httpclient


class APIHandler(tornado.web.RequestHandler):
    def __init__(self, app, request, **kwargs):
        super(APIHandler, self).__init__(app, request, **kwargs)
        self.ctrl = app.ctrl


    def delete(self, *args, **kwargs):
        self.delete_ok(*args, **kwargs)
        self.write_json('ok')


    def delete_ok(self): raise tornado.httpclient.HTTPError(405)


    def put(self, *args, **kwargs):
        self.put_ok(*args, **kwargs)
        self.write_json('ok')


    def put_ok(self): raise tornado.httpclient.HTTPError(405)


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


    def write_json(self, data, pretty = False):
        if pretty: data = json.dumps(data, indent = 2, separators = (',', ': '))
        else: data = json.dumps(data)
        self.write(data)
