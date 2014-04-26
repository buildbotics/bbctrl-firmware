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

#ifndef CBANG_HTTP_CHUNKED_STREAM_FILTER_H
#define CBANG_HTTP_CHUNKED_STREAM_FILTER_H

#include <cbang/StdTypes.h>
#include <cbang/String.h>
#include <cbang/log/Logger.h>

#include <iosfwd> // streamsize
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/operations.hpp>
namespace io = boost::iostreams;


namespace cb {
  namespace HTTP {
    class ChunkedStreamFilter {
      bool readEnable;
      char readLength[32];
      unsigned readLengthBytes;
      std::streamsize readBytes;

      bool writeEnable;
      char writeLength[32];
      unsigned writeLengthBytes;
      std::streamsize writeBytes;

    public:
      typedef char char_type;
      struct category :
        io::multichar_tag, io::dual_use, io::bidirectional, io::filter_tag {};

      ChunkedStreamFilter(bool readEnable = true, bool writeEnable = true) :
        readEnable(readEnable), readLengthBytes(0), readBytes(0),
        writeEnable(writeEnable), writeLengthBytes(0), writeBytes(0) {}

      bool getReadEnabled() const {return readEnable;}
      void setReadEnabled(bool enable) {this->readEnable = enable;}
      bool getWriteEnabled() const {return writeEnable;}
      void setWriteEnabled(bool enable) {this->writeEnable = enable;}


      template<typename Source>
      std::streamsize read(Source &src, char *s, std::streamsize n) {
        if (!readEnable) return io::read(src, s, n);

        while (!readBytes) {
          if (!readLengthBytes) {
            readLength[0] = '0';
            readLength[1] = 'x';
            readLengthBytes = 2;
          }

          int c = io::get(src);

          switch (c) {
          case EOF: return -1;
          case io::WOULD_BLOCK: return 0;
          case '\r': break;

          case '\n':
            readLength[readLengthBytes] = 0;
            readBytes = String::parseU64(readLength);
            readLengthBytes = 0;
            CBANG_LOG_DEBUG(6, "Chunk size: " << readBytes);
            if (!readBytes) {
              // Last chunk
              readEnable = false;
              return 0;
            }
            break;

          default:
            if (('0' <= c && c <= '9') || ('a' <= c && c <= 'f') ||
                ('A' <= c && c <= 'F'))
              readLength[readLengthBytes++] = (char)c;

            else throw BOOST_IOSTREAMS_FAILURE
                   ("Unexpected character in chunked stream");

            if (readLengthBytes == 32)
              throw BOOST_IOSTREAMS_FAILURE("Read chunk length too long");
            break;
          }
        }

        n = io::read(src, s, readBytes < n ? readBytes : n);
        readBytes -= n;
        return n;
      }


      template<typename Sink>
      std::streamsize write(Sink &dst, const char *s, std::streamsize n) {
        if (!writeEnable) return io::write(dst, s, n);

        if (!writeBytes || writeLengthBytes) {
          if (!writeLengthBytes) {
            std::string l = String(n);
            writeLengthBytes = l.length() < 31 ? l.length() : 31;
            memcpy(writeLength, l.c_str(), writeLengthBytes);
            writeLength[writeLengthBytes] = 0;
            writeBytes = n;
          }

          std::streamsize bytes = io::write(dst, writeLength, writeLengthBytes);
          writeLengthBytes -= bytes;
          if (writeLengthBytes) {
            // Shift length buffer
            for (unsigned i = 0; i <= writeLengthBytes; i++)
              writeLength[i] = writeLength[i + bytes];
            return 0;
          }
        }

        n = io::write(dst, s, writeBytes < n ? writeBytes : n);
        writeBytes -= n;
        return n;
      }
    };
  }
}

#endif // CBANG_HTTP_CHUNKED_STREAM_FILTER_H

