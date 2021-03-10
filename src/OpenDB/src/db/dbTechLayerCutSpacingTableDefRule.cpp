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
#include "dbTechLayerCutSpacingTableDefRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin Includes
#include "dbTech.h"
#include "dbTechLayerCutClassRule.h"
// User Code End Includes
namespace odb {

template class dbTable<_dbTechLayerCutSpacingTableDefRule>;

bool _dbTechLayerCutSpacingTableDefRule::operator==(
    const _dbTechLayerCutSpacingTableDefRule& rhs) const
{
  if (flags_.default_valid_ != rhs.flags_.default_valid_)
    return false;

  if (flags_.same_mask_ != rhs.flags_.same_mask_)
    return false;

  if (flags_.same_net_ != rhs.flags_.same_net_)
    return false;

  if (flags_.same_metal_ != rhs.flags_.same_metal_)
    return false;

  if (flags_.same_via_ != rhs.flags_.same_via_)
    return false;

  if (flags_.layer_valid_ != rhs.flags_.layer_valid_)
    return false;

  if (flags_.no_stack_ != rhs.flags_.no_stack_)
    return false;

  if (flags_.non_zero_enclosure_ != rhs.flags_.non_zero_enclosure_)
    return false;

  if (flags_.prl_for_aligned_cut_ != rhs.flags_.prl_for_aligned_cut_)
    return false;

  if (flags_.center_to_center_valid_ != rhs.flags_.center_to_center_valid_)
    return false;

  if (flags_.center_and_edge_valid_ != rhs.flags_.center_and_edge_valid_)
    return false;

  if (flags_.no_prl_ != rhs.flags_.no_prl_)
    return false;

  if (flags_.prl_valid_ != rhs.flags_.prl_valid_)
    return false;

  if (flags_.max_x_y_ != rhs.flags_.max_x_y_)
    return false;

  if (flags_.end_extension_valid_ != rhs.flags_.end_extension_valid_)
    return false;

  if (flags_.side_extension_valid_ != rhs.flags_.side_extension_valid_)
    return false;

  if (flags_.exact_aligned_spacing_valid_
      != rhs.flags_.exact_aligned_spacing_valid_)
    return false;

  if (flags_.horizontal_ != rhs.flags_.horizontal_)
    return false;

  if (flags_.prl_horizontal_ != rhs.flags_.prl_horizontal_)
    return false;

  if (flags_.vertical_ != rhs.flags_.vertical_)
    return false;

  if (flags_.prl_vertical_ != rhs.flags_.prl_vertical_)
    return false;

  if (flags_.non_opposite_enclosure_spacing_valid_
      != rhs.flags_.non_opposite_enclosure_spacing_valid_)
    return false;

  if (flags_.opposite_enclosure_resize_spacing_valid_
      != rhs.flags_.opposite_enclosure_resize_spacing_valid_)
    return false;

  if (default_ != rhs.default_)
    return false;

  if (second_layer_ != rhs.second_layer_)
    return false;

  if (prl_ != rhs.prl_)
    return false;

  if (extension_ != rhs.extension_)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerCutSpacingTableDefRule::operator<(
    const _dbTechLayerCutSpacingTableDefRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerCutSpacingTableDefRule::differences(
    dbDiff&                                   diff,
    const char*                               field,
    const _dbTechLayerCutSpacingTableDefRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(flags_.default_valid_);
  DIFF_FIELD(flags_.same_mask_);
  DIFF_FIELD(flags_.same_net_);
  DIFF_FIELD(flags_.same_metal_);
  DIFF_FIELD(flags_.same_via_);
  DIFF_FIELD(flags_.layer_valid_);
  DIFF_FIELD(flags_.no_stack_);
  DIFF_FIELD(flags_.non_zero_enclosure_);
  DIFF_FIELD(flags_.prl_for_aligned_cut_);
  DIFF_FIELD(flags_.center_to_center_valid_);
  DIFF_FIELD(flags_.center_and_edge_valid_);
  DIFF_FIELD(flags_.no_prl_);
  DIFF_FIELD(flags_.prl_valid_);
  DIFF_FIELD(flags_.max_x_y_);
  DIFF_FIELD(flags_.end_extension_valid_);
  DIFF_FIELD(flags_.side_extension_valid_);
  DIFF_FIELD(flags_.exact_aligned_spacing_valid_);
  DIFF_FIELD(flags_.horizontal_);
  DIFF_FIELD(flags_.prl_horizontal_);
  DIFF_FIELD(flags_.vertical_);
  DIFF_FIELD(flags_.prl_vertical_);
  DIFF_FIELD(flags_.non_opposite_enclosure_spacing_valid_);
  DIFF_FIELD(flags_.opposite_enclosure_resize_spacing_valid_);
  DIFF_FIELD(default_);
  DIFF_FIELD(second_layer_);
  DIFF_FIELD(prl_);
  DIFF_FIELD(extension_);
  // User Code Begin Differences
  // User Code End Differences
  DIFF_END
}
void _dbTechLayerCutSpacingTableDefRule::out(dbDiff&     diff,
                                             char        side,
                                             const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.default_valid_);
  DIFF_OUT_FIELD(flags_.same_mask_);
  DIFF_OUT_FIELD(flags_.same_net_);
  DIFF_OUT_FIELD(flags_.same_metal_);
  DIFF_OUT_FIELD(flags_.same_via_);
  DIFF_OUT_FIELD(flags_.layer_valid_);
  DIFF_OUT_FIELD(flags_.no_stack_);
  DIFF_OUT_FIELD(flags_.non_zero_enclosure_);
  DIFF_OUT_FIELD(flags_.prl_for_aligned_cut_);
  DIFF_OUT_FIELD(flags_.center_to_center_valid_);
  DIFF_OUT_FIELD(flags_.center_and_edge_valid_);
  DIFF_OUT_FIELD(flags_.no_prl_);
  DIFF_OUT_FIELD(flags_.prl_valid_);
  DIFF_OUT_FIELD(flags_.max_x_y_);
  DIFF_OUT_FIELD(flags_.end_extension_valid_);
  DIFF_OUT_FIELD(flags_.side_extension_valid_);
  DIFF_OUT_FIELD(flags_.exact_aligned_spacing_valid_);
  DIFF_OUT_FIELD(flags_.horizontal_);
  DIFF_OUT_FIELD(flags_.prl_horizontal_);
  DIFF_OUT_FIELD(flags_.vertical_);
  DIFF_OUT_FIELD(flags_.prl_vertical_);
  DIFF_OUT_FIELD(flags_.non_opposite_enclosure_spacing_valid_);
  DIFF_OUT_FIELD(flags_.opposite_enclosure_resize_spacing_valid_);
  DIFF_OUT_FIELD(default_);
  DIFF_OUT_FIELD(second_layer_);
  DIFF_OUT_FIELD(prl_);
  DIFF_OUT_FIELD(extension_);

  // User Code Begin Out
  // User Code End Out
  DIFF_END
}
_dbTechLayerCutSpacingTableDefRule::_dbTechLayerCutSpacingTableDefRule(
    _dbDatabase* db)
{
  uint32_t* flags__bit_field = (uint32_t*) &flags_;
  *flags__bit_field          = 0;
  // User Code Begin Constructor
  // User Code End Constructor
}
_dbTechLayerCutSpacingTableDefRule::_dbTechLayerCutSpacingTableDefRule(
    _dbDatabase*                              db,
    const _dbTechLayerCutSpacingTableDefRule& r)
{
  flags_.default_valid_               = r.flags_.default_valid_;
  flags_.same_mask_                   = r.flags_.same_mask_;
  flags_.same_net_                    = r.flags_.same_net_;
  flags_.same_metal_                  = r.flags_.same_metal_;
  flags_.same_via_                    = r.flags_.same_via_;
  flags_.layer_valid_                 = r.flags_.layer_valid_;
  flags_.no_stack_                    = r.flags_.no_stack_;
  flags_.non_zero_enclosure_          = r.flags_.non_zero_enclosure_;
  flags_.prl_for_aligned_cut_         = r.flags_.prl_for_aligned_cut_;
  flags_.center_to_center_valid_      = r.flags_.center_to_center_valid_;
  flags_.center_and_edge_valid_       = r.flags_.center_and_edge_valid_;
  flags_.no_prl_                      = r.flags_.no_prl_;
  flags_.prl_valid_                   = r.flags_.prl_valid_;
  flags_.max_x_y_                     = r.flags_.max_x_y_;
  flags_.end_extension_valid_         = r.flags_.end_extension_valid_;
  flags_.side_extension_valid_        = r.flags_.side_extension_valid_;
  flags_.exact_aligned_spacing_valid_ = r.flags_.exact_aligned_spacing_valid_;
  flags_.horizontal_                  = r.flags_.horizontal_;
  flags_.prl_horizontal_              = r.flags_.prl_horizontal_;
  flags_.vertical_                    = r.flags_.vertical_;
  flags_.prl_vertical_                = r.flags_.prl_vertical_;
  flags_.non_opposite_enclosure_spacing_valid_
      = r.flags_.non_opposite_enclosure_spacing_valid_;
  flags_.opposite_enclosure_resize_spacing_valid_
      = r.flags_.opposite_enclosure_resize_spacing_valid_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  default_           = r.default_;
  second_layer_      = r.second_layer_;
  prl_               = r.prl_;
  extension_         = r.extension_;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream&                          stream,
                      _dbTechLayerCutSpacingTableDefRule& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream >> *flags__bit_field;
  stream >> obj.default_;
  stream >> obj.second_layer_;
  stream >> obj.prl_for_aligned_cut_tbl_;
  stream >> obj.center_to_center_tbl_;
  stream >> obj.center_and_edge_tbl_;
  stream >> obj.prl_;
  stream >> obj.prl_tbl_;
  stream >> obj.extension_;
  stream >> obj.end_extension_tbl_;
  stream >> obj.side_extension_tbl_;
  stream >> obj.exact_aligned_spacing_tbl_;
  stream >> obj.non_opp_enc_spacing_tbl_;
  stream >> obj.opp_enc_spacing_tbl_;
  stream >> obj.spacing_tbl_;
  stream >> obj.row_map_;
  stream >> obj.col_map_;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream&                                stream,
                      const _dbTechLayerCutSpacingTableDefRule& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream << *flags__bit_field;
  stream << obj.default_;
  stream << obj.second_layer_;
  stream << obj.prl_for_aligned_cut_tbl_;
  stream << obj.center_to_center_tbl_;
  stream << obj.center_and_edge_tbl_;
  stream << obj.prl_;
  stream << obj.prl_tbl_;
  stream << obj.extension_;
  stream << obj.end_extension_tbl_;
  stream << obj.side_extension_tbl_;
  stream << obj.exact_aligned_spacing_tbl_;
  stream << obj.non_opp_enc_spacing_tbl_;
  stream << obj.opp_enc_spacing_tbl_;
  stream << obj.spacing_tbl_;
  stream << obj.row_map_;
  stream << obj.col_map_;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerCutSpacingTableDefRule::~_dbTechLayerCutSpacingTableDefRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutSpacingTableDefRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCutSpacingTableDefRule::setDefault(int default_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->default_ = default_;
}

int dbTechLayerCutSpacingTableDefRule::getDefault() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  return obj->default_;
}

void dbTechLayerCutSpacingTableDefRule::setSecondLayer(
    dbTechLayer* second_layer_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->second_layer_ = second_layer_->getImpl()->getOID();
}

void dbTechLayerCutSpacingTableDefRule::setPrl(int prl_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->prl_ = prl_;
}

int dbTechLayerCutSpacingTableDefRule::getPrl() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  return obj->prl_;
}

void dbTechLayerCutSpacingTableDefRule::setExtension(int extension_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->extension_ = extension_;
}

int dbTechLayerCutSpacingTableDefRule::getExtension() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  return obj->extension_;
}

