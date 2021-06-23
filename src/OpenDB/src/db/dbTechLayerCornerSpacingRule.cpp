///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, OpenRoad Project
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
#include "dbTechLayerCornerSpacingRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin Includes
// User Code End Includes
namespace odb {

template class dbTable<_dbTechLayerCornerSpacingRule>;

bool _dbTechLayerCornerSpacingRule::operator==(
    const _dbTechLayerCornerSpacingRule& rhs) const
{
  if (flags_.corner_type_ != rhs.flags_.corner_type_)
    return false;

  if (flags_.same_mask_ != rhs.flags_.same_mask_)
    return false;

  if (flags_.corner_only_ != rhs.flags_.corner_only_)
    return false;

  if (flags_.except_eol_ != rhs.flags_.except_eol_)
    return false;

  if (flags_.except_jog_length_ != rhs.flags_.except_jog_length_)
    return false;

  if (flags_.edge_length_valid_ != rhs.flags_.edge_length_valid_)
    return false;

  if (flags_.include_shape_ != rhs.flags_.include_shape_)
    return false;

  if (flags_.min_length_valid_ != rhs.flags_.min_length_valid_)
    return false;

  if (flags_.except_notch_ != rhs.flags_.except_notch_)
    return false;

  if (flags_.except_notch_length_valid_
      != rhs.flags_.except_notch_length_valid_)
    return false;

  if (flags_.except_same_net_ != rhs.flags_.except_same_net_)
    return false;

  if (flags_.except_same_metal_ != rhs.flags_.except_same_metal_)
    return false;

  if (within_ != rhs.within_)
    return false;

  if (eol_width_ != rhs.eol_width_)
    return false;

  if (jog_length_ != rhs.jog_length_)
    return false;

  if (edge_length_ != rhs.edge_length_)
    return false;

  if (min_length_ != rhs.min_length_)
    return false;

  if (except_notch_length_ != rhs.except_notch_length_)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerCornerSpacingRule::operator<(
    const _dbTechLayerCornerSpacingRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerCornerSpacingRule::differences(
    dbDiff& diff,
    const char* field,
    const _dbTechLayerCornerSpacingRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(flags_.corner_type_);
  DIFF_FIELD(flags_.same_mask_);
  DIFF_FIELD(flags_.corner_only_);
  DIFF_FIELD(flags_.except_eol_);
  DIFF_FIELD(flags_.except_jog_length_);
  DIFF_FIELD(flags_.edge_length_valid_);
  DIFF_FIELD(flags_.include_shape_);
  DIFF_FIELD(flags_.min_length_valid_);
  DIFF_FIELD(flags_.except_notch_);
  DIFF_FIELD(flags_.except_notch_length_valid_);
  DIFF_FIELD(flags_.except_same_net_);
  DIFF_FIELD(flags_.except_same_metal_);
  DIFF_FIELD(within_);
  DIFF_FIELD(eol_width_);
  DIFF_FIELD(jog_length_);
  DIFF_FIELD(edge_length_);
  DIFF_FIELD(min_length_);
  DIFF_FIELD(except_notch_length_);
  // User Code Begin Differences
  // User Code End Differences
  DIFF_END
}
void _dbTechLayerCornerSpacingRule::out(dbDiff& diff,
                                        char side,
                                        const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.corner_type_);
  DIFF_OUT_FIELD(flags_.same_mask_);
  DIFF_OUT_FIELD(flags_.corner_only_);
  DIFF_OUT_FIELD(flags_.except_eol_);
  DIFF_OUT_FIELD(flags_.except_jog_length_);
  DIFF_OUT_FIELD(flags_.edge_length_valid_);
  DIFF_OUT_FIELD(flags_.include_shape_);
  DIFF_OUT_FIELD(flags_.min_length_valid_);
  DIFF_OUT_FIELD(flags_.except_notch_);
  DIFF_OUT_FIELD(flags_.except_notch_length_valid_);
  DIFF_OUT_FIELD(flags_.except_same_net_);
  DIFF_OUT_FIELD(flags_.except_same_metal_);
  DIFF_OUT_FIELD(within_);
  DIFF_OUT_FIELD(eol_width_);
  DIFF_OUT_FIELD(jog_length_);
  DIFF_OUT_FIELD(edge_length_);
  DIFF_OUT_FIELD(min_length_);
  DIFF_OUT_FIELD(except_notch_length_);

  // User Code Begin Out
  // User Code End Out
  DIFF_END
}
_dbTechLayerCornerSpacingRule::_dbTechLayerCornerSpacingRule(_dbDatabase* db)
{
  uint32_t* flags__bit_field = (uint32_t*) &flags_;
  *flags__bit_field = 0;
  within_ = 0;
  eol_width_ = 0;
  jog_length_ = 0;
  edge_length_ = 0;
  min_length_ = 0;
  except_notch_length_ = 0;
  // User Code Begin Constructor
  // User Code End Constructor
}
_dbTechLayerCornerSpacingRule::_dbTechLayerCornerSpacingRule(
    _dbDatabase* db,
    const _dbTechLayerCornerSpacingRule& r)
{
  flags_.corner_type_ = r.flags_.corner_type_;
  flags_.same_mask_ = r.flags_.same_mask_;
  flags_.corner_only_ = r.flags_.corner_only_;
  flags_.except_eol_ = r.flags_.except_eol_;
  flags_.except_jog_length_ = r.flags_.except_jog_length_;
  flags_.edge_length_valid_ = r.flags_.edge_length_valid_;
  flags_.include_shape_ = r.flags_.include_shape_;
  flags_.min_length_valid_ = r.flags_.min_length_valid_;
  flags_.except_notch_ = r.flags_.except_notch_;
  flags_.except_notch_length_valid_ = r.flags_.except_notch_length_valid_;
  flags_.except_same_net_ = r.flags_.except_same_net_;
  flags_.except_same_metal_ = r.flags_.except_same_metal_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  within_ = r.within_;
  eol_width_ = r.eol_width_;
  jog_length_ = r.jog_length_;
  edge_length_ = r.edge_length_;
  min_length_ = r.min_length_;
  except_notch_length_ = r.except_notch_length_;
  // User Code Begin CopyConstructor
  _width_tbl = r._width_tbl;
  _spacing_tbl = r._spacing_tbl;
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCornerSpacingRule& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream >> *flags__bit_field;
  stream >> obj.within_;
  stream >> obj.eol_width_;
  stream >> obj.jog_length_;
  stream >> obj.edge_length_;
  stream >> obj.min_length_;
  stream >> obj.except_notch_length_;
  // User Code Begin >>
  stream >> obj._width_tbl;
  stream >> obj._spacing_tbl;
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCornerSpacingRule& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream << *flags__bit_field;
  stream << obj.within_;
  stream << obj.eol_width_;
  stream << obj.jog_length_;
  stream << obj.edge_length_;
  stream << obj.min_length_;
  stream << obj.except_notch_length_;
  // User Code Begin <<
  stream << obj._width_tbl;
  stream << obj._spacing_tbl;
  // User Code End <<
  return stream;
}

_dbTechLayerCornerSpacingRule::~_dbTechLayerCornerSpacingRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbTechLayerCornerSpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCornerSpacingRule::setWithin(int within)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->within_ = within;
}

int dbTechLayerCornerSpacingRule::getWithin() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->within_;
}

