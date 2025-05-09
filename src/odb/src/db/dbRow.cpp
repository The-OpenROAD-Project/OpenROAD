// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbRow.h"

#include <string>

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbLib.h"
#include "dbSite.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbSet.h"
#include "odb/dbTransform.h"

namespace odb {

template class dbTable<_dbRow>;

bool _dbRow::operator==(const _dbRow& rhs) const
{
  if (_flags._orient != rhs._flags._orient) {
    return false;
  }

  if (_flags._dir != rhs._flags._dir) {
    return false;
  }

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0) {
      return false;
    }
  } else if (_name || rhs._name) {
    return false;
  }

  if (_lib != rhs._lib) {
    return false;
  }

  if (_site != rhs._site) {
    return false;
  }

  if (_x != rhs._x) {
    return false;
  }

  if (_y != rhs._y) {
    return false;
  }

  if (_site_cnt != rhs._site_cnt) {
    return false;
  }

  if (_spacing != rhs._spacing) {
    return false;
  }

  return true;
}

bool _dbRow::operator<(const _dbRow& rhs) const
{
  int r = strcmp(_name, rhs._name);

  if (r < 0) {
    return true;
  }

  if (r > 0) {
    return false;
  }

  if (_site < rhs._site) {
    return true;
  }

  if (_site > rhs._site) {
    return false;
  }

  if (_x < rhs._x) {
    return true;
  }

  if (_x > rhs._x) {
    return false;
  }

  if (_y < rhs._y) {
    return true;
  }

  if (_y > rhs._y) {
    return false;
  }

  if (_site_cnt < rhs._site_cnt) {
    return true;
  }

  if (_site_cnt > rhs._site_cnt) {
    return false;
  }

  if (_spacing < rhs._spacing) {
    return true;
  }

  if (_spacing > rhs._spacing) {
    return false;
  }

  if (_flags._orient < rhs._flags._orient) {
    return true;
  }

  if (_flags._orient > rhs._flags._orient) {
    return false;
  }

  if (_flags._dir < rhs._flags._dir) {
    return true;
  }

  if (_flags._dir > rhs._flags._dir) {
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
  return row->_name;
}

const char* dbRow::getConstName()
{
  _dbRow* row = (_dbRow*) this;
  return row->_name;
}

dbSite* dbRow::getSite()
{
  _dbRow* row = (_dbRow*) this;
  _dbDatabase* db = (_dbDatabase*) row->getDatabase();
  _dbLib* lib = db->_lib_tbl->getPtr(row->_lib);
  _dbSite* site = lib->_site_tbl->getPtr(row->_site);
  return (dbSite*) site;
}

Point dbRow::getOrigin()
{
  _dbRow* row = (_dbRow*) this;
  return {row->_x, row->_y};
}

dbOrientType dbRow::getOrient()
{
  _dbRow* row = (_dbRow*) this;
  dbOrientType t(row->_flags._orient);
  return t;
}

dbRowDir dbRow::getDirection()
{
  _dbRow* row = (_dbRow*) this;
  dbRowDir d(row->_flags._dir);
  return d;
}

int dbRow::getSiteCount()
{
  _dbRow* row = (_dbRow*) this;
  return row->_site_cnt;
}

int dbRow::getSpacing()
{
  _dbRow* row = (_dbRow*) this;
  return row->_spacing;
}

Rect dbRow::getBBox()
{
  _dbRow* row = (_dbRow*) this;

  if (row->_site_cnt == 0) {
    return Rect(0, 0, 0, 0);
  }

  dbSite* site = getSite();
  int w = site->getWidth();
  int h = site->getHeight();
  dbTransform transform(getOrient());
  Rect r(0, 0, w, h);
  transform.apply(r);
  int dx = (int) r.dx();
  int dy = (int) r.dy();
  Point origin = getOrigin();

  if (row->_flags._dir == dbRowDir::HORIZONTAL) {
    int xMax = origin.x() + (row->_site_cnt - 1) * row->_spacing + dx;
    int yMax = origin.y() + dy;
    return Rect(origin.x(), origin.y(), xMax, yMax);
  }
  int xMax = origin.x() + dx;
  int yMax = origin.y() + (row->_site_cnt - 1) * row->_spacing + dy;
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
  _dbRow* row = block->_row_tbl->create();
  row->_name = strdup(name);
  ZALLOCATED(row->_name);
  row->_lib = lib->getOID();
  row->_site = site->getOID();
  row->_flags._orient = orient;
  row->_flags._dir = direction;
  row->_x = origin_x;
  row->_y = origin_y;
  row->_site_cnt = num_sites;
  row->_spacing = spacing;
  for (auto callback : block->_callbacks) {
    callback->inDbRowCreate((dbRow*) row);
  }
  return (dbRow*) row;
}

void dbRow::destroy(dbRow* row_)
{
  _dbRow* row = (_dbRow*) row_;
  _dbBlock* block = (_dbBlock*) row->getOwner();
  for (auto callback : block->_callbacks) {
    callback->inDbRowDestroy((dbRow*) row);
  }
  dbProperty::destroyProperties(row);
  block->_row_tbl->destroy(row);
}

dbSet<dbRow>::iterator dbRow::destroy(dbSet<dbRow>::iterator& itr)
{
  dbRow* r = *itr;
  dbSet<dbRow>::iterator next = ++itr;
  destroy(r);
  return next;
}

dbRow* dbRow::getRow(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbRow*) block->_row_tbl->getPtr(dbid_);
}

void _dbRow::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["name"].add(_name);
}

}  // namespace odb
