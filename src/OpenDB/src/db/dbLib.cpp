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

#include "dbLib.h"

#include "db.h"
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
#include "dbTransform.h"

namespace odb {

template class dbTable<_dbLib>;
template class dbHashTable<_dbMaster>;
template class dbHashTable<_dbSite>;

bool _dbLib::operator==(const _dbLib& rhs) const
{
  if (_lef_units != rhs._lef_units)
    return false;

  if (_dbu_per_micron != rhs._dbu_per_micron)
    return false;

  if (_hier_delimeter != rhs._hier_delimeter)
    return false;

  if (_left_bus_delimeter != rhs._left_bus_delimeter)
    return false;

  if (_right_bus_delimeter != rhs._right_bus_delimeter)
    return false;

  if (_spare != rhs._spare)
    return false;

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_master_hash != rhs._master_hash)
    return false;

  if (_site_hash != rhs._site_hash)
    return false;

  if (*_master_tbl != *rhs._master_tbl)
    return false;

  if (*_site_tbl != *rhs._site_tbl)
    return false;

  if (*_prop_tbl != *rhs._prop_tbl)
    return false;

  if (*_name_cache != *rhs._name_cache)
    return false;

  return true;
}

void _dbLib::differences(dbDiff& diff,
                         const char* field,
                         const _dbLib& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_lef_units);
  DIFF_FIELD(_dbu_per_micron);
  DIFF_FIELD(_hier_delimeter);
  DIFF_FIELD(_left_bus_delimeter);
  DIFF_FIELD(_right_bus_delimeter);
  DIFF_FIELD(_spare);
  DIFF_FIELD(_name);
  DIFF_HASH_TABLE(_master_hash);
  DIFF_HASH_TABLE(_site_hash);
  DIFF_TABLE_NO_DEEP(_master_tbl);
  DIFF_TABLE_NO_DEEP(_site_tbl);
  DIFF_TABLE_NO_DEEP(_prop_tbl);
  DIFF_NAME_CACHE(_name_cache);
  DIFF_END
}

void _dbLib::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_lef_units);
  DIFF_OUT_FIELD(_dbu_per_micron);
  DIFF_OUT_FIELD(_hier_delimeter);
  DIFF_OUT_FIELD(_left_bus_delimeter);
  DIFF_OUT_FIELD(_right_bus_delimeter);
  DIFF_OUT_FIELD(_spare);
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_HASH_TABLE(_master_hash);
  DIFF_OUT_HASH_TABLE(_site_hash);
  DIFF_OUT_TABLE_NO_DEEP(_master_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_site_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_prop_tbl);
  DIFF_OUT_NAME_CACHE(_name_cache);
  DIFF_END
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
  _hier_delimeter = 0;
  _left_bus_delimeter = 0;
  _right_bus_delimeter = 0;
  _spare = 0;
  _name = 0;

  _master_tbl = new dbTable<_dbMaster>(
      db, this, (GetObjTbl_t) &_dbLib::getObjectTable, dbMasterObj);
  ZALLOCATED(_master_tbl);

  _site_tbl = new dbTable<_dbSite>(
      db, this, (GetObjTbl_t) &_dbLib::getObjectTable, dbSiteObj);
  ZALLOCATED(_site_tbl);

  _prop_tbl = new dbTable<_dbProperty>(
      db, this, (GetObjTbl_t) &_dbLib::getObjectTable, dbPropertyObj);
  ZALLOCATED(_prop_tbl);

  _name_cache
      = new _dbNameCache(db, this, (GetObjTbl_t) &_dbLib::getObjectTable);
  ZALLOCATED(_name_cache);

  _prop_itr = new dbPropertyItr(_prop_tbl);
  ZALLOCATED(_prop_itr);

  _master_hash.setTable(_master_tbl);
  _site_hash.setTable(_site_tbl);
}

_dbLib::_dbLib(_dbDatabase* db, const _dbLib& l)
    : _lef_units(l._lef_units),
      _dbu_per_micron(l._dbu_per_micron),
      _hier_delimeter(l._hier_delimeter),
      _left_bus_delimeter(l._left_bus_delimeter),
      _right_bus_delimeter(l._right_bus_delimeter),
      _spare(l._spare),
      _name(NULL),
      _master_hash(l._master_hash),
      _site_hash(l._site_hash)
{
  if (l._name) {
    _name = strdup(l._name);
    ZALLOCATED(_name);
  }

  _master_tbl = new dbTable<_dbMaster>(db, this, *l._master_tbl);
  ZALLOCATED(_master_tbl);

  _site_tbl = new dbTable<_dbSite>(db, this, *l._site_tbl);
  ZALLOCATED(_site_tbl);

  _prop_tbl = new dbTable<_dbProperty>(db, this, *l._prop_tbl);
  ZALLOCATED(_prop_tbl);

  _name_cache = new _dbNameCache(db, this, *l._name_cache);
  ZALLOCATED(_name_cache);

  _prop_itr = new dbPropertyItr(_prop_tbl);
  ZALLOCATED(_prop_itr);

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

  if (_name)
    free((void*) _name);
}

dbOStream& operator<<(dbOStream& stream, const _dbLib& lib)
{
  stream << lib._lef_units;
  stream << lib._dbu_per_micron;
  stream << lib._hier_delimeter;
  stream << lib._left_bus_delimeter;
  stream << lib._right_bus_delimeter;
  stream << lib._spare;
  stream << lib._name;
  stream << lib._master_hash;
  stream << lib._site_hash;
  stream << *lib._master_tbl;
  stream << *lib._site_tbl;
  stream << *lib._prop_tbl;
  stream << *lib._name_cache;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbLib& lib)
{
  stream >> lib._lef_units;
  stream >> lib._dbu_per_micron;
  stream >> lib._hier_delimeter;
  stream >> lib._left_bus_delimeter;
  stream >> lib._right_bus_delimeter;
  stream >> lib._spare;
  stream >> lib._name;
  stream >> lib._master_hash;
  stream >> lib._site_hash;
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
  _dbDatabase* db = getImpl()->getDatabase();
  return (dbTech*) db->_tech_tbl->getPtr(db->_tech);
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

char dbLib::getHierarchyDelimeter()
{
  _dbLib* lib = (_dbLib*) this;
  return lib->_hier_delimeter;
}

void dbLib::setBusDelimeters(char left, char right)
{
  _dbLib* lib = (_dbLib*) this;
  lib->_left_bus_delimeter = left;
  lib->_right_bus_delimeter = right;
}

void dbLib::getBusDelimeters(char& left, char& right)
{
  _dbLib* lib = (_dbLib*) this;
  left = lib->_left_bus_delimeter;
  right = lib->_right_bus_delimeter;
}

dbLib* dbLib::create(dbDatabase* db_, const char* name, char hier_delimeter)
{
  if (db_->findLib(name))
    return NULL;

  _dbDatabase* db = (_dbDatabase*) db_;
  _dbLib* lib = db->_lib_tbl->create();
  lib->_name = strdup(name);
  ZALLOCATED(lib->_name);
  lib->_hier_delimeter = hier_delimeter;
  _dbTech* tech = (_dbTech*) db_->getTech();
  lib->_dbu_per_micron = tech->_dbu_per_micron;
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

}  // namespace odb
