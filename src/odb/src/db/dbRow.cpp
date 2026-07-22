// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbRow.h"

#include <cstdint>
#include <cstring>
#include <string>

#include "dbBlock.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbLib.h"
#include "dbSite.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbSet.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace odb {

template class dbTable<_dbRow>;

bool _dbRow::operator==(const _dbRow& rhs) const
{
  if (flags_.orient != rhs.flags_.orient) {
    return false;
  }

  if (flags_.dir != rhs.flags_.dir) {
    return false;
  }

  if (name_ && rhs.name_) {
    if (strcmp(name_, rhs.name_) != 0) {
      return false;
    }
  } else if (name_ || rhs.name_) {
    return false;
  }

  if (lib_ != rhs.lib_) {
    return false;
  }

  if (site_ != rhs.site_) {
    return false;
  }

  if (x_ != rhs.x_) {
    return false;
  }

  if (y_ != rhs.y_) {
    return false;
  }

  if (site_cnt_ != rhs.site_cnt_) {
    return false;
  }

  if (spacing_ != rhs.spacing_) {
    return false;
  }

  return true;
}

bool _dbRow::operator<(const _dbRow& rhs) const
{
  int r = strcmp(name_, rhs.name_);

  if (r < 0) {
    return true;
  }

  if (r > 0) {
    return false;
  }

  if (site_ < rhs.site_) {
    return true;
  }

  if (site_ > rhs.site_) {
    return false;
  }

  if (x_ < rhs.x_) {
    return true;
  }

  if (x_ > rhs.x_) {
    return false;
  }

  if (y_ < rhs.y_) {
    return true;
  }

  if (y_ > rhs.y_) {
    return false;
  }

  if (site_cnt_ < rhs.site_cnt_) {
    return true;
  }

  if (site_cnt_ > rhs.site_cnt_) {
    return false;
  }

  if (spacing_ < rhs.spacing_) {
    return true;
  }

  if (spacing_ > rhs.spacing_) {
    return false;
  }

  if (flags_.orient < rhs.flags_.orient) {
    return true;
  }

  if (flags_.orient > rhs.flags_.orient) {
    return false;
  }

  if (flags_.dir < rhs.flags_.dir) {
    return true;
  }

  if (flags_.dir > rhs.flags_.dir) {
    return false;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//
// dbRow - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbRow::getName()
{
  _dbRow* row = (_dbRow*) this;
  return row->name_;
}

const char* dbRow::getConstName()
{
  _dbRow* row = (_dbRow*) this;
  return row->name_;
}

dbSite* dbRow::getSite()
{
  _dbRow* row = (_dbRow*) this;
  _dbDatabase* db = row->getDatabase();
  _dbLib* lib = db->lib_tbl_->getPtr(row->lib_);
  _dbSite* site = lib->site_tbl_->getPtr(row->site_);
  return (dbSite*) site;
}

Point dbRow::getOrigin()
{
  _dbRow* row = (_dbRow*) this;
  return {row->x_, row->y_};
}

dbOrientType dbRow::getOrient()
{
  _dbRow* row = (_dbRow*) this;
  dbOrientType t(row->flags_.orient);
  return t;
}

dbRowDir dbRow::getDirection()
{
  _dbRow* row = (_dbRow*) this;
  dbRowDir d(row->flags_.dir);
  return d;
}

int dbRow::getSiteCount()
{
  _dbRow* row = (_dbRow*) this;
  return row->site_cnt_;
}

int dbRow::getSpacing()
{
  _dbRow* row = (_dbRow*) this;
  return row->spacing_;
}

Rect dbRow::getBBox()
{
  _dbRow* row = (_dbRow*) this;

  if (row->site_cnt_ == 0) {
    return Rect(0, 0, 0, 0);
  }

  dbSite* site = getSite();
  int w = site->getWidth();
  int h = site->getHeight();
  dbTransform transform(getOrient());
  Rect r(0, 0, w, h);
  transform.apply(r);
  int dx = r.dx();
  int dy = r.dy();
  Point origin = getOrigin();

  if (row->flags_.dir == dbRowDir::HORIZONTAL) {
    int xMax = origin.x() + ((row->site_cnt_ - 1) * row->spacing_) + dx;
    int yMax = origin.y() + dy;
    return Rect(origin.x(), origin.y(), xMax, yMax);
  }
  int xMax = origin.x() + dx;
  int yMax = origin.y() + ((row->site_cnt_ - 1) * row->spacing_) + dy;
  return Rect(origin.x(), origin.y(), xMax, yMax);
}

dbBlock* dbRow::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

dbRow* dbRow::create(dbBlock* block_,
                     const char* name,
                     dbSite* site_,
                     int origin_x,
                     int origin_y,
                     dbOrientType orient,
                     dbRowDir direction,
                     int num_sites,
                     int spacing)
{
  _dbBlock* block = (_dbBlock*) block_;
  _dbSite* site = (_dbSite*) site_;
  _dbLib* lib = (_dbLib*) site->getOwner();
  _dbRow* row = block->row_tbl_->create();
  row->name_ = safe_strdup(name);
  row->lib_ = lib->getOID();
  row->site_ = site->getOID();
  row->flags_.orient = orient;
  row->flags_.dir = direction;
  row->x_ = origin_x;
  row->y_ = origin_y;
  row->site_cnt_ = num_sites;
  row->spacing_ = spacing;
  for (auto callback : block->callbacks_) {
    callback->inDbRowCreate((dbRow*) row);
  }
  return (dbRow*) row;
}

void dbRow::destroy(dbRow* row_)
{
  _dbRow* row = (_dbRow*) row_;
  _dbBlock* block = (_dbBlock*) row->getOwner();
  for (auto callback : block->callbacks_) {
    callback->inDbRowDestroy((dbRow*) row);
  }
  dbProperty::destroyProperties(row);
  block->row_tbl_->destroy(row);
}

dbSet<dbRow>::iterator dbRow::destroy(dbSet<dbRow>::iterator& itr)
{
  dbRow* r = *itr;
  dbSet<dbRow>::iterator next = ++itr;
  destroy(r);
  return next;
}

dbRow* dbRow::getRow(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbRow*) block->row_tbl_->getPtr(dbid_);
}

void _dbRow::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
}

}  // namespace odb
