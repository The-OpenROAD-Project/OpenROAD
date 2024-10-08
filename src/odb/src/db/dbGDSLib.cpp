///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

#include "dbGDSLib.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"

namespace odb {
template class dbTable<_dbGDSLib>;

bool _dbGDSLib::operator==(const _dbGDSLib& rhs) const
{
  if (_libname != rhs._libname) {
    return false;
  }
  if (_libDirSize != rhs._libDirSize) {
    return false;
  }
  if (_srfName != rhs._srfName) {
    return false;
  }
  if (_uu_per_dbu != rhs._uu_per_dbu) {
    return false;
  }
  if (_dbu_per_meter != rhs._dbu_per_meter) {
    return false;
  }
  if (*_structure_tbl != *rhs._structure_tbl) {
    return false;
  }
  if (_structure_hash != rhs._structure_hash) {
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//
// dbGDSLib - Methods
//
////////////////////////////////////////////////////////////////////

void _dbGDSLib::differences(dbDiff& diff,
                            const char* field,
                            const _dbGDSLib& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_libname);
  DIFF_FIELD(_libDirSize);
  DIFF_FIELD(_srfName);
  DIFF_FIELD(_uu_per_dbu);
  DIFF_FIELD(_dbu_per_meter);
  DIFF_HASH_TABLE(_structure_hash);
  DIFF_TABLE_NO_DEEP(_structure_tbl);
  DIFF_END
}

void _dbGDSLib::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_libname);
  DIFF_OUT_FIELD(_libDirSize);
  DIFF_OUT_FIELD(_srfName);
  DIFF_OUT_FIELD(_uu_per_dbu);
  DIFF_OUT_FIELD(_dbu_per_meter);
  DIFF_OUT_HASH_TABLE(_structure_hash);
  DIFF_OUT_TABLE_NO_DEEP(_structure_tbl);
  DIFF_END
}

dbObjectTable* _dbGDSLib::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbGDSStructureObj:
      return _structure_tbl;
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}

_dbGDSLib::_dbGDSLib(_dbDatabase* db)
{
  _libDirSize = 0;
  _uu_per_dbu = 1.0;
  _dbu_per_meter = 1e9;

  _structure_tbl = new dbTable<_dbGDSStructure>(
      db, this, (GetObjTbl_t) &_dbGDSLib::getObjectTable, dbGDSStructureObj);

  _structure_hash.setTable(_structure_tbl);
  std::mktime(&_lastAccessed);
  std::mktime(&_lastModified);
}

_dbGDSLib::_dbGDSLib(_dbDatabase* db, const _dbGDSLib& r)
    : _libname(r._libname),
      _lastAccessed(r._lastAccessed),
      _lastModified(r._lastModified),
      _libDirSize(r._libDirSize),
      _srfName(r._srfName),
      _uu_per_dbu(r._uu_per_dbu),
      _dbu_per_meter(r._dbu_per_meter),
      _structure_hash(r._structure_hash),
      _structure_tbl(r._structure_tbl)
{
}

_dbGDSLib::~_dbGDSLib()
{
  delete _structure_tbl;
}

dbIStream& operator>>(dbIStream& stream, std::tm& tm)
{
  stream >> tm.tm_sec;
  stream >> tm.tm_min;
  stream >> tm.tm_hour;
  stream >> tm.tm_mday;
  stream >> tm.tm_mon;
  stream >> tm.tm_year;
  stream >> tm.tm_wday;
  stream >> tm.tm_yday;
  stream >> tm.tm_isdst;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const std::tm& tm)
{
  stream << tm.tm_sec;
  stream << tm.tm_min;
  stream << tm.tm_hour;
  stream << tm.tm_mday;
  stream << tm.tm_mon;
  stream << tm.tm_year;
  stream << tm.tm_wday;
  stream << tm.tm_yday;
  stream << tm.tm_isdst;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSLib& obj)
{
  stream >> obj._libname;
  stream >> obj._lastAccessed;
  stream >> obj._lastModified;
  stream >> obj._libDirSize;
  stream >> obj._srfName;
  stream >> obj._uu_per_dbu;
  stream >> obj._dbu_per_meter;
  stream >> *obj._structure_tbl;
  stream >> obj._structure_hash;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSLib& obj)
{
  stream << obj._libname;
  stream << obj._lastAccessed;
  stream << obj._lastModified;
  stream << obj._libDirSize;
  stream << obj._srfName;
  stream << obj._uu_per_dbu;
  stream << obj._dbu_per_meter;
  stream << NamedTable("_structure_tbl", obj._structure_tbl);
  stream << obj._structure_hash;
  return stream;
}

