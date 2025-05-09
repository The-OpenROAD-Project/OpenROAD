// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbLib.h"

#include <string>

#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbMaster.h"
#include "dbNameCache.h"
#include "dbProperty.h"
#include "dbPropertyItr.h"
#include "dbSite.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "odb/db.h"
#include "odb/dbTransform.h"

namespace odb {

template class dbTable<_dbLib>;
template class dbHashTable<_dbMaster>;
template class dbHashTable<_dbSite>;

bool _dbLib::operator==(const _dbLib& rhs) const
{
  if (_lef_units != rhs._lef_units) {
    return false;
  }

  if (_dbu_per_micron != rhs._dbu_per_micron) {
    return false;
  }

  if (_hier_delimiter != rhs._hier_delimiter) {
    return false;
  }

  if (_left_bus_delimiter != rhs._left_bus_delimiter) {
    return false;
  }

  if (_right_bus_delimiter != rhs._right_bus_delimiter) {
    return false;
  }

  if (_spare != rhs._spare) {
    return false;
  }

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0) {
      return false;
    }
  } else if (_name || rhs._name) {
    return false;
  }

  if (_master_hash != rhs._master_hash) {
    return false;
  }

  if (_site_hash != rhs._site_hash) {
    return false;
  }

  if (*_master_tbl != *rhs._master_tbl) {
    return false;
  }

  if (*_site_tbl != *rhs._site_tbl) {
    return false;
  }

  if (*_prop_tbl != *rhs._prop_tbl) {
    return false;
  }

  if (_tech != rhs._tech) {
    return false;
  }

  if (*_name_cache != *rhs._name_cache) {
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
  _lef_units = 0;
  _dbu_per_micron = 1000;
  _hier_delimiter = 0;
  _left_bus_delimiter = 0;
  _right_bus_delimiter = 0;
  _spare = 0;
  _name = nullptr;

  _master_tbl = new dbTable<_dbMaster>(
      db, this, (GetObjTbl_t) &_dbLib::getObjectTable, dbMasterObj);

  _site_tbl = new dbTable<_dbSite>(
      db, this, (GetObjTbl_t) &_dbLib::getObjectTable, dbSiteObj);

  _prop_tbl = new dbTable<_dbProperty>(
      db, this, (GetObjTbl_t) &_dbLib::getObjectTable, dbPropertyObj);

  _name_cache
      = new _dbNameCache(db, this, (GetObjTbl_t) &_dbLib::getObjectTable);

  _prop_itr = new dbPropertyItr(_prop_tbl);

  _master_hash.setTable(_master_tbl);
  _site_hash.setTable(_site_tbl);
}

_dbLib::~_dbLib()
{
  delete _master_tbl;
  delete _site_tbl;
  delete _prop_tbl;
  delete _name_cache;
  delete _prop_itr;

  if (_name) {
    free((void*) _name);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbLib& lib)
{
  dbOStreamScope scope(stream, fmt::format("dbLib({})", lib._name));
  stream << lib._lef_units;
  stream << lib._dbu_per_micron;
  stream << lib._hier_delimiter;
  stream << lib._left_bus_delimiter;
  stream << lib._right_bus_delimiter;
  stream << lib._spare;
  stream << lib._name;
  stream << lib._master_hash;
  stream << lib._site_hash;
  stream << lib._tech;
  stream << NamedTable("master_tbl", lib._master_tbl);
  stream << NamedTable("site_tbl", lib._site_tbl);
  stream << NamedTable("prop_tbl", lib._prop_tbl);
  stream << *lib._name_cache;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbLib& lib)
{
  stream >> lib._lef_units;
  stream >> lib._dbu_per_micron;
  stream >> lib._hier_delimiter;
  stream >> lib._left_bus_delimiter;
  stream >> lib._right_bus_delimiter;
  stream >> lib._spare;
  stream >> lib._name;
  stream >> lib._master_hash;
  stream >> lib._site_hash;
  // In the older schema we can't set the tech here, we handle this later in
  // dbDatabase.
  if (lib.getDatabase()->isSchema(db_schema_block_tech)) {
    stream >> lib._tech;
  }
  stream >> *lib._master_tbl;
  stream >> *lib._site_tbl;
  stream >> *lib._prop_tbl;
  stream >> *lib._name_cache;

  return stream;
}

dbObjectTable* _dbLib::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbMasterObj:
      return _master_tbl;
    case dbSiteObj:
      return _site_tbl;
    case dbPropertyObj:
      return _prop_tbl;
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
  return lib->_name;
}

