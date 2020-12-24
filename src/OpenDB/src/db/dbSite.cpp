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

#include "db.h"
#include "dbDatabase.h"
#include "dbLib.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

template class dbTable<_dbSite>;

bool _dbSite::operator==(const _dbSite& rhs) const
{
  if (_flags._x_symmetry != rhs._flags._x_symmetry)
    return false;

  if (_flags._y_symmetry != rhs._flags._y_symmetry)
    return false;

  if (_flags._R90_symmetry != rhs._flags._R90_symmetry)
    return false;

  if (_flags._class != rhs._flags._class)
    return false;

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_height != rhs._height)
    return false;

  if (_width != rhs._width)
    return false;

  if (_next_entry != rhs._next_entry)
    return false;

  return true;
}

void _dbSite::differences(dbDiff&        diff,
                          const char*    field,
                          const _dbSite& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._x_symmetry);
  DIFF_FIELD(_flags._y_symmetry);
  DIFF_FIELD(_flags._R90_symmetry);
  DIFF_FIELD(_flags._class);
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
      _name(NULL),
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
  _name                = NULL;
  _height              = 0;
  _width               = 0;
  _flags._x_symmetry   = 0;
  _flags._y_symmetry   = 0;
  _flags._R90_symmetry = 0;
  _flags._class        = dbSiteClass::CORE;
  _flags._spare_bits   = 0;
}

_dbSite::~_dbSite()
{
  if (_name)
    free((void*) _name);
}

////////////////////////////////////////////////////////////////////
//
// dbSite - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbSite::getName()
{
  _dbSite* site = (_dbSite*) this;
  return site->_name;
}

const char* dbSite::getConstName()
{
  _dbSite* site = (_dbSite*) this;
  return site->_name;
}

uint dbSite::getWidth()
{
  _dbSite* site = (_dbSite*) this;
  return site->_width;
}

void dbSite::setWidth(uint w)
{
  _dbSite* site = (_dbSite*) this;
  site->_width  = w;
}

uint dbSite::getHeight()
{
  _dbSite* site = (_dbSite*) this;
  return site->_height;
}

void dbSite::setHeight(uint h)
{
  _dbSite* site = (_dbSite*) this;
  site->_height = h;
}

void dbSite::setSymmetryX()
{
  _dbSite* site            = (_dbSite*) this;
  site->_flags._x_symmetry = 1;
}

bool dbSite::getSymmetryX()
{
  _dbSite* site = (_dbSite*) this;
  return site->_flags._x_symmetry != 0;
}

void dbSite::setSymmetryY()
{
  _dbSite* site            = (_dbSite*) this;
  site->_flags._y_symmetry = 1;
}

bool dbSite::getSymmetryY()
{
  _dbSite* site = (_dbSite*) this;
  return site->_flags._y_symmetry != 0;
}

void dbSite::setSymmetryR90()
{
  _dbSite* site              = (_dbSite*) this;
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
  _dbSite* site       = (_dbSite*) this;
  site->_flags._class = type.getValue();
}

dbLib* dbSite::getLib()
{
  return (dbLib*) getImpl()->getOwner();
}

dbSite* dbSite::create(dbLib* lib_, const char* name_)
{
  if (lib_->findSite(name_))
    return NULL;

  _dbLib*  lib  = (_dbLib*) lib_;
  _dbSite* site = lib->_site_tbl->create();
  site->_name   = strdup(name_);
  ZALLOCATED(site->_name);
  lib->_site_hash.insert(site);
  return (dbSite*) site;
}

dbSite* dbSite::getSite(dbLib* lib_, uint dbid_)
{
  _dbLib* lib = (_dbLib*) lib_;
  return (dbSite*) lib->_site_tbl->getPtr(dbid_);
}

}  // namespace odb