void dbTechLayerCutSpacingTableDefRule::setDefaultValid(bool default_valid_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.default_valid_ = default_valid_;
}

bool dbTechLayerCutSpacingTableDefRule::isDefaultValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.default_valid_;
}

void dbTechLayerCutSpacingTableDefRule::setSameMask(bool same_mask_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.same_mask_ = same_mask_;
}

bool dbTechLayerCutSpacingTableDefRule::isSameMask() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.same_mask_;
}

void dbTechLayerCutSpacingTableDefRule::setSameNet(bool same_net_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.same_net_ = same_net_;
}

bool dbTechLayerCutSpacingTableDefRule::isSameNet() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.same_net_;
}

void dbTechLayerCutSpacingTableDefRule::setSameMetal(bool same_metal_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.same_metal_ = same_metal_;
}

bool dbTechLayerCutSpacingTableDefRule::isSameMetal() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.same_metal_;
}

void dbTechLayerCutSpacingTableDefRule::setSameVia(bool same_via_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.same_via_ = same_via_;
}

bool dbTechLayerCutSpacingTableDefRule::isSameVia() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.same_via_;
}

void dbTechLayerCutSpacingTableDefRule::setLayerValid(bool layer_valid_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.layer_valid_ = layer_valid_;
}

bool dbTechLayerCutSpacingTableDefRule::isLayerValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.layer_valid_;
}