void dbTechLayerCornerSpacingRule::setEolWidth(int eol_width)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->eol_width_ = eol_width;
}

int dbTechLayerCornerSpacingRule::getEolWidth() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->eol_width_;
}

void dbTechLayerCornerSpacingRule::setJogLength(int jog_length)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->jog_length_ = jog_length;
}

int dbTechLayerCornerSpacingRule::getJogLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->jog_length_;
}

void dbTechLayerCornerSpacingRule::setEdgeLength(int edge_length)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->edge_length_ = edge_length;
}

int dbTechLayerCornerSpacingRule::getEdgeLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->edge_length_;
}

void dbTechLayerCornerSpacingRule::setMinLength(int min_length)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->min_length_ = min_length;
}

int dbTechLayerCornerSpacingRule::getMinLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->min_length_;
}

void dbTechLayerCornerSpacingRule::setExceptNotchLength(int except_notch_length)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->except_notch_length_ = except_notch_length;
}

int dbTechLayerCornerSpacingRule::getExceptNotchLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->except_notch_length_;
}

void dbTechLayerCornerSpacingRule::setSameMask(bool same_mask)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.same_mask_ = same_mask;
}

bool dbTechLayerCornerSpacingRule::isSameMask() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.same_mask_;
}

void dbTechLayerCornerSpacingRule::setCornerOnly(bool corner_only)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.corner_only_ = corner_only;
}

