/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2014, Cauldron Development LLC
                 Copyright (c) 2003-2014, Stanford University
                             All rights reserved.

        The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
        as published by the Free Software Foundation, either version 2.1 of
        the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
        Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
        License along with the C! library.  If not, see
        <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
        You may request written permission by emailing the authors.

                For information regarding this software email:
                               Joseph Coffland
                        joseph@cauldrondevelopment.com

\******************************************************************************/

#include "Request.h"
#include "Buffer.h"
#include "BufferDevice.h"
#include "Headers.h"
#include "Connection.h"

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>
#include <cbang/http/Cookie.h>
#include <cbang/json/JSON.h>

#include <event2/http.h>
#include <event2/http_struct.h>

#include <netinet/in.h>

using namespace cb::Event;
using namespace cb;
using namespace std;


Request::Request(evhttp_request *req, bool deallocate) :
  req(req), deallocate(deallocate), incomming(false), secure(false) {
  if (!req) THROW("Event request cannot be null");

  // Parse URI
  const char *uri = evhttp_request_get_uri(req);
  if (uri) this->uri = uri;

  // Parse client IP
  evhttp_connection *_con = evhttp_request_get_connection(req);
  if (_con) {
    Connection con(_con, false);
    clientIP = con.getPeer();
  }
}


Request::Request(evhttp_request *req, const URI &uri, bool deallocate) :
  req(req), deallocate(deallocate), uri(uri),
  clientIP(uri.getHost(), uri.getPort()), incomming(false), secure(false) {
  if (!req) THROW("Event request cannot be null");
}


Request::~Request() {
  if (req && deallocate) evhttp_request_free(req);
}


string Request::getHost() const {
  const char *host = evhttp_request_get_host(req);
  return host ? host : "";
}


RequestMethod Request::getMethod() const {
  switch (evhttp_request_get_command(req)) {
  case EVHTTP_REQ_GET:     return HTTP_GET;
  case EVHTTP_REQ_POST:    return HTTP_POST;
  case EVHTTP_REQ_HEAD:    return HTTP_HEAD;
  case EVHTTP_REQ_PUT:     return HTTP_PUT;
  case EVHTTP_REQ_DELETE:  return HTTP_DELETE;
  case EVHTTP_REQ_OPTIONS: return HTTP_OPTIONS;
  case EVHTTP_REQ_TRACE:   return HTTP_TRACE;
  case EVHTTP_REQ_CONNECT: return HTTP_CONNECT;
  case EVHTTP_REQ_PATCH:   return HTTP_PATCH;
  default:                 return RequestMethod::HTTP_UNKNOWN;
  }
}


unsigned Request::getResponseCode() const {
  return evhttp_request_get_response_code(req);
}


string Request::getResponseMessage() const {
  const char *s = evhttp_request_get_response_code_line(req);
  return s ? s : "";
}


string Request::getResponseLine() const {
  return SSTR("HTTP/" << (int)req->major << '.' << (int)req->minor << ' '
              << getResponseCode() << ' ' << getResponseMessage());
}


Headers Request::getInputHeaders() const {
  return evhttp_request_get_input_headers(req);
}


Headers Request::getOutputHeaders() const {
  return evhttp_request_get_output_headers(req);
}


bool Request::inHas(const string &name) const {
  return getInputHeaders().has(name);
}


string Request::inFind(const string &name) const {
  return getInputHeaders().find(name);
}


string Request::inGet(const string &name) const {
  return getInputHeaders().get(name);
}


void Request::inAdd(const string &name, const string &value) {
  getInputHeaders().add(name, value);
}


void Request::inSet(const string &name, const string &value) {
  getInputHeaders().set(name, value);
}


void Request::inRemove(const string &name) {
  getInputHeaders().remove(name);
}


bool Request::outHas(const string &name) const {
  return getOutputHeaders().has(name);
}


string Request::outFind(const string &name) const {
  return getOutputHeaders().find(name);
}


string Request::outGet(const string &name) const {
  return getOutputHeaders().get(name);
}


void Request::outAdd(const string &name, const string &value) {
  getOutputHeaders().add(name, value);
}


void Request::outSet(const string &name, const string &value) {
  getOutputHeaders().set(name, value);
}


void Request::outRemove(const string &name) {
  getOutputHeaders().remove(name);
}


string Request::getContentType() const {
  return getOutputHeaders().getContentType();
}


