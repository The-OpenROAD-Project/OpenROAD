// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbProperty.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <variant>

#include "dbBlock.h"
#include "dbChip.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbLib.h"
#include "dbName.h"
#include "dbNameCache.h"
#include "dbPropertyItr.h"
#include "dbTable.h"
#include "dbTech.h"
#include "odb/db.h"
// User Code Begin Includes
#include <cassert>
#include <cstdlib>
#include <sstream>

#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "spdlog/fmt/ostr.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbProperty>;

bool _dbProperty::operator==(const _dbProperty& rhs) const
{
  if (flags_.owner_type != rhs.flags_.owner_type) {
    return false;
  }
  if (name_ != rhs.name_) {
    return false;
  }
  if (next_ != rhs.next_) {
    return false;
  }

  // User Code Begin ==
  if (flags_.owner_type
      != dbDatabaseObj) {  // database owners are never the same...
    if (owner_ != rhs.owner_) {
      return false;
    }
  }
  if (value_ != rhs.value_) {
    return false;
  }
  // User Code End ==
  return true;
}

bool _dbProperty::operator<(const _dbProperty& rhs) const
{
  // User Code Begin <
  if (name_ >= rhs.name_) {
    return false;
  }
  // User Code End <
  return true;
}

_dbProperty::_dbProperty(_dbDatabase* db)
{
  flags_ = {};
  owner_ = 0;
  // User Code Begin Constructor
  flags_.type = kDbStringProp;
  name_ = 0;
  // User Code End Constructor
}

dbIStream& operator>>(dbIStream& stream, _dbProperty& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.name_;
  stream >> obj.next_;
  stream >> obj.owner_;
  // User Code Begin >>
  switch (obj.flags_.type) {
    case kDbBoolProp: {
      // Older versions of the spec treated bools as uints
      // retain backwards compatability
      uint32_t boolean;
      stream >> boolean;
      obj.value_ = static_cast<bool>(boolean);
      break;
    }
    case kDbIntProp: {
      int integer;
      stream >> integer;
      obj.value_ = integer;
      break;
    }
    case kDbStringProp: {
      char* char_string;
      stream >> char_string;
      obj.value_ = "";
      if (char_string != nullptr) {
        obj.value_ = std::string(char_string);
        free(char_string);
      }
      break;
    }
    case kDbDoubleProp: {
      double double_property;
      stream >> double_property;
      obj.value_ = double_property;
      break;
    }
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbProperty& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.name_;
  stream << obj.next_;
  stream << obj.owner_;
  // User Code Begin <<
  switch (obj.flags_.type) {
    case kDbBoolProp:
      // Older versions of the spec treated bools as uints
      // retain backwards compatability
      stream << static_cast<uint32_t>(std::get<bool>(obj.value_));
      break;

    case kDbIntProp:
      stream << std::get<int>(obj.value_);
      break;

    case kDbStringProp:
      stream << std::get<std::string>(obj.value_).c_str();
      break;

    case kDbDoubleProp:
      stream << std::get<double>(obj.value_);
      break;
  }
  // User Code End <<
  return stream;
}

void _dbProperty::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  if (flags_.type == kDbStringProp) {
    info.children["string"].add(std::get<std::string>(value_));
  }
  // User Code End collectMemInfo
}

