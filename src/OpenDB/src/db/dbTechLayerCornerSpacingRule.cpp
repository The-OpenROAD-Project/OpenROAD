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

// Generator Code Begin cpp
#include "dbTechLayerCornerSpacingRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin includes
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbTechLayerCornerSpacingRule>;

bool _dbTechLayerCornerSpacingRule::operator==(
    const _dbTechLayerCornerSpacingRule& rhs) const
{
  if (_flags.corner_type_ != rhs._flags.corner_type_)
    return false;

  if (_flags.same_mask_ != rhs._flags.same_mask_)
    return false;

  if (_flags.corner_only_ != rhs._flags.corner_only_)
    return false;

  if (_flags.except_eol_ != rhs._flags.except_eol_)
    return false;

  if (_flags.except_jog_length_ != rhs._flags.except_jog_length_)
    return false;

  if (_flags.edge_length_valid_ != rhs._flags.edge_length_valid_)
    return false;

  if (_flags.include_shape_ != rhs._flags.include_shape_)
    return false;

  if (_flags.min_length_valid_ != rhs._flags.min_length_valid_)
    return false;

  if (_flags.except_notch_ != rhs._flags.except_notch_)
    return false;

  if (_flags.except_notch_length_valid_
      != rhs._flags.except_notch_length_valid_)
    return false;

  if (_flags.except_same_net_ != rhs._flags.except_same_net_)
    return false;

  if (_flags.except_same_metal_ != rhs._flags.except_same_metal_)
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
    dbDiff&                              diff,
    const char*                          field,
    const _dbTechLayerCornerSpacingRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_flags.corner_type_);
  DIFF_FIELD(_flags.same_mask_);
  DIFF_FIELD(_flags.corner_only_);
  DIFF_FIELD(_flags.except_eol_);
  DIFF_FIELD(_flags.except_jog_length_);
  DIFF_FIELD(_flags.edge_length_valid_);
  DIFF_FIELD(_flags.include_shape_);
  DIFF_FIELD(_flags.min_length_valid_);
  DIFF_FIELD(_flags.except_notch_);
  DIFF_FIELD(_flags.except_notch_length_valid_);
  DIFF_FIELD(_flags.except_same_net_);
  DIFF_FIELD(_flags.except_same_metal_);
  DIFF_FIELD(within_);
  DIFF_FIELD(eol_width_);
  DIFF_FIELD(jog_length_);
  DIFF_FIELD(edge_length_);
  DIFF_FIELD(min_length_);
  DIFF_FIELD(except_notch_length_);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerCornerSpacingRule::out(dbDiff&     diff,
                                        char        side,
                                        const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags.corner_type_);
  DIFF_OUT_FIELD(_flags.same_mask_);
  DIFF_OUT_FIELD(_flags.corner_only_);
  DIFF_OUT_FIELD(_flags.except_eol_);
  DIFF_OUT_FIELD(_flags.except_jog_length_);
  DIFF_OUT_FIELD(_flags.edge_length_valid_);
  DIFF_OUT_FIELD(_flags.include_shape_);
  DIFF_OUT_FIELD(_flags.min_length_valid_);
  DIFF_OUT_FIELD(_flags.except_notch_);
  DIFF_OUT_FIELD(_flags.except_notch_length_valid_);
  DIFF_OUT_FIELD(_flags.except_same_net_);
  DIFF_OUT_FIELD(_flags.except_same_metal_);
  DIFF_OUT_FIELD(within_);
  DIFF_OUT_FIELD(eol_width_);
  DIFF_OUT_FIELD(jog_length_);
  DIFF_OUT_FIELD(edge_length_);
  DIFF_OUT_FIELD(min_length_);
  DIFF_OUT_FIELD(except_notch_length_);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerCornerSpacingRule::_dbTechLayerCornerSpacingRule(_dbDatabase* db)
{
  uint32_t* _flags_bit_field = (uint32_t*) &_flags;
  *_flags_bit_field          = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerCornerSpacingRule::_dbTechLayerCornerSpacingRule(
    _dbDatabase*                         db,
    const _dbTechLayerCornerSpacingRule& r)
{
  _flags.corner_type_               = r._flags.corner_type_;
  _flags.same_mask_                 = r._flags.same_mask_;
  _flags.corner_only_               = r._flags.corner_only_;
  _flags.except_eol_                = r._flags.except_eol_;
  _flags.except_jog_length_         = r._flags.except_jog_length_;
  _flags.edge_length_valid_         = r._flags.edge_length_valid_;
  _flags.include_shape_             = r._flags.include_shape_;
  _flags.min_length_valid_          = r._flags.min_length_valid_;
  _flags.except_notch_              = r._flags.except_notch_;
  _flags.except_notch_length_valid_ = r._flags.except_notch_length_valid_;
  _flags.except_same_net_           = r._flags.except_same_net_;
  _flags.except_same_metal_         = r._flags.except_same_metal_;
  _flags._spare_bits                = r._flags._spare_bits;
  within_                           = r.within_;
  eol_width_                        = r.eol_width_;
  jog_length_                       = r.jog_length_;
  edge_length_                      = r.edge_length_;
  min_length_                       = r.min_length_;
  except_notch_length_              = r.except_notch_length_;
  // User Code Begin CopyConstructor
  _width_tbl   = r._width_tbl;
  _spacing_tbl = r._spacing_tbl;
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCornerSpacingRule& obj)
{
  uint32_t* _flags_bit_field = (uint32_t*) &obj._flags;
  stream >> *_flags_bit_field;
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
dbOStream& operator<<(dbOStream&                           stream,
                      const _dbTechLayerCornerSpacingRule& obj)
{
  uint32_t* _flags_bit_field = (uint32_t*) &obj._flags;
  stream << *_flags_bit_field;
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

void dbTechLayerCornerSpacingRule::setWithin(int within_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->within_ = within_;
}

int dbTechLayerCornerSpacingRule::getWithin() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->within_;
}

void dbTechLayerCornerSpacingRule::setEolWidth(int eol_width_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->eol_width_ = eol_width_;
}

int dbTechLayerCornerSpacingRule::getEolWidth() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->eol_width_;
}

void dbTechLayerCornerSpacingRule::setJogLength(int jog_length_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->jog_length_ = jog_length_;
}

int dbTechLayerCornerSpacingRule::getJogLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->jog_length_;
}

