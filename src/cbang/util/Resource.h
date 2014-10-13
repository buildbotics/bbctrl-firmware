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

#ifndef CBANG_RESOURCE_H
#define CBANG_RESOURCE_H

#include <cbang/Exception.h>

#include <string>
#include <ostream>


namespace cb {
  struct Resource {
    const char *name;

    Resource(const char *name) : name(name) {}
    virtual ~Resource() {}

    const char *getName() const {return name;}

    virtual bool isDirectory() const {return false;}

    virtual const char *getData() const
    {CBANG_THROWS(__func__ << "() not supported by resource");}
    virtual unsigned getLength() const
    {CBANG_THROWS(__func__ << "() not supported by resource");}

    virtual const Resource *find(const std::string &path) const
    {CBANG_THROWS(__func__ << "() not supported by resource");}

    const Resource &get(const std::string &path) const;
  };


  struct FileResource : public Resource {
    const char *data;
    const unsigned length;

    FileResource(const char *name, const char *data, unsigned length) :
      Resource(name), data(data), length(length) {}

    const char *getData() const {return data;}
    unsigned getLength() const {return length;}
  };


  struct DirectoryResource : public Resource {
    const Resource **children;

    DirectoryResource(const char *name, const Resource **children) :
      Resource(name), children(children) {}

    bool isDirectory() const {return true;}
    const Resource *find(const std::string &path) const;
  };


  static inline
  std::ostream &operator<<(std::ostream &stream, const Resource &r) {
    stream.write(r.getData(), r.getLength());
    return stream;
  }
}

#endif // CBANG_RESOURCE_H

