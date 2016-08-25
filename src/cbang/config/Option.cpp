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

#include "Option.h"
#include "Options.h"

#include <cbang/String.h>

#include <cbang/log/Logger.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/util/DefaultCatch.h>
#include <cbang/json/Sink.h>
#include <cbang/json/Integer.h>

using namespace std;
using namespace cb;

const string Option::DEFAULT_DELIMS = " \t\r\n";

// Proxy constructor
Option::Option(const SmartPointer<Option> &parent) :
  name(parent->name), shortName(parent->shortName), type(parent->type),
  help(parent->help), flags(parent->flags & ~(SET_FLAG | DEFAULT_SET_FLAG)),
  filename(parent->filename), aliases(parent->aliases), parent(parent),
  action(parent->action), defaultSetAction(parent->defaultSetAction) {
}


Option::Option(const string &name, const char shortName,
               SmartPointer<OptionActionBase> action, const string &help) :
  name(name), shortName(shortName), type(STRING_TYPE), help(help), flags(0),
  filename(0), action(action) {}


Option::Option(const string &name, const string &help,
               const SmartPointer<Constraint> &constraint) :
  name(name), shortName(0), type(STRING_TYPE), help(help), flags(0),
  filename(0), constraint(constraint) {}


const string Option::getTypeString() const {
  switch (type) {
  case BOOLEAN_TYPE: return "boolean";
  case STRING_TYPE: return "string";
  case INTEGER_TYPE: return "integer";
  case DOUBLE_TYPE: return "double";
  case STRINGS_TYPE: return "string ...";
  case INTEGERS_TYPE: return "integer ...";
  case DOUBLES_TYPE: return "double ...";
  default: THROWS("Invalid type " << type);
  }
}


const string &Option::getDefault() const {
  if (flags & DEFAULT_SET_FLAG) return defaultValue;
  if (!parent.isNull() && parent->hasValue()) return parent->toString();
  return defaultValue;
}


void Option::setDefault(const string &defaultValue) {
  setDefault(defaultValue, STRING_TYPE);
}


void Option::setDefault(const char *defaultValue) {
  setDefault(string(defaultValue), STRING_TYPE);
}


void Option::setDefault(int64_t defaultValue) {
  setDefault(String(defaultValue), INTEGER_TYPE);
}


void Option::setDefault(double defaultValue) {
  setDefault(String(defaultValue), DOUBLE_TYPE);
}


void Option::setDefault(bool defaultValue) {
  setDefault(String(defaultValue), BOOLEAN_TYPE);
}


bool Option::hasDefault() const {
  return flags & DEFAULT_SET_FLAG || (!parent.isNull() && parent->hasValue());
}


bool Option::isDefault() const {
  return hasDefault() && isSet() && value == getDefault();
}


bool Option::isHidden() const {
  return isDepreciated() || name.empty() || name[0] == '_';
}


void Option::reset() {
  if (!isSet() && value.empty()) return;  // Don't run action

  flags &= ~SET_FLAG;
  value.clear();

  if (hasAction()) (*action)(*this);
}


void Option::unset() {
  flags &= ~DEFAULT_SET_FLAG;
  defaultValue.clear();
  reset();
}


void Option::set(const string &value) {
  if (isDepreciated())
    THROWS("Option '" << name << "' has been depreciated: " << help);
  if (isSet() && this->value == value) return;

  uint32_t oldFlags = flags;
  string oldValue = this->value;

  flags |= SET_FLAG;
  this->value = value;

  // Clear the command line flag
  flags &= ~COMMAND_LINE_FLAG;

  try {
    validate();
  } catch (const Exception &e) {
    flags = oldFlags;
    this->value = oldValue;

    string errStr = string("Invalid value for option '") + name + "'";

    if (Options::warnWhenInvalid)
      LOG_WARNING(errStr << ": " << e.getMessage());

    else {
      ostringstream str;
      str << errStr << ".  Option help:\n";
      printHelp(str);
      THROWC(str.str(), e);
    }
  }

  if (hasAction()) (*action)(*this);
}


void Option::set(int64_t value) {
  set(String(value));
}


void Option::set(double value) {
  set(String(value));
}


void Option::set(bool value) {
  set(String(value));
}


void Option::set(const strings_t &values) {
  string value;

  for (unsigned i = 1; i < values.size(); i++) {
    if (i != 1) value += " ";
    value += values[i];
  }

  set(value);
}


void Option::set(const integers_t &values) {
  string value;

  for (unsigned i = 1; i < values.size(); i++) {
    if (i != 1) value += " ";
    value += String(values[i]);
  }

  set(value);
}


void Option::set(const doubles_t &values) {
  string value;

  for (unsigned i = 1; i < values.size(); i++) {
    if (i != 1) value += " ";
    value += String(values[i]);
  }

  set(value);
}


void Option::append(const string &value) {
  if (isSet() && !this->value.empty()) set(this->value + " " + value);
  else set(value);
}


void Option::append(int64_t value) {
  append(String(value));
}


void Option::append(double value) {
  append(String(value));
}


bool Option::hasValue() const {
  return isSet() || hasDefault();
}


