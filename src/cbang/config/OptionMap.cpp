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

#include "OptionMap.h"

#include <cbang/String.h>

#include <cbang/log/Logger.h>

#include <cbang/script/StdLibrary.h>

using namespace std;
using namespace cb;
using namespace cb::Script;


void OptionMap::add(SmartPointer<Option> option) {
  add(option->getName(), option);
}


SmartPointer<Option>
OptionMap::add(const string &name, const string &help,
               const SmartPointer<Constraint> &constraint) {
  SmartPointer<Option> option = new Option(name, help, constraint);
  add(option);
  return option;
}


SmartPointer<Option> OptionMap::add(const string &name, const char shortName,
                                    SmartPointer<OptionActionBase> action,
                                    const string &help) {
  SmartPointer<Option> option = new Option(name, shortName, action, help);
  add(option);
  return option;
}


bool OptionMap::eval(const Context &ctx) {
  if (!has(ctx.args[0])) return StdLibrary::instance().eval(ctx);

  const SmartPointer<Option> &option = get(ctx.args[0]);
  if (option->hasValue()) ctx.handler.eval(ctx, option->toString());

  return true;
}


void OptionMap::startElement(const string &name, const XMLAttributes &attrs) {
  // Unknown option
  if (!autoAdd && !has(name)) {
    LOG_WARNING("Unrecognized option '" << name << "'");
    return;
  }

  setDefault = attrs.has("default") && attrs["default"] == "true";

  Option &option = *localize(name);
  option.setFilename(&fileTracker.getCurrentFile());

  XMLAttributes::const_iterator it = attrs.find("v");
  if (it == attrs.end()) it = attrs.find("value");

  if (it != attrs.end()) {
    if (setDefault) option.setDefault(it->second);
    else if (!allowReset && option.isPlural()) option.append(it->second);
    else {
      if (!allowReset && option.isSet())
        LOG_WARNING("Option '" << name << "' already set to '" << option
                  << "' reseting to '" << it->second << "'.");

      option.set(it->second);
    }

    xmlValueSet = true;

  } else xmlValueSet = false;

  xmlValue = "";
}


void OptionMap::endElement(const string &name) {
  // Unknown option
  if (!autoAdd && !has(name)) return;

  Option &option = *localize(name);

  if (xmlValue.empty()) {
    // Empty option assume boolean
    if (!xmlValueSet) {
      if (setDefault) option.setDefault(true);
      else option.set(true);
    }

  } else {
    if (setDefault) option.setDefault(xmlValue);
    if (option.isPlural()) {
      if (allowReset) option.set(String::trim(xmlValue));
      else option.append(String::trim(xmlValue));

    } else {
      if (!allowReset && option.isSet())
        LOG_WARNING("Option '" << name << "' already set to '" << option
                    << "' reseting to '" << xmlValue << "'.");

      option.set(xmlValue);
    }
  }
}


void OptionMap::text(const string &text) {
  xmlValue.append(text);
}


void OptionMap::cdata(const string &data) {
  xmlValue.append(data);
}
