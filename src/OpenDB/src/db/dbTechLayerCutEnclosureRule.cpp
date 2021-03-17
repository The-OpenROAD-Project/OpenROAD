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
#include "dbTechLayerCutEnclosureRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "dbTechLayerCutClassRule.h"
// User Code Begin Includes
// User Code End Includes
namespace odb {

template class dbTable<_dbTechLayerCutEnclosureRule>;

bool _dbTechLayerCutEnclosureRule::operator==(
    const _dbTechLayerCutEnclosureRule& rhs) const
{
  if (flags_.type_ != rhs.flags_.type_)
    return false;

  if (flags_.cut_class_valid_ != rhs.flags_.cut_class_valid_)
    return false;

  if (flags_.above_ != rhs.flags_.above_)
    return false;

  if (flags_.below_ != rhs.flags_.below_)
    return false;

  if (flags_.eol_min_length_valid_ != rhs.flags_.eol_min_length_valid_)
    return false;

  if (flags_.eol_only_ != rhs.flags_.eol_only_)
    return false;

  if (flags_.short_edge_only_ != rhs.flags_.short_edge_only_)
    return false;

  if (flags_.side_spacing_valid_ != rhs.flags_.side_spacing_valid_)
    return false;

  if (flags_.end_spacing_valid_ != rhs.flags_.end_spacing_valid_)
    return false;

  if (flags_.off_center_line_ != rhs.flags_.off_center_line_)
    return false;

  if (flags_.width_valid_ != rhs.flags_.width_valid_)
    return false;

  if (flags_.include_abutted_ != rhs.flags_.include_abutted_)
    return false;

  if (flags_.except_extra_cut_ != rhs.flags_.except_extra_cut_)
    return false;

  if (flags_.prl_ != rhs.flags_.prl_)
    return false;

  if (flags_.no_shared_edge_ != rhs.flags_.no_shared_edge_)
    return false;

  if (flags_.length_valid_ != rhs.flags_.length_valid_)
    return false;

  if (flags_.extra_cut_valid_ != rhs.flags_.extra_cut_valid_)
    return false;

  if (flags_.extra_only != rhs.flags_.extra_only)
    return false;

  if (flags_.redundant_cut_valid_ != rhs.flags_.redundant_cut_valid_)
    return false;

  if (flags_.parallel_valid_ != rhs.flags_.parallel_valid_)
    return false;

  if (flags_.second_parallel_valid != rhs.flags_.second_parallel_valid)
    return false;

  if (flags_.second_par_within_valid_ != rhs.flags_.second_par_within_valid_)
    return false;

  if (flags_.below_enclosure_valid_ != rhs.flags_.below_enclosure_valid_)
    return false;

  if (flags_.concave_corners_valid_ != rhs.flags_.concave_corners_valid_)
    return false;

  if (cut_class_ != rhs.cut_class_)
    return false;

  if (eol_width_ != rhs.eol_width_)
    return false;

  if (eol_min_length_ != rhs.eol_min_length_)
    return false;

  if (first_overhang_ != rhs.first_overhang_)
    return false;

  if (second_overhang_ != rhs.second_overhang_)
    return false;

  if (spacing_ != rhs.spacing_)
    return false;

  if (extension_ != rhs.extension_)
    return false;

  if (forward_extension_ != rhs.forward_extension_)
    return false;

  if (backward_extension_ != rhs.backward_extension_)
    return false;

  if (min_width_ != rhs.min_width_)
    return false;

  if (cut_within_ != rhs.cut_within_)
    return false;

  if (min_length_ != rhs.min_length_)
    return false;

  if (par_length_ != rhs.par_length_)
    return false;

  if (second_par_length_ != rhs.second_par_length_)
    return false;

  if (par_within_ != rhs.par_within_)
    return false;

  if (second_par_within_ != rhs.second_par_within_)
    return false;

  if (below_enclosure_ != rhs.below_enclosure_)
    return false;

  if (num_corners_ != rhs.num_corners_)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerCutEnclosureRule::operator<(
    const _dbTechLayerCutEnclosureRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerCutEnclosureRule::differences(
    dbDiff& diff,
    const char* field,
    const _dbTechLayerCutEnclosureRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(flags_.type_);
  DIFF_FIELD(flags_.cut_class_valid_);
  DIFF_FIELD(flags_.above_);
  DIFF_FIELD(flags_.below_);
  DIFF_FIELD(flags_.eol_min_length_valid_);
  DIFF_FIELD(flags_.eol_only_);
  DIFF_FIELD(flags_.short_edge_only_);
  DIFF_FIELD(flags_.side_spacing_valid_);
  DIFF_FIELD(flags_.end_spacing_valid_);
  DIFF_FIELD(flags_.off_center_line_);
  DIFF_FIELD(flags_.width_valid_);
  DIFF_FIELD(flags_.include_abutted_);
  DIFF_FIELD(flags_.except_extra_cut_);
  DIFF_FIELD(flags_.prl_);
  DIFF_FIELD(flags_.no_shared_edge_);
  DIFF_FIELD(flags_.length_valid_);
  DIFF_FIELD(flags_.extra_cut_valid_);
  DIFF_FIELD(flags_.extra_only);
  DIFF_FIELD(flags_.redundant_cut_valid_);
  DIFF_FIELD(flags_.parallel_valid_);
  DIFF_FIELD(flags_.second_parallel_valid);
  DIFF_FIELD(flags_.second_par_within_valid_);
  DIFF_FIELD(flags_.below_enclosure_valid_);
  DIFF_FIELD(flags_.concave_corners_valid_);
  DIFF_FIELD(cut_class_);
  DIFF_FIELD(eol_width_);
  DIFF_FIELD(eol_min_length_);
  DIFF_FIELD(first_overhang_);
  DIFF_FIELD(second_overhang_);
  DIFF_FIELD(spacing_);
  DIFF_FIELD(extension_);
  DIFF_FIELD(forward_extension_);
  DIFF_FIELD(backward_extension_);
  DIFF_FIELD(min_width_);
  DIFF_FIELD(cut_within_);
  DIFF_FIELD(min_length_);
  DIFF_FIELD(par_length_);
  DIFF_FIELD(second_par_length_);
  DIFF_FIELD(par_within_);
  DIFF_FIELD(second_par_within_);
  DIFF_FIELD(below_enclosure_);
  DIFF_FIELD(num_corners_);
  // User Code Begin Differences
  // User Code End Differences
  DIFF_END
}
void _dbTechLayerCutEnclosureRule::out(dbDiff& diff,
                                       char side,
                                       const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.type_);
  DIFF_OUT_FIELD(flags_.cut_class_valid_);
  DIFF_OUT_FIELD(flags_.above_);
  DIFF_OUT_FIELD(flags_.below_);
  DIFF_OUT_FIELD(flags_.eol_min_length_valid_);
  DIFF_OUT_FIELD(flags_.eol_only_);
  DIFF_OUT_FIELD(flags_.short_edge_only_);
  DIFF_OUT_FIELD(flags_.side_spacing_valid_);
  DIFF_OUT_FIELD(flags_.end_spacing_valid_);
  DIFF_OUT_FIELD(flags_.off_center_line_);
  DIFF_OUT_FIELD(flags_.width_valid_);
  DIFF_OUT_FIELD(flags_.include_abutted_);
  DIFF_OUT_FIELD(flags_.except_extra_cut_);
  DIFF_OUT_FIELD(flags_.prl_);
  DIFF_OUT_FIELD(flags_.no_shared_edge_);
  DIFF_OUT_FIELD(flags_.length_valid_);
  DIFF_OUT_FIELD(flags_.extra_cut_valid_);
  DIFF_OUT_FIELD(flags_.extra_only);
  DIFF_OUT_FIELD(flags_.redundant_cut_valid_);
  DIFF_OUT_FIELD(flags_.parallel_valid_);
  DIFF_OUT_FIELD(flags_.second_parallel_valid);
  DIFF_OUT_FIELD(flags_.second_par_within_valid_);
  DIFF_OUT_FIELD(flags_.below_enclosure_valid_);
  DIFF_OUT_FIELD(flags_.concave_corners_valid_);
  DIFF_OUT_FIELD(cut_class_);
  DIFF_OUT_FIELD(eol_width_);
  DIFF_OUT_FIELD(eol_min_length_);
  DIFF_OUT_FIELD(first_overhang_);
  DIFF_OUT_FIELD(second_overhang_);
  DIFF_OUT_FIELD(spacing_);
  DIFF_OUT_FIELD(extension_);
  DIFF_OUT_FIELD(forward_extension_);
  DIFF_OUT_FIELD(backward_extension_);
  DIFF_OUT_FIELD(min_width_);
  DIFF_OUT_FIELD(cut_within_);
  DIFF_OUT_FIELD(min_length_);
  DIFF_OUT_FIELD(par_length_);
  DIFF_OUT_FIELD(second_par_length_);
  DIFF_OUT_FIELD(par_within_);
  DIFF_OUT_FIELD(second_par_within_);
  DIFF_OUT_FIELD(below_enclosure_);
  DIFF_OUT_FIELD(num_corners_);

  // User Code Begin Out
  // User Code End Out
  DIFF_END
}
_dbTechLayerCutEnclosureRule::_dbTechLayerCutEnclosureRule(_dbDatabase* db)
{
  uint32_t* flags__bit_field = (uint32_t*) &flags_;
  *flags__bit_field = 0;
  // User Code Begin Constructor
  // User Code End Constructor
}
_dbTechLayerCutEnclosureRule::_dbTechLayerCutEnclosureRule(
    _dbDatabase* db,
    const _dbTechLayerCutEnclosureRule& r)
{
  flags_.type_ = r.flags_.type_;
  flags_.cut_class_valid_ = r.flags_.cut_class_valid_;
  flags_.above_ = r.flags_.above_;
  flags_.below_ = r.flags_.below_;
  flags_.eol_min_length_valid_ = r.flags_.eol_min_length_valid_;
  flags_.eol_only_ = r.flags_.eol_only_;
  flags_.short_edge_only_ = r.flags_.short_edge_only_;
  flags_.side_spacing_valid_ = r.flags_.side_spacing_valid_;
  flags_.end_spacing_valid_ = r.flags_.end_spacing_valid_;
  flags_.off_center_line_ = r.flags_.off_center_line_;
  flags_.width_valid_ = r.flags_.width_valid_;
  flags_.include_abutted_ = r.flags_.include_abutted_;
  flags_.except_extra_cut_ = r.flags_.except_extra_cut_;
  flags_.prl_ = r.flags_.prl_;
  flags_.no_shared_edge_ = r.flags_.no_shared_edge_;
  flags_.length_valid_ = r.flags_.length_valid_;
  flags_.extra_cut_valid_ = r.flags_.extra_cut_valid_;
  flags_.extra_only = r.flags_.extra_only;
  flags_.redundant_cut_valid_ = r.flags_.redundant_cut_valid_;
  flags_.parallel_valid_ = r.flags_.parallel_valid_;
  flags_.second_parallel_valid = r.flags_.second_parallel_valid;
  flags_.second_par_within_valid_ = r.flags_.second_par_within_valid_;
  flags_.below_enclosure_valid_ = r.flags_.below_enclosure_valid_;
  flags_.concave_corners_valid_ = r.flags_.concave_corners_valid_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  cut_class_ = r.cut_class_;
  eol_width_ = r.eol_width_;
  eol_min_length_ = r.eol_min_length_;
  first_overhang_ = r.first_overhang_;
  second_overhang_ = r.second_overhang_;
  spacing_ = r.spacing_;
  extension_ = r.extension_;
  forward_extension_ = r.forward_extension_;
  backward_extension_ = r.backward_extension_;
  min_width_ = r.min_width_;
  cut_within_ = r.cut_within_;
  min_length_ = r.min_length_;
  par_length_ = r.par_length_;
  second_par_length_ = r.second_par_length_;
  par_within_ = r.par_within_;
  second_par_within_ = r.second_par_within_;
  below_enclosure_ = r.below_enclosure_;
  num_corners_ = r.num_corners_;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutEnclosureRule& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream >> *flags__bit_field;
  stream >> obj.cut_class_;
  stream >> obj.eol_width_;
  stream >> obj.eol_min_length_;
  stream >> obj.first_overhang_;
  stream >> obj.second_overhang_;
  stream >> obj.spacing_;
  stream >> obj.extension_;
  stream >> obj.forward_extension_;
  stream >> obj.backward_extension_;
  stream >> obj.min_width_;
  stream >> obj.cut_within_;
  stream >> obj.min_length_;
  stream >> obj.par_length_;
  stream >> obj.second_par_length_;
  stream >> obj.par_within_;
  stream >> obj.second_par_within_;
  stream >> obj.below_enclosure_;
  stream >> obj.num_corners_;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCutEnclosureRule& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream << *flags__bit_field;
  stream << obj.cut_class_;
  stream << obj.eol_width_;
  stream << obj.eol_min_length_;
  stream << obj.first_overhang_;
  stream << obj.second_overhang_;
  stream << obj.spacing_;
  stream << obj.extension_;
  stream << obj.forward_extension_;
  stream << obj.backward_extension_;
  stream << obj.min_width_;
  stream << obj.cut_within_;
  stream << obj.min_length_;
  stream << obj.par_length_;
  stream << obj.second_par_length_;
  stream << obj.par_within_;
  stream << obj.second_par_within_;
  stream << obj.below_enclosure_;
  stream << obj.num_corners_;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerCutEnclosureRule::~_dbTechLayerCutEnclosureRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutEnclosureRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCutEnclosureRule::setCutClass(
    dbTechLayerCutClassRule* cut_class)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->cut_class_ = cut_class->getImpl()->getOID();
}

dbTechLayerCutClassRule* dbTechLayerCutEnclosureRule::getCutClass() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  if (obj->cut_class_ == 0)
    return NULL;
  _dbTechLayer* par = (_dbTechLayer*) obj->getOwner();
  return (dbTechLayerCutClassRule*) par->cut_class_rules_tbl_->getPtr(
      obj->cut_class_);
}

void dbTechLayerCutEnclosureRule::setEolWidth(int eol_width)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->eol_width_ = eol_width;
}

