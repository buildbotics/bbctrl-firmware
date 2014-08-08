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

#include "Connection.h"

#include "Server.h"
#include "Context.h"
#include "Handler.h"

#include <cbang/Zap.h>
#include <cbang/String.h>

#include <cbang/buffer/BufferDevice.h>

#include <cbang/socket/SocketDevice.h>

#include <cbang/packet/Packet.h>

#include <cbang/time/Time.h>
#include <cbang/util/SmartLock.h>
#include <cbang/log/Logger.h>

#ifdef HAVE_VALGRIND_DRD_H
#include <valgrind/drd.h>
#endif

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#include <algorithm>


using namespace std;
namespace io = boost::iostreams;
using namespace cb;
using namespace cb::HTTP;

Connection::Connection(Server &server, SmartPointer<Socket> socket,
                       const IPAddress &clientIP) :
  SocketConnection(socket, clientIP), ConnectionStream(*this),
  server(server), readBuf(4096), utilBuf(4096), contentLength(0),
  lastUpdate(startTime), priority(0), state(READING_HEADER), restartTime(0),
  handler(0), ctx(0), failed(false) {

  memset(readBuf.begin(), 0, readBuf.getCapacity());
  memset(utilBuf.begin(), 0, utilBuf.getCapacity());
  memset(dataBuf.begin(), 0, dataBuf.getCapacity());

#ifdef HAVE_VALGRIND_DRD_H
  DRD_IGNORE_VAR(state);
#endif // HAVE_VALGRIND_DRD_H
}


Connection::~Connection() {
  zap(ctx);
}


Connection::state_t Connection::getState() const {
  SmartLock lock(this);
  return state;
}


void Connection::setState(state_t state) {
  SmartLock lock(this);
  if (this->state != state) {
    this->state = state;
    LOG_DEBUG(5, *this << " state=" << state);

    if (state == READING_HEADER) response.reset();
  }
}


void Connection::delayUntil(double restartTime) {
  this->restartTime = restartTime;
}


bool Connection::isPersistent() const {
  return request.isPersistent() && response.isPersistent();
}


void Connection::error(StatusCode reason) {
  if (reason) response.setStatus(reason);
  THROWS("Request from " << *this << " failed"
         << (reason ? string(": ") + reason.toString() : ""));
}


void Connection::fail(StatusCode reason) {
  failed = true;
  dataBuf.clear();

  if (reason) response.setStatus(reason);
  response.setPersistent(false);
}


bool Connection::operator<(const Connection &c) const {
  if (c.priority < priority) return true;
  if (priority < c.priority) return false;
  return getStartTime() < c.getStartTime();
}


streamsize Connection::read(char *s, streamsize n) {
  return dataBuf.read(s, n);
}


streamsize Connection::write(const char *s, streamsize n) {
  streamsize total = 0;

  while (n) {
    if (responseBuf.empty() || responseBuf.back()->isFull())
      responseBuf.push_back(new MemoryBuffer(4096 < n ? n : 4096));

    unsigned count = responseBuf.back()->write(s, n);
    total += count;
    n -= count;
    s += count;
  }

  return total;
}


void Connection::readRequest(istream &stream) {
  // Read header
  utilBuf.clear();
  stream.read(utilBuf.begin(), utilBuf.getSpace());
  utilBuf.incFill(stream.gcount());

  // Parse header
  unsigned size = tryParsingHeader(utilBuf);
  if (!size) THROWS("Failed to parse header");
  utilBuf.incPosition(size);

  // Read data
  contentLength = request.getContentLength();
  dataBuf.clear();
  dataBuf.increase(contentLength); // Allocate space
  dataBuf.write(utilBuf.begin(), utilBuf.getFill());
  stream.read(dataBuf.end(), dataBuf.getSpace());
  dataBuf.incFill(stream.gcount());
}


void Connection::writeResponse(ostream &stream) {
  // Write header
  response.finalize(getResponseSize());
  response.write(stream);

  // Write data
  responseBuf_t::const_iterator it;
  for (it = responseBuf.begin(); it != responseBuf.end(); it++) {
    // Ignore non-MemoryBuffers
    if (!it->isInstance<MemoryBuffer>()) continue;

    MemoryBuffer &buf = *it->cast<MemoryBuffer>();
    stream.write(buf.begin(), buf.getFill());
  }
}


void Connection::writeRequest(ostream &stream) {
  // Write header
  request.write(stream);

  // Write data
  stream.write(dataBuf.begin(), dataBuf.getFill());
}


void Connection::clearResponseBuffer() {
  flush();
  responseBuf.clear();
}


void Connection::addResponseBuffer(const SmartPointer<Buffer> &buf) {
  responseBuf.push_back(buf);
}


void Connection::addResponseBuffer(char *buffer, unsigned length) {
  addResponseBuffer(new MemoryBuffer(length, buffer));
}


void Connection::addResponseBuffer(Packet &packet) {
  addResponseBuffer(packet.getData(), packet.getSize());
}


unsigned Connection::getResponseSize() const {
  unsigned total = 0;
  responseBuf_t::const_iterator it;

  for (it = responseBuf.begin(); it != responseBuf.end(); it++)
    total += (*it)->getFill();

  return total;
}


