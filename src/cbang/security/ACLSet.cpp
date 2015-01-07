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

#include "ACLSet.h"

#include <cbang/Exception.h>
#include <cbang/json/JSON.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;


void ACLSet::clear() {
  cache.clear();
  acls.clear();
  groups.clear();
  users.clear();
}


bool ACLSet::allow(const string &path, const string &user) const {
  // Check cache
  if (!flushDirtyCache()) {
    cache_t::const_iterator it = cache.find(path);
    if (it != cache.end()) {
      cache_users_t::const_iterator it2 = it->second.find(user);
      if (it2 != it->second.end()) return it2->second;
    }
  }

  return cache[path][user] = allowNoCache(path, user);
}


bool ACLSet::allowGroup(const string &path, const string &group) const {
  // Check cache
  if (!flushDirtyCache()) {
    cache_t::const_iterator it = groupCache.find(path);
    if (it != groupCache.end()) {
      cache_users_t::const_iterator it2 = it->second.find(group);
      if (it2 != it->second.end()) return it2->second;
    }
  }

  return groupCache[path][group] = allowGroupNoCache(path, group);
}


bool ACLSet::hasUser(const string &user) const {
  return users.find(user) != users.end();
}


void ACLSet::addUser(const string &user) {
  dirty = true;
  users.insert(user);
}


void ACLSet::delUser(const string &user) {
  dirty = true;
  users.erase(user);

  // Remove from groups
  for (groups_t::iterator it = groups.begin(); it != groups.end(); it++)
    it->second.users.erase(user);

  // Remove from ACLS
  for (acls_t::iterator it = acls.begin(); it != acls.end(); it++)
    it->second.users.erase(user);
}



bool ACLSet::hasGroup(const string &group) const {
  return groups.find(group) != groups.end();
}


void ACLSet::addGroup(const string &group) {
  dirty = true;
  groups.insert(groups_t::value_type(group, Group()));
}


void ACLSet::delGroup(const string &group) {
  dirty = true;
  groups.erase(group);

  // Remove from ACLS
  for (acls_t::iterator it = acls.begin(); it != acls.end(); it++)
    it->second.groups.erase(group);
}


bool ACLSet::groupHasUser(const string &groupName, const string &user) const {
  groups_t::const_iterator it = groups.find(groupName);
  if (it == groups.end()) return false;

  const Group &group = it->second;
  return group.users.find(user) != group.users.end();
}


void ACLSet::groupAddUser(const string &group, const string &user) {
  dirty = true;
  addUser(user);
  groups.insert(groups_t::value_type(group, Group())).
    first->second.users.insert(user);
}


void ACLSet::groupDelUser(const string &groupName, const string &user) {
  dirty = true;
  groups_t::iterator it = groups.find(groupName);
  if (it == groups.end()) THROWS("Group '" << groupName << "' does not exist");
  Group &group = it->second;

  group.users.erase(user);
}


bool ACLSet::hasACL(const string &path) const {
  return acls.find(path) != acls.end();
}


void ACLSet::addACL(const string &path) {
  dirty = true;
  acls.insert(acls_t::value_type(path, ACL()));
}


void ACLSet::delACL(const string &path) {
  dirty = true;
  acls.erase(path);
}


bool ACLSet::aclHasUser(const string &path, const string &user) const {
  acls_t::const_iterator it = acls.find(path);
  if (it == acls.end()) return false;

  const ACL &acl = it->second;
  return acl.users.find(user) != acl.users.end();
}


void ACLSet::aclAddUser(const string &path, const string &user) {
  dirty = true;
  addUser(user);
  acls.insert(acls_t::value_type(path, ACL())).
    first->second.users.insert(user);
}


void ACLSet::aclDelUser(const string &path, const string &user) {
  dirty = true;
  acls_t::iterator it = acls.find(path);
  if (it == acls.end()) THROWS("ACL '" << path << "' does not exist");
  ACL &acl = it->second;

  acl.users.erase(user);
}


bool ACLSet::aclHasGroup(const string &path, const string &group) const {
  acls_t::const_iterator it = acls.find(path);
  if (it == acls.end()) return false;

  const ACL &acl = it->second;
  return acl.groups.find(group) != acl.groups.end();
}


void ACLSet::aclAddGroup(const string &path, const string &group) {
  dirty = true;
  addGroup(group);
  acls.insert(acls_t::value_type(path, ACL())).
    first->second.groups.insert(group);
}


void ACLSet::aclDelGroup(const string &path, const string &group) {
  dirty = true;
  acls_t::iterator it = acls.find(path);
  if (it == acls.end()) THROWS("ACL '" << path << "' does not exist");
  ACL &acl = it->second;

  acl.groups.erase(group);
}


void ACLSet::write(JSON::Sync &sync) const {
  sync.beginDict();

  // Users
  if (!users.empty()) {
    sync.insertList("users");
    for (users_t::const_iterator it = users.begin(); it != users.end(); it++)
      sync.append(*it);
    sync.endList();
  }

  // Groups
  if (!groups.empty()) {
    sync.insertDict("groups");
    groups_t::const_iterator it;
    for (it = groups.begin(); it != groups.end(); it++) {
      sync.insertList(it->first);

      // Users
      const string_set_t &users = it->second.users;
      string_set_t::const_iterator it2;
      for (it2 = users.begin(); it2 != users.end(); it2++)
        sync.append(*it2);

      sync.endList();
    }
    sync.endDict();
  }

  // ACLS
  if (!acls.empty()) {
    sync.insertDict("acls");
    for (acls_t::const_iterator it = acls.begin(); it != acls.end(); it++) {
      sync.insertDict(it->first);

      // Users
      const string_set_t &users = it->second.users;
      if (!users.empty()) {
        sync.insertList("users");

        string_set_t::const_iterator it2;
        for (it2 = users.begin(); it2 != users.end(); it2++)
          sync.append(*it2);

        sync.endList();
      }

      // Groups
      const string_set_t &groups = it->second.groups;
      if (!groups.empty()) {
        sync.insertList("groups");

        string_set_t::const_iterator it2;
        for (it2 = groups.begin(); it2 != groups.end(); it2++)
          sync.append(*it2);

        sync.endList();
      }

      sync.endDict();
    }

    sync.endDict();
  }

  sync.endDict();
}


void ACLSet::set(const JSON::Value &json) {
  clear();

  const JSON::Dict &dict = json.getDict();

  // Users
  if (dict.has("users")) {
    const JSON::List &users = dict.get("users")->getList();
    for (unsigned i = 0; i < users.size(); i++) addUser(users[i]->getString());
  }

  // Groups
  if (dict.has("groups")) {
    const JSON::Dict &groups = dict.get("groups")->getDict();
    for (unsigned i = 0; i < groups.size(); i++) {
      const string &name = groups.keyAt(i);
      addGroup(name);

      // Users
      const JSON::List &users = groups.get(i)->getList();
      for (unsigned j = 0; j < users.size(); j++)
        groupAddUser(name, users[j]->getString());
    }
  }

  // ACLs
  if (dict.has("acls")) {
    const JSON::Dict &acls = dict.get("acls")->getDict();
    for (unsigned i = 0; i < acls.size(); i++) {
      const JSON::Dict &acl = acls.get(i)->getDict();
      const string &path = acls.keyAt(i);
      addACL(path);

      // Users
      if (acl.has("users")) {
        const JSON::List &users = acl.get("users")->getList();
        for (unsigned j = 0; j < users.size(); j++)
          aclAddUser(path, users[j]->getString());
      }

      // Groups
      if (acl.has("groups")) {
        const JSON::List &groups = acl.get("groups")->getList();
        for (unsigned j = 0; j < groups.size(); j++)
          aclAddGroup(path, groups[j]->getString());
      }
    }
  }
}


void ACLSet::write(ostream &stream) const {
  JSON::Writer writer(stream);
  write(writer);
}


void ACLSet::read(istream &stream) {
  JSON::Reader reader(stream);
  set(*reader.parse());
}


bool ACLSet::flushDirtyCache() const {
  if (!dirty) return false;

  cache.clear();
  groupCache.clear();
  dirty = false;

  return true;
}


string ACLSet::parentPath(const string &path) {
  size_t pos = path.find_last_of('/');
  if (!pos && path != "/") return "/";
  if (!pos || pos == string::npos) return "";
  return path.substr(0, pos);
}


bool ACLSet::allowNoCache(const string &_path, const string &user) const {
  // Check if user exists
  if (users.find(user) == users.end()) return false;

  // Find ACL
  for (string path = _path; !path.empty(); path = parentPath(path)) {
    LOG_DEBUG(5, __func__ << '(' << path << ", " << user << ')');

    acls_t::const_iterator it = acls.find(path);
    if (it != acls.end()) {
      const ACL &acl = it->second;

      // Check users
      if (acl.users.find(user) != acl.users.end()) return true;

      // Check groups
      string_set_t::const_iterator it2;
      for (it2 = acl.groups.begin(); it2 != acl.groups.end(); it2++) {
        groups_t::const_iterator it3 = groups.find(*it2);
        if (it3 == groups.end())
          THROWS("ACL contains non-existant group '" << *it2);

        const Group &group = it3->second;
        if (group.users.find(user) != group.users.end()) return true;
      }

      break;
    }
  }

  return false;
}


bool ACLSet::allowGroupNoCache(const string &_path, const string &group) const {
  // Check if group exists
  if (groups.find(group) == groups.end()) return false;

  // Find ACL
  for (string path = _path; !path.empty(); path = parentPath(path)) {
    LOG_DEBUG(5, __func__ << '(' << path << ", @" << group << ')');

    acls_t::const_iterator it = acls.find(path);
    if (it != acls.end()) {
      const ACL &acl = it->second;

      // Check groups
      return acl.groups.find(group) != acl.groups.end();
    }
  }

  return false;
}