bool Option::toBoolean() const {
  return parseBoolean(toString());
}


const string &Option::toString() const {
  if (isSet()) return value;
  else if (hasDefault()) return getDefault();
  else if (getType() == STRINGS_TYPE) return value;
  THROWS("Option '" << name << "' has no default and is not set.");
}


int64_t Option::toInteger() const {
  return parseInteger(toString());
}


double Option::toDouble() const {
  return parseDouble(toString());
}


Option::strings_t Option::toStrings(const string &delims) const {
  return parseStrings(toString());
}


Option::integers_t Option::toIntegers(const string &delims) const {
  return parseIntegers(toString());
}


Option::doubles_t Option::toDoubles(const string &delims) const {
  return parseDoubles(toString());
}


bool Option::parseBoolean(const string &value) {
  return String::parseBool(value);
}


int64_t Option::parseInteger(const string &value) {
  return String::parseS64(value);
}


double Option::parseDouble(const string &value) {
  return String::parseDouble(value);
}


Option::strings_t Option::parseStrings(const string &value,
                                       const string &delims) {
  strings_t result;
  String::tokenize(value, result, delims);
  return result;
}


Option::integers_t Option::parseIntegers(const string &value,
                                         const string &delims) {
  integers_t result;
  strings_t tokens;

  String::tokenize(value, tokens, delims);

  for (strings_t::iterator it = tokens.begin(); it != tokens.end(); it++)
    result.push_back(String::parseS32(*it));

  return result;
}


Option::doubles_t Option::parseDoubles(const string &value,
                                       const string &delims) {
  doubles_t result;
  strings_t tokens;

  String::tokenize(value, tokens, delims);

  for (strings_t::iterator it = tokens.begin(); it != tokens.end(); it++)
    result.push_back(String::parseDouble(*it));

  return result;
}


void Option::validate() const {
  switch (type) {
  case BOOLEAN_TYPE: checkConstraint(toBoolean()); break;
  case STRING_TYPE: checkConstraint(value); break;
  case INTEGER_TYPE: checkConstraint(toInteger()); break;
  case DOUBLE_TYPE: checkConstraint(toDouble()); break;
  case STRINGS_TYPE: checkConstraint(toStrings()); break;
  case INTEGERS_TYPE: checkConstraint(toIntegers()); break;
  case DOUBLES_TYPE: checkConstraint(toDoubles()); break;
  default: THROWS("Invalid type " << type);
  }
}


void Option::parse(unsigned &i, const vector<string> &args) {
  string arg = args[i++];
  string name;
  string value;
  bool hasValue = false;

  string::size_type pos = arg.find('=');
  if (pos != string::npos) {
    name = arg.substr(0, pos);
    value = arg.substr(pos + 1);
    hasValue = true;

  } else name = arg;

  // Required
  if (hasValue) set(value);

  else if (type == BOOLEAN_TYPE) set(true);

  else if (!isOptional()) {
    if (i == args.size()) {
      ostringstream str;
      str << "Missing required argument for option:\n";
      printHelp(str, true);
      LOG_WARNING(str.str());

    } else set(args[i++]);

  } else if (i < args.size() && args[i][0] != '-') set(args[i++]);

  else if (hasAction()) (*action)(*this); // No arg
}


ostream &Option::printHelp(ostream &stream, bool cmdLine) const {
  stream << "  ";

  // Short option name
  if (shortName && cmdLine) stream << '-' << shortName;

  // Long option name
  if (name != "") {
    if (shortName && cmdLine)  stream << "|";
    if (cmdLine) stream << "--";
    stream << name;
  }

  // Arg
  if (type != BOOLEAN_TYPE || !cmdLine) {
    stream << ' ' << (isOptional() ? '[' : '<');
    stream << getTypeString();
    if (hasDefault()) stream << '=' << getDefault();
    stream << (isOptional() ? ']' : '>');
  }

  // Depreciated
  if (isDepreciated()) stream << " (Depreciated)";

  // Help
  unsigned width = 80;
  try {
    const char *ohw = SystemUtilities::getenv("OPTIONS_HELP_WIDTH");
    if (ohw) width = String::parseU32(ohw);
  } CATCH_WARNING;

  stream << '\n';
  String::fill(stream, help, 0, cmdLine ? 6 : 4, width);

  return stream;
}


ostream &Option::print(ostream &stream) const {
  stream << String::escapeC(name) << ':';
  if (hasValue()) stream << ' ' << String::escapeC(toString());
  return stream;
}


void Option::writeBoolean(JSON::Sink &sink, const string &value) {
  sink.writeBoolean(parseBoolean(value));
}


void Option::writeInteger(JSON::Sink &sink, const string &value) {
  int64_t x = parseInteger(value);

  if (JSON_MIN_INT < x && x < JSON_MAX_INT) sink.write(x);
  else sink.write(SSTR("0x" << hex << x));
}


void Option::writeDouble(JSON::Sink &sink, const string &value) {
  sink.write(parseDouble(value));
}


void Option::writeStrings(JSON::Sink &sink, const string &value,
                          const string &delims) {
  strings_t l = parseStrings(value, delims);

  sink.beginList();
  for (unsigned i = 0; i < l.size(); i++) sink.append(l[i]);
  sink.endList();
}


void Option::writeIntegers(JSON::Sink &sink, const string &value,
                           const string &delims) {
  integers_t l = parseIntegers(value, delims);

  sink.beginList();
  for (unsigned i = 0; i < l.size(); i++) {
    sink.beginAppend();
    if (JSON_MIN_INT < l[i] && l[i] < JSON_MAX_INT) sink.write(l[i]);
    else sink.write(SSTR("0x" << hex << l[i]));
  }
  sink.endList();
}


void Option::writeDoubles(JSON::Sink &sink, const string &value,
                          const string &delims) {
  doubles_t l = parseDoubles(value, delims);

  sink.beginList();
  for (unsigned i = 0; i < value.size(); i++) {
    sink.beginAppend();
    sink.append(l[i]);
  }
  sink.endList();
}


void Option::writeValue(JSON::Sink &sink, const string &value,
                        const string &delims) const {
  switch (type) {
  case BOOLEAN_TYPE: writeBoolean(sink, value); break;
  case STRING_TYPE: sink.write(value); break;
  case INTEGER_TYPE: writeInteger(sink, value); break;
  case DOUBLE_TYPE: writeDouble(sink, value); break;
  case STRINGS_TYPE: writeStrings(sink, value, delims); break;
  case INTEGERS_TYPE: writeIntegers(sink, value, delims); break;
  case DOUBLES_TYPE: writeDoubles(sink, value, delims); break;
  default: THROWS("Invalid type " << type);
  }
}


void Option::write(JSON::Sink &sink, bool config, const string &delims) const {
  if (config) {
    string value = toString();

    if (isObscured() && !(flags & OBSCURED_FLAG))
      sink.write(string(value.size(), '*'));
    else writeValue(sink, value, delims);

    return;
  }

  sink.beginDict();

  if (!getHelp().empty()) sink.insert("help", getHelp());

  if (hasValue()) {
    sink.beginInsert("value");
    string value = toString();

    if (isObscured() && !(flags & OBSCURED_FLAG))
      sink.write(string(value.size(), '*'));
    else writeValue(sink, value, delims);
  }

  if (hasDefault()) {
    sink.beginInsert("default");
    writeValue(sink, getDefault(), delims);
  }

  sink.insert("type", getTypeString());
  if (isOptional()) sink.insertBoolean("optional", true);
  if (shortName) sink.insert("short", string(1, shortName));
  if (isSet()) sink.insertBoolean("set", true);
  if (isCommandLine()) sink.insertBoolean("command_line", true);
  if (isDepreciated()) sink.insertBoolean("depreciated", true);
  if (!constraint.isNull()) sink.insert("constraint", constraint->getHelp());

  sink.endDict();
}


void Option::write(XMLHandler &handler, uint32_t flags) const {
  XMLAttributes attrs;

  string value = toString();
  if (isObscured() && !(flags & OBSCURED_FLAG))
    value = string(value.size(), '*');

  if (!isPlural()) attrs["v"] = value;

  handler.startElement(name, attrs);

  if (isPlural()) handler.text(value);

  handler.endElement(name);
}


void Option::printHelpTOC(XMLHandler &handler, const string &prefix) const {
  handler.startElement("li");

  XMLAttributes attrs;
  attrs["href"] = "#" + prefix + "option-" + getName();
  handler.startElement("a", attrs);
  handler.text(getName());
  handler.endElement("a");

  handler.endElement("li");
}


void Option::printHelp(XMLHandler &handler, const string &prefix) const {
  XMLAttributes attrs;

  attrs["class"] = "option";
  attrs["id"] = prefix + "option-" + getName();
  handler.startElement("div", attrs);

  // Name
  attrs.clear();
  attrs["class"] = "name";
  handler.startElement("span", attrs);
  handler.text(getName());
  handler.endElement("span");

  // Type
  attrs["class"] = "type";
  handler.startElement("span", attrs);
  handler.text(isOptional() ? "[" : "<");
  handler.text(getTypeString());

  // Default
  if (hasDefault()) {
    handler.text(" = ");

    bool isString = type == STRING_TYPE || type == STRINGS_TYPE;

    attrs["class"] = "default";
    handler.startElement("span", attrs);
    handler.text((isString ? "\"" : "") + getDefault() +
                 (isString ? "\"" : ""));
    handler.endElement("span");
  }

  handler.text(isOptional() ? "]" : ">");
  handler.endElement("span");

  // Help
  if (getHelp() != "") {
    attrs["class"] = "help";
    handler.startElement("div", attrs);
    string help = getHelp();
    vector<string> tokens;
    String::tokenize(help, tokens, "\t");
    handler.text(String::join(tokens, "  "));
    handler.text(" ");
    handler.endElement("div");
  }


  handler.endElement("div");
}


void Option::setDefault(const string &value, type_t type) {
  defaultValue = value;
  flags |= DEFAULT_SET_FLAG;
  this->type = type;

  if (defaultSetAction.get()) (*defaultSetAction)(*this);
}
