// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbSite.h"

#include <cstdint>
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
  if (flags_.x_symmetry != rhs.flags_.x_symmetry) {
    return false;
  }

  if (flags_.y_symmetry != rhs.flags_.y_symmetry) {
    return false;
  }

  if (flags_.R90_symmetry != rhs.flags_.R90_symmetry) {
    return false;
  }

  if (flags_.site_class != rhs.flags_.site_class) {
    return false;
  }

  if (flags_.is_hybrid != rhs.flags_.is_hybrid) {
    return false;
  }

  if (name_ && rhs.name_) {
    if (strcmp(name_, rhs.name_) != 0) {
      return false;
    }
  } else if (name_ || rhs.name_) {
    return false;
  }

  if (height_ != rhs.height_) {
    return false;
  }

  if (width_ != rhs.width_) {
    return false;
  }

  if (next_entry_ != rhs.next_entry_) {
    return false;
  }

  if (row_pattern_ != rhs.row_pattern_) {
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
      height_(s.height_),
      width_(s.width_),
      next_entry_(s.next_entry_)
{
  if (s.name_) {
    name_ = safe_strdup(s.name_);
  }
}

_dbSite::_dbSite(_dbDatabase*)
{
  name_ = nullptr;
  height_ = 0;
  width_ = 0;
  flags_.x_symmetry = 0;
  flags_.y_symmetry = 0;
  flags_.R90_symmetry = 0;
  flags_.site_class = dbSiteClass::CORE;
  flags_.is_hybrid = 0;
  flags_.spare_bits = 0;
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
  return site->width_;
}

void dbSite::setWidth(int w)
{
  _dbSite* site = (_dbSite*) this;
  site->width_ = w;
}

int dbSite::getHeight() const
{
  _dbSite* site = (_dbSite*) this;
  return site->height_;
}

void dbSite::setHeight(int h)
{
  _dbSite* site = (_dbSite*) this;
  site->height_ = h;
}

void dbSite::setSymmetryX()
{
  _dbSite* site = (_dbSite*) this;
  site->flags_.x_symmetry = 1;
}

bool dbSite::getSymmetryX()
{
  _dbSite* site = (_dbSite*) this;
  return site->flags_.x_symmetry != 0;
}

void dbSite::setSymmetryY()
{
  _dbSite* site = (_dbSite*) this;
  site->flags_.y_symmetry = 1;
}

bool dbSite::getSymmetryY()
{
  _dbSite* site = (_dbSite*) this;
  return site->flags_.y_symmetry != 0;
}

void dbSite::setSymmetryR90()
{
  _dbSite* site = (_dbSite*) this;
  site->flags_.R90_symmetry = 1;
}

bool dbSite::getSymmetryR90()
{
  _dbSite* site = (_dbSite*) this;
  return site->flags_.R90_symmetry != 0;
}

dbSiteClass dbSite::getClass()
{
  _dbSite* site = (_dbSite*) this;
  return dbSiteClass(site->flags_.site_class);
}

void dbSite::setClass(dbSiteClass type)
{
  _dbSite* site = (_dbSite*) this;
  site->flags_.site_class = type.getValue();
}

void dbSite::setRowPattern(const RowPattern& row_pattern)
{
  _dbSite* site = (_dbSite*) this;
  site->flags_.is_hybrid = true;
  site->row_pattern_.reserve(row_pattern.size());
  for (auto& row : row_pattern) {
    auto child_site = (_dbSite*) row.site;
    site->row_pattern_.push_back({child_site->getOwner()->getId(),
                                  child_site->getId(),
                                  row.orientation});
  }
}

bool dbSite::hasRowPattern() const
{
  _dbSite* site = (_dbSite*) this;
  return !site->row_pattern_.empty();
}

bool dbSite::isHybrid() const
{
  _dbSite* site = (_dbSite*) this;
  return site->flags_.is_hybrid != 0;
}

dbSite::RowPattern dbSite::getRowPattern()
{
  _dbSite* site = (_dbSite*) this;
  dbDatabase* db = (dbDatabase*) site->getDatabase();
  auto& rp = site->row_pattern_;

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
  _dbSite* site = lib->site_tbl_->create();
  site->name_ = safe_strdup(name_);
  lib->site_hash_.insert(site);
  return (dbSite*) site;
}

dbSite* dbSite::getSite(dbLib* lib, uint32_t oid)
{
  _dbLib* lib_impl = (_dbLib*) lib;
  return (dbSite*) lib_impl->site_tbl_->getPtr(oid);
}

void _dbSite::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
  info.children["row_pattern"].add(row_pattern_);
}

dbOStream& operator<<(dbOStream& stream, const _dbSite& site)
{
  uint32_t* bit_field = (uint32_t*) &site.flags_;
  stream << *bit_field;
  stream << site.name_;
  stream << site.height_;
  stream << site.width_;
  stream << site.next_entry_;
  stream << site.row_pattern_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbSite& site)
{
  uint32_t* bit_field = (uint32_t*) &site.flags_;
  stream >> *bit_field;
  stream >> site.name_;
  stream >> site.height_;
  stream >> site.width_;
  stream >> site.next_entry_;
  _dbDatabase* db = site.getImpl()->getDatabase();
  if (db->isSchema(kSchemaSiteRowPattern)) {
    stream >> site.row_pattern_;
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
