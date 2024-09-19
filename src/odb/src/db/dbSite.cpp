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

#include "dbSite.h"

#include "dbDatabase.h"
#include "dbLib.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"

namespace odb {

template class dbTable<_dbSite>;

bool OrientedSiteInternal::operator==(const OrientedSiteInternal& rhs) const
{
  return std::tie(lib, site, orientation)
         == std::tie(rhs.lib, rhs.site, rhs.orientation);
}

dbOStream& operator<<(dbOStream& stream, const OrientedSiteInternal& s)
{
  stream << s.lib;
  stream << s.site;
  stream << int(s.orientation.getValue());
  return stream;
}

dbIStream& operator>>(dbIStream& stream, OrientedSiteInternal& s)
{
  stream >> s.lib;
  stream >> s.site;
  int value;
  stream >> value;
  s.orientation = dbOrientType::Value(value);
  return stream;
}

bool _dbSite::operator==(const _dbSite& rhs) const
{
  if (_flags._x_symmetry != rhs._flags._x_symmetry) {
    return false;
  }

  if (_flags._y_symmetry != rhs._flags._y_symmetry) {
    return false;
  }

  if (_flags._R90_symmetry != rhs._flags._R90_symmetry) {
    return false;
  }

  if (_flags._class != rhs._flags._class) {
    return false;
  }

  if (_flags._is_hybrid != rhs._flags._is_hybrid) {
    return false;
  }

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0) {
      return false;
    }
  } else if (_name || rhs._name) {
    return false;
  }

  if (_height != rhs._height) {
    return false;
  }

  if (_width != rhs._width) {
    return false;
  }

  if (_next_entry != rhs._next_entry) {
    return false;
  }

  if (_row_pattern != rhs._row_pattern) {
    return false;
  }

  return true;
}

void _dbSite::differences(dbDiff& diff,
                          const char* field,
                          const _dbSite& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._x_symmetry);
  DIFF_FIELD(_flags._y_symmetry);
  DIFF_FIELD(_flags._R90_symmetry);
  DIFF_FIELD(_flags._class);
  DIFF_FIELD(_flags._is_hybrid);
  DIFF_FIELD(_name);
  DIFF_FIELD(_height);
  DIFF_FIELD(_width);
  DIFF_FIELD(_next_entry);
  DIFF_END
}

void _dbSite::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._x_symmetry);
  DIFF_OUT_FIELD(_flags._y_symmetry);
  DIFF_OUT_FIELD(_flags._R90_symmetry);
  DIFF_OUT_FIELD(_flags._class);
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_height);
  DIFF_OUT_FIELD(_width);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_END
}

////////////////////////////////////////////////////////////////////
//
// _dbSite - Methods
//
////////////////////////////////////////////////////////////////////
_dbSite::_dbSite(_dbDatabase*, const _dbSite& s)
    : _flags(s._flags),
      _name(nullptr),
      _height(s._height),
      _width(s._width),
      _next_entry(s._next_entry)
{
  if (s._name) {
    _name = strdup(s._name);
    ZALLOCATED(_name);
  }
}

_dbSite::_dbSite(_dbDatabase*)
{
  _name = nullptr;
  _height = 0;
  _width = 0;
  _flags._x_symmetry = 0;
  _flags._y_symmetry = 0;
  _flags._R90_symmetry = 0;
  _flags._class = dbSiteClass::CORE;
  _flags._is_hybrid = 0;
  _flags._spare_bits = 0;
}

_dbSite::~_dbSite()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbSite - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbSite::getName() const
{
  _dbSite* site = (_dbSite*) this;
  return site->_name;
}

const char* dbSite::getConstName()
{
  _dbSite* site = (_dbSite*) this;
  return site->_name;
}

int dbSite::getWidth()
{
  _dbSite* site = (_dbSite*) this;
  return site->_width;
}

void dbSite::setWidth(int w)
{
  _dbSite* site = (_dbSite*) this;
  site->_width = w;
}

int dbSite::getHeight() const
{
  _dbSite* site = (_dbSite*) this;
  return site->_height;
}

void dbSite::setHeight(int h)
{
  _dbSite* site = (_dbSite*) this;
  site->_height = h;
}

void dbSite::setSymmetryX()
{
  _dbSite* site = (_dbSite*) this;
  site->_flags._x_symmetry = 1;
}

