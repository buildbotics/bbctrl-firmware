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

#include "JSImpl.h"
#include "Context.h"
#include "Factory.h"

#include <cbang/js/Javascript.h>
#include <cbang/util/SmartFunctor.h>

using namespace cb::gv8;
using namespace cb;
using namespace std;


JSImpl::JSImpl(js::Javascript &js) : ctx(*this) {}


void JSImpl::init(int *argc, char *argv[]) {
  if (argv) v8::V8::SetFlagsFromCommandLine(argc, argv, true);
}


JSImpl &JSImpl::current() {return Context::current().getImpl();}
SmartPointer<js::Factory> JSImpl::getFactory() {return new Factory;}
SmartPointer<js::Scope> JSImpl::enterScope() {return new Context::Scope(ctx);}


SmartPointer<js::Scope> JSImpl::newScope() {
  return new Context::Scope(new Context(*this));
}


void JSImpl::interrupt() {v8::V8::TerminateExecution();}
