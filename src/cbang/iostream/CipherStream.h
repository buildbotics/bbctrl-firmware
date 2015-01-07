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

#ifndef CB_CIPHER_STREAM_H
#define CB_CIPHER_STREAM_H

#include <cbang/SmartPointer.h>

#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/close.hpp>
#include <boost/iostreams/detail/ios.hpp>
namespace io = boost::iostreams;

#include <iostream>

namespace cb {
  class CipherStream {
    SmartPointer<Cipher> cipher;
    bool done;

  public:
    typedef char char_type;
    struct category :
      io::dual_use, io::filter_tag, io::multichar_tag, io::closable_tag {};

    CipherStream(const SmartPointer<Cipher> &cipher) :
      cipher(cipher), done(false) {}

    template<typename Source>
    std::streamsize read(Source &src, char *s, std::streamsize n) {
      if (n < (std::streamsize)cipher->getBlockSize()) return 0;

      unsigned readSize = n - cipher->getBlockSize() + 1;
      SmartPointer<char>::Array buffer = new char[readSize];
      std::streamsize bytes = io::read(src, buffer.get(), readSize);

      if (0 < bytes) return cipher->update(s, n, buffer.get(), bytes);
      else if (!done) {
        done = true;
        return cipher->final(s, n);
      }
      return bytes;
    }


    template<typename Sink>
    std::streamsize write(Sink &dst, const char *s, std::streamsize n) {
      SmartPointer<char>::Array buffer = new char[n + 32];
      unsigned bytes = cipher->update(buffer.get(), n + 32, s, n);
      io::write(dst, buffer.get(), bytes);
      return n;
    }


    template<typename Sink> void close(Sink &dst, BOOST_IOS::openmode m) {
      if (m & BOOST_IOS::out) {
        char buffer[4096];
        unsigned bytes = cipher->final(buffer, 4096);
        io::write(dst, buffer, bytes);
      }
    }
  };
}

#endif // CB_CIPHER_STREAM_H

