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

#include "TarFileReader.h"

#include <cbang/os/SystemUtilities.h>
#include <cbang/os/SysError.h>
#include <cbang/log/Logger.h>
#include <cbang/iostream/BZip2Decompressor.h>

#include <boost/ref.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>
namespace io = boost::iostreams;

using namespace cb;
using namespace std;


struct TarFileReader::private_t {
  io::filtering_istream filter;
};


TarFileReader::TarFileReader(const string &path, compression_t compression) :
  pri(new private_t), stream(SystemUtilities::iopen(path)),
  didReadHeader(false) {

  addCompression(compression == TARFILE_AUTO ? infer(path) : compression);
  pri->filter.push(*this->stream);
}


TarFileReader::TarFileReader(istream &stream, compression_t compression) :
  pri(new private_t), stream(SmartPointer<istream>::Null(&stream)),
  didReadHeader(false) {

  addCompression(compression);
  pri->filter.push(*this->stream);
}


TarFileReader::~TarFileReader() {
  delete pri;
}


bool TarFileReader::hasMore() {
  if (!didReadHeader) {
    SysError::clear();
    if (!readHeader(pri->filter))
      THROWS("Tar file read failed: " << SysError());
    didReadHeader = true;
  }

  return !isEOF();
}


bool TarFileReader::next() {
  if (didReadHeader) {
    skipFile(pri->filter);
    didReadHeader = false;
  }

  return hasMore();
}


std::string TarFileReader::extract(const string &_path) {
  if (_path.empty()) THROW("path cannot be empty");
  if (!hasMore()) THROW("No more tar files");

  string path = _path;
  if (SystemUtilities::isDirectory(path)) path += "/" + getFilename();

  LOG_DEBUG(5, "Extracting: " << path);

  return extract(*SystemUtilities::oopen(path));
}


string TarFileReader::extract(ostream &out) {
  if (!hasMore()) THROW("No more tar files");

  readFile(out, pri->filter);
  didReadHeader = false;

  return getFilename();
}


void TarFileReader::addCompression(compression_t compression) {
  switch (compression) {
  case TARFILE_NONE: break; // none
  case TARFILE_BZIP2: pri->filter.push(BZip2Decompressor()); break;
  case TARFILE_GZIP: pri->filter.push(io::zlib_decompressor()); break;
  default: THROWS("Invalid compression type " << compression);
  }
}
