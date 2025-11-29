// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbSite.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <tuple>
#include <vector>

#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbLib.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"

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
  if (flags_._x_symmetry != rhs.flags_._x_symmetry) {
    return false;
  }

  if (flags_._y_symmetry != rhs.flags_._y_symmetry) {
    return false;
  }

  if (flags_._R90_symmetry != rhs.flags_._R90_symmetry) {
    return false;
  }

  if (flags_._class != rhs.flags_._class) {
    return false;
  }

  if (flags_._is_hybrid != rhs.flags_._is_hybrid) {
    return false;
  }

  if (name_ && rhs.name_) {
    if (strcmp(name_, rhs.name_) != 0) {
      return false;
    }
  } else if (name_ || rhs.name_) {
    return false;
  }

  if (_height != rhs._height) {
    return false;
  }

  if (_width != rhs._width) {
    return false;
  }

  if (next_entry_ != rhs.next_entry_) {
    return false;
  }

  if (_row_pattern != rhs._row_pattern) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// _dbSite - Methods
//
////////////////////////////////////////////////////////////////////
_dbSite::_dbSite(_dbDatabase*, const _dbSite& s)
    : flags_(s.flags_),
      name_(nullptr),
      _height(s._height),
      _width(s._width),
      next_entry_(s.next_entry_)
{
  if (s.name_) {
    name_ = safe_strdup(s.name_);
  }
}

_dbSite::_dbSite(_dbDatabase*)
{
  name_ = nullptr;
  _height = 0;
  _width = 0;
  flags_._x_symmetry = 0;
  flags_._y_symmetry = 0;
  flags_._R90_symmetry = 0;
  flags_._class = dbSiteClass::CORE;
  flags_._is_hybrid = 0;
  flags_._spare_bits = 0;
}

_dbSite::~_dbSite()
{
  if (name_) {
    free((void*) name_);
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
  return site->name_;
}

const char* dbSite::getConstName()
{
  _dbSite* site = (_dbSite*) this;
  return site->name_;
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
  site->flags_._x_symmetry = 1;
}

bool dbSite::getSymmetryX()
{
  _dbSite* site = (_dbSite*) this;
  return site->flags_._x_symmetry != 0;
}

void dbSite::setSymmetryY()
{
  _dbSite* site = (_dbSite*) this;
  site->flags_._y_symmetry = 1;
}

bool dbSite::getSymmetryY()
{
  _dbSite* site = (_dbSite*) this;
  return site->flags_._y_symmetry != 0;
}

void dbSite::setSymmetryR90()
{
  _dbSite* site = (_dbSite*) this;
  site->flags_._R90_symmetry = 1;
}

bool dbSite::getSymmetryR90()
{
  _dbSite* site = (_dbSite*) this;
  return site->flags_._R90_symmetry != 0;
}

dbSiteClass dbSite::getClass()
{
  _dbSite* site = (_dbSite*) this;
  return dbSiteClass(site->flags_._class);
}

void dbSite::setClass(dbSiteClass type)
{
  _dbSite* site = (_dbSite*) this;
  site->flags_._class = type.getValue();
}

void dbSite::setRowPattern(const RowPattern& row_pattern)
{
  _dbSite* site = (_dbSite*) this;
  site->flags_._is_hybrid = true;
  site->_row_pattern.reserve(row_pattern.size());
  for (auto& row : row_pattern) {
    auto child_site = (_dbSite*) row.site;
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
  return site->flags_._is_hybrid != 0;
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
  site->name_ = safe_strdup(name_);
  lib->_site_hash.insert(site);
  return (dbSite*) site;
}

dbSite* dbSite::getSite(dbLib* lib, uint oid)
{
  _dbLib* lib_impl = (_dbLib*) lib;
  return (dbSite*) lib_impl->_site_tbl->getPtr(oid);
}

void _dbSite::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["name"].add(name_);
  info.children_["row_pattern"].add(_row_pattern);
}

dbOStream& operator<<(dbOStream& stream, const _dbSite& site)
{
  uint* bit_field = (uint*) &site.flags_;
  stream << *bit_field;
  stream << site.name_;
  stream << site._height;
  stream << site._width;
  stream << site.next_entry_;
  stream << site._row_pattern;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbSite& site)
{
  uint* bit_field = (uint*) &site.flags_;
  stream >> *bit_field;
  stream >> site.name_;
  stream >> site._height;
  stream >> site._width;
  stream >> site.next_entry_;
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
