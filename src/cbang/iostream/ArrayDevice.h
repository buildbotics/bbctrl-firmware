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

#ifndef CBANG_ARRAY_DEVICE_H
#define CBANG_ARRAY_DEVICE_H

#include <boost/iostreams/categories.hpp>   // bidirectional_device_tag
#include <boost/iostreams/stream.hpp>

#include <string.h>


namespace cb {
  template <typename T>
  class ArrayDevice {
    T *array;
    std::streamsize size;
    std::streampos ptr;

  public:
    typedef char char_type;
    typedef boost::iostreams::seekable_device_tag category;


    ArrayDevice(T *array, std::streamsize size) :
      array(array), size(size), ptr(0) {}


    std::streamsize read(char_type *s, std::streamsize n) {
      if (size <= ptr) return -1;
      if (size < (std::streamsize)ptr + n) n = size - (std::streamsize)ptr;
      memcpy(s, array + ptr, sizeof(T) * n);
      ptr += n;
      return n;
    }


    std::streamsize write(const char_type *s, std::streamsize n) {
      if (size <= ptr) return -1;
      if (size < (std::streamsize)ptr + n) n = size - (std::streamsize)ptr;
      memcpy((void *)(array + ptr), s, sizeof(T) * n);
      ptr += n;
      return n;
    }


    std::streampos seek(std::streampos off, std::ios_base::seekdir way)  {
      std::streampos next;

      switch (way) {
      case std::ios_base::beg: next = off; break;
      case std::ios_base::cur: next = ptr + off; break;
      case std::ios_base::end: next = size + off; break;
      default: return -1;
      }

      // Clamp
      if (next < 0) ptr = 0;
      else if (size < next) ptr = (std::streampos)size;
      else ptr = next;

      return ptr;
    }
  };


  template <typename T>
  class ArrayStream : public boost::iostreams::stream<ArrayDevice<T> > {
  public:
    ArrayStream(T *array, std::streamsize size) :
      boost::iostreams::stream<ArrayDevice<T> >(array, size) {}
  };
}

#endif // CBANG_ARRAY_DEVICE_H

