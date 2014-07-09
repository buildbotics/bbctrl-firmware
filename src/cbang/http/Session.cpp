#include "Session.h"

#include <cbang/time/Time.h>
#include <cbang/json/Value.h>
#include <cbang/json/Sync.h>

using namespace cb::HTTP;


void Session::read(const JSON::Value &value) {
  creationTime =
    value.has("created") ?
    (uint64_t)Time::parse(value.getString("created")) : 0;
  lastUsed =
    value.has("last_used") ?
    (uint64_t)Time::parse(value.getString("last_used")) : 0;
  user = value.getString("user", "");
  ip = value.has("ip") ? IPAddress(value.getString("ip")) : IPAddress();
}


void Session::write(JSON::Sync &sync) const {
  sync.beginDict();
  sync.insert("created", Time(creationTime).toString());
  sync.insert("last_used", Time(lastUsed).toString());
  sync.insert("user", user);
  sync.insert("ip", ip.toString());
  sync.endDict();
}
