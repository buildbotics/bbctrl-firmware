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

#include "Javascript.h"
#include "Isolate.h"

#include <cbang/os/SystemUtilities.h>

#include <libplatform/libplatform.h>

using namespace std;
using namespace cb;
using namespace cb::js;


Javascript *Javascript::singleton = 0;


Javascript::Javascript() {
  v8::V8::InitializeICU();
  v8::V8::InitializeExternalStartupData
    (SystemUtilities::getExecutablePath().c_str());

  platform = v8::platform::CreateDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();

  isolate = new Isolate;
  scope = isolate->getScope();
}


Javascript::~Javascript() {
  scope.release();
  isolate.release();

  v8::V8::Dispose();
  v8::V8::ShutdownPlatform();
}


void Javascript::init(int *argc, char *argv[]) {
  if (!singleton) singleton = new Javascript;
  if (argv) v8::V8::SetFlagsFromCommandLine(argc, argv, true);
}


void Javascript::deinit() {
  if (singleton) delete singleton;
  singleton = 0;
}


void Javascript::terminate() {
  // Terminate script execution
  if (singleton) singleton->isolate->interrupt();
}