const char* dbLib::getConstName()
{
  _dbLib* lib = (_dbLib*) this;
  return lib->_name;
}

dbTech* dbLib::getTech()
{
  _dbLib* lib = (_dbLib*) this;
  return (dbTech*) lib->getTech();
}

_dbTech* _dbLib::getTech()
{
  _dbDatabase* db = getDatabase();
  return db->_tech_tbl->getPtr(_tech);
}

int dbLib::getDbUnitsPerMicron()
{
  _dbLib* lib = (_dbLib*) this;
  return lib->_dbu_per_micron;
}

dbSet<dbMaster> dbLib::getMasters()
{
  _dbLib* lib = (_dbLib*) this;
  return dbSet<dbMaster>(lib, lib->_master_tbl);
}

dbMaster* dbLib::findMaster(const char* name)
{
  _dbLib* lib = (_dbLib*) this;
  return (dbMaster*) lib->_master_hash.find(name);
}

dbSet<dbSite> dbLib::getSites()
{
  _dbLib* lib = (_dbLib*) this;
  return dbSet<dbSite>(lib, lib->_site_tbl);
}

dbSite* dbLib::findSite(const char* name)
{
  _dbLib* lib = (_dbLib*) this;
  return (dbSite*) lib->_site_hash.find(name);
}

int dbLib::getLefUnits()
{
  _dbLib* lib = (_dbLib*) this;
  return lib->_lef_units;
}

void dbLib::setLefUnits(int units)
{
  _dbLib* lib = (_dbLib*) this;
  lib->_lef_units = units;
}

char dbLib::getHierarchyDelimiter()
{
  _dbLib* lib = (_dbLib*) this;
  return lib->_hier_delimiter;
}

void dbLib::setBusDelimiters(char left, char right)
{
  _dbLib* lib = (_dbLib*) this;
  lib->_left_bus_delimiter = left;
  lib->_right_bus_delimiter = right;
}

void dbLib::getBusDelimiters(char& left, char& right)
{
  _dbLib* lib = (_dbLib*) this;
  left = lib->_left_bus_delimiter;
  right = lib->_right_bus_delimiter;
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
  _dbLib* lib = db->_lib_tbl->create();
  lib->_name = strdup(name);
  ZALLOCATED(lib->_name);
  lib->_hier_delimiter = hier_delimiter;
  lib->_dbu_per_micron = tech->getDbUnitsPerMicron();
  lib->_tech = tech->getId();
  return (dbLib*) lib;
}

dbLib* dbLib::getLib(dbDatabase* db_, uint dbid_)
{
  _dbDatabase* db = (_dbDatabase*) db_;
  return (dbLib*) db->_lib_tbl->getPtr(dbid_);
}

void dbLib::destroy(dbLib* lib_)
{
  _dbLib* lib = (_dbLib*) lib_;
  _dbDatabase* db = lib->getDatabase();
  dbProperty::destroyProperties(lib);
  db->_lib_tbl->destroy(lib);
}

void _dbLib::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["name"].add(_name);
  info.children_["master_hash"].add(_master_hash);
  info.children_["site_hash"].add(_site_hash);
  _master_tbl->collectMemInfo(info.children_["master"]);
  _site_tbl->collectMemInfo(info.children_["site"]);
  _prop_tbl->collectMemInfo(info.children_["prop"]);
  _name_cache->collectMemInfo(info.children_["name_cache"]);
}

}  // namespace odb
