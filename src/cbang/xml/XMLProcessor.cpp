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

#include "XMLProcessor.h"

#include "XMLHandlerContext.h"

#include <cbang/Exception.h>

#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;


XMLProcessor::XMLProcessor() {
  // NOTE: Cannot call virtual function pushContext() from constructor
  contextStack.push_back(new XMLHandlerContext);
}


XMLProcessor::~XMLProcessor() {
  while (!contextStack.empty()) {
    delete contextStack.back();
    contextStack.pop_back();
  }
}


void XMLProcessor::addFactory(const string &name, XMLHandlerFactory *factory) {
  if (!factory) THROWS("Cannot add NULL factory");
  contextStack.back()->add(name, factory);
}


XMLHandlerFactory *XMLProcessor::getFactory(const string &name) {
  return contextStack.back()->get(name);
}


void XMLProcessor::pushContext() {
  contextStack.push_back(new XMLHandlerContext);
  LOG_DEBUG(5, __FUNCTION__ << "()");
}


void XMLProcessor::popContext() {
  if (contextStack.size() == 1) THROW("Cannot pop off last XMLHandlerContext");
  delete contextStack.back();
  contextStack.pop_back();
  LOG_DEBUG(5, __FUNCTION__ << "()");
}
