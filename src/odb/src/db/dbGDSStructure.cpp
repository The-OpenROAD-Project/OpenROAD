// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGDSStructure.h"

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbGDSARef.h"
#include "dbGDSBoundary.h"
#include "dbGDSBox.h"
#include "dbGDSLib.h"
#include "dbGDSPath.h"
#include "dbGDSSRef.h"
#include "dbGDSText.h"
#include "dbHashTable.hpp"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
// User Code Begin Includes
#include "dbCommon.h"
#include "odb/dbObject.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbGDSStructure>;

bool _dbGDSStructure::operator==(const _dbGDSStructure& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (next_entry_ != rhs.next_entry_) {
    return false;
  }
  if (*boundaries_ != *rhs.boundaries_) {
    return false;
  }
  if (*boxes_ != *rhs.boxes_) {
    return false;
  }
  if (*paths_ != *rhs.paths_) {
    return false;
  }
  if (*srefs_ != *rhs.srefs_) {
    return false;
  }
  if (*arefs_ != *rhs.arefs_) {
    return false;
  }
  if (*texts_ != *rhs.texts_) {
    return false;
  }

  return true;
}

bool _dbGDSStructure::operator<(const _dbGDSStructure& rhs) const
{
  return true;
}

_dbGDSStructure::_dbGDSStructure(_dbDatabase* db)
{
  name_ = nullptr;
  boundaries_ = new dbTable<_dbGDSBoundary>(
      db,
      this,
      (GetObjTbl_t) &_dbGDSStructure::getObjectTable,
      dbGDSBoundaryObj);
  boxes_ = new dbTable<_dbGDSBox>(
      db, this, (GetObjTbl_t) &_dbGDSStructure::getObjectTable, dbGDSBoxObj);
  paths_ = new dbTable<_dbGDSPath>(
      db, this, (GetObjTbl_t) &_dbGDSStructure::getObjectTable, dbGDSPathObj);
  srefs_ = new dbTable<_dbGDSSRef>(
      db, this, (GetObjTbl_t) &_dbGDSStructure::getObjectTable, dbGDSSRefObj);
  arefs_ = new dbTable<_dbGDSARef>(
      db, this, (GetObjTbl_t) &_dbGDSStructure::getObjectTable, dbGDSARefObj);
  texts_ = new dbTable<_dbGDSText>(
      db, this, (GetObjTbl_t) &_dbGDSStructure::getObjectTable, dbGDSTextObj);
}

dbIStream& operator>>(dbIStream& stream, _dbGDSStructure& obj)
{
  stream >> obj.name_;
  stream >> obj.next_entry_;
  stream >> *obj.boundaries_;
  stream >> *obj.boxes_;
  stream >> *obj.paths_;
  stream >> *obj.srefs_;
  stream >> *obj.arefs_;
  stream >> *obj.texts_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSStructure& obj)
{
  stream << obj.name_;
  stream << obj.next_entry_;
  stream << *obj.boundaries_;
  stream << *obj.boxes_;
  stream << *obj.paths_;
  stream << *obj.srefs_;
  stream << *obj.arefs_;
  stream << *obj.texts_;
  return stream;
}

dbObjectTable* _dbGDSStructure::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbGDSBoundaryObj:
      return boundaries_;
    case dbGDSBoxObj:
      return boxes_;
    case dbGDSPathObj:
      return paths_;
    case dbGDSSRefObj:
      return srefs_;
    case dbGDSARefObj:
      return arefs_;
    case dbGDSTextObj:
      return texts_;
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}
void _dbGDSStructure::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  boundaries_->collectMemInfo(info.children["boundaries_"]);

  boxes_->collectMemInfo(info.children["boxes_"]);

  paths_->collectMemInfo(info.children["paths_"]);

  srefs_->collectMemInfo(info.children["srefs_"]);

  arefs_->collectMemInfo(info.children["arefs_"]);

  texts_->collectMemInfo(info.children["texts_"]);

  // User Code Begin collectMemInfo
  info.children["name"].add(name_);
  // User Code End collectMemInfo
}

_dbGDSStructure::~_dbGDSStructure()
{
  delete boundaries_;
  delete boxes_;
  delete paths_;
  delete srefs_;
  delete arefs_;
  delete texts_;
}

////////////////////////////////////////////////////////////////////
//
// dbGDSStructure - Methods
//
////////////////////////////////////////////////////////////////////

char* dbGDSStructure::getName() const
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;
  return obj->name_;
}

dbSet<dbGDSBoundary> dbGDSStructure::getGDSBoundaries() const
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;
  return dbSet<dbGDSBoundary>(obj, obj->boundaries_);
}

dbSet<dbGDSBox> dbGDSStructure::getGDSBoxs() const
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;
  return dbSet<dbGDSBox>(obj, obj->boxes_);
}

dbSet<dbGDSPath> dbGDSStructure::getGDSPaths() const
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;
  return dbSet<dbGDSPath>(obj, obj->paths_);
}

dbSet<dbGDSSRef> dbGDSStructure::getGDSSRefs() const
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;
  return dbSet<dbGDSSRef>(obj, obj->srefs_);
}

dbSet<dbGDSARef> dbGDSStructure::getGDSARefs() const
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;
  return dbSet<dbGDSARef>(obj, obj->arefs_);
}

dbSet<dbGDSText> dbGDSStructure::getGDSTexts() const
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;
  return dbSet<dbGDSText>(obj, obj->texts_);
}

// User Code Begin dbGDSStructurePublicMethods

dbGDSStructure* dbGDSStructure::create(dbGDSLib* lib_, const char* name_)
{
  if (lib_->findGDSStructure(name_)) {
    return nullptr;
  }

  _dbGDSLib* lib = (_dbGDSLib*) lib_;
  _dbGDSStructure* structure = lib->gdsstructure_tbl_->create();
  structure->name_ = safe_strdup(name_);

  // TODO: ID for structure

  lib->gdsstructure_hash_.insert(structure);
  return (dbGDSStructure*) structure;
}

void dbGDSStructure::destroy(dbGDSStructure* structure)
{
  _dbGDSStructure* str_impl = (_dbGDSStructure*) structure;
  _dbGDSLib* lib = (_dbGDSLib*) structure->getGDSLib();
  lib->gdsstructure_hash_.remove(str_impl);
  lib->gdsstructure_tbl_->destroy(str_impl);
}

dbGDSLib* dbGDSStructure::getGDSLib()
{
  return (dbGDSLib*) getImpl()->getOwner();
}

// User Code End dbGDSStructurePublicMethods
}  // namespace odb
   // Generator Code End Cpp
