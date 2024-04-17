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

#include "dbProperty.h"

#include <spdlog/fmt/ostr.h>

#include <sstream>

#include "dbBlock.h"
#include "dbChip.h"
#include "dbDatabase.h"
#include "dbLib.h"
#include "dbName.h"
#include "dbNameCache.h"
#include "dbPropertyItr.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "odb/db.h"

namespace odb {

template class dbTable<_dbProperty>;

_dbProperty::_dbProperty(_dbDatabase*, const _dbProperty& n)
    : _flags(n._flags),
      _name(n._name),
      _next(n._next),
      _owner(n._owner),
      _value(n._value)
{
}

_dbProperty::_dbProperty(_dbDatabase*)
{
  _flags._type = DB_STRING_PROP;
  _flags._spare_bits = 0;
  _name = 0;
  _owner = 0;
}

_dbProperty::~_dbProperty()
{
}

bool _dbProperty::operator==(const _dbProperty& rhs) const
{
  if (_flags._type != rhs._flags._type) {
    return false;
  }

  if (_flags._owner_type != rhs._flags._owner_type) {
    return false;
  }

  if (_name != rhs._name) {
    return false;
  }

  if (_flags._owner_type
      != dbDatabaseObj) {  // database owners are never the same...
    if (_owner != rhs._owner) {
      return false;
    }
  }

  if (_next != rhs._next) {
    return false;
  }

  if (_value != rhs._value) {
    return false;
  }

  return true;
}

void _dbProperty::differences(dbDiff& diff,
                              const char* field,
                              const _dbProperty& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._type);
  DIFF_FIELD(_flags._owner_type);
  DIFF_FIELD(_name);

  if (_flags._owner_type
      != dbDatabaseObj)  // database owners are never the same...
  {
    DIFF_FIELD(_owner);
  }

  DIFF_FIELD(_next);
  DIFF_END
}

void _dbProperty::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._type);
  DIFF_OUT_FIELD(_flags._owner_type);
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_owner);
  DIFF_OUT_FIELD(_next);
  DIFF_END
}

dbOStream& operator<<(dbOStream& stream, const _dbProperty& prop)
{
  uint* bit_field = (uint*) &prop._flags;
  stream << *bit_field;
  stream << prop._name;
  stream << prop._next;
  stream << prop._owner;

  switch (prop._flags._type) {
    case DB_BOOL_PROP:
      // Older versions of the spec treated bools as uints
      // retain backwards compatability
      stream << static_cast<uint>(std::get<bool>(prop._value));
      break;

    case DB_INT_PROP:
      stream << std::get<int>(prop._value);
      break;

    case DB_STRING_PROP:
      stream << std::get<std::string>(prop._value).c_str();
      break;

    case DB_DOUBLE_PROP:
      stream << std::get<double>(prop._value);
      break;
  }

  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbProperty& prop)
{
  uint* bit_field = (uint*) &prop._flags;
  stream >> *bit_field;
  stream >> prop._name;
  stream >> prop._next;
  stream >> prop._owner;

  switch (prop._flags._type) {
    case DB_BOOL_PROP: {
      // Older versions of the spec treated bools as uints
      // retain backwards compatability
      uint boolean;
      stream >> boolean;
      prop._value = static_cast<bool>(boolean);
      break;
    }
    case DB_INT_PROP: {
      int integer;
      stream >> integer;
      prop._value = integer;
      break;
    }
    case DB_STRING_PROP: {
      char* char_string;
      stream >> char_string;
      prop._value = "";
      if (char_string != nullptr) {
        prop._value = std::string(char_string);
        free(char_string);
      }
      break;
    }
    case DB_DOUBLE_PROP: {
      double double_property;
      stream >> double_property;
      prop._value = double_property;
      break;
    }
  }

  return stream;
}

bool _dbProperty::operator<(const _dbProperty& rhs) const
{
  return _name < rhs._name;
}

dbPropertyItr* _dbProperty::getItr(dbObject* object)
{
next_object:
  switch (object->getObjectType()) {
    case dbDatabaseObj: {
      _dbDatabase* db = (_dbDatabase*) object;
      return db->_prop_itr;
    }

    case dbChipObj: {
      _dbChip* chip = (_dbChip*) object;
      return chip->_prop_itr;
    }

    case dbBlockObj: {
      _dbBlock* blk = (_dbBlock*) object;
      return blk->_prop_itr;
    }

    case dbLibObj: {
      _dbLib* lib = (_dbLib*) object;
      return lib->_prop_itr;
    }

    case dbTechObj: {
      _dbTech* tech = (_dbTech*) object;
      return tech->_prop_itr;
    }

    default:
      object = object->getImpl()->getOwner();
      goto next_object;
  }

  assert(0);
  return nullptr;
}

_dbNameCache* _dbProperty::getNameCache(dbObject* object)
{
next_object:
  switch (object->getObjectType()) {
    case dbDatabaseObj: {
      _dbDatabase* db = (_dbDatabase*) object;
      return db->_name_cache;
    }

    case dbChipObj: {
      _dbChip* chip = (_dbChip*) object;
      return chip->_name_cache;
    }

    case dbBlockObj: {
      _dbBlock* blk = (_dbBlock*) object;
      return blk->_name_cache;
    }

    case dbLibObj: {
      _dbLib* lib = (_dbLib*) object;
      return lib->_name_cache;
    }

    case dbTechObj: {
      _dbTech* tech = (_dbTech*) object;
      return tech->_name_cache;
    }

    default:
      object = object->getImpl()->getOwner();
      goto next_object;
  }

  assert(0);
  return nullptr;
}

dbTable<_dbProperty>* _dbProperty::getPropTable(dbObject* object)
{
next_object:
  switch (object->getObjectType()) {
    case dbDatabaseObj: {
      _dbDatabase* db = (_dbDatabase*) object;
      return db->_prop_tbl;
    }

    case dbChipObj: {
      _dbChip* chip = (_dbChip*) object;
      return chip->_prop_tbl;
    }

    case dbBlockObj: {
      _dbBlock* blk = (_dbBlock*) object;
      return blk->_prop_tbl;
    }

    case dbLibObj: {
      _dbLib* lib = (_dbLib*) object;
      return lib->_prop_tbl;
    }

    case dbTechObj: {
      _dbTech* tech = (_dbTech*) object;
      return tech->_prop_tbl;
    }

    default:
      object = object->getImpl()->getOwner();
      goto next_object;
  }

  assert(0);
  return nullptr;
}

_dbProperty* _dbProperty::createProperty(dbObject* object_,
                                         const char* name,
                                         _PropTypeEnum type)
{
  _dbObject* object = (_dbObject*) object_;
  dbTable<_dbProperty>* propTable = getPropTable(object);

  // Create property
  _dbProperty* prop = propTable->create();
  uint oid = object->getOID();
  prop->_flags._type = type;
  prop->_flags._owner_type = object->getType();
  prop->_owner = oid;

  // Get name-id, increment reference count
  _dbNameCache* cache = getNameCache(object);
  uint name_id = cache->addName(name);
  prop->_name = name_id;

  // Link property into owner's prop-list
  dbObjectTable* table = object->getTable();
  dbId<_dbProperty> propList = table->getPropList(oid);
  prop->_next = propList;
  propList = prop->getImpl()->getOID();
  table->setPropList(oid, propList);
  return prop;
}

/////////////////////////////////////////////
// property
/////////////////////////////////////////////

dbProperty::Type dbProperty::getType()
{
  _dbProperty* prop = (_dbProperty*) this;
  return (dbProperty::Type) prop->_flags._type;
}

std::string dbProperty::getName()
{
  _dbProperty* prop = (_dbProperty*) this;
  _dbNameCache* cache = _dbProperty::getNameCache(this);
  const char* name = cache->getName(prop->_name);
  return name;
}

dbObject* dbProperty::getPropOwner()
{
  _dbProperty* prop = (_dbProperty*) this;
  dbObjectTable* table = prop->getTable()->getObjectTable(
      (dbObjectType) prop->_flags._owner_type);
  return table->getObject(prop->_owner);
}

dbProperty* dbProperty::find(dbObject* object, const char* name)
{
  _dbNameCache* cache = _dbProperty::getNameCache(object);

  uint name_id = cache->findName(name);

  if (name_id == 0) {
    return nullptr;
  }

  dbSet<dbProperty> props = getProperties(object);

  dbSet<dbProperty>::iterator itr;

  for (itr = props.begin(); itr != props.end(); ++itr) {
    _dbProperty* p = (_dbProperty*) *itr;

    if (p->_name == name_id) {
      return (dbProperty*) p;
    }
  }

  return nullptr;
}

dbProperty* dbProperty::find(dbObject* object, const char* name, Type type)
{
  _dbNameCache* cache = _dbProperty::getNameCache(object);

  uint name_id = cache->findName(name);

  if (name_id == 0) {
    return nullptr;
  }

  dbSet<dbProperty> props = getProperties(object);

  dbSet<dbProperty>::iterator itr;

  for (itr = props.begin(); itr != props.end(); ++itr) {
    _dbProperty* p = (_dbProperty*) *itr;

    if ((p->_name == name_id) && (p->_flags._type == (_PropTypeEnum) type)) {
      return (dbProperty*) p;
    }
  }

  return nullptr;
}

dbSet<dbProperty> dbProperty::getProperties(dbObject* object)
{
  dbSet<dbProperty> props(object, _dbProperty::getItr(object));
  return props;
}

void dbProperty::destroy(dbProperty* prop_)
{
  _dbProperty* prop = (_dbProperty*) prop_;

  // unlink property from owner
  dbTable<_dbProperty>* propTable = _dbProperty::getPropTable(prop);
  dbObjectTable* ownerTable = prop->getTable()->getObjectTable(
      (dbObjectType) prop->_flags._owner_type);

  dbId<_dbProperty> propList = ownerTable->getPropList(prop->_owner);
  dbId<_dbProperty> cur = propList;
  uint oid = prop->getOID();

  while (cur) {
    _dbProperty* p = propTable->getPtr(cur);

    if (cur == oid) {
      if (cur == propList) {
        ownerTable->setPropList(prop->_owner, p->_next);
      } else {
        p->_next = prop->_next;
      }

      break;
    }

    cur = p->_next;
  }

  // Remove reference to name
  _dbNameCache* cache = _dbProperty::getNameCache(prop);
  cache->removeName(prop->_name);

  // destroy hier. props.
  dbProperty::destroyProperties(prop);
  // destroy the prop
  propTable->destroy(prop);
}

void dbProperty::destroyProperties(dbObject* obj)
{
  _dbObject* object = obj->getImpl();
  uint oid = object->getOID();
  dbObjectTable* objTable = object->getTable();
  dbId<_dbProperty> cur = objTable->getPropList(oid);

  if (!cur) {
    return;
  }

  _dbNameCache* cache = _dbProperty::getNameCache(obj);
  dbTable<_dbProperty>* propTable = _dbProperty::getPropTable(obj);
  while (cur) {
    _dbProperty* p = propTable->getPtr(cur);
    cache->removeName(p->_name);
    cur = p->_next;
    dbProperty::destroyProperties(p);
    propTable->destroy(p);
  }

  objTable->setPropList(oid, cur);
}

dbSet<dbProperty>::iterator dbProperty::destroy(dbSet<dbProperty>::iterator itr)
{
  dbProperty* p = *itr;
  dbSet<dbProperty>::iterator n = ++itr;
  dbProperty::destroy(p);
  return n;
}

/////////////////////////////////////////////
// bool property
/////////////////////////////////////////////

bool dbBoolProperty::getValue()
{
  _dbProperty* prop = (_dbProperty*) this;
  return std::get<bool>(prop->_value);
}

void dbBoolProperty::setValue(bool value)
{
  _dbProperty* prop = (_dbProperty*) this;
  prop->_value = value;
}

dbBoolProperty* dbBoolProperty::create(dbObject* object,
                                       const char* name,
                                       bool value)
{
  if (find(object, name)) {
    return nullptr;
  }

  _dbProperty* prop = _dbProperty::createProperty(object, name, DB_BOOL_PROP);
  prop->_value = value;
  return (dbBoolProperty*) prop;
}

dbBoolProperty* dbBoolProperty::find(dbObject* object, const char* name)
{
  return (dbBoolProperty*) dbProperty::find(
      object, name, dbProperty::BOOL_PROP);
}

/////////////////////////////////////////////
// string property
/////////////////////////////////////////////

std::string dbStringProperty::getValue()
{
  _dbProperty* prop = (_dbProperty*) this;
  return std::get<std::string>(prop->_value);
}

void dbStringProperty::setValue(const char* value)
{
  _dbProperty* prop = (_dbProperty*) this;
  assert(value);
  prop->_value = std::string(value);
}