bool Connection::readHeader() {
  uint64_t count = readSocket(readBuf.end(), readBuf.getSpace());

  if (count) {
    readBuf.incFill(count);
    unsigned size = tryParsingHeader(readBuf);

    if (size) {
      dataBuf.clear();
      contentLength = 0;

      LOG_DEBUG(5, *this << " HTTP Request (\n"
                << string(readBuf.begin(), size) << ")");

      readBuf.incPosition(size);
      return true;
    }
  }

  // Check if header is too large
  if (readBuf.isFull()) {
    LOG_ERROR("Header too large: "
              << String::hexdump(readBuf.begin(), readBuf.getFill()));
    error(StatusCode::HTTP_BAD_REQUEST);
  }

  return false;
}


bool Connection::read() {
  if (!contentLength) {
    switch (request.getMethod()) {
    case RequestMethod::HTTP_OPTIONS:
    case RequestMethod::HTTP_GET:
      LOG_INFO(1, *this << ' ' << request.getMethod() << ' '
               << request.getURI());
      readBuf.shiftToFront();
      return true;

    case RequestMethod::HTTP_POST: {
      // Check Content-Length
      if (!request.has("Content-Length"))
        error(StatusCode::HTTP_LENGTH_REQUIRED);

      contentLength = request.getContentLength();

      if (contentLength > server.getMaxRequestLength())
        error(StatusCode::HTTP_REQUEST_ENTITY_TOO_LARGE);

      // Allocate buffer
      dataBuf.increase(contentLength);

      // Copy any extra data into buffer
      if (contentLength < readBuf.getFill()) {
        dataBuf.write(readBuf.begin(), contentLength);
        readBuf.incPosition(contentLength);
        readBuf.shiftToFront();

      } else if (!readBuf.isEmpty()) {
        dataBuf.write(readBuf.begin(), readBuf.getFill());
        readBuf.clear();
      }

      if (!contentLength) return true;  // Empty POST
      break;
    }

    default:
      // Unsupported request method
      error(StatusCode::HTTP_METHOD_NOT_ALLOWED);
    }
  }

  if (dataBuf.getSpace()) {
    LOG_DEBUG(5, *this << " Data buf space " << dataBuf.getSpace());
    dataBuf.incFill(readSocket(dataBuf.end(), dataBuf.getSpace()));
    LOG_DEBUG(5, *this << " Data buf space " << dataBuf.getSpace());
  }

  if (dataBuf.isFull()) {
    LOG_INFO(1, *this << " POST " << contentLength << " bytes "
             << request.getURI());
    return true;
  }

  return false;
}


void Connection::process() {
  if (handler && ctx) {
    try {
      Logger::instance().setThreadID(getID());
      handler->buildResponse(ctx);
      utilBuf.clear();
      return;

    } CBANG_CATCH(LOG_ERROR_LEVEL, ": " << *this);

    // Error occured
    fail(StatusCode::HTTP_INTERNAL_SERVER_ERROR);
  }
}


bool Connection::writeHeader() {
  if (utilBuf.isClear()) {
    BufferStream stream(utilBuf);
    response.finalize(getResponseSize());
    response.write(stream);
    stream.flush();

    LOG_DEBUG(5, *this << " HTTP Response (\n" << response << ")");
  }

  utilBuf.incPosition(writeSocket(utilBuf.begin(), utilBuf.getFill()));

  return utilBuf.isEmpty();
}


bool Connection::write() {
  while (!responseBuf.empty()) {
    MemoryBuffer *buf;

    if (utilBuf.isEmpty()) {
      // Find the first non-empty buffer
      SmartPointer<Buffer> &bufPtr = responseBuf.front();
      if (bufPtr->isEmpty()) {
        responseBuf.pop_front();
        continue;
      }

      // If this is not a MemoryBuffer use utilBuf to read it
      if (!bufPtr.isInstance<MemoryBuffer>()) {
        buf = &utilBuf;
        buf->clear();
        buf->increase(4096);
        buf->incFill(bufPtr->read(buf->begin(), buf->getSpace()));

      } else buf = bufPtr.castPtr<MemoryBuffer>();

    } else buf = &utilBuf;

    // Write the data to the socket
    uint64_t count = writeSocket(buf->begin(), buf->getFill());
    if (!count) break;
    buf->incPosition(count);
  }

  return responseBuf.empty();
}


void Connection::reportResults() {
  if (handler) handler->reportResults(ctx, !failed);
}


ostream &Connection::print(ostream &stream) const {
  return stream << getID() << ":" << getClientIP().getHost();
}


unsigned Connection::tryParsingHeader(MemoryBuffer &buffer) {
  // Look for end of HTTP header
  char *ptr = buffer.begin();
  unsigned match = 0;
  while (ptr < buffer.end() && match != 4) {
    switch (*ptr++) {
    case '\r': match = match == 2 ? 3 : 1; break;
    case '\n': match = (match == 1 || match == 3) ? match + 1 : 0; break;
    default: match = 0;
    }
  }

  if (match == 4) {
    // Found end of header, parse it
    unsigned size = ptr - buffer.begin();
    io::stream<io::array> stream(buffer.begin(), size);
    request.reset();
    request.read(stream);

    return size;
  }

  return 0;
}


uint64_t Connection::readSocket(char *buffer, unsigned length) {
  if (!length) return 0;
  uint64_t count = socket->read(buffer, length, Socket::NONBLOCKING);
  if (count) lastUpdate = Time::now();
  return count;
}


uint64_t Connection::writeSocket(const char *buffer, unsigned length) {
  if (!length) return 0;
  uint64_t count = socket->write(buffer, length, Socket::NONBLOCKING);
  if (count) {
    lastUpdate = Time::now();
    LOG_DEBUG(5, *this << " wrote " << count);
  }

  return count;
}