void dbTechLayerCutSpacingTableDefRule::setNoStack(bool no_stack_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.no_stack_ = no_stack_;
}

bool dbTechLayerCutSpacingTableDefRule::isNoStack() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.no_stack_;
}

void dbTechLayerCutSpacingTableDefRule::setNonZeroEnclosure(
    bool non_zero_enclosure_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.non_zero_enclosure_ = non_zero_enclosure_;
}

bool dbTechLayerCutSpacingTableDefRule::isNonZeroEnclosure() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.non_zero_enclosure_;
}

void dbTechLayerCutSpacingTableDefRule::setPrlForAlignedCut(
    bool prl_for_aligned_cut_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.prl_for_aligned_cut_ = prl_for_aligned_cut_;
}

bool dbTechLayerCutSpacingTableDefRule::isPrlForAlignedCut() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.prl_for_aligned_cut_;
}

void dbTechLayerCutSpacingTableDefRule::setCenterToCenterValid(
    bool center_to_center_valid_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.center_to_center_valid_ = center_to_center_valid_;
}

bool dbTechLayerCutSpacingTableDefRule::isCenterToCenterValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.center_to_center_valid_;
}

void dbTechLayerCutSpacingTableDefRule::setCenterAndEdgeValid(
    bool center_and_edge_valid_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.center_and_edge_valid_ = center_and_edge_valid_;
}

bool dbTechLayerCutSpacingTableDefRule::isCenterAndEdgeValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.center_and_edge_valid_;
}

void dbTechLayerCutSpacingTableDefRule::setNoPrl(bool no_prl_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.no_prl_ = no_prl_;
}

bool dbTechLayerCutSpacingTableDefRule::isNoPrl() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.no_prl_;
}

void dbTechLayerCutSpacingTableDefRule::setPrlValid(bool prl_valid_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.prl_valid_ = prl_valid_;
}

bool dbTechLayerCutSpacingTableDefRule::isPrlValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.prl_valid_;
}

void dbTechLayerCutSpacingTableDefRule::setMaxXY(bool max_x_y_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.max_x_y_ = max_x_y_;
}

bool dbTechLayerCutSpacingTableDefRule::isMaxXY() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.max_x_y_;
}

void dbTechLayerCutSpacingTableDefRule::setEndExtensionValid(
    bool end_extension_valid_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.end_extension_valid_ = end_extension_valid_;
}

bool dbTechLayerCutSpacingTableDefRule::isEndExtensionValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.end_extension_valid_;
}

void dbTechLayerCutSpacingTableDefRule::setSideExtensionValid(
    bool side_extension_valid_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.side_extension_valid_ = side_extension_valid_;
}

bool dbTechLayerCutSpacingTableDefRule::isSideExtensionValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.side_extension_valid_;
}

void dbTechLayerCutSpacingTableDefRule::setExactAlignedSpacingValid(
    bool exact_aligned_spacing_valid_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.exact_aligned_spacing_valid_ = exact_aligned_spacing_valid_;
}

bool dbTechLayerCutSpacingTableDefRule::isExactAlignedSpacingValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.exact_aligned_spacing_valid_;
}

void dbTechLayerCutSpacingTableDefRule::setHorizontal(bool horizontal_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.horizontal_ = horizontal_;
}

bool dbTechLayerCutSpacingTableDefRule::isHorizontal() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.horizontal_;
}

void dbTechLayerCutSpacingTableDefRule::setPrlHorizontal(bool prl_horizontal_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.prl_horizontal_ = prl_horizontal_;
}

bool dbTechLayerCutSpacingTableDefRule::isPrlHorizontal() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.prl_horizontal_;
}

void dbTechLayerCutSpacingTableDefRule::setVertical(bool vertical_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.vertical_ = vertical_;
}

bool dbTechLayerCutSpacingTableDefRule::isVertical() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.vertical_;
}

void dbTechLayerCutSpacingTableDefRule::setPrlVertical(bool prl_vertical_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.prl_vertical_ = prl_vertical_;
}

bool dbTechLayerCutSpacingTableDefRule::isPrlVertical() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.prl_vertical_;
}

void dbTechLayerCutSpacingTableDefRule::setNonOppositeEnclosureSpacingValid(
    bool non_opposite_enclosure_spacing_valid_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.non_opposite_enclosure_spacing_valid_
      = non_opposite_enclosure_spacing_valid_;
}

bool dbTechLayerCutSpacingTableDefRule::isNonOppositeEnclosureSpacingValid()
    const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.non_opposite_enclosure_spacing_valid_;
}

void dbTechLayerCutSpacingTableDefRule::setOppositeEnclosureResizeSpacingValid(
    bool opposite_enclosure_resize_spacing_valid_)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.opposite_enclosure_resize_spacing_valid_
      = opposite_enclosure_resize_spacing_valid_;
}

bool dbTechLayerCutSpacingTableDefRule::isOppositeEnclosureResizeSpacingValid()
    const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.opposite_enclosure_resize_spacing_valid_;
}

// User Code Begin dbTechLayerCutSpacingTableDefRulePublicMethods
void dbTechLayerCutSpacingTableDefRule::addPrlForAlignedCutEntry(
    dbTechLayerCutClassRule* from,
    dbTechLayerCutClassRule* to)
{
  if (from == nullptr || to == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->prl_for_aligned_cut_tbl_.push_back(
      {from->getImpl()->getOID(), to->getImpl()->getOID()});
}

std::vector<std::pair<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*>>
dbTechLayerCutSpacingTableDefRule::getPrlForAlignedCutTable() const
{
  std::vector<std::pair<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*>>
                                      res;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [from, to] : obj->prl_for_aligned_cut_tbl_) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(from),
         (dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(to)});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addCenterToCenterEntry(
    dbTechLayerCutClassRule* from,
    dbTechLayerCutClassRule* to)
{
  if (from == nullptr || to == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->center_to_center_tbl_.push_back(
      {from->getImpl()->getOID(), to->getImpl()->getOID()});
}

std::vector<std::pair<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*>>
dbTechLayerCutSpacingTableDefRule::getCenterToCenterTable() const
{
  std::vector<std::pair<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*>>
                                      res;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [from, to] : obj->center_to_center_tbl_) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(from),
         (dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(to)});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addCenterAndEdgeEntry(
    dbTechLayerCutClassRule* from,
    dbTechLayerCutClassRule* to)
{
  if (from == nullptr || to == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->center_and_edge_tbl_.push_back(
      {from->getImpl()->getOID(), to->getImpl()->getOID()});
}

std::vector<std::pair<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*>>
dbTechLayerCutSpacingTableDefRule::getCenterAndEdgeTable() const
{
  std::vector<std::pair<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*>>
                                      res;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [from, to] : obj->center_and_edge_tbl_) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(from),
         (dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(to)});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addPrlEntry(
    dbTechLayerCutClassRule* from,
    dbTechLayerCutClassRule* to,
    int                      ccPrl)
{
  if (from == nullptr || to == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->prl_tbl_.push_back(
      {from->getImpl()->getOID(), to->getImpl()->getOID(), ccPrl});
}

std::vector<std::tuple<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*, int>>
dbTechLayerCutSpacingTableDefRule::getPrlTable() const
{
  std::vector<
      std::tuple<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*, int>>
                                      res;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto&& [from, to, ccPrl] : obj->prl_tbl_) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(from),
         (dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(to),
         ccPrl});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addEndExtensionEntry(
    dbTechLayerCutClassRule* cls,
    int                      ext)
{
  if (cls == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->end_extension_tbl_.push_back({cls->getImpl()->getOID(), ext});
}

std::vector<std::pair<dbTechLayerCutClassRule*, int>>
dbTechLayerCutSpacingTableDefRule::getEndExtensionTable() const
{
  std::vector<std::pair<dbTechLayerCutClassRule*, int>> res;
  _dbTechLayerCutSpacingTableDefRule*                   obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [cls, ext] : obj->end_extension_tbl_) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(cls),
         ext});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addSideExtensionEntry(
    dbTechLayerCutClassRule* cls,
    int                      ext)
{
  if (cls == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->side_extension_tbl_.push_back({cls->getImpl()->getOID(), ext});
}

std::vector<std::pair<dbTechLayerCutClassRule*, int>>
dbTechLayerCutSpacingTableDefRule::getSideExtensionTable() const
{
  std::vector<std::pair<dbTechLayerCutClassRule*, int>> res;
  _dbTechLayerCutSpacingTableDefRule*                   obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [cls, ext] : obj->side_extension_tbl_) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(cls),
         ext});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addExactElignedEntry(
    dbTechLayerCutClassRule* cls,
    int                      spacing)
{
  if (cls == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->exact_aligned_spacing_tbl_.push_back(
      {cls->getImpl()->getOID(), spacing});
}

std::vector<std::pair<dbTechLayerCutClassRule*, int>>
dbTechLayerCutSpacingTableDefRule::getExactAlignedTable() const
{
  std::vector<std::pair<dbTechLayerCutClassRule*, int>> res;
  _dbTechLayerCutSpacingTableDefRule*                   obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [cls, spacing] : obj->exact_aligned_spacing_tbl_) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(cls),
         spacing});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addNonOppEncSpacingEntry(
    dbTechLayerCutClassRule* cls,
    int                      spacing)
{
  if (cls == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->non_opp_enc_spacing_tbl_.push_back({cls->getImpl()->getOID(), spacing});
}

std::vector<std::pair<dbTechLayerCutClassRule*, int>>
dbTechLayerCutSpacingTableDefRule::getNonOppEncSpacingTable() const
{
  std::vector<std::pair<dbTechLayerCutClassRule*, int>> res;
  _dbTechLayerCutSpacingTableDefRule*                   obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [cls, spacing] : obj->non_opp_enc_spacing_tbl_) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(cls),
         spacing});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addOppEncSpacingEntry(
    dbTechLayerCutClassRule* cls,
    int                      rsz1,
    int                      rsz2,
    int                      spacing)
{
  if (cls == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->opp_enc_spacing_tbl_.push_back(
      {cls->getImpl()->getOID(), rsz1, rsz2, spacing});
}

std::vector<std::tuple<dbTechLayerCutClassRule*, int, int, int>>
dbTechLayerCutSpacingTableDefRule::getOppEncSpacingTable() const
{
  std::vector<std::tuple<dbTechLayerCutClassRule*, int, int, int>> res;
  _dbTechLayerCutSpacingTableDefRule*                              obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [cls, rsz1, rsz2, spacing] : obj->opp_enc_spacing_tbl_) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(cls),
         rsz1,
         rsz2,
         spacing});
  }
  return res;
}

