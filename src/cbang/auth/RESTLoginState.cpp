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

#include "RESTLoginState.h"

#include <cbang/util/ID.h>
#include <cbang/time/Time.h>
#include <cbang/security/KeyContext.h>
#include <cbang/net/Base64.h>
#include <cbang/io/StringInputSource.h>
#include <cbang/util/Random.h>
#include <cbang/json/JSON.h>
#include <cbang/json/BufferWriter.h>

using namespace cb;
using namespace std;


RESTLoginState::RESTLoginState(const KeyPair &key) :
  key(key), nonce(((uint64_t)lrand48() << 16) | lrand48()), ts(Time::now()),
  auth(0) {
}


RESTLoginState::RESTLoginState(const KeyPair &key, const string &state) :
  key(key) {
  read(*JSON::Reader(StringInputSource(decode(state))).parse());
}


string RESTLoginState::toString() const {
  JSON::BufferWriter buf;
  write(buf);
  return encode(string(buf.data(), buf.size()));
}


void RESTLoginState::read(const JSON::Value &value) {
  nonce = String::parseU64(value.getString("nonce"));
  ts = Time::parse(value.getString("ts"));
  user = value.getString("user");
  auth = String::parseU64(value.getString("auth"));
}


void RESTLoginState::write(JSON::Sync &sync) const {
  sync.beginDict();
  sync.insert("nonce", ID(nonce).toString());
  sync.insert("ts", Time(ts).toString());
  sync.insert("user", user);
  sync.insert("auth", ID(auth).toString());
  sync.endDict();
}


string RESTLoginState::encode(const string &state) const {
  if (key.size() < state.length())
    THROWS("REST login state is (" << state.length()
           << " bytes) larger than key size (" << key.size() << " bytes)");

  return Base64('=', '-', '_', 0).encode(KeyContext(key).encrypt(state));
}


string RESTLoginState::decode(const string &state) const {
  return KeyContext(key).decrypt(Base64('=', '-', '_', 0).decode(state));
}