bool dbSite::getSymmetryX()
{
  _dbSite* site = (_dbSite*) this;
  return site->_flags._x_symmetry != 0;
}

void dbSite::setSymmetryY()
{
  _dbSite* site = (_dbSite*) this;
  site->_flags._y_symmetry = 1;
}

bool dbSite::getSymmetryY()
{
  _dbSite* site = (_dbSite*) this;
  return site->_flags._y_symmetry != 0;
}

void dbSite::setSymmetryR90()
{
  _dbSite* site = (_dbSite*) this;
  site->_flags._R90_symmetry = 1;
}

bool dbSite::getSymmetryR90()
{
  _dbSite* site = (_dbSite*) this;
  return site->_flags._R90_symmetry != 0;
}

dbSiteClass dbSite::getClass()
{
  _dbSite* site = (_dbSite*) this;
  return dbSiteClass(site->_flags._class);
}

void dbSite::setClass(dbSiteClass type)
{
  _dbSite* site = (_dbSite*) this;
  site->_flags._class = type.getValue();
}

void dbSite::setRowPattern(const RowPattern& row_pattern)
{
  _dbSite* site = (_dbSite*) this;
  site->_flags._is_hybrid = true;
  site->_row_pattern.reserve(row_pattern.size());
  for (auto& row : row_pattern) {
    auto child_site = (_dbSite*) row.site;
    child_site->_flags._is_hybrid = true;
    site->_row_pattern.push_back({child_site->getOwner()->getId(),
                                  child_site->getId(),
                                  row.orientation});
  }
}

bool dbSite::hasRowPattern() const
{
  _dbSite* site = (_dbSite*) this;
  return !site->_row_pattern.empty();
}

bool dbSite::isHybrid() const
{
  _dbSite* site = (_dbSite*) this;
  return site->_flags._is_hybrid != 0;
}

dbSite::RowPattern dbSite::getRowPattern()
{
  _dbSite* site = (_dbSite*) this;
  dbDatabase* db = (dbDatabase*) site->getDatabase();
  auto& rp = site->_row_pattern;

  std::vector<OrientedSite> row_pattern;
  row_pattern.reserve(rp.size());
  for (const auto& oriented_site : rp) {
    dbLib* lib = dbLib::getLib(db, oriented_site.lib);
    dbSite* site = dbSite::getSite(lib, oriented_site.site);
    auto orientation = oriented_site.orientation;
    row_pattern.push_back({site, orientation});
  }
  return row_pattern;
}

dbLib* dbSite::getLib()
{
  return (dbLib*) getImpl()->getOwner();
}

dbSite* dbSite::create(dbLib* lib_, const char* name_)
{
  if (lib_->findSite(name_)) {
    return nullptr;
  }

  _dbLib* lib = (_dbLib*) lib_;
  _dbSite* site = lib->_site_tbl->create();
  site->_name = strdup(name_);
  ZALLOCATED(site->_name);
  lib->_site_hash.insert(site);
  return (dbSite*) site;
}

dbSite* dbSite::getSite(dbLib* lib, uint oid)
{
  _dbLib* lib_impl = (_dbLib*) lib;
  return (dbSite*) lib_impl->_site_tbl->getPtr(oid);
}

dbOStream& operator<<(dbOStream& stream, const _dbSite& site)
{
  uint* bit_field = (uint*) &site._flags;
  stream << *bit_field;
  stream << site._name;
  stream << site._height;
  stream << site._width;
  stream << site._next_entry;
  stream << site._row_pattern;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbSite& site)
{
  uint* bit_field = (uint*) &site._flags;
  stream >> *bit_field;
  stream >> site._name;
  stream >> site._height;
  stream >> site._width;
  stream >> site._next_entry;
  _dbDatabase* db = site.getImpl()->getDatabase();
  if (db->isSchema(db_schema_site_row_pattern)) {
    stream >> site._row_pattern;
  }
  return stream;
}

bool operator==(const dbSite::OrientedSite& lhs,
                const dbSite::OrientedSite& rhs)
{
  return std::tie(lhs.site, lhs.orientation)
         == std::tie(rhs.site, rhs.orientation);
}

bool operator!=(const dbSite::OrientedSite& lhs,
                const dbSite::OrientedSite& rhs)
{
  return !(lhs == rhs);
}

}  // namespace odb
