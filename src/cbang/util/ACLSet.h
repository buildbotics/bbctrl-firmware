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

#ifndef CBANG_ACLSET_H
#define CBANG_ACLSET_H

#include <map>
#include <set>
#include <string>
#include <iostream>


namespace cb {
  namespace JSON {
    class Sink;
    class Value;
  }

  class ACLSet {
    mutable bool dirty;
    typedef std::map<std::string, bool> cache_users_t;
    typedef std::map<std::string, cache_users_t> cache_t;
    mutable cache_t cache;
    mutable cache_t groupCache;

    typedef std::set<std::string> string_set_t;

    struct ACL {
      string_set_t users;
      string_set_t groups;
    };

    struct Group {
      string_set_t users;
    };

    typedef std::map<std::string, ACL> acls_t;
    acls_t acls;

    typedef std::map<std::string, Group> groups_t;
    groups_t groups;

    typedef string_set_t users_t;
    users_t users;

  public:
    ACLSet() : dirty(false) {}

    void clear();

    // Allow
    bool allow(const std::string &path, const std::string &user) const;
    bool allowGroup(const std::string &path, const std::string &group) const;

    // Users
    typedef users_t::const_iterator users_iterator;
    users_iterator beginUsers() const {return users.begin();}
    users_iterator endUsers() const {return users.end();}
    const std::string &get(const users_iterator &it) const {return *it;}

    bool hasUser(const std::string &user) const;
    void addUser(const std::string &user);
    void delUser(const std::string &user);

    // Groups
    typedef groups_t::const_iterator groups_iterator;
    groups_iterator beginGroups() const {return groups.begin();}
    groups_iterator endGroups() const {return groups.end();}
    const std::string &get(const groups_iterator &it) const {return it->first;}

    bool hasGroup(const std::string &group) const;
    void addGroup(const std::string &group);
    void delGroup(const std::string &group);

    bool groupHasUser(const std::string &group, const std::string &user) const;
    void groupAddUser(const std::string &group, const std::string &user);
    void groupDelUser(const std::string &group, const std::string &user);

    // ACLs
    typedef acls_t::const_iterator acls_iterator;
    acls_iterator beginACLs() const {return acls.begin();}
    acls_iterator endACLs() const {return acls.end();}
    const std::string &get(const acls_iterator &it) const {return it->first;}

    bool hasACL(const std::string &path) const;
    void addACL(const std::string &path);
    void delACL(const std::string &path);

    bool aclHasUser(const std::string &path, const std::string &user) const;
    void aclAddUser(const std::string &path, const std::string &user);
    void aclDelUser(const std::string &path, const std::string &user);

    bool aclHasGroup(const std::string &path, const std::string &group) const;
    void aclAddGroup(const std::string &path, const std::string &group);
    void aclDelGroup(const std::string &path, const std::string &group);

    // IO
    void write(JSON::Sink &sink) const;
    void set(const JSON::Value &json);

    void write(std::ostream &stream) const;
    void read(std::istream &stream);

  protected:
    bool flushDirtyCache() const;
    static std::string parentPath(const std::string &path);
    bool allowNoCache(const std::string &path, const std::string &user) const;
    bool allowGroupNoCache(const std::string &path,
                           const std::string &group) const;
  };


  inline static
  std::ostream &operator<<(std::ostream &stream, const ACLSet &aclSet) {
    aclSet.write(stream);
    return stream;
  }

  inline static
  std::istream &operator>>(std::istream &stream, ACLSet &aclSet) {
    aclSet.read(stream);
    return stream;
  }
}

#endif // CBANG_ACLSET_H