int dbTechLayerCutEnclosureRule::getEolWidth() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->eol_width_;
}

void dbTechLayerCutEnclosureRule::setEolMinLength(int eol_min_length)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->eol_min_length_ = eol_min_length;
}

int dbTechLayerCutEnclosureRule::getEolMinLength() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->eol_min_length_;
}

void dbTechLayerCutEnclosureRule::setFirstOverhang(int first_overhang)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->first_overhang_ = first_overhang;
}

int dbTechLayerCutEnclosureRule::getFirstOverhang() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->first_overhang_;
}

void dbTechLayerCutEnclosureRule::setSecondOverhang(int second_overhang)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->second_overhang_ = second_overhang;
}

int dbTechLayerCutEnclosureRule::getSecondOverhang() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->second_overhang_;
}

void dbTechLayerCutEnclosureRule::setSpacing(int spacing)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->spacing_ = spacing;
}

int dbTechLayerCutEnclosureRule::getSpacing() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->spacing_;
}

void dbTechLayerCutEnclosureRule::setExtension(int extension)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->extension_ = extension;
}

int dbTechLayerCutEnclosureRule::getExtension() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->extension_;
}

void dbTechLayerCutEnclosureRule::setForwardExtension(int forward_extension)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->forward_extension_ = forward_extension;
}

int dbTechLayerCutEnclosureRule::getForwardExtension() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->forward_extension_;
}

void dbTechLayerCutEnclosureRule::setBackwardExtension(int backward_extension)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->backward_extension_ = backward_extension;
}

int dbTechLayerCutEnclosureRule::getBackwardExtension() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->backward_extension_;
}

void dbTechLayerCutEnclosureRule::setMinWidth(int min_width)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->min_width_ = min_width;
}

int dbTechLayerCutEnclosureRule::getMinWidth() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->min_width_;
}

void dbTechLayerCutEnclosureRule::setCutWithin(int cut_within)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->cut_within_ = cut_within;
}

int dbTechLayerCutEnclosureRule::getCutWithin() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->cut_within_;
}

void dbTechLayerCutEnclosureRule::setMinLength(int min_length)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->min_length_ = min_length;
}

int dbTechLayerCutEnclosureRule::getMinLength() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->min_length_;
}

void dbTechLayerCutEnclosureRule::setParLength(int par_length)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->par_length_ = par_length;
}

int dbTechLayerCutEnclosureRule::getParLength() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->par_length_;
}

void dbTechLayerCutEnclosureRule::setSecondParLength(int second_par_length)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->second_par_length_ = second_par_length;
}

int dbTechLayerCutEnclosureRule::getSecondParLength() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->second_par_length_;
}