dbStringProperty* dbStringProperty::create(dbObject* object,
                                           const char* name,
                                           const char* value)
{
  _dbProperty* prop = (_dbProperty*) find(object, name);
  if (prop) {
    prop->_value = std::get<std::string>(prop->_value) + " " + value;
    return (dbStringProperty*) prop;
  }

  prop = _dbProperty::createProperty(object, name, DB_STRING_PROP);
  prop->_value = std::string(value);
  return (dbStringProperty*) prop;
}

dbStringProperty* dbStringProperty::find(dbObject* object, const char* name)
{
  return (dbStringProperty*) dbProperty::find(
      object, name, dbProperty::STRING_PROP);
}

/////////////////////////////////////////////
// int property
/////////////////////////////////////////////

int dbIntProperty::getValue()
{
  _dbProperty* prop = (_dbProperty*) this;
  return std::get<int>(prop->_value);
}

void dbIntProperty::setValue(int value)
{
  _dbProperty* prop = (_dbProperty*) this;
  prop->_value = value;
}

dbIntProperty* dbIntProperty::create(dbObject* object,
                                     const char* name,
                                     int value)
{
  if (find(object, name)) {
    return nullptr;
  }

  _dbProperty* prop = _dbProperty::createProperty(object, name, DB_INT_PROP);
  prop->_value = value;
  return (dbIntProperty*) prop;
}

dbIntProperty* dbIntProperty::find(dbObject* object, const char* name)
{
  return (dbIntProperty*) dbProperty::find(object, name, dbProperty::INT_PROP);
}

/////////////////////////////////////////////
// double property
/////////////////////////////////////////////

double dbDoubleProperty::getValue()
{
  _dbProperty* prop = (_dbProperty*) this;
  return std::get<double>(prop->_value);
}

void dbDoubleProperty::setValue(double value)
{
  _dbProperty* prop = (_dbProperty*) this;
  prop->_value = value;
}

dbDoubleProperty* dbDoubleProperty::create(dbObject* object,
                                           const char* name,
                                           double value)
{
  if (find(object, name)) {
    return nullptr;
  }

  _dbProperty* prop = _dbProperty::createProperty(object, name, DB_DOUBLE_PROP);
  prop->_value = value;
  return (dbDoubleProperty*) prop;
}

dbDoubleProperty* dbDoubleProperty::find(dbObject* object, const char* name)
{
  return (dbDoubleProperty*) dbProperty::find(
      object, name, dbProperty::DOUBLE_PROP);
}

std::string dbProperty::writePropValue(dbProperty* prop)
{
  switch (prop->getType()) {
    case dbProperty::STRING_PROP: {
      dbStringProperty* p = (dbStringProperty*) prop;
      std::string v = p->getValue();
      return fmt::format("\"{}\" ", v);
    }

    case dbProperty::INT_PROP: {
      dbIntProperty* p = (dbIntProperty*) prop;
      int v = p->getValue();
      return fmt::format("{} ", v);
    }

    case dbProperty::DOUBLE_PROP: {
      dbDoubleProperty* p = (dbDoubleProperty*) prop;
      double v = p->getValue();
      return fmt::format("{:G} ", v);
    }
    default:
      return "";
  }
}

std::string dbProperty::writeProperties(dbObject* object)
{
  dbSet<dbProperty> props = dbProperty::getProperties(object);
  dbSet<dbProperty>::iterator itr;
  std::ostringstream out;

  for (itr = props.begin(); itr != props.end(); ++itr) {
    dbProperty* prop = *itr;
    std::string name = prop->getName();
    fmt::print(out, "    PROPERTY {} {};\n", name, writePropValue(prop));
  }

  return out.str();
}

/* Sample Code to access dbTechLayer properties
void dbProperty::writeProperties( dbTechLayer * object, FILE *out )
{
    dbSet<dbProperty> props = dbProperty::getProperties(object);
    dbSet<dbProperty>::iterator itr;

    for( itr = props.begin(); itr != props.end(); ++itr )
    {
        dbProperty * prop = *itr;
        std::string name = prop->getName();

        to get value of a  string type:
             dbStringProperty * p = (dbStringProperty *) prop;
            std::string v = p->getValue();
        look function dbProperty::writePropValue on how to retrieve int and
double values
    }
}
*/

}  // namespace odb
