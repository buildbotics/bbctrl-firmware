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

#include "InputSource.h"

#include "File.h"

#include <cbang/os/SystemUtilities.h>
#include <cbang/iostream/ArrayDevice.h>
#include <cbang/util/Resource.h>
#include <cbang/buffer/BufferDevice.h>

#include <sstream>

using namespace std;
using namespace cb;


InputSource::InputSource(Buffer &buffer, const string &name) :
  Named(name), streamPtr(new BufferStream(buffer)), stream(*streamPtr),
  length(buffer.getFill()) {}


InputSource::InputSource(const char *array, streamsize length,
                         const string &name) :
  Named(name), streamPtr(new ArrayStream<const char>(array, length)),
  stream(*streamPtr), length(length) {}


InputSource::InputSource(const string &filename) :
  Named(filename), streamPtr(SystemUtilities::iopen(filename)),
  stream(*streamPtr), length(-1) {}


InputSource::InputSource(istream &stream, const string &name,
                         streamsize length) :
  Named(name), stream(stream), length(length) {}


InputSource::InputSource(const Resource &resource) :
  Named(resource.getName()),
  streamPtr(new ArrayStream<const char>(resource.getData(),
                                        resource.getLength())),
  stream(*streamPtr) {}


streamsize InputSource::getLength() const {
  if (length == -1) {
    boost::iostreams::stream<FileDevice> *boostStream =
      dynamic_cast<boost::iostreams::stream<FileDevice> *>(&stream);

    if (boostStream) return length = (*boostStream)->size();
  }

  return length;
}


string InputSource::toString() const {
  ostringstream str;
  SystemUtilities::cp(getStream(), str);
  return str.str();
}