void dbTechLayerCutEnclosureRule::setParWithin(int par_within)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->par_within_ = par_within;
}

int dbTechLayerCutEnclosureRule::getParWithin() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->par_within_;
}

void dbTechLayerCutEnclosureRule::setSecondParWithin(int second_par_within)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->second_par_within_ = second_par_within;
}

int dbTechLayerCutEnclosureRule::getSecondParWithin() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->second_par_within_;
}

void dbTechLayerCutEnclosureRule::setBelowEnclosure(int below_enclosure)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->below_enclosure_ = below_enclosure;
}

int dbTechLayerCutEnclosureRule::getBelowEnclosure() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->below_enclosure_;
}

void dbTechLayerCutEnclosureRule::setNumCorners(uint num_corners)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->num_corners_ = num_corners;
}

uint dbTechLayerCutEnclosureRule::getNumCorners() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->num_corners_;
}

void dbTechLayerCutEnclosureRule::setCutClassValid(bool cut_class_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.cut_class_valid_ = cut_class_valid;
}

bool dbTechLayerCutEnclosureRule::isCutClassValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.cut_class_valid_;
}

void dbTechLayerCutEnclosureRule::setAbove(bool above)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.above_ = above;
}

bool dbTechLayerCutEnclosureRule::isAbove() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.above_;
}

