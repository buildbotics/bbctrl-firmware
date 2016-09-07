/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2015, Cauldron Development LLC
                 Copyright (c) 2003-2015, Stanford University
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

#ifndef CBANG_HTTP_TRANSACTION_H
#define CBANG_HTTP_TRANSACTION_H

#include "Request.h"
#include "Response.h"
#include "ChunkedStreamFilter.h"

#include <cbang/socket/SocketDevice.h>
#include <cbang/iostream/Transfer.h>

#include <cbang/net/URI.h>

#include <boost/iostreams/filtering_stream.hpp>
namespace io = boost::iostreams;

#include <iostream>


namespace cb {
  class  SSLContext;

  namespace HTTP {
    class Transaction :
      public Socket, public io::filtering_stream<io::bidirectional> {
    protected:
      char buffer;
      SocketStream stream;
      ChunkedStreamFilter chunkedFilter;
      Response response;
      IPAddress address;
      double timeout;

    public:
      Transaction(double timeout = 30);
      Transaction(const SmartPointer<SSLContext> &sslCtx,
                  double timeout = 30);
      ~Transaction();

      const IPAddress getAddress() const {return address;}

      void send(const Request &request);
      void receive(Response &response);

      void get(const URI &uri, float version = 1.0);
      void post(const URI &uri, const char *data, unsigned length,
                const std::string &contentType = "application/octet-stream",
                float version = 1.0);
      Response &exchange(Request &request);

      std::streamsize receiveHeader();

      typedef io::filtering_stream<io::bidirectional> Stream_T;
      using Stream_T::read;
      using Stream_T::write;
      using Stream_T::peek;
      using Stream_T::get;

      void upload(std::istream &in, std::streamsize length,
                  SmartPointer<TransferCallback> callback = 0);
      void download(std::ostream &out, std::streamsize length,
                    SmartPointer<TransferCallback> callback = 0);

      std::string getString(const URI &uri,
                            SmartPointer<TransferCallback> callback = 0);

      // From Socket
      void open();
      void connect(const IPAddress &ip);
      void close();
      using Socket::get;

      const Response &getResponse() const {return response;}

    protected:
      void init();

      /// Add the connect Address and Port to URI if not otherwise set
      URI resolve(const URI &uri);
    };
  }
}

#endif // CBANG_HTTP_TRANSACTION_H
