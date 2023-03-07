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

// Generator Code Begin Cpp
#include "dbTechLayerPitchRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin Includes
// User Code End Includes
namespace odb {

template class dbTable<_dbTechLayerPitchRule>;

bool _dbTechLayerPitchRule::operator==(const _dbTechLayerPitchRule& rhs) const
{
  if (first_last_pitch_ != rhs.first_last_pitch_)
    return false;

  if (pitch_x_ != rhs.pitch_x_)
    return false;

  if (pitch_y_ != rhs.pitch_y_)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerPitchRule::operator<(const _dbTechLayerPitchRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerPitchRule::differences(dbDiff& diff,
                                        const char* field,
                                        const _dbTechLayerPitchRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(first_last_pitch_);
  DIFF_FIELD(pitch_x_);
  DIFF_FIELD(pitch_y_);
  // User Code Begin Differences
  // User Code End Differences
  DIFF_END
}
void _dbTechLayerPitchRule::out(dbDiff& diff,
                                char side,
                                const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(first_last_pitch_);
  DIFF_OUT_FIELD(pitch_x_);
  DIFF_OUT_FIELD(pitch_y_);

  // User Code Begin Out
  // User Code End Out
  DIFF_END
}
_dbTechLayerPitchRule::_dbTechLayerPitchRule(_dbDatabase* db)
{
  // User Code Begin Constructor
  // User Code End Constructor
}
_dbTechLayerPitchRule::_dbTechLayerPitchRule(_dbDatabase* db,
                                             const _dbTechLayerPitchRule& r)
{
  first_last_pitch_ = r.first_last_pitch_;
  pitch_x_ = r.pitch_x_;
  pitch_y_ = r.pitch_y_;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerPitchRule& obj)
{
  stream >> obj.first_last_pitch_;
  stream >> obj.pitch_x_;
  stream >> obj.pitch_y_;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerPitchRule& obj)
{
  stream << obj.first_last_pitch_;
  stream << obj.pitch_x_;
  stream << obj.pitch_y_;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerPitchRule::~_dbTechLayerPitchRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbTechLayerPitchRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerPitchRule::setFirstLastPitch(int first_last_pitch)
{
  _dbTechLayerPitchRule* obj = (_dbTechLayerPitchRule*) this;

  obj->first_last_pitch_ = first_last_pitch;
}

int dbTechLayerPitchRule::getFirstLastPitch() const
{
  _dbTechLayerPitchRule* obj = (_dbTechLayerPitchRule*) this;
  return obj->first_last_pitch_;
}

void dbTechLayerPitchRule::setPitchX(int pitch_x)
{
  _dbTechLayerPitchRule* obj = (_dbTechLayerPitchRule*) this;

  obj->pitch_x_ = pitch_x;
}

int dbTechLayerPitchRule::getPitchX() const
{
  _dbTechLayerPitchRule* obj = (_dbTechLayerPitchRule*) this;
  return obj->pitch_x_;
}

void dbTechLayerPitchRule::setPitchY(int pitch_y)
{
  _dbTechLayerPitchRule* obj = (_dbTechLayerPitchRule*) this;

  obj->pitch_y_ = pitch_y;
}

int dbTechLayerPitchRule::getPitchY() const
{
  _dbTechLayerPitchRule* obj = (_dbTechLayerPitchRule*) this;
  return obj->pitch_y_;
}

// User Code Begin dbTechLayerPitchRulePublicMethods
dbTechLayerPitchRule* dbTechLayerPitchRule::create(dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerPitchRule* newrule = layer->pitch_rules_tbl_->create();
  newrule->first_last_pitch_ = -1;
  newrule->pitch_x_ = 0;
  newrule->pitch_y_ = 0;
  return ((dbTechLayerPitchRule*) newrule);
}

void dbTechLayerPitchRule::setPitch(int pitch)
{
  _dbTechLayerPitchRule* obj = (_dbTechLayerPitchRule*) this;

  obj->pitch_x_ = pitch;
  obj->pitch_y_ = pitch;
}

int dbTechLayerPitchRule::getPitch() const
{
  _dbTechLayerPitchRule* obj = (_dbTechLayerPitchRule*) this;
  return obj->pitch_x_;
}

void dbTechLayerPitchRule::destroy(dbTechLayerPitchRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->pitch_rules_tbl_->destroy((_dbTechLayerPitchRule*) rule);
}
// User Code End dbTechLayerPitchRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp