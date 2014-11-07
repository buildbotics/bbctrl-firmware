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

#ifndef CB_EVENT_REQUEST_H
#define CB_EVENT_REQUEST_H

#include "RequestMethod.h"
#include "HTTPStatus.h"
#include "HTTPHandler.h"

#include <cbang/SmartPointer.h>
#include <cbang/net/IPAddress.h>
#include <cbang/net/URI.h>

#include <string>
#include <iostream>
#include <typeinfo>

struct evhttp_request;


namespace cb {
  class URI;
  class IPAddress;

  namespace JSON {
    class Value;
    class Writer;
  }

  namespace Event {
    class Buffer;
    class Headers;

    class Request : public RequestMethod, public HTTPStatus {
    protected:
      evhttp_request *req;
      bool deallocate;

      URI uri;
      IPAddress clientIP;
      bool incoming;
      bool secure;
      bool finalized;

      typedef std::vector<std::string> path_args_t;
      path_args_t pathArgs;

    public:
      Request(evhttp_request *req, bool deallocate = false);
      Request(evhttp_request *req, const URI &uri, bool deallocate = false);
      virtual ~Request();

      template <class T>
      T &cast() {
        T *ptr = dynamic_cast<T *>(this);
        if (!ptr) THROWS("Cannot cast Request to " << typeid(T).name());
        return *ptr;
      }

      evhttp_request *getRequest() const {return req;}
      evhttp_request *adopt() {deallocate = false; return req;}

      void setIncoming(bool incoming) {this->incoming = incoming;}
      bool isIncoming() const {return incoming;}
      bool isSecure() const {return secure;}
      void setSecure(bool secure) {this->secure = secure;}

      virtual void appendPathArg(const std::string &arg)
      {pathArgs.push_back(arg);}
      virtual const path_args_t &getPathArgs() const {return pathArgs;}
      virtual const std::string &getPathArg(unsigned i) const
      {return pathArgs.at(i);}

      virtual std::string getHost() const;
      virtual const URI &getURI() const {return uri;}
      virtual const IPAddress &getClientIP() const {return clientIP;}
      virtual RequestMethod getMethod() const;
      virtual unsigned getResponseCode() const;
      virtual std::string getResponseMessage() const;
      virtual std::string getResponseLine() const;

      virtual Headers getInputHeaders() const;
      virtual Headers getOutputHeaders() const;

      virtual bool inHas(const std::string &name) const;
      virtual std::string inFind(const std::string &name) const;
      virtual std::string inGet(const std::string &name) const;
      virtual void inAdd(const std::string &name, const std::string &value);
      virtual void inSet(const std::string &name, const std::string &value);
      virtual void inRemove(const std::string &name);

      virtual bool outHas(const std::string &name) const;
      virtual std::string outFind(const std::string &name) const;
      virtual std::string outGet(const std::string &name) const;
      virtual void outAdd(const std::string &name, const std::string &value);
      virtual void outSet(const std::string &name, const std::string &value);
      virtual void outRemove(const std::string &name);

      virtual bool hasContentType() const;
      virtual std::string getContentType() const;
      virtual void setContentType(const std::string &contentType);
      virtual void guessContentType();

      virtual bool hasCookie(const std::string &name) const;
      virtual std::string findCookie(const std::string &name) const;
      virtual std::string getCookie(const std::string &name) const;
      virtual void setCookie(const std::string &name, const std::string &value,
                             const std::string &domain = std::string(),
                             const std::string &path = std::string(),
                             uint64_t expires = 0, uint64_t maxAge = 0,
                             bool httpOnly = false, bool secure = false);

      virtual std::string getInput() const;
      virtual std::string getOutput() const;

      virtual Buffer getInputBuffer() const;
      virtual Buffer getOutputBuffer() const;

      virtual SmartPointer<JSON::Value> getInputJSON() const;
      virtual SmartPointer<JSON::Writer>
      getJSONWriter(unsigned indent = 0, bool compact = false) const;

      virtual SmartPointer<std::istream> getInputStream() const;
      virtual SmartPointer<std::ostream> getOutputStream() const;

      virtual void sendError(int code);

      virtual void send(const Buffer &buf);
      virtual void send(const char *data, unsigned length);
      virtual void send(const char *s);
      virtual void send(const std::string &s);
      virtual void sendFile(const std::string &path);

      virtual void reply(int code = HTTP_OK);
      virtual void reply(const Buffer &buf);
      virtual void reply(const char *data, unsigned length);
      virtual void reply(int code, const Buffer &buf);
      virtual void reply(int code, const char *data, unsigned length);

      virtual void startChunked(int code);
      virtual void sendChunk(const Buffer &buf);
      virtual void sendChunk(const char *data, unsigned length);
      virtual void endChunked();

      virtual void redirect(const URI &uri,
                            int code = HTTPStatus::HTTP_TEMPORARY_REDIRECT);
      virtual void cancel();

      static const char *getErrorStr(int error);

    protected:
      virtual void finalize();
    };
  }
}

#endif // CB_EVENT_REQUEST_H