void dbTechLayerCutEnclosureRule::setBelow(bool below)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.below_ = below;
}

bool dbTechLayerCutEnclosureRule::isBelow() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.below_;
}

void dbTechLayerCutEnclosureRule::setEolMinLengthValid(
    bool eol_min_length_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.eol_min_length_valid_ = eol_min_length_valid;
}

bool dbTechLayerCutEnclosureRule::isEolMinLengthValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.eol_min_length_valid_;
}

void dbTechLayerCutEnclosureRule::setEolOnly(bool eol_only)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.eol_only_ = eol_only;
}

bool dbTechLayerCutEnclosureRule::isEolOnly() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.eol_only_;
}

void dbTechLayerCutEnclosureRule::setShortEdgeOnly(bool short_edge_only)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.short_edge_only_ = short_edge_only;
}

bool dbTechLayerCutEnclosureRule::isShortEdgeOnly() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.short_edge_only_;
}

void dbTechLayerCutEnclosureRule::setSideSpacingValid(bool side_spacing_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.side_spacing_valid_ = side_spacing_valid;
}

bool dbTechLayerCutEnclosureRule::isSideSpacingValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.side_spacing_valid_;
}

void dbTechLayerCutEnclosureRule::setEndSpacingValid(bool end_spacing_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.end_spacing_valid_ = end_spacing_valid;
}

bool dbTechLayerCutEnclosureRule::isEndSpacingValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.end_spacing_valid_;
}

void dbTechLayerCutEnclosureRule::setOffCenterLine(bool off_center_line)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.off_center_line_ = off_center_line;
}

bool dbTechLayerCutEnclosureRule::isOffCenterLine() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.off_center_line_;
}

void dbTechLayerCutEnclosureRule::setWidthValid(bool width_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.width_valid_ = width_valid;
}

bool dbTechLayerCutEnclosureRule::isWidthValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.width_valid_;
}

void dbTechLayerCutEnclosureRule::setIncludeAbutted(bool include_abutted)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.include_abutted_ = include_abutted;
}

bool dbTechLayerCutEnclosureRule::isIncludeAbutted() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.include_abutted_;
}