// User Code Begin PrivateMethods
_dbProperty::_dbProperty(_dbDatabase*, const _dbProperty& n)
    : flags_(n.flags_),
      name_(n.name_),
      next_(n.next_),
      owner_(n.owner_),
      value_(n.value_)
{
}
dbPropertyItr* _dbProperty::getItr(dbObject* object)
{
next_object:
  switch (object->getObjectType()) {
    case dbDatabaseObj: {
      _dbDatabase* db = (_dbDatabase*) object;
      return db->prop_itr_;
    }

    case dbChipObj: {
      _dbChip* chip = (_dbChip*) object;
      return chip->prop_itr_;
    }

    case dbBlockObj: {
      _dbBlock* blk = (_dbBlock*) object;
      return blk->prop_itr_;
    }

    case dbLibObj: {
      _dbLib* lib = (_dbLib*) object;
      return lib->prop_itr_;
    }

    case dbTechObj: {
      _dbTech* tech = (_dbTech*) object;
      return tech->prop_itr_;
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
      return db->name_cache_;
    }

    case dbChipObj: {
      _dbChip* chip = (_dbChip*) object;
      return chip->name_cache_;
    }

    case dbBlockObj: {
      _dbBlock* blk = (_dbBlock*) object;
      return blk->name_cache_;
    }

    case dbLibObj: {
      _dbLib* lib = (_dbLib*) object;
      return lib->name_cache_;
    }

    case dbTechObj: {
      _dbTech* tech = (_dbTech*) object;
      return tech->name_cache_;
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
      return db->prop_tbl_;
    }

    case dbChipObj: {
      _dbChip* chip = (_dbChip*) object;
      return chip->prop_tbl_;
    }

    case dbBlockObj: {
      _dbBlock* blk = (_dbBlock*) object;
      return blk->prop_tbl_;
    }

    case dbLibObj: {
      _dbLib* lib = (_dbLib*) object;
      return lib->prop_tbl_;
    }

    case dbTechObj: {
      _dbTech* tech = (_dbTech*) object;
      return tech->prop_tbl_;
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
                                         PropTypeEnum type)
{
  _dbObject* object = (_dbObject*) object_;
  dbTable<_dbProperty>* propTable = getPropTable(object);

  // Create property
  _dbProperty* prop = propTable->create();
  uint32_t oid = object->getOID();
  prop->flags_.type = type;
  prop->flags_.owner_type = object->getType();
  prop->owner_ = oid;

  // Get name-id, increment reference count
  _dbNameCache* cache = getNameCache(object);
  uint32_t name_id = cache->addName(name);
  prop->name_ = name_id;

  // Link property into owner's prop-list
  dbObjectTable* table = object->getTable();
  dbId<_dbProperty> propList = table->getPropList(oid);
  prop->next_ = propList;
  propList = prop->getImpl()->getOID();
  table->setPropList(oid, propList);
  return prop;
}
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbProperty - Methods
//
////////////////////////////////////////////////////////////////////

// User Code Begin dbPropertyPublicMethods

dbProperty::Type dbProperty::getType()
{
  _dbProperty* prop = (_dbProperty*) this;
  return (dbProperty::Type) prop->flags_.type;
}

std::string dbProperty::getName()
{
  _dbProperty* prop = (_dbProperty*) this;
  _dbNameCache* cache = _dbProperty::getNameCache(this);
  const char* name = cache->getName(prop->name_);
  return name;
}

dbObject* dbProperty::getPropOwner()
{
  _dbProperty* prop = (_dbProperty*) this;
  dbObjectTable* table = prop->getTable()->getObjectTable(
      (dbObjectType) prop->flags_.owner_type);
  return table->getObject(prop->owner_);
}

dbProperty* dbProperty::find(dbObject* object, const char* name)
{
  _dbNameCache* cache = _dbProperty::getNameCache(object);

  uint32_t name_id = cache->findName(name);

  if (name_id == 0) {
    return nullptr;
  }

  for (dbProperty* p : getProperties(object)) {
    _dbProperty* p_impl = (_dbProperty*) p;

    if (p_impl->name_ == name_id) {
      return p;
    }
  }

  return nullptr;
}

dbProperty* dbProperty::find(dbObject* object, const char* name, Type type)
{
  _dbNameCache* cache = _dbProperty::getNameCache(object);

  uint32_t name_id = cache->findName(name);

  if (name_id == 0) {
    return nullptr;
  }

  for (dbProperty* p : getProperties(object)) {
    _dbProperty* p_impl = (_dbProperty*) p;

    if ((p_impl->name_ == name_id)
        && (p_impl->flags_.type == (PropTypeEnum) type)) {
      return p;
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
      (dbObjectType) prop->flags_.owner_type);

  dbId<_dbProperty> propList = ownerTable->getPropList(prop->owner_);
  dbId<_dbProperty> cur = propList;
  _dbProperty* prev = nullptr;
  uint32_t oid = prop->getOID();

  while (cur) {
    _dbProperty* p = propTable->getPtr(cur);

    if (cur == oid) {
      if (cur == propList) {
        ownerTable->setPropList(prop->owner_, p->next_);
      } else {
        prev->next_ = prop->next_;
      }

      break;
    }

    prev = p;
    cur = p->next_;
  }

  // Remove reference to name
  _dbNameCache* cache = _dbProperty::getNameCache(prop);
  cache->removeName(prop->name_);

  // destroy hier. props.
  dbProperty::destroyProperties(prop);
  // destroy the prop
  propTable->destroy(prop);
}

void dbProperty::destroyProperties(dbObject* obj)
{
  _dbObject* object = obj->getImpl();
  uint32_t oid = object->getOID();
  dbObjectTable* objTable = object->getTable();
  dbId<_dbProperty> cur = objTable->getPropList(oid);

  if (!cur) {
    return;
  }

  _dbNameCache* cache = _dbProperty::getNameCache(obj);
  dbTable<_dbProperty>* propTable = _dbProperty::getPropTable(obj);
  while (cur) {
    _dbProperty* p = propTable->getPtr(cur);
    cache->removeName(p->name_);
    cur = p->next_;
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
  return std::get<bool>(prop->value_);
}

void dbBoolProperty::setValue(bool value)
{
  _dbProperty* prop = (_dbProperty*) this;
  prop->value_ = value;
}

dbBoolProperty* dbBoolProperty::create(dbObject* object,
                                       const char* name,
                                       bool value)
{
  if (find(object, name)) {
    return nullptr;
  }

  _dbProperty* prop = _dbProperty::createProperty(object, name, kDbBoolProp);
  prop->value_ = value;
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
  return std::get<std::string>(prop->value_);
}

void dbStringProperty::setValue(const char* value)
{
  _dbProperty* prop = (_dbProperty*) this;
  assert(value);
  prop->value_ = std::string(value);
}

dbStringProperty* dbStringProperty::create(dbObject* object,
                                           const char* name,
                                           const char* value)
{
  _dbProperty* prop = (_dbProperty*) find(object, name);
  if (prop) {
    prop->value_ = std::get<std::string>(prop->value_) + " " + value;
    return (dbStringProperty*) prop;
  }

  prop = _dbProperty::createProperty(object, name, kDbStringProp);
  prop->value_ = std::string(value);
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
  return std::get<int>(prop->value_);
}

void dbIntProperty::setValue(int value)
{
  _dbProperty* prop = (_dbProperty*) this;
  prop->value_ = value;
}

dbIntProperty* dbIntProperty::create(dbObject* object,
                                     const char* name,
                                     int value)
{
  if (find(object, name)) {
    return nullptr;
  }

  _dbProperty* prop = _dbProperty::createProperty(object, name, kDbIntProp);
  prop->value_ = value;
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
  return std::get<double>(prop->value_);
}

void dbDoubleProperty::setValue(double value)
{
  _dbProperty* prop = (_dbProperty*) this;
  prop->value_ = value;
}

dbDoubleProperty* dbDoubleProperty::create(dbObject* object,
                                           const char* name,
                                           double value)
{
  if (find(object, name)) {
    return nullptr;
  }

  _dbProperty* prop = _dbProperty::createProperty(object, name, kDbDoubleProp);
  prop->value_ = value;
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
  std::ostringstream out;

  for (dbProperty* prop : dbProperty::getProperties(object)) {
    std::string name = prop->getName();
    fmt::print(out, "    PROPERTY {} {};\n", name, writePropValue(prop));
  }

  return out.str();
}
// User Code End dbPropertyPublicMethods
}  // namespace odb
// Generator Code End Cpp
