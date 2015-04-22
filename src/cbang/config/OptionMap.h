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

#ifndef CBANG_OPTION_MAP_H
#define CBANG_OPTION_MAP_H

#include <string>
#include <iostream>

#include <cbang/SmartPointer.h>

#include "Option.h"
#include "OptionActionSet.h"

#include <cbang/xml/XMLHandlerFactory.h>
#include <cbang/xml/XMLFileTracker.h>

#include <cbang/script/Handler.h>

namespace cb {
  class Option;

  /// A base class for configuration option handling
  class OptionMap : public XMLHandler, public Script::Handler {
    XMLFileTracker fileTracker;

    std::string xmlValue;
    bool xmlValueSet;
    bool setDefault;
    bool autoAdd;
    bool allowReset;

  public:
    OptionMap() : xmlValueSet(false), setDefault(false), autoAdd(false),
                  allowReset(false) {}
    virtual ~OptionMap() {}

    bool getAutoAdd() const {return autoAdd;}
    void setAutoAdd(bool x) {autoAdd = x;}
    bool getAllowReset() const {return allowReset;}
    void setAllowReset(bool x) {allowReset = x;}

    void add(SmartPointer<Option> option);
    SmartPointer<Option> add(const std::string &name, const std::string &help,
                             const SmartPointer<Constraint> &constraint = 0);
    SmartPointer<Option> add(const std::string &name, const char shortName = 0,
                             SmartPointer<OptionActionBase> action = 0,
                             const std::string &help = "");
    template <typename T>
    SmartPointer<Option> addTarget(const std::string &name, T &target,
                                   const std::string &help = "") {
      SmartPointer<OptionActionBase> action = new OptionActionSet<T>(target);
      SmartPointer<Option> option = add(name, 0, action, help);
      option->setDefault(target);
      option->setDefaultSetAction(action);
      return option;
    }

    Option &operator[](const std::string &key) const {return *get(key);}

    // Virtual interface
    virtual void add(const std::string &name, SmartPointer<Option> option) = 0;
    virtual bool remove(const std::string &key) = 0;
    virtual bool has(const std::string &key) const = 0;
    virtual bool local(const std::string &key) const {return true;}
    virtual const SmartPointer<Option> &localize(const std::string &key)
    {return get(key);}
    virtual const SmartPointer<Option> &get(const std::string &key) const = 0;
    virtual void alias(const std::string &name, const std::string &alias) = 0;

    // From Script::Handler
    bool eval(const Script::Context &ctx);

    // From XMLHandler
    void pushFile(const std::string &filename) {fileTracker.pushFile(filename);}
    void popFile() {fileTracker.popFile();}
    void startElement(const std::string &name, const XMLAttributes &attrs);
    void endElement(const std::string &name);
    void text(const std::string &text);
    void cdata(const std::string &data);
  };
}

#endif // CBANG_OPTION_MAP_H