_dbGDSStructure* _dbGDSLib::findStructure(const char* name)
{
  return _structure_hash.find(name);
}

////////////////////////////////////////////////////////////////////
//
// dbLib - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSLib::setLibname(std::string libname)
{
  _dbGDSLib* obj = (_dbGDSLib*) this;

  obj->_libname = std::move(libname);
}

std::string dbGDSLib::getLibname() const
{
  _dbGDSLib* obj = (_dbGDSLib*) this;
  return obj->_libname;
}

void dbGDSLib::set_lastAccessed(std::tm lastAccessed)
{
  _dbGDSLib* obj = (_dbGDSLib*) this;

  obj->_lastAccessed = lastAccessed;
}

std::tm dbGDSLib::get_lastAccessed() const
{
  _dbGDSLib* obj = (_dbGDSLib*) this;
  return obj->_lastAccessed;
}

void dbGDSLib::set_lastModified(std::tm lastModified)
{
  _dbGDSLib* obj = (_dbGDSLib*) this;

  obj->_lastModified = lastModified;
}

std::tm dbGDSLib::get_lastModified() const
{
  _dbGDSLib* obj = (_dbGDSLib*) this;
  return obj->_lastModified;
}

void dbGDSLib::set_libDirSize(int16_t libDirSize)
{
  _dbGDSLib* obj = (_dbGDSLib*) this;

  obj->_libDirSize = libDirSize;
}

int16_t dbGDSLib::get_libDirSize() const
{
  _dbGDSLib* obj = (_dbGDSLib*) this;
  return obj->_libDirSize;
}

void dbGDSLib::set_srfName(std::string srfName)
{
  _dbGDSLib* obj = (_dbGDSLib*) this;

  obj->_srfName = std::move(srfName);
}

std::string dbGDSLib::get_srfName() const
{
  _dbGDSLib* obj = (_dbGDSLib*) this;
  return obj->_srfName;
}

void dbGDSLib::setUnits(double uu_per_dbu, double dbu_per_meter)
{
  _dbGDSLib* obj = (_dbGDSLib*) this;

  obj->_uu_per_dbu = uu_per_dbu;
  obj->_dbu_per_meter = dbu_per_meter;
}

std::pair<double, double> dbGDSLib::getUnits() const
{
  _dbGDSLib* obj = (_dbGDSLib*) this;
  return std::make_pair(obj->_uu_per_dbu, obj->_dbu_per_meter);
}

dbGDSStructure* dbGDSLib::findGDSStructure(const char* name) const
{
  _dbGDSLib* obj = (_dbGDSLib*) this;
  return (dbGDSStructure*) obj->_structure_hash.find(name);
}

dbSet<dbGDSStructure> dbGDSLib::getGDSStructures()
{
  _dbGDSLib* obj = (_dbGDSLib*) this;
  return dbSet<dbGDSStructure>(obj, obj->_structure_tbl);
}

dbGDSLib* dbGDSLib::create(dbDatabase* db, const std::string& name)
{
  auto* obj = (_dbDatabase*) db;
  auto lib = (dbGDSLib*) obj->_gds_lib_tbl->create();
  lib->setLibname(name);
  return lib;
}

void dbGDSLib::destroy(dbGDSLib* lib)
{
  auto* obj = (_dbGDSLib*) lib;
  auto* db = (_dbDatabase*) obj->getOwner();
  db->_gds_lib_tbl->destroy(obj);
}

}  // namespace odb
