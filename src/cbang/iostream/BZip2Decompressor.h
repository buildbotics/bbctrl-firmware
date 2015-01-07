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

#ifndef CBANG_BZIP2_DECOMPRESSOR_H
#define CBANG_BZIP2_DECOMPRESSOR_H

#include <iosfwd> // streamsize
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/close.hpp>
#include <boost/iostreams/detail/ios.hpp>
namespace io = boost::iostreams;

#include <cbang/SmartPointer.h>

#include <bzlib.h>
#include <string.h>


namespace cb {
  class BZip2Decompressor {
    static const unsigned BUFFER_SIZE = 4096;

    class BZip2DecompressorImpl {
      bz_stream bz;
      char buffer[BUFFER_SIZE];
      bool done;
      const char *remain_ptr;
      std::streamsize remain;

    public:
      BZip2DecompressorImpl() : done(false), remain_ptr(0), remain(0) {
        memset(&bz, 0, sizeof(bz_stream));
        BZ2_bzDecompressInit(&bz, 0, 0);
      }


      ~BZip2DecompressorImpl() {release();}


      template<typename Source>
      std::streamsize read(Source &src, char *s, std::streamsize n) {
        if (done) {
          // Just pass through any over read data
          if (remain) {
            std::streamsize size = remain < n ? remain : n;
            memcpy(s, remain_ptr, size);
            remain -= size;
            remain_ptr += size;

            return size;
          }

          return 0;
        }

        bz.next_out = s;
        bz.avail_out = n;

        while (bz.avail_out) {

          if (!bz.avail_in) {
            bz.avail_in = io::read(src, buffer, BUFFER_SIZE);
            bz.next_in = buffer;

            if (!bz.avail_in) break;
          }

          int ret;
          if ((ret = BZ2_bzDecompress(&bz)) != BZ_OK) {
            if (ret > 0) {
              remain = bz.avail_in;
              remain_ptr = bz.next_in;
            }

            release();
            break;
          }

        }

        return n - bz.avail_out;
      }


      template<typename Sink>
      std::streamsize write(Sink &dest, const char *s, std::streamsize n) {
        if (done) return 0;

        bz.next_in = (char *)s;
        bz.avail_in = n;

        while (bz.avail_in) {
          bz.next_out = buffer;
          bz.avail_out = BUFFER_SIZE;

          int result = BZ2_bzDecompress(&bz);
          io::write(dest, buffer, BUFFER_SIZE - bz.avail_out);

          if (result != BZ_OK) {
            release();

            return n - bz.avail_in;
          }
        }

        return n;
      }


      template<typename Sink> void close(Sink &dest, BOOST_IOS::openmode m) {
        if (done) return;

        if (m & BOOST_IOS::out) {
          bz.next_in = 0;
          bz.avail_in = 0;

          int result;
          do {
            bz.next_out = buffer;
            bz.avail_out = BUFFER_SIZE;

            result = BZ2_bzDecompress(&bz);

            io::write(dest, buffer, BUFFER_SIZE - bz.avail_out);

          } while (result == BZ_OK && bz.avail_out != BUFFER_SIZE);
        }

        release();
      }


      void release() {
        if (done) return;

        BZ2_bzDecompressEnd(&bz);
        done = true;
      }
    };

    SmartPointer<BZip2DecompressorImpl> impl;


  public:
    typedef char char_type;
    struct category :
      io::dual_use, io::filter_tag, io::multichar_tag, io::closable_tag {};


    BZip2Decompressor() : impl(new BZip2DecompressorImpl) {}


    template<typename Source>
    std::streamsize read(Source &src, char *s, std::streamsize n) {
      return impl->read(src, s, n);
    }


    template<typename Sink>
    std::streamsize write(Sink &dest, const char *s, std::streamsize n) {
      return impl->write(dest, s, n);
    }


    template<typename Sink> void close(Sink &dest, BOOST_IOS::openmode m) {
      impl->close(dest, m);
    }
  };
}

#endif // CBANG_BZIP2_DECOMPRESSOR_H

