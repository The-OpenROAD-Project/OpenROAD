// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbLib.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.h"
#include "dbHashTable.hpp"
#include "dbMaster.h"
#include "dbNameCache.h"
#include "dbProperty.h"
#include "dbPropertyItr.h"
#include "dbSite.h"
#include "dbTable.h"
#include "dbTech.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbStream.h"
#include "odb/dbTransform.h"

namespace odb {

template class dbTable<_dbLib>;
template class dbHashTable<_dbMaster>;
template class dbHashTable<_dbSite>;

bool _dbLib::operator==(const _dbLib& rhs) const
{
  if (lef_units_ != rhs.lef_units_) {
    return false;
  }

  if (dbu_per_micron_ != rhs.dbu_per_micron_) {
    return false;
  }

  if (hier_delimiter_ != rhs.hier_delimiter_) {
    return false;
  }

  if (left_bus_delimiter_ != rhs.left_bus_delimiter_) {
    return false;
  }

  if (right_bus_delimiter_ != rhs.right_bus_delimiter_) {
    return false;
  }

  if (spare_ != rhs.spare_) {
    return false;
  }

  if (name_ && rhs.name_) {
    if (strcmp(name_, rhs.name_) != 0) {
      return false;
    }
  } else if (name_ || rhs.name_) {
    return false;
  }

  if (master_hash_ != rhs.master_hash_) {
    return false;
  }

  if (site_hash_ != rhs.site_hash_) {
    return false;
  }

  if (*master_tbl_ != *rhs.master_tbl_) {
    return false;
  }

  if (*site_tbl_ != *rhs.site_tbl_) {
    return false;
  }

  if (*prop_tbl_ != *rhs.prop_tbl_) {
    return false;
  }

  if (tech_ != rhs.tech_) {
    return false;
  }

  if (*name_cache_ != *rhs.name_cache_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// _dbLib - Methods
//
////////////////////////////////////////////////////////////////////
_dbLib::_dbLib(_dbDatabase* db)
{
  lef_units_ = 0;
  dbu_per_micron_ = 1000;
  hier_delimiter_ = '/';
  left_bus_delimiter_ = 0;
  right_bus_delimiter_ = 0;
  spare_ = 0;
  name_ = nullptr;

  master_tbl_ = new dbTable<_dbMaster>(
      db, this, (GetObjTbl_t) &_dbLib::getObjectTable, dbMasterObj);

  site_tbl_ = new dbTable<_dbSite>(
      db, this, (GetObjTbl_t) &_dbLib::getObjectTable, dbSiteObj);

  prop_tbl_ = new dbTable<_dbProperty>(
      db, this, (GetObjTbl_t) &_dbLib::getObjectTable, dbPropertyObj);

  name_cache_
      = new _dbNameCache(db, this, (GetObjTbl_t) &_dbLib::getObjectTable);

  prop_itr_ = new dbPropertyItr(prop_tbl_);

  master_hash_.setTable(master_tbl_);
  site_hash_.setTable(site_tbl_);
}

_dbLib::~_dbLib()
{
  delete master_tbl_;
  delete site_tbl_;
  delete prop_tbl_;
  delete name_cache_;
  delete prop_itr_;

  if (name_) {
    free((void*) name_);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbLib& lib)
{
  dbOStreamScope scope(stream, fmt::format("dbLib({})", lib.name_));
  stream << lib.lef_units_;
  stream << lib.dbu_per_micron_;
  stream << lib.hier_delimiter_;
  stream << lib.left_bus_delimiter_;
  stream << lib.right_bus_delimiter_;
  stream << lib.spare_;
  stream << lib.name_;
  stream << lib.master_hash_;
  stream << lib.site_hash_;
  stream << lib.tech_;
  stream << NamedTable("master_tbl", lib.master_tbl_);
  stream << NamedTable("site_tbl", lib.site_tbl_);
  stream << NamedTable("prop_tbl", lib.prop_tbl_);
  stream << *lib.name_cache_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbLib& lib)
{
  stream >> lib.lef_units_;
  stream >> lib.dbu_per_micron_;
  stream >> lib.hier_delimiter_;
  stream >> lib.left_bus_delimiter_;
  stream >> lib.right_bus_delimiter_;
  stream >> lib.spare_;
  stream >> lib.name_;
  stream >> lib.master_hash_;
  stream >> lib.site_hash_;
  // In the older schema we can't set the tech here, we handle this later in
  // dbDatabase.
  if (lib.getDatabase()->isSchema(kSchemaBlockTech)) {
    stream >> lib.tech_;
  }
  stream >> *lib.master_tbl_;
  stream >> *lib.site_tbl_;
  stream >> *lib.prop_tbl_;
  stream >> *lib.name_cache_;

  return stream;
}

dbObjectTable* _dbLib::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbMasterObj:
      return master_tbl_;
    case dbSiteObj:
      return site_tbl_;
    case dbPropertyObj:
      return prop_tbl_;
    default:
      break;
  }

  return getTable()->getObjectTable(type);
}

////////////////////////////////////////////////////////////////////
//
// dbLib - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbLib::getName()
{
  _dbLib* lib = (_dbLib*) this;
  return lib->name_;
}

const char* dbLib::getConstName()
{
  _dbLib* lib = (_dbLib*) this;
  return lib->name_;
}

dbTech* dbLib::getTech()
{
  _dbLib* lib = (_dbLib*) this;
  return (dbTech*) lib->getTech();
}

_dbTech* _dbLib::getTech()
{
  _dbDatabase* db = getDatabase();
  return db->tech_tbl_->getPtr(tech_);
}

int dbLib::getDbUnitsPerMicron()
{
  _dbLib* lib = (_dbLib*) this;
  return lib->dbu_per_micron_;
}

dbSet<dbMaster> dbLib::getMasters()
{
  _dbLib* lib = (_dbLib*) this;
  return dbSet<dbMaster>(lib, lib->master_tbl_);
}

dbMaster* dbLib::findMaster(const char* name)
{
  _dbLib* lib = (_dbLib*) this;
  return (dbMaster*) lib->master_hash_.find(name);
}

dbSet<dbSite> dbLib::getSites()
{
  _dbLib* lib = (_dbLib*) this;
  return dbSet<dbSite>(lib, lib->site_tbl_);
}

dbSite* dbLib::findSite(const char* name)
{
  _dbLib* lib = (_dbLib*) this;
  return (dbSite*) lib->site_hash_.find(name);
}

int dbLib::getLefUnits()
{
  _dbLib* lib = (_dbLib*) this;
  return lib->lef_units_;
}

void dbLib::setLefUnits(int units)
{
  _dbLib* lib = (_dbLib*) this;
  lib->lef_units_ = units;
}

char dbLib::getHierarchyDelimiter() const
{
  _dbLib* lib = (_dbLib*) this;
  return lib->hier_delimiter_;
}

void dbLib::setBusDelimiters(char left, char right)
{
  _dbLib* lib = (_dbLib*) this;
  lib->left_bus_delimiter_ = left;
  lib->right_bus_delimiter_ = right;
}

void dbLib::getBusDelimiters(char& left, char& right)
{
  _dbLib* lib = (_dbLib*) this;
  left = lib->left_bus_delimiter_;
  right = lib->right_bus_delimiter_;
}

dbLib* dbLib::create(dbDatabase* db_,
                     const char* name,
                     dbTech* tech,
                     char hier_delimiter)
{
  if (db_->findLib(name)) {
    return nullptr;
  }

  if (tech == nullptr) {
    return nullptr;
  }
  _dbDatabase* db = (_dbDatabase*) db_;
  _dbLib* lib = db->lib_tbl_->create();
  lib->name_ = safe_strdup(name);
  lib->hier_delimiter_ = hier_delimiter;
  lib->dbu_per_micron_ = tech->getDbUnitsPerMicron();
  lib->tech_ = tech->getId();
  return (dbLib*) lib;
}

dbLib* dbLib::getLib(dbDatabase* db_, uint32_t dbid_)
{
  _dbDatabase* db = (_dbDatabase*) db_;
  return (dbLib*) db->lib_tbl_->getPtr(dbid_);
}

void dbLib::destroy(dbLib* lib_)
{
  _dbLib* lib = (_dbLib*) lib_;
  _dbDatabase* db = lib->getDatabase();
  dbProperty::destroyProperties(lib);
  db->lib_tbl_->destroy(lib);
}

void _dbLib::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
  info.children["master_hash"].add(master_hash_);
  info.children["site_hash"].add(site_hash_);
  master_tbl_->collectMemInfo(info.children["master"]);
  site_tbl_->collectMemInfo(info.children["site"]);
  prop_tbl_->collectMemInfo(info.children["prop"]);
  name_cache_->collectMemInfo(info.children["name_cache"]);
}

}  // namespace odb
