///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "ZNamespace.h"
#include "ZException.h"
#include "ZObject.h"
#include "db.h"

namespace odb {

class ZNamespace::ZEntry
{
 public:
  const char* _name;
  ZObject*    _obj;

  ZEntry(const char* name, ZObject* object)
  {
    _name = strdup(name);
    assert(_name);
    _obj = object;
  }

  ~ZEntry() { free((void*) _name); }
};

class ZNamespace::dbEntry
{
 public:
  const char* _name;
  dbDatabase* _db;

  dbEntry(const char* name, dbDatabase* db)
  {
    _name = strdup(name);
    ZALLOCATED(_name);
    _db = db;
  }

  ~dbEntry() { free((void*) _name); }
};

ZNamespace::ZNamespace()
{
}

ZNamespace::~ZNamespace()
{
  std::map<ZObject*, ZEntry*>::iterator zitr;
  for (zitr = _zobj_entries.begin(); zitr != _zobj_entries.end(); ++zitr)
    delete (*zitr).second;

  std::map<dbDatabase*, dbEntry*>::iterator ditr;
  for (ditr = _db_entries.begin(); ditr != _db_entries.end(); ++ditr)
    delete (*ditr).second;
}

const char* ZNamespace::addZObject(ZObject* obj)
{
  ZEntry*& entry = _zobj_entries[obj];

  if (entry != NULL)
    return entry->_name;

  char buf[BUFSIZ];
  snprintf(buf, BUFSIZ, "_zobj_%d", _unique_id++);
  entry = new ZEntry(buf, obj);
  ZALLOCATED(entry);
  _zobj_names[entry->_name] = entry;
  return entry->_name;
}

void ZNamespace::removeZObject(ZObject* obj)
{
  std::map<ZObject*, ZEntry*>::iterator itr;
  itr = _zobj_entries.find(obj);

  if (itr == _zobj_entries.end())
    return;

  ZEntry* e = (*itr).second;
  _zobj_entries.erase(itr);
  _zobj_names.erase(e->_name);
  delete e;
}

ZObject* ZNamespace::resolveZObject(const char* name)
{
  std::map<const char*, ZEntry*, ltstr>::iterator itr;

  itr = _zobj_names.find(name);

  if (itr == _zobj_names.end())
    return NULL;

  ZEntry* e = (*itr).second;
  return e->_obj;
}

void ZNamespace::registerDb(dbDatabase* db)
{
  dbEntry*& e = _db_entries[db];

  if (e != NULL)
    return;

  char dbname[256];
  db->getDbName(dbname);
  e = new dbEntry(dbname, db);
  ZALLOCATED(e);
  _db_names[e->_name] = e;
}

void ZNamespace::unregisterDb(dbDatabase* db)
{
  std::map<dbDatabase*, dbEntry*>::iterator itr;

  itr = _db_entries.find(db);

  if (itr == _db_entries.end())
    return;

  dbEntry* e = (*itr).second;
  _db_entries.erase(itr);
  _db_names.erase(e->_name);
  delete e;
}

dbObject* ZNamespace::resolveDB(const char* dbname)
{
  char name[256];

  if (dbname[0] != '/' || dbname[1] != 'D')
    throw ZException("invalid database name (%s)", dbname);

  const char* p = strchr(&dbname[2], '/');

  if (p == NULL) {
    dbEntry* e = _db_names[dbname];

    if (e == NULL)
      throw ZException("cannot find database (%s)", dbname);

    return e->_db;
  }

  const char* s = dbname;
  char*       n = name;

  for (; (s != p); ++s, ++n)
    *n = *s;

  *n = '\0';

  dbEntry* e = _db_names[name];

  if (e == NULL)
    throw ZException("cannot find database (%s)", name);

  dbObject* o = dbObject::resolveDbName(e->_db, dbname);

  if (o == NULL)
    throw ZException("cannot resolve name (%s)", dbname);

  return o;
}

}  // namespace odb