bool dbTechLayerCornerSpacingRule::isCornerOnly() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.corner_only_;
}

void dbTechLayerCornerSpacingRule::setExceptEol(bool except_eol)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_eol_ = except_eol;
}

bool dbTechLayerCornerSpacingRule::isExceptEol() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_eol_;
}

void dbTechLayerCornerSpacingRule::setExceptJogLength(bool except_jog_length)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_jog_length_ = except_jog_length;
}

bool dbTechLayerCornerSpacingRule::isExceptJogLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_jog_length_;
}

void dbTechLayerCornerSpacingRule::setEdgeLengthValid(bool edge_length_valid)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.edge_length_valid_ = edge_length_valid;
}

bool dbTechLayerCornerSpacingRule::isEdgeLengthValid() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.edge_length_valid_;
}

void dbTechLayerCornerSpacingRule::setIncludeShape(bool include_shape)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.include_shape_ = include_shape;
}

bool dbTechLayerCornerSpacingRule::isIncludeShape() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.include_shape_;
}

void dbTechLayerCornerSpacingRule::setMinLengthValid(bool min_length_valid)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.min_length_valid_ = min_length_valid;
}

bool dbTechLayerCornerSpacingRule::isMinLengthValid() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.min_length_valid_;
}

void dbTechLayerCornerSpacingRule::setExceptNotch(bool except_notch)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_notch_ = except_notch;
}

bool dbTechLayerCornerSpacingRule::isExceptNotch() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_notch_;
}

void dbTechLayerCornerSpacingRule::setExceptNotchLengthValid(
    bool except_notch_length_valid)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_notch_length_valid_ = except_notch_length_valid;
}

bool dbTechLayerCornerSpacingRule::isExceptNotchLengthValid() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_notch_length_valid_;
}

void dbTechLayerCornerSpacingRule::setExceptSameNet(bool except_same_net)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_same_net_ = except_same_net;
}

bool dbTechLayerCornerSpacingRule::isExceptSameNet() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_same_net_;
}

void dbTechLayerCornerSpacingRule::setExceptSameMetal(bool except_same_metal)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.except_same_metal_ = except_same_metal;
}

bool dbTechLayerCornerSpacingRule::isExceptSameMetal() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->flags_.except_same_metal_;
}

// User Code Begin dbTechLayerCornerSpacingRulePublicMethods
void dbTechLayerCornerSpacingRule::addSpacing(uint width,
                                              uint spacing1,
                                              uint spacing2)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  obj->_width_tbl.push_back(width);
  obj->_spacing_tbl.push_back(std::make_pair(spacing1, spacing2));
}

void dbTechLayerCornerSpacingRule::getSpacingTable(
    std::vector<std::pair<int, int>>& tbl)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  tbl = obj->_spacing_tbl;
}

void dbTechLayerCornerSpacingRule::getWidthTable(std::vector<int>& tbl)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  tbl = obj->_width_tbl;
}

void dbTechLayerCornerSpacingRule::setType(CornerType _type)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->flags_.corner_type_ = (uint) _type;
}

dbTechLayerCornerSpacingRule::CornerType dbTechLayerCornerSpacingRule::getType()
    const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return (dbTechLayerCornerSpacingRule::CornerType) obj->flags_.corner_type_;
}

dbTechLayerCornerSpacingRule* dbTechLayerCornerSpacingRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerCornerSpacingRule* newrule
      = layer->corner_spacing_rules_tbl_->create();
  return ((dbTechLayerCornerSpacingRule*) newrule);
}

dbTechLayerCornerSpacingRule*
dbTechLayerCornerSpacingRule::getTechLayerCornerSpacingRule(dbTechLayer* inly,
                                                            uint dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerCornerSpacingRule*)
      layer->corner_spacing_rules_tbl_->getPtr(dbid);
}
void dbTechLayerCornerSpacingRule::destroy(dbTechLayerCornerSpacingRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->corner_spacing_rules_tbl_->destroy(
      (_dbTechLayerCornerSpacingRule*) rule);
}
// User Code End dbTechLayerCornerSpacingRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp