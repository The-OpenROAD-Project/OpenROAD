// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerCutEnclosureRule.h"

#include <cstdint>
#include <cstring>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "dbTechLayerCutClassRule.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerCutEnclosureRule>;

bool _dbTechLayerCutEnclosureRule::operator==(
    const _dbTechLayerCutEnclosureRule& rhs) const
{
  if (flags_.type != rhs.flags_.type) {
    return false;
  }
  if (flags_.cut_class_valid != rhs.flags_.cut_class_valid) {
    return false;
  }
  if (flags_.above != rhs.flags_.above) {
    return false;
  }
  if (flags_.below != rhs.flags_.below) {
    return false;
  }
  if (flags_.eol_min_length_valid != rhs.flags_.eol_min_length_valid) {
    return false;
  }
  if (flags_.eol_only != rhs.flags_.eol_only) {
    return false;
  }
  if (flags_.short_edge_on_eol != rhs.flags_.short_edge_on_eol) {
    return false;
  }
  if (flags_.side_spacing_valid != rhs.flags_.side_spacing_valid) {
    return false;
  }
  if (flags_.end_spacing_valid != rhs.flags_.end_spacing_valid) {
    return false;
  }
  if (flags_.off_center_line != rhs.flags_.off_center_line) {
    return false;
  }
  if (flags_.width_valid != rhs.flags_.width_valid) {
    return false;
  }
  if (flags_.include_abutted != rhs.flags_.include_abutted) {
    return false;
  }
  if (flags_.except_extra_cut != rhs.flags_.except_extra_cut) {
    return false;
  }
  if (flags_.prl != rhs.flags_.prl) {
    return false;
  }
  if (flags_.no_shared_edge != rhs.flags_.no_shared_edge) {
    return false;
  }
  if (flags_.length_valid != rhs.flags_.length_valid) {
    return false;
  }
  if (flags_.extra_cut_valid != rhs.flags_.extra_cut_valid) {
    return false;
  }
  if (flags_.extra_only != rhs.flags_.extra_only) {
    return false;
  }
  if (flags_.redundant_cut_valid != rhs.flags_.redundant_cut_valid) {
    return false;
  }
  if (flags_.parallel_valid != rhs.flags_.parallel_valid) {
    return false;
  }
  if (flags_.second_parallel_valid != rhs.flags_.second_parallel_valid) {
    return false;
  }
  if (flags_.second_par_within_valid != rhs.flags_.second_par_within_valid) {
    return false;
  }
  if (flags_.below_enclosure_valid != rhs.flags_.below_enclosure_valid) {
    return false;
  }
  if (flags_.concave_corners_valid != rhs.flags_.concave_corners_valid) {
    return false;
  }
  if (cut_class_ != rhs.cut_class_) {
    return false;
  }
  if (eol_width_ != rhs.eol_width_) {
    return false;
  }
  if (eol_min_length_ != rhs.eol_min_length_) {
    return false;
  }
  if (first_overhang_ != rhs.first_overhang_) {
    return false;
  }
  if (second_overhang_ != rhs.second_overhang_) {
    return false;
  }
  if (spacing_ != rhs.spacing_) {
    return false;
  }
  if (extension_ != rhs.extension_) {
    return false;
  }
  if (forward_extension_ != rhs.forward_extension_) {
    return false;
  }
  if (backward_extension_ != rhs.backward_extension_) {
    return false;
  }
  if (min_width_ != rhs.min_width_) {
    return false;
  }
  if (cut_within_ != rhs.cut_within_) {
    return false;
  }
  if (min_length_ != rhs.min_length_) {
    return false;
  }
  if (par_length_ != rhs.par_length_) {
    return false;
  }
  if (second_par_length_ != rhs.second_par_length_) {
    return false;
  }
  if (par_within_ != rhs.par_within_) {
    return false;
  }
  if (second_par_within_ != rhs.second_par_within_) {
    return false;
  }
  if (below_enclosure_ != rhs.below_enclosure_) {
    return false;
  }
  if (num_corners_ != rhs.num_corners_) {
    return false;
  }

  return true;
}

bool _dbTechLayerCutEnclosureRule::operator<(
    const _dbTechLayerCutEnclosureRule& rhs) const
{
  return true;
}

_dbTechLayerCutEnclosureRule::_dbTechLayerCutEnclosureRule(_dbDatabase* db)
{
  flags_ = {};
  eol_width_ = 0;
  eol_min_length_ = 0;
  first_overhang_ = 0;
  second_overhang_ = 0;
  spacing_ = 0;
  extension_ = 0;
  forward_extension_ = 0;
  backward_extension_ = 0;
  min_width_ = 0;
  cut_within_ = 0;
  min_length_ = 0;
  par_length_ = 0;
  second_par_length_ = 0;
  par_within_ = 0;
  second_par_within_ = 0;
  below_enclosure_ = 0;
  num_corners_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutEnclosureRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
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
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCutEnclosureRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
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
  return stream;
}

void _dbTechLayerCutEnclosureRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

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
  if (obj->cut_class_ == 0) {
    return nullptr;
  }
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

void dbTechLayerCutEnclosureRule::setNumCorners(uint32_t num_corners)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->num_corners_ = num_corners;
}

uint32_t dbTechLayerCutEnclosureRule::getNumCorners() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;
  return obj->num_corners_;
}

void dbTechLayerCutEnclosureRule::setCutClassValid(bool cut_class_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.cut_class_valid = cut_class_valid;
}

bool dbTechLayerCutEnclosureRule::isCutClassValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.cut_class_valid;
}

void dbTechLayerCutEnclosureRule::setAbove(bool above)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.above = above;
}

bool dbTechLayerCutEnclosureRule::isAbove() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.above;
}

void dbTechLayerCutEnclosureRule::setBelow(bool below)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.below = below;
}

bool dbTechLayerCutEnclosureRule::isBelow() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.below;
}

void dbTechLayerCutEnclosureRule::setEolMinLengthValid(
    bool eol_min_length_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.eol_min_length_valid = eol_min_length_valid;
}

bool dbTechLayerCutEnclosureRule::isEolMinLengthValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.eol_min_length_valid;
}

void dbTechLayerCutEnclosureRule::setEolOnly(bool eol_only)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.eol_only = eol_only;
}

bool dbTechLayerCutEnclosureRule::isEolOnly() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.eol_only;
}

void dbTechLayerCutEnclosureRule::setShortEdgeOnEol(bool short_edge_on_eol)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.short_edge_on_eol = short_edge_on_eol;
}

bool dbTechLayerCutEnclosureRule::isShortEdgeOnEol() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.short_edge_on_eol;
}

void dbTechLayerCutEnclosureRule::setSideSpacingValid(bool side_spacing_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.side_spacing_valid = side_spacing_valid;
}

bool dbTechLayerCutEnclosureRule::isSideSpacingValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.side_spacing_valid;
}

void dbTechLayerCutEnclosureRule::setEndSpacingValid(bool end_spacing_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.end_spacing_valid = end_spacing_valid;
}

bool dbTechLayerCutEnclosureRule::isEndSpacingValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.end_spacing_valid;
}

void dbTechLayerCutEnclosureRule::setOffCenterLine(bool off_center_line)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.off_center_line = off_center_line;
}

bool dbTechLayerCutEnclosureRule::isOffCenterLine() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.off_center_line;
}

void dbTechLayerCutEnclosureRule::setWidthValid(bool width_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.width_valid = width_valid;
}

bool dbTechLayerCutEnclosureRule::isWidthValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.width_valid;
}

void dbTechLayerCutEnclosureRule::setIncludeAbutted(bool include_abutted)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.include_abutted = include_abutted;
}

bool dbTechLayerCutEnclosureRule::isIncludeAbutted() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.include_abutted;
}

void dbTechLayerCutEnclosureRule::setExceptExtraCut(bool except_extra_cut)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.except_extra_cut = except_extra_cut;
}

bool dbTechLayerCutEnclosureRule::isExceptExtraCut() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.except_extra_cut;
}

void dbTechLayerCutEnclosureRule::setPrl(bool prl)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.prl = prl;
}

bool dbTechLayerCutEnclosureRule::isPrl() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.prl;
}

void dbTechLayerCutEnclosureRule::setNoSharedEdge(bool no_shared_edge)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.no_shared_edge = no_shared_edge;
}

bool dbTechLayerCutEnclosureRule::isNoSharedEdge() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.no_shared_edge;
}

void dbTechLayerCutEnclosureRule::setLengthValid(bool length_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.length_valid = length_valid;
}

bool dbTechLayerCutEnclosureRule::isLengthValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.length_valid;
}

void dbTechLayerCutEnclosureRule::setExtraCutValid(bool extra_cut_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.extra_cut_valid = extra_cut_valid;
}

bool dbTechLayerCutEnclosureRule::isExtraCutValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.extra_cut_valid;
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

  obj->flags_.redundant_cut_valid = redundant_cut_valid;
}

bool dbTechLayerCutEnclosureRule::isRedundantCutValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.redundant_cut_valid;
}

void dbTechLayerCutEnclosureRule::setParallelValid(bool parallel_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.parallel_valid = parallel_valid;
}

bool dbTechLayerCutEnclosureRule::isParallelValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.parallel_valid;
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

  obj->flags_.second_par_within_valid = second_par_within_valid;
}

bool dbTechLayerCutEnclosureRule::isSecondParWithinValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.second_par_within_valid;
}

void dbTechLayerCutEnclosureRule::setBelowEnclosureValid(
    bool below_enclosure_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.below_enclosure_valid = below_enclosure_valid;
}

bool dbTechLayerCutEnclosureRule::isBelowEnclosureValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.below_enclosure_valid;
}

void dbTechLayerCutEnclosureRule::setConcaveCornersValid(
    bool concave_corners_valid)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.concave_corners_valid = concave_corners_valid;
}

bool dbTechLayerCutEnclosureRule::isConcaveCornersValid() const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return obj->flags_.concave_corners_valid;
}

// User Code Begin dbTechLayerCutEnclosureRulePublicMethods
void dbTechLayerCutEnclosureRule::setType(ENC_TYPE type)
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  obj->flags_.type = (uint32_t) type;
}

dbTechLayerCutEnclosureRule::ENC_TYPE dbTechLayerCutEnclosureRule::getType()
    const
{
  _dbTechLayerCutEnclosureRule* obj = (_dbTechLayerCutEnclosureRule*) this;

  return (dbTechLayerCutEnclosureRule::ENC_TYPE) obj->flags_.type;
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
                                                          uint32_t dbid)
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