dbTechLayer* dbTechLayerCutSpacingTableDefRule::getSecondLayer() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  if (obj->second_layer_ == 0)
    return nullptr;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  _dbTech*      _tech = (_dbTech*) layer->getOwner();
  return (dbTechLayer*) _tech->_layer_tbl->getPtr(obj->second_layer_);
}

void dbTechLayerCutSpacingTableDefRule::setSpacingTable(
    std::vector<std::vector<std::pair<int, int>>> table,
    std::map<std::string, uint>                   row_map,
    std::map<std::string, uint>                   col_map)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->row_map_ = row_map;
  obj->col_map_ = col_map;
  for (auto spacing : table) {
    dbVector<std::pair<int, int>> tmp;
    tmp = spacing;
    obj->spacing_tbl_.push_back(tmp);
  }
}

void dbTechLayerCutSpacingTableDefRule::getSpacingTable(
    std::vector<std::vector<std::pair<int, int>>>& table,
    std::map<std::string, uint>&                   row_map,
    std::map<std::string, uint>&                   col_map)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  row_map = obj->row_map_;
  col_map = obj->col_map_;
  table.clear();
  for (auto spacing : obj->spacing_tbl_) {
    table.push_back(spacing);
  }
}

std::pair<int, int> dbTechLayerCutSpacingTableDefRule::getSpacing(
    const char* class1,
    bool        SIDE1,
    const char* class2,
    bool        SIDE2)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  std::string c1 = class1;
  std::string c2 = class2;
  if (SIDE1)
    c1 += "/SIDE";
  else
    c1 += "/END";

  if (SIDE2)
    c2 += "/SIDE";
  else
    c2 += "/END";

  if (obj->row_map_.find(c1) != obj->row_map_.end()
      && obj->col_map_.find(c2) != obj->col_map_.end())
    return obj->spacing_tbl_[obj->row_map_[c1]][obj->col_map_[c2]];
  else if (obj->row_map_.find(c2) != obj->row_map_.end()
           && obj->col_map_.find(c1) != obj->col_map_.end())
    return obj->spacing_tbl_[obj->row_map_[c2]][obj->col_map_[c1]];
  else
    return {obj->default_, obj->default_};
}

dbTechLayer* dbTechLayerCutSpacingTableDefRule::getTechLayer() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  return (odb::dbTechLayer*) obj->getOwner();
}

dbTechLayerCutSpacingTableDefRule* dbTechLayerCutSpacingTableDefRule::create(
    dbTechLayer* parent)
{
  _dbTechLayer*                       _parent = (_dbTechLayer*) parent;
  _dbTechLayerCutSpacingTableDefRule* newrule
      = _parent->cut_spacing_table_def_tbl_->create();
  return ((dbTechLayerCutSpacingTableDefRule*) newrule);
}

dbTechLayerCutSpacingTableDefRule*
dbTechLayerCutSpacingTableDefRule::getTechLayerCutSpacingTableDefSubRule(
    dbTechLayer* parent,
    uint         dbid)
{
  _dbTechLayer* _parent = (_dbTechLayer*) parent;
  return (dbTechLayerCutSpacingTableDefRule*)
      _parent->cut_spacing_table_def_tbl_->getPtr(dbid);
}
void dbTechLayerCutSpacingTableDefRule::destroy(
    dbTechLayerCutSpacingTableDefRule* rule)
{
  _dbTechLayer* _parent = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  _parent->cut_spacing_table_def_tbl_->destroy(
      (_dbTechLayerCutSpacingTableDefRule*) rule);
}

// User Code End dbTechLayerCutSpacingTableDefRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp