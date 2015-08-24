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

#include "Options.h"

#include "OptionCategory.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cbang/json/Dict.h>
#include <cbang/json/Sink.h>

#include <cbang/log/Logger.h>

#include <cctype>
#include <iomanip>

using namespace std;
using namespace cb;


bool Options::warnWhenInvalid = false;


Options::Options() {
  pushCategory(""); // Default category
}


Options::~Options() {}


void Options::add(const string &_key, SmartPointer<Option> option) {
  string key = cleanKey(_key);
  iterator it = map.find(key);

  if (it != map.end()) THROWS("Option '" << key << "' already exists.");

  map[key] = option;
  categoryStack.back()->add(option);
}


bool Options::remove(const string &key) {
  return map.erase(cleanKey(key));
}


bool Options::has(const string &key) const {
  return map.find(cleanKey(key)) != map.end();
}


const SmartPointer<Option> &Options::get(const string &_key) const {
  string key = cleanKey(_key);

  const_iterator it = map.find(key);

  if (it == map.end()) {
    if (getAutoAdd()) {
      const SmartPointer<Option> &option =
        const_cast<Options *>(this)->map[key] = new Option(key);
      categoryStack.back()->add(option);
      return option;

    } else THROWS("Option '" << key << "' does not exist.");
  }

  return it->second;
}


void Options::alias(const string &_key, const string &_alias) {
  string key = cleanKey(_key);
  string alias = cleanKey(_alias);

  const SmartPointer<Option> &option = localize(key);

  iterator it = map.find(alias);
  if (it != map.end())
    THROWS("Cannot alias, option '" << alias << "' already exists.");

  option->addAlias(alias);
  map[alias] = option;
}


void Options::insert(JSON::Sink &sink, bool config,
                     const string &delims) const {
  categories_t::const_iterator it;
  for (it = categories.begin(); it != categories.end(); it++)
    if (!it->second->getHidden() && !it->second->isEmpty() &&
        (!config || it->second->hasSetOption())) {
      if (!config) sink.beginInsert(it->first);
      it->second->write(sink, config, delims);
    }
}


void Options::write(JSON::Sink &sink, bool config, const string &delims) const {
  sink.beginDict();
  insert(sink, config, delims);
  sink.endDict();
}


ostream &Options::print(ostream &stream) const {
  const_iterator it;
  unsigned width = 30;

  for (it = begin(); it != end(); it++) {
    string name = it->second->getName();

    if (!name.empty() && name[0] != '_') {
      unsigned len = name.length();
      if (width < len) width = len;
    }
  }

  for (it = begin(); it != end(); it++) {
    string name = it->second->getName();

    if (!name.empty() && name[0] != '_') {
      stream << setw(width) << name << " = ";

      if (it->second->hasValue()) stream << *it->second << '\n';
      else stream << "<undefined>" << '\n';
    }
  }

  return stream;
}


void Options::printHelp(ostream &stream) const {
  categories_t::const_iterator it;

  bool first = true;
  for (it = categories.begin(); it != categories.end(); it++)
    if (!it->second->getHidden()) {
      if (first) first = false;
      else stream << "\n\n";
      it->second->printHelp(stream);
    }
}


SmartPointer<JSON::Dict> Options::getDict(bool defaults, bool all) const {
  SmartPointer<JSON::Dict> dict = new JSON::Dict;

  for (const_iterator it = begin(); it != end(); it++) {
    Option &option = *it->second;

    if (!all && !option.isSet() && (!defaults || !option.hasValue()))
      continue;

    if (option.hasValue()) dict->insert(option.getName(), option.toString());
    else dict->insertNull(option.getName());
  }

  return dict;
}


const SmartPointer<OptionCategory> &Options::getCategory(const string &name) {
  categories_t::iterator it = categories.find(name);

  if (it == categories.end())
    it = categories.insert
      (categories_t::value_type(name, new OptionCategory(name))).first;

  return it->second;
}


const SmartPointer<OptionCategory> &Options::pushCategory(const string &name) {
  const SmartPointer<OptionCategory> &category = getCategory(name);
  categoryStack.push_back(category);
  return category;
}


void Options::popCategory() {
  if (categoryStack.size() <= 1) THROW("Cannot pop category stack");
  categoryStack.pop_back();
}


void Options::write(XMLHandler &handler, uint32_t flags) const {
  categories_t::const_iterator it;

  for (it = categories.begin(); it != categories.end(); it++)
    it->second->write(handler, flags);
}


void Options::printHelpTOC(XMLHandler &handler, const string &prefix) const {
  handler.startElement("ul");

  categories_t::const_iterator it;

  for (it = categories.begin(); it != categories.end(); it++)
    it->second->printHelpTOC(handler, prefix);

  handler.endElement("ul");
}


void Options::printHelp(XMLHandler &handler, const string &prefix) const {
  categories_t::const_iterator it;

  for (it = categories.begin(); it != categories.end(); it++)
    it->second->printHelp(handler, prefix);
}


const char *Options::getHelpStyle() const {
  return
    ".option {"
    "  padding: 1.5em;"
    "  text-align: left;"
    "}"
    ""
    ".option .name {"
    "  font-weight: bold;"
       "  margin-right: 1em;"
    "}"
    ""
    ".option .type {"
    "  color: green;"
    "}"
    ""
    ".option .default {"
       "  color: red;"
    "}"
    ""
    ".option .help {"
    "  margin-top: 1em;"
    "  white-space: pre-wrap;"
    "}"
       ""
    ".options td {"
    "  text-align: left;"
    "}"
    ".option-category-name {"
    "  font-size: 20pt;"
    "  margin: 2em 0 1em 0;"
    "}";
}


void Options::printHelpPage(XMLHandler &handler) const {
  handler.startElement("html");
  handler.startElement("head");

  XMLAttributes attrs;
  attrs["charset"] = "utf-8";
  handler.startElement("meta", attrs);
  handler.endElement("meta");

  handler.startElement("style");
  handler.text(getHelpStyle());

  handler.endElement("style");
  handler.endElement("head");

  handler.startElement("body");

  printHelpTOC(handler, "");
  printHelp(handler, "");

  handler.endElement("body");
  handler.endElement("html");
}


string Options::cleanKey(const string &key) {
  return String::replace(key, '_', '-');
}
