///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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

// Generator Code Begin Cpp
#include "dbAccessPoint.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTypes.h"
// User Code Begin Includes
#include <algorithm>

#include "dbMPin.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {

template class dbTable<_dbAccessPoint>;

bool _dbAccessPoint::operator==(const _dbAccessPoint& rhs) const
{
  if (layer_ != rhs.layer_)
    return false;

  if (low_type_ != rhs.low_type_)
    return false;

  if (high_type_ != rhs.high_type_)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbAccessPoint::operator<(const _dbAccessPoint& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbAccessPoint::differences(dbDiff& diff,
                                 const char* field,
                                 const _dbAccessPoint& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(layer_);
  DIFF_FIELD(low_type_);
  DIFF_FIELD(high_type_);
  // User Code Begin Differences
  // User Code End Differences
  DIFF_END
}
void _dbAccessPoint::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(layer_);
  DIFF_OUT_FIELD(low_type_);
  DIFF_OUT_FIELD(high_type_);

  // User Code Begin Out
  // User Code End Out
  DIFF_END
}
_dbAccessPoint::_dbAccessPoint(_dbDatabase* db)
{
  // User Code Begin Constructor
  accesses_.resize(6, false);
  // User Code End Constructor
}
_dbAccessPoint::_dbAccessPoint(_dbDatabase* db, const _dbAccessPoint& r)
{
  layer_ = r.layer_;
  low_type_ = r.low_type_;
  high_type_ = r.high_type_;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbAccessPoint& obj)
{
  stream >> obj.point_;
  stream >> obj.layer_;
  stream >> obj.accesses_;
  stream >> obj.low_type_;
  stream >> obj.high_type_;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbAccessPoint& obj)
{
  stream << obj.point_;
  stream << obj.layer_;
  stream << obj.accesses_;
  stream << obj.low_type_;
  stream << obj.high_type_;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbAccessPoint::~_dbAccessPoint()
{
  // User Code Begin Destructor
  // User Code End Destructor
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbAccessPoint - Methods
//
////////////////////////////////////////////////////////////////////

void dbAccessPoint::setPoint(Point point)
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;

  obj->point_ = point;
}

Point dbAccessPoint::getPoint() const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  return obj->point_;
}

void dbAccessPoint::setLayer(dbTechLayer* layer)
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;

  obj->layer_ = layer->getImpl()->getOID();
}

// User Code Begin dbAccessPointPublicMethods

void dbAccessPoint::setAccesses(const std::vector<dbDirection>& accesses)
{
  for (auto dir : accesses) {
    setAccess(true, dir);
  }
}

void dbAccessPoint::getAccesses(std::vector<dbDirection>& tbl) const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  for (int dir = 0; dir < 6; dir++) {
    if (obj->accesses_[dir])
      tbl.push_back(dbDirection::Value(dir + 1));
  }
}

void dbAccessPoint::setLowType(AccessType low_type_)
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;

  obj->low_type_ = low_type_;
}

dbAccessPoint::AccessType dbAccessPoint::getLowType() const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  return dbAccessPoint::AccessType(obj->low_type_);
}

void dbAccessPoint::setHighType(AccessType high_type_)
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;

  obj->high_type_ = high_type_;
}

dbAccessPoint::AccessType dbAccessPoint::getHighType() const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  return dbAccessPoint::AccessType(obj->high_type_);
}

void dbAccessPoint::setAccess(bool access, dbDirection dir)
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  switch (dir) {
    case dbDirection::EAST:
    case dbDirection::SOUTH:
    case dbDirection::WEST:
    case dbDirection::NORTH:
    case dbDirection::UP:
    case dbDirection::DOWN:
      obj->accesses_[dir - 1] = access;
      return;
    case dbDirection::NONE:
      break;
  }
  getImpl()->getLogger()->error(
      utl::ODB, 1100, "Access direction is of unknown type");
}

bool dbAccessPoint::hasAccess(dbDirection dir) const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  switch (dir) {
    case dbDirection::EAST:
    case dbDirection::SOUTH:
    case dbDirection::WEST:
    case dbDirection::NORTH:
    case dbDirection::UP:
    case dbDirection::DOWN:
      return obj->accesses_[dir - 1];
      break;
    case dbDirection::NONE:
      return (std::count(obj->accesses_.begin(), obj->accesses_.end(), true)
              > 0);
  }
  getImpl()->getLogger()->error(
      utl::ODB, 1101, "Access direction is of unknown type");
}

dbTechLayer* dbAccessPoint::getLayer() const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  dbDatabase* db = (dbDatabase*) obj->getDatabase();
  _dbTech* tech = (_dbTech*) db->getTech();
  return (dbTechLayer*) tech->_layer_tbl->getPtr(obj->layer_);
}

dbMPin* dbAccessPoint::getMPin() const
{
  _dbAccessPoint* obj = (_dbAccessPoint*) this;
  return (dbMPin*) obj->getOwner();
}

dbAccessPoint* dbAccessPoint::create(dbMPin* pin)
{
  _dbMPin* mpin = (_dbMPin*) pin;
  _dbAccessPoint* ap = mpin->ap_tbl_->create();
  return ((dbAccessPoint*) ap);
}

dbAccessPoint* dbAccessPoint::getAccessPoint(dbMPin* pin, uint dbid)
{
  _dbMPin* mpin = (_dbMPin*) pin;
  return (dbAccessPoint*) mpin->ap_tbl_->getPtr(dbid);
}

void dbAccessPoint::destroy(dbAccessPoint* ap)
{
  _dbMPin* mpin = (_dbMPin*) ap->getImpl()->getOwner();
  dbProperty::destroyProperties(ap);
  mpin->ap_tbl_->destroy((_dbAccessPoint*) ap);
}
// User Code End dbAccessPointPublicMethods
}  // namespace odb
   // Generator Code End Cpp