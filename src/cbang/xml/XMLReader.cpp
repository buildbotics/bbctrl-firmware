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

#include "XMLReader.h"

#include "XMLHandlerFactory.h"
#include "XMLAdapter.h"
#include "XMLSkipHandler.h"
#include "XIncludeHandler.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/Zap.h>

#include <cbang/log/Logger.h>

#include <cbang/os/SystemUtilities.h>

using namespace cb;
using namespace std;


struct XMLReader::HandlerRecord {
  XMLHandler *handler;
  unsigned depth;
  XMLHandlerFactory *factory;

  HandlerRecord(XMLHandler *handler, unsigned depth,
                XMLHandlerFactory *factory) :
    handler(handler), depth(depth), factory(factory) {}
};


XMLReader::XMLReader(bool skipRoot) :
  skipRoot(skipRoot), depth(0), xIncludeHandler(new XIncludeHandler) {
  addFactory("include", xIncludeHandler);
}


XMLReader::~XMLReader() {
  zap(xIncludeHandler);
}


void XMLReader::read(const string &filename, XMLHandler *handler) {
  pushFile(filename);
  if (handler) push(handler, 0);
  read(*SystemUtilities::open(filename, ios::in), 0);
  if (handler) pop();
  popFile();
}


void XMLReader::read(istream &stream, XMLHandler *handler) {
  if (handler) push(handler, 0);

  SmartPointer<XMLAdapter> adapter = XMLAdapter::create();
  adapter->setFilename(getCurrentFile());
  adapter->pushHandler(this);

  XMLSkipHandler skipper(*this);
  if (skipRoot) adapter->pushHandler(&skipper);

  adapter->read(stream);

  if (handler) pop();
}


void XMLReader::pushContext() {
  XMLProcessor::pushContext();
  addFactory("include", xIncludeHandler);
}


void XMLReader::pushFile(const std::string &filename) {
  XMLProcessor::pushFile(filename);
  if (!handlers.empty()) get().pushFile(filename);
}


void XMLReader::popFile() {
  XMLProcessor::popFile();
  if (!handlers.empty()) get().popFile();
}


void XMLReader::startElement(const string &name, const XMLAttributes &attrs) {
  LOG_DEBUG(5, __FUNCTION__ << "(" << name << ", " << attrs.toString() << ")");

  depth++;

  XMLHandlerFactory *factory = getFactory(name);
  if (factory) {
    push(factory->getHandler(*this, attrs), factory);
    LOG_DEBUG(5, "XMLReader pushed " << name << " handler");
  } else get().startElement(name, attrs);
}


void XMLReader::endElement(const string &name) {
  LOG_DEBUG(5, __FUNCTION__ << "(" << name << ")");

  depth--;

  if (!pop()) get().endElement(name);
  else LOG_DEBUG(5, "XMLReader popped " << name << " handler");
}


void XMLReader::text(const string &text) {
  if (depth) get().text(text);
}


void XMLReader::cdata(const string &data) {
  if (depth) get().cdata(data);
}


void XMLReader::push(XMLHandler *handler, XMLHandlerFactory *factory) {
  handlers.push_back(HandlerRecord(handler, depth, factory));
  get().pushFile(getCurrentFile());
}


bool XMLReader::pop() {
  if (!handlers.empty() && handlers.back().depth == depth + 1) {
    get().popFile();

    HandlerRecord &record = handlers.back();
    if (record.factory) record.factory->freeHandler(*this, record.handler);
    handlers.pop_back();
    return true;
  }

  return false;
}


XMLHandler &XMLReader::get() {
  if (handlers.empty()) THROW("Handlers empty!");
  return *handlers.back().handler;
}