void Request::setContentType(const string &contentType) {
  getOutputHeaders().setContentType(contentType);
}


void Request::guessContentType() {
  getOutputHeaders().guessContentType(uri.getExtension());
}


bool Request::hasCookie(const string &name) const {
  if (!inHas("Cookie")) return false;

  vector<string> cookies;
  String::tokenize(inGet("Cookie"), cookies, "; \t\n\r");

  for (unsigned i = 0; i < cookies.size(); i++)
    if (name == cookies[i].substr(0, cookies[i].find('='))) return true;

  return false;
}


string Request::findCookie(const string &name) const {
  if (inHas("Cookie")) {
    // Return only the first matching cookie
    vector<string> cookies;
    String::tokenize(inGet("Cookie"), cookies, "; \t\n\r");

    for (unsigned i = 0; i < cookies.size(); i++) {
      size_t pos = cookies[i].find('=');

      if (name == cookies[i].substr(0, pos))
        return pos == string::npos ? string() : cookies[i].substr(pos + 1);
    }
  }

  return "";
}


string Request::getCookie(const string &name) const {
  if (!hasCookie(name)) THROWS("Cookie '" << name << "' not set");
  return findCookie(name);
}


void Request::setCookie(const string &name, const string &value,
                        const string &domain, const string &path,
                        uint64_t expires, uint64_t maxAge, bool httpOnly,
                        bool secure) {
  outAdd("Set-Cookie", HTTP::Cookie(name, value, domain, path, expires, maxAge,
                                    httpOnly, secure).toString());
}


void Request::cancel() {
  evhttp_cancel_request(req);
}


Event::Buffer Request::getInputBuffer() const {
  return Buffer(evhttp_request_get_input_buffer(req), false);
}


Event::Buffer Request::getOutputBuffer() const {
  return Buffer(evhttp_request_get_output_buffer(req), false);
}


SmartPointer<JSON::Value> Request::getInputJSON() const {
  Buffer buf = getInputBuffer();
  if (!buf.getLength()) return 0;
  Event::BufferStream<> stream(buf);
  return JSON::Reader(stream).parse();
}


SmartPointer<JSON::Value> Request::getOutputJSON() const {
  Buffer buf = getOutputBuffer();
  if (!buf.getLength()) return 0;
  Event::BufferStream<> stream(buf);
  return JSON::Reader(stream).parse();
}


void Request::sendError(int code) {
  string msg = HTTPStatus((HTTPStatus::enum_t)code).getDescription();
  msg = SSTR(code << " " << String::replace(msg, "_", " "));

  evhttp_send_error(req, code, msg.c_str());
}


void Request::sendReply(const Buffer &buf) {
  sendReply(HTTP_OK, buf);
}


void Request::sendReply(const char *data, unsigned length) {
  sendReply(HTTP_OK, data, length);
}


void Request::sendReply(int code, const Buffer &buf) {
  evhttp_send_reply(req, code,
                    HTTPStatus((HTTPStatus::enum_t)code).getDescription(),
                    buf.getBuffer());
}


void Request::sendReply(int code, const char *data, unsigned length) {
  sendReply(code, Buffer(data, length));
}


void Request::sendReplyStart(int code) {
  evhttp_send_reply_start
    (req, code, HTTPStatus((HTTPStatus::enum_t)code).getDescription());
}


void Request::sendReplyChunk(const Buffer &buf) {
  evhttp_send_reply_chunk(req, buf.getBuffer());
}


void Request::sendReplyChunk(const char *data, unsigned length) {
  sendReplyChunk(Buffer(data, length));
}


void Request::sendReplyEnd() {
  evhttp_send_reply_end(req);
}


void Request::redirect(const URI &uri, int code) {
  outSet("Location", uri);
  outSet("Content-Length", "0");
  sendReply(code, "", 0);
}


const char *Request::getErrorStr(int error) {
  switch (error) {
  case EVREQ_HTTP_TIMEOUT:        return "Timeout";
  case EVREQ_HTTP_EOF:            return "End of file";
  case EVREQ_HTTP_INVALID_HEADER: return "Invalid header";
  case EVREQ_HTTP_BUFFER_ERROR:   return "Buffer error";
  case EVREQ_HTTP_REQUEST_CANCEL: return "Requeset canceled";
  case EVREQ_HTTP_DATA_TOO_LONG:  return "Data too long";
  default:                        return "Unknown";
  }
}