void dbTechLayerCornerSpacingRule::setEdgeLength(int edge_length_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->edge_length_ = edge_length_;
}

int dbTechLayerCornerSpacingRule::getEdgeLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->edge_length_;
}

void dbTechLayerCornerSpacingRule::setMinLength(int min_length_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->min_length_ = min_length_;
}

int dbTechLayerCornerSpacingRule::getMinLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->min_length_;
}

void dbTechLayerCornerSpacingRule::setExceptNotchLength(
    int except_notch_length_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->except_notch_length_ = except_notch_length_;
}

int dbTechLayerCornerSpacingRule::getExceptNotchLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  return obj->except_notch_length_;
}

void dbTechLayerCornerSpacingRule::setSameMask(bool same_mask_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags.same_mask_ = same_mask_;
}

bool dbTechLayerCornerSpacingRule::isSameMask() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags.same_mask_;
}

void dbTechLayerCornerSpacingRule::setCornerOnly(bool corner_only_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags.corner_only_ = corner_only_;
}

bool dbTechLayerCornerSpacingRule::isCornerOnly() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags.corner_only_;
}

void dbTechLayerCornerSpacingRule::setExceptEol(bool except_eol_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags.except_eol_ = except_eol_;
}

bool dbTechLayerCornerSpacingRule::isExceptEol() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags.except_eol_;
}

void dbTechLayerCornerSpacingRule::setExceptJogLength(bool except_jog_length_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags.except_jog_length_ = except_jog_length_;
}

bool dbTechLayerCornerSpacingRule::isExceptJogLength() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags.except_jog_length_;
}

void dbTechLayerCornerSpacingRule::setEdgeLengthValid(bool edge_length_valid_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags.edge_length_valid_ = edge_length_valid_;
}

bool dbTechLayerCornerSpacingRule::isEdgeLengthValid() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags.edge_length_valid_;
}

void dbTechLayerCornerSpacingRule::setIncludeShape(bool include_shape_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags.include_shape_ = include_shape_;
}

bool dbTechLayerCornerSpacingRule::isIncludeShape() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags.include_shape_;
}

void dbTechLayerCornerSpacingRule::setMinLengthValid(bool min_length_valid_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags.min_length_valid_ = min_length_valid_;
}

bool dbTechLayerCornerSpacingRule::isMinLengthValid() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags.min_length_valid_;
}

void dbTechLayerCornerSpacingRule::setExceptNotch(bool except_notch_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags.except_notch_ = except_notch_;
}

bool dbTechLayerCornerSpacingRule::isExceptNotch() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags.except_notch_;
}

void dbTechLayerCornerSpacingRule::setExceptNotchLengthValid(
    bool except_notch_length_valid_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags.except_notch_length_valid_ = except_notch_length_valid_;
}

bool dbTechLayerCornerSpacingRule::isExceptNotchLengthValid() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags.except_notch_length_valid_;
}

void dbTechLayerCornerSpacingRule::setExceptSameNet(bool except_same_net_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags.except_same_net_ = except_same_net_;
}

bool dbTechLayerCornerSpacingRule::isExceptSameNet() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags.except_same_net_;
}

void dbTechLayerCornerSpacingRule::setExceptSameMetal(bool except_same_metal_)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags.except_same_metal_ = except_same_metal_;
}

bool dbTechLayerCornerSpacingRule::isExceptSameMetal() const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return obj->_flags.except_same_metal_;
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
  tbl                                = obj->_spacing_tbl;
}

void dbTechLayerCornerSpacingRule::getWidthTable(std::vector<int>& tbl)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;
  tbl                                = obj->_width_tbl;
}

void dbTechLayerCornerSpacingRule::setType(CornerType _type)
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  obj->_flags.corner_type_ = (uint) _type;
}

dbTechLayerCornerSpacingRule::CornerType dbTechLayerCornerSpacingRule::getType()
    const
{
  _dbTechLayerCornerSpacingRule* obj = (_dbTechLayerCornerSpacingRule*) this;

  return (dbTechLayerCornerSpacingRule::CornerType) obj->_flags.corner_type_;
}

dbTechLayerCornerSpacingRule* dbTechLayerCornerSpacingRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer*                  layer = (_dbTechLayer*) _layer;
  _dbTechLayerCornerSpacingRule* newrule
      = layer->_corner_spacing_rules_tbl->create();
  return ((dbTechLayerCornerSpacingRule*) newrule);
}

dbTechLayerCornerSpacingRule*
dbTechLayerCornerSpacingRule::getTechLayerCornerSpacingRule(dbTechLayer* inly,
                                                            uint         dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerCornerSpacingRule*)
      layer->_corner_spacing_rules_tbl->getPtr(dbid);
}
void dbTechLayerCornerSpacingRule::destroy(dbTechLayerCornerSpacingRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->_corner_spacing_rules_tbl->destroy(
      (_dbTechLayerCornerSpacingRule*) rule);
}
// User Code End dbTechLayerCornerSpacingRulePublicMethods
}  // namespace odb
   // Generator Code End cpp