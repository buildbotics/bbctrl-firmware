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

#include "XMLAdapter.h"

#include <cbang/Exception.h>

#include <cbang/os/SystemUtilities.h>

#include "ExpatXMLAdapter.h"
#include "GLibXMLAdapter.h"

using namespace std;
using namespace cb;


void XMLAdapter::pushHandler(XMLHandler *handler) {
  handlers.push_back(handler);
}


void XMLAdapter::popHandler() {
  if (handlers.empty()) THROW("No handlers cannot pop");
  handlers.back()->popFile();
}


XMLHandler &XMLAdapter::getHandler() {
  if (handlers.empty()) THROW("No XML handler");
  return *handlers.back();
}


void XMLAdapter::read(const string &filename) {
  setFilename(filename);
  if (!handlers.empty()) getHandler().pushFile(filename);
  read(*SystemUtilities::open(filename, ios::in));
  if (!handlers.empty()) getHandler().popFile();
  setFilename(string());
}


XMLAdapter *XMLAdapter::create() {
#ifdef HAVE_EXPAT
  return new ExpatXMLAdapter();
#elif HAVE_GLIB
  return new GLibXMLAdapter();
#else
#error "No XML parser"
#endif
}
