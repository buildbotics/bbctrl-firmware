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

#ifndef OPTION_H
#define OPTION_H

#include "OptionAction.h"
#include "Constraint.h"

#include <cbang/SmartPointer.h>
#include <cbang/StdTypes.h>

#include <cbang/xml/XMLHandler.h>

#include <ostream>
#include <string>
#include <vector>
#include <set>

namespace cb {

  /**
   * A Configuration option.  Holds option defaults, user values, and parses
   * option strings.  Can also call an OptionAction when an option is set.
   */
  class Option {
  public:
    static const std::string DEFAULT_DELIMS;

    typedef std::vector<std::string> strings_t;
    typedef std::vector<int64_t> integers_t;
    typedef std::vector<double> doubles_t;
    typedef enum {
      BOOLEAN_TYPE,
      STRING_TYPE,
      INTEGER_TYPE,
      DOUBLE_TYPE,
      STRINGS_TYPE,
      INTEGERS_TYPE,
      DOUBLES_TYPE,
    } type_t;

    typedef enum {
      DEFAULT_SET_FLAG  = 1 << 0,
      SET_FLAG          = 1 << 1,
      OPTIONAL_FLAG     = 1 << 2,
      OBSCURED_FLAG     = 1 << 3,
      COMMAND_LINE_FLAG = 1 << 4,
    } flags_t;

  protected:
    std::string name;
    char shortName;
    type_t type;
    std::string defaultValue;
    std::string help;
    std::string value;
    uint32_t flags;
    const std::string *filename;

    typedef std::set<std::string> aliases_t;
    aliases_t aliases;

    SmartPointer<Option> parent;
    SmartPointer<OptionActionBase> action;
    SmartPointer<OptionActionBase> defaultSetAction;
    SmartPointer<Constraint> constraint;

  public:
    Option(const SmartPointer<Option> &parent);
    Option(const std::string &name, char shortName = 0,
           SmartPointer<OptionActionBase> action = 0,
           const std::string &help = "");
    Option(const std::string &name, const std::string &help,
           const SmartPointer<Constraint> &constraint = 0);

    const std::string &getName() const {return name;}
    char getShortName() const {return shortName;}

    void setType(type_t type) {this->type = type;}
    type_t getType() const {return type;}
    const std::string getTypeString() const;

    const std::string &getDefault() const;
    void setDefault(const std::string &defaultValue);
    void setDefault(const char *defaultValue);
    void setDefault(int64_t defaultValue);
    void setDefault(uint64_t defaultValue) {setDefault((int64_t)defaultValue);}
    void setDefault(int32_t defaultValue) {setDefault((int64_t)defaultValue);}
    void setDefault(uint32_t defaultValue) {setDefault((int64_t)defaultValue);}
    void setDefault(double defaultValue);
    void setDefault(bool defaultValue);
    bool hasDefault() const;
    bool isDefault() const;

    void setOptional() {flags |= OPTIONAL_FLAG;}
    bool isOptional() const {return flags & OPTIONAL_FLAG;}
    void setObscured() {flags |= OBSCURED_FLAG;}
    bool isObscured() const {return flags & OBSCURED_FLAG;}
    bool isPlural() const {return type >= STRINGS_TYPE;}
    void setCommandLine() {flags |= COMMAND_LINE_FLAG;}
    bool isCommandLine() const {return flags & COMMAND_LINE_FLAG;}

    const std::string &getHelp() const {return help;}
    void setHelp(const std::string &help) {this->help = help;}

    void setFilename(const std::string *filename) {this->filename = filename;}
    const std::string *getFilename() const {return filename;}

    /// This function should only be called by Options::alias()
    void addAlias(const std::string &alias) {aliases.insert(alias);}
    typedef aliases_t::const_iterator iterator;
    iterator aliasesBegin() const {return aliases.begin();}
    iterator aliasesEnd() const {return aliases.end();}

    void setAction(const SmartPointer<OptionActionBase> &action)
    {this->action = action;}
    void setDefaultSetAction(const SmartPointer<OptionActionBase> &action)
    {defaultSetAction = action;}

    void setConstraint(const SmartPointer<Constraint> &constraint)
    {this->constraint = constraint;}

    void reset();
    void unset();
    void set(const std::string &value);
    void set(const char *value) {set(std::string(value));}
    void set(int64_t value);
    void set(uint64_t value) {set((int64_t)value);}
    void set(int32_t value) {set((int64_t)value);}
    void set(uint32_t value) {set((int64_t)value);}
    void set(double value);
    void set(bool value);
    void set(const strings_t &values);
    void set(const integers_t &values);
    void set(const doubles_t &values);

    void append(const std::string &value);
    void append(int64_t value);
    void append(double value);

    bool isSet() const {return flags & SET_FLAG;}
    bool hasValue() const;

    bool toBoolean() const;
    const std::string &toString() const;
    int64_t toInteger() const;
    double toDouble() const;
    const strings_t toStrings(const std::string &delims = DEFAULT_DELIMS) const;
    const integers_t toIntegers(const std::string &delims =
                                DEFAULT_DELIMS) const;
    const doubles_t toDoubles(const std::string &delims = DEFAULT_DELIMS) const;

    template <typename T>
    void checkConstraint(T value) const {
      if (!constraint.isNull()) constraint->validate(value);
      if (!parent.isNull()) parent->checkConstraint(value);
    }

    void validate() const;

    bool hasAction() const {return action.get();}

    void parse(unsigned &i, const std::vector<std::string> &args);
    std::ostream &printHelp(std::ostream &stream, bool cmdLine = true) const;
    std::ostream &print(std::ostream &stream) const;

    operator const std::string &() const {return toString();}

    void write(XMLHandler &handler, uint32_t flags) const;
    void printHelp(XMLHandler &handler) const;

  protected:
    void setDefault(const std::string &value, type_t type);
  };

  inline std::ostream &operator<<(std::ostream &stream, const Option &o) {
    return stream << o.toString();
  }

  inline std::string operator+(const std::string &s, const Option &o) {
    return s + o.toString();
  }
};

#endif // OPTIONS_H
