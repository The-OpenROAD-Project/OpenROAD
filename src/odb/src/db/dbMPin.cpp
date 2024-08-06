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

#include "dbMPin.h"

#include "dbAccessPoint.h"
#include "dbBlock.h"
#include "dbBoxItr.h"
#include "dbMPinItr.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbPolygonItr.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"

namespace odb {

template class dbTable<_dbMTerm>;

_dbMPin::_dbMPin(_dbDatabase* db)
{
}

_dbMPin::_dbMPin(_dbDatabase* db, const _dbMPin& p)
    : _mterm(p._mterm), _geoms(p._geoms), _next_mpin(p._next_mpin)
{
}

_dbMPin::~_dbMPin()
{
}

dbOStream& operator<<(dbOStream& stream, const _dbMPin& mpin)
{
  stream << mpin._mterm;
  stream << mpin._geoms;
  stream << mpin._poly_geoms;
  stream << mpin._next_mpin;
  stream << mpin.aps_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbMPin& mpin)
{
  stream >> mpin._mterm;
  stream >> mpin._geoms;
  _dbDatabase* db = mpin.getImpl()->getDatabase();
  if (db->isSchema(db_schema_polygon)) {
    stream >> mpin._poly_geoms;
  }
  stream >> mpin._next_mpin;
  stream >> mpin.aps_;
  return stream;
}

bool _dbMPin::operator==(const _dbMPin& rhs) const
{
  if (_mterm != rhs._mterm) {
    return false;
  }

  if (_geoms != rhs._geoms) {
    return false;
  }

  if (_next_mpin != rhs._next_mpin) {
    return false;
  }

  if (aps_ != rhs.aps_) {
    return false;
  }

  return true;
}

void _dbMPin::differences(dbDiff& diff,
                          const char* field,
                          const _dbMPin& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_mterm);
  DIFF_FIELD(_geoms);
  DIFF_FIELD(_next_mpin);
  // DIFF_VECTOR(aps_);
  DIFF_END
}

void _dbMPin::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_mterm);
  DIFF_OUT_FIELD(_geoms);
  DIFF_OUT_FIELD(_next_mpin);
  // DIFF_OUT_VECTOR(aps_);
  DIFF_END
}

void _dbMPin::addAccessPoint(uint idx, _dbAccessPoint* ap)
{
  if (aps_.size() <= idx) {
    aps_.resize(idx + 1);
  }
  aps_[idx].push_back(ap->getOID());
}

////////////////////////////////////////////////////////////////////
//
// dbMPin - Methods
//
////////////////////////////////////////////////////////////////////

dbMTerm* dbMPin::getMTerm()
{
  _dbMPin* pin = (_dbMPin*) this;
  _dbMaster* master = (_dbMaster*) pin->getOwner();
  return (dbMTerm*) master->_mterm_tbl->getPtr(pin->_mterm);
}

dbMaster* dbMPin::getMaster()
{
  return (dbMaster*) getImpl()->getOwner();
}

dbSet<dbBox> dbMPin::getGeometry(bool include_decomposed_polygons)
{
  _dbMPin* pin = (_dbMPin*) this;
  _dbMaster* master = (_dbMaster*) pin->getOwner();
  if (include_decomposed_polygons) {
    return dbSet<dbBox>(pin, master->_box_itr);
  }
  return dbSet<dbBox>(pin, master->_pbox_box_itr);
}

dbSet<dbPolygon> dbMPin::getPolygonGeometry()
{
  _dbMPin* pin = (_dbMPin*) this;
  _dbMaster* master = (_dbMaster*) pin->getOwner();
  return dbSet<dbPolygon>(pin, master->_pbox_itr);
}

Rect dbMPin::getBBox()
{
  Rect bbox;
  bbox.mergeInit();
  for (dbBox* box : getGeometry()) {
    Rect rect = box->getBox();
    bbox.merge(rect);
  }
  return bbox;
}

std::vector<std::vector<odb::dbAccessPoint*>> dbMPin::getPinAccess() const
{
  _dbMPin* pin = (_dbMPin*) this;
  // TODO: fix for multi chip block heirarchy
  _dbBlock* block = (_dbBlock*) getDb()->getChip()->getBlock();
  std::vector<std::vector<odb::dbAccessPoint*>> result;
  for (const auto& pa : pin->aps_) {
    result.push_back(std::vector<odb::dbAccessPoint*>());
    for (const auto& ap : pa) {
      result.back().push_back((dbAccessPoint*) block->ap_tbl_->getPtr(ap));
    }
  }
  return result;
}

dbMPin* dbMPin::create(dbMTerm* mterm_)
{
  _dbMTerm* mterm = (_dbMTerm*) mterm_;
  _dbMaster* master = (_dbMaster*) mterm->getOwner();
  _dbMPin* mpin = master->_mpin_tbl->create();
  mpin->_mterm = mterm->getOID();
  mpin->_next_mpin = mterm->_pins;
  mterm->_pins = mpin->getOID();
  return (dbMPin*) mpin;
}

dbMPin* dbMPin::getMPin(dbMaster* master_, uint dbid_)
{
  _dbMaster* master = (_dbMaster*) master_;
  return (dbMPin*) master->_mpin_tbl->getPtr(dbid_);
}

}  // namespace odb
