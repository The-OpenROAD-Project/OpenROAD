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

#include "dbRow.h"

#include "db.h"
#include "dbBlock.h"
#include "dbBlockCallBackObj.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbLib.h"
#include "dbSet.h"
#include "dbSite.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTransform.h"

namespace odb {

template class dbTable<_dbRow>;

bool _dbRow::operator==(const _dbRow& rhs) const
{
  if (_flags._orient != rhs._flags._orient)
    return false;

  if (_flags._dir != rhs._flags._dir)
    return false;

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_lib != rhs._lib)
    return false;

  if (_site != rhs._site)
    return false;

  if (_x != rhs._x)
    return false;

  if (_y != rhs._y)
    return false;

  if (_site_cnt != rhs._site_cnt)
    return false;

  if (_spacing != rhs._spacing)
    return false;

  return true;
}

bool _dbRow::operator<(const _dbRow& rhs) const
{
  int r = strcmp(_name, rhs._name);

  if (r < 0)
    return true;

  if (r > 0)
    return false;

  if (_site < rhs._site)
    return true;

  if (_site > rhs._site)
    return false;

  if (_x < rhs._x)
    return true;

  if (_x > rhs._x)
    return false;

  if (_y < rhs._y)
    return true;

  if (_y > rhs._y)
    return false;

  if (_site_cnt < rhs._site_cnt)
    return true;

  if (_site_cnt > rhs._site_cnt)
    return false;

  if (_spacing < rhs._spacing)
    return true;

  if (_spacing > rhs._spacing)
    return false;

  if (_flags._orient < rhs._flags._orient)
    return true;

  if (_flags._orient > rhs._flags._orient)
    return false;

  if (_flags._dir < rhs._flags._dir)
    return true;

  if (_flags._dir > rhs._flags._dir)
    return false;

  return false;
}

void _dbRow::differences(dbDiff& diff,
                         const char* field,
                         const _dbRow& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_flags._orient);
  DIFF_FIELD(_flags._dir);
  DIFF_FIELD(_lib);
  DIFF_FIELD(_site);
  DIFF_FIELD(_x);
  DIFF_FIELD(_y);
  DIFF_FIELD(_site_cnt);
  DIFF_FIELD(_spacing);
  DIFF_END
}

void _dbRow::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_flags._orient);
  DIFF_OUT_FIELD(_flags._dir);
  DIFF_OUT_FIELD(_lib);
  DIFF_OUT_FIELD(_site);
  DIFF_OUT_FIELD(_x);
  DIFF_OUT_FIELD(_y);
  DIFF_OUT_FIELD(_site_cnt);
  DIFF_OUT_FIELD(_spacing);
  DIFF_END
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

void dbRow::getOrigin(int& x, int& y)
{
  _dbRow* row = (_dbRow*) this;
  x = row->_x;
  y = row->_y;
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

void dbRow::getBBox(Rect& bbox)
{
  _dbRow* row = (_dbRow*) this;

  if (row->_site_cnt == 0) {
    bbox = Rect(0, 0, 0, 0);
    return;
  }

  dbSite* site = getSite();
  int w = site->getWidth();
  int h = site->getHeight();
  dbTransform transform(getOrient());
  Rect r(0, 0, w, h);
  transform.apply(r);
  int dx = (int) r.dx();
  int dy = (int) r.dy();
  int x, y;
  getOrigin(x, y);

  if (row->_flags._dir == dbRowDir::HORIZONTAL) {
    int xMax = x + (row->_site_cnt - 1) * row->_spacing + dx;
    int yMax = y + dy;
    bbox.init(x, y, xMax, yMax);
  } else {
    int xMax = x + dx;
    int yMax = y + (row->_site_cnt - 1) * row->_spacing + dy;
    bbox.init(x, y, xMax, yMax);
  }
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
  for (auto callback : block->_callbacks)
    callback->inDbRowCreate((dbRow*) row);
  return (dbRow*) row;
}

void dbRow::destroy(dbRow* row_)
{
  _dbRow* row = (_dbRow*) row_;
  _dbBlock* block = (_dbBlock*) row->getOwner();
  for (auto callback : block->_callbacks)
    callback->inDbRowDestroy((dbRow*) row);
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

}  // namespace odb
