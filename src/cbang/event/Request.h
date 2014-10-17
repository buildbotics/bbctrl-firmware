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
#include <typeinfo>

struct evhttp_request;


namespace cb {
  class URI;
  class IPAddress;

  namespace JSON {class Value;}

  namespace Event {
    class Buffer;
    class Headers;

    class Request : public RequestMethod, public HTTPStatus {
    protected:
      evhttp_request *req;
      bool deallocate;

      URI uri;
      IPAddress clientIP;
      bool incomming;
      bool secure;

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

      void setIncomming(bool incomming) {this->incomming = incomming;}
      bool isIncomming() const {return incomming;}
      bool isSecure() const {return secure;}
      void setSecure(bool secure) {this->secure = secure;}

      std::string getHost() const;
      const URI &getURI() const {return uri;}
      const IPAddress &getClientIP() const {return clientIP;}
      RequestMethod getMethod() const;
      unsigned getResponseCode() const;
      std::string getResponseMessage() const;
      std::string getResponseLine() const;

      Headers getInputHeaders() const;
      Headers getOutputHeaders() const;

      bool inHas(const std::string &name) const;
      std::string inFind(const std::string &name) const;
      std::string inGet(const std::string &name) const;
      void inAdd(const std::string &name, const std::string &value);
      void inSet(const std::string &name, const std::string &value);
      void inRemove(const std::string &name);

      bool outHas(const std::string &name) const;
      std::string outFind(const std::string &name) const;
      std::string outGet(const std::string &name) const;
      void outAdd(const std::string &name, const std::string &value);
      void outSet(const std::string &name, const std::string &value);
      void outRemove(const std::string &name);

      std::string getContentType() const;
      void setContentType(const std::string &contentType);
      void guessContentType();

      bool hasCookie(const std::string &name) const;
      std::string findCookie(const std::string &name) const;
      std::string getCookie(const std::string &name) const;
      void setCookie(const std::string &name, const std::string &value,
                     const std::string &domain = std::string(),
                     const std::string &path = std::string(),
                     uint64_t expires = 0, uint64_t maxAge = 0,
                     bool httpOnly = false, bool secure = false);

      void cancel();

      std::string getInput() const;
      std::string getOutput() const;

      Buffer getInputBuffer() const;
      Buffer getOutputBuffer() const;

      SmartPointer<JSON::Value> getInputJSON() const;
      SmartPointer<JSON::Value> getOutputJSON() const;

      void sendError(int code);

      void sendReply(const Buffer &buf);
      void sendReply(const char *data, unsigned length);
      void sendReply(int code, const Buffer &buf);
      void sendReply(int code, const char *data, unsigned length);
      void sendReplyStart(int code);
      void sendReplyChunk(const Buffer &buf);
      void sendReplyChunk(const char *data, unsigned length);
      void sendReplyEnd();

      void redirect(const URI &uri,
                    int code = HTTPStatus::HTTP_TEMPORARY_REDIRECT);

      static const char *getErrorStr(int error);
    };
  }
}

#endif // CB_EVENT_REQUEST_H