void dbTechLayerCutEnclosureRule::setExceptExtraCut(bool except_extra_cut)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.except_extra_cut_ = except_extra_cut;
}

bool dbTechLayerCutEnclosureRule::isExceptExtraCut() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.except_extra_cut_;
}

void dbTechLayerCutEnclosureRule::setPrl(bool prl)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.prl_ = prl;
}

bool dbTechLayerCutEnclosureRule::isPrl() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.prl_;
}

void dbTechLayerCutEnclosureRule::setNoSharedEdge(bool no_shared_edge)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.no_shared_edge_ = no_shared_edge;
}

bool dbTechLayerCutEnclosureRule::isNoSharedEdge() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.no_shared_edge_;
}

void dbTechLayerCutEnclosureRule::setLengthValid(bool length_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.length_valid_ = length_valid;
}

bool dbTechLayerCutEnclosureRule::isLengthValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.length_valid_;
}

void dbTechLayerCutEnclosureRule::setExtraCutValid(bool extra_cut_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.extra_cut_valid_ = extra_cut_valid;
}

bool dbTechLayerCutEnclosureRule::isExtraCutValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.extra_cut_valid_;
}

void dbTechLayerCutEnclosureRule::setExtraOnly(bool extra_only)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.extra_only = extra_only;
}

bool dbTechLayerCutEnclosureRule::isExtraOnly() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.extra_only;
}

void dbTechLayerCutEnclosureRule::setRedundantCutValid(bool redundant_cut_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.redundant_cut_valid_ = redundant_cut_valid;
}

bool dbTechLayerCutEnclosureRule::isRedundantCutValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.redundant_cut_valid_;
}

void dbTechLayerCutEnclosureRule::setParallelValid(bool parallel_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.parallel_valid_ = parallel_valid;
}

bool dbTechLayerCutEnclosureRule::isParallelValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.parallel_valid_;
}

void dbTechLayerCutEnclosureRule::setSecondParallelValid(
    bool second_parallel_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.second_parallel_valid = second_parallel_valid;
}

bool dbTechLayerCutEnclosureRule::isSecondParallelValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.second_parallel_valid;
}

void dbTechLayerCutEnclosureRule::setSecondParWithinValid(
    bool second_par_within_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.second_par_within_valid_ = second_par_within_valid;
}

bool dbTechLayerCutEnclosureRule::isSecondParWithinValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.second_par_within_valid_;
}

void dbTechLayerCutEnclosureRule::setBelowEnclosureValid(
    bool below_enclosure_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.below_enclosure_valid_ = below_enclosure_valid;
}

bool dbTechLayerCutEnclosureRule::isBelowEnclosureValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.below_enclosure_valid_;
}

void dbTechLayerCutEnclosureRule::setConcaveCornersValid(
    bool concave_corners_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.concave_corners_valid_ = concave_corners_valid;
}

bool dbTechLayerCutEnclosureRule::isConcaveCornersValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.concave_corners_valid_;
}

// User Code Begin dbTechLayerCutEnclosureRulePublicMethods
void dbTechLayerCutEnclosureRule::setType(ENC_TYPE type)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.type_ = (uint) type;
}

dbTechLayerCutEnclosureRule::ENC_TYPE dbTechLayerCutEnclosureRule::getType()
    const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return (dbTechLayerCutEnclosureRule::ENC_TYPE) obj->flags_.type_;
}

dbTechLayerCutEnclosureRule* dbTechLayerCutEnclosureRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerCutEnclosureRule* newrule = layer->cut_enc_rules_tbl_->create();
  return ((dbTechLayerCutEnclosureRule*) newrule);
}

dbTechLayerCutEnclosureRule*
dbTechLayerCutEnclosureRule::getTechLayerCutEnclosureRule(dbTechLayer* inly,
                                                          uint dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerCutEnclosureRule*) layer->cut_enc_rules_tbl_->getPtr(dbid);
}
void dbTechLayerCutEnclosureRule::destroy(dbTechLayerCutEnclosureRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->cut_enc_rules_tbl_->destroy((_dbTechLayerCutEnclosureRule*) rule);
}
// User Code End dbTechLayerCutEnclosureRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp