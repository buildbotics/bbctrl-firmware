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

#include <cbang/config/CommandLine.h>

#include <cbang/Exception.h>

#include <cbang/log/Logger.h>

#include <stdlib.h>

using namespace std;
using namespace cb;


CommandLine::CommandLine() :
  keywords(0), allowConfigAsFirstArg(false), allowSingleDashLongOpts(false),
  allowExtraOpts(false), allowPositionalArgs(true), warnOnInvalidArgs(false) {
  SmartPointer<Option> opt;
  opt = add("help", 0,
            new OptionAction<CommandLine>(this, &CommandLine::usageAction),
            "Print help screen or help on a particular option and exit.");
  opt->setType(Option::STRING_TYPE);
  opt->setOptional();

  add(string(), 'v',
      new OptionAction<CommandLine>(this, &CommandLine::incVerbosityAction),
      "Increase verbosity level.");

  alias("-v", "--verbose");

  add("license", 0, new OptionAction<CommandLine>
      (this, &CommandLine::licenseAction), "License information and exit.");
}


void CommandLine::add(const string &name, SmartPointer<Option> option) {
  option->setType(Option::BOOLEAN_TYPE); // The default for command line opts

  // Short name
  if (option->getShortName())
    Options::add(string("-") + option->getShortName(), option);

  // Long name
  if (!name.empty())
    Options::add(string("--") + option->getName(), option);
}


const SmartPointer<Option> &CommandLine::get(const string &key) const {
  try {
    return Options::get(key);

  } catch (const Exception &e) {
    // If its not in the command line options try the keywords.
    if (keywords && key.length() > 2 && key[0] == '-' && key[1] == '-')
      return keywords->get(key.substr(2));
    else throw;
  }
}


int CommandLine::parse(int argc, char *argv[]) {
  return parse(vector<string>(argv, argv + argc));
}


int CommandLine::parse(const vector<string> &args) {
  name = args[0];
  bool configSet = false;
  unsigned i;

  for (i = 1; i < args.size();) {
    string optionStr = args[i];
    SmartPointer<Option> option;

    if (allowExtraOpts && optionStr == "--") {
      i++;
      break;
    }

    if (optionStr.empty() || optionStr[0] != '-') {
      if (allowConfigAsFirstArg && !configSet && !optionStr.empty()) {
        option = get("--config");

        if (option.get()) {
          unsigned cmdI = 0;
          vector<string> cmdArgs;

          cmdArgs.push_back("--config");
          cmdArgs.push_back(args[i++]);

          option->parse(cmdI, cmdArgs);
          configSet = true;
          continue;
        }
      }

      if (allowPositionalArgs) {
        positionalArgs.push_back(args[i++]);
        continue;
      }
    }

    // Split --arg=value
    string::size_type pos = optionStr.find('=');
    if (pos != string::npos) optionStr = optionStr.substr(0, pos);

    try {
      if (allowSingleDashLongOpts && 1 < optionStr.size() &&
          optionStr[1] != '-' && !has(optionStr))
        option = get(string("-") + optionStr);
      else option = get(optionStr);

    } catch (const Exception &e) {
      if (warnOnInvalidArgs) {
        LOG_WARNING("Ignoring invalid or unsupported argument '"
                    << args[i++] << "'");
        continue;
      }

      THROWCS("Invalid argument '" << args[i] << "'", e);
    }

    option->parse(i, args);
    option->setCommandLine();
  }

  return i;
}


int CommandLine::usageAction(Option &option) {
  if (option.hasValue()) {
    string name = option.toString();
    SmartPointer<Option> opt;

    if (has(string("--") + name)) opt = get(string("--") + name);
    else if (keywords && keywords->has(name)) opt = keywords->get(name);
    else THROWS("Unrecognized command line option or keyword '" << name << "'");

    opt->printHelp(cout) << endl;

  } else usage(cout, name);

  exit(0); // TODO cb::Option calls this action so we have to exit(0). FIXME!
  return -1;
}


void CommandLine::usage(ostream &stream, const string &name) const {
  stream << "Usage: " << name;
  if (allowConfigAsFirstArg) stream << " [[--config] <filename>]";

  stream << " [OPTIONS]...";
  if (allowExtraOpts) stream << " [-- OPTIONS]";
  stream << endl << "Command line options:" << endl;

  for (const_iterator it = begin(); it != end(); it++)
    it->second->printHelp(stream) << "\n\n";

  if (keywords) {
    stream << "\nConfiguration options:\n";
    String::fill(stream, "The following options can be specified in a "
                 "configuration file or on the command line using the "
                 "following syntax:\n");
    stream <<
      "\n"
      "    --<option> <value>\n"
      "\n"
      "  or:\n"
      "\n"
      "    --<option>=<value>\n"
      "\n"
      "  or when marking boolean values true:\n"
      "\n"
      "    --<option>";

    keywords->printHelp(stream);

    stream << endl;
  }

  // Extra usage lines added by the application
  if (usageExtras.size()) stream << endl;
  for (unsigned i = 0; i < usageExtras.size(); i++)
    stream << usageExtras[i] << endl;
}


int CommandLine::licenseAction() {
  for (unsigned i = 0; i < licenseText.size(); i++)
    cout << licenseText[i] << endl << endl;

  exit(0); // TODO Option calls the action so we have to exit(). FIXME!
  return -1;
}


int CommandLine::incVerbosityAction() {
  Logger::instance().setVerbosity(Logger::instance().getVerbosity() + 1);
  return 0;
}
