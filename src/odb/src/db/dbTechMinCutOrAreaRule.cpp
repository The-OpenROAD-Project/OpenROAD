// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechMinCutOrAreaRule.h"

#include <cassert>
#include <cstdint>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "odb/db.h"
#include "odb/lefout.h"

namespace odb {

template class dbTable<_dbTechMinCutRule>;
template class dbTable<_dbTechMinEncRule>;

void _dbTechMinCutRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

void _dbTechMinEncRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

bool _dbTechMinCutRule::operator==(const _dbTechMinCutRule& rhs) const
{
  if (flags_.rule != rhs.flags_.rule) {
    return false;
  }

  if (flags_.cuts_length != rhs.flags_.cuts_length) {
    return false;
  }

  if (num_cuts_ != rhs.num_cuts_) {
    return false;
  }

  if (width_ != rhs.width_) {
    return false;
  }

  if (cut_distance_ != rhs.cut_distance_) {
    return false;
  }

  if (length_ != rhs.length_) {
    return false;
  }

  if (distance_ != rhs.distance_) {
    return false;
  }

  return true;
}

bool _dbTechMinEncRule::operator==(const _dbTechMinEncRule& rhs) const
{
  if (flags_.has_width != rhs.flags_.has_width) {
    return false;
  }

  if (area_ != rhs.area_) {
    return false;
  }

  if (width_ != rhs.width_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// _dbTechMinCutRule - Methods
//
////////////////////////////////////////////////////////////////////

dbOStream& operator<<(dbOStream& stream, const _dbTechMinCutRule& rule)
{
  uint32_t* bit_field = (uint32_t*) &rule.flags_;
  stream << *bit_field;
  stream << rule.num_cuts_;
  stream << rule.width_;
  stream << rule.cut_distance_;
  stream << rule.length_;
  stream << rule.distance_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechMinCutRule& rule)
{
  uint32_t* bit_field = (uint32_t*) &rule.flags_;
  stream >> *bit_field;
  stream >> rule.num_cuts_;
  stream >> rule.width_;
  stream >> rule.cut_distance_;
  stream >> rule.length_;
  stream >> rule.distance_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// _dbTechMinEncRule - Methods
//
////////////////////////////////////////////////////////////////////

dbOStream& operator<<(dbOStream& stream, const _dbTechMinEncRule& rule)
{
  uint32_t* bit_field = (uint32_t*) &rule.flags_;
  stream << *bit_field;
  stream << rule.area_;
  stream << rule.width_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechMinEncRule& rule)
{
  uint32_t* bit_field = (uint32_t*) &rule.flags_;
  stream >> *bit_field;
  stream >> rule.area_;
  stream >> rule.width_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechMinCutRule - Methods
//
////////////////////////////////////////////////////////////////////

bool dbTechMinCutRule::getMinimumCuts(uint32_t& numcuts, uint32_t& width) const
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;

  if (_lsm->flags_.rule == _dbTechMinCutRule::kNone) {
    return false;
  }

  numcuts = _lsm->num_cuts_;
  width = _lsm->width_;
  return true;
}

void dbTechMinCutRule::setMinimumCuts(uint32_t numcuts,
                                      uint32_t width,
                                      bool above_only,
                                      bool below_only)
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;

  _lsm->num_cuts_ = numcuts;
  _lsm->width_ = width;

  if (above_only && below_only) {  // For default encoding, rule applies from
                                   // both above and below
    above_only = false;
    below_only = false;
  }

  if (above_only) {
    _lsm->flags_.rule = _dbTechMinCutRule::kMinimumCutAbove;
  } else if (below_only) {
    _lsm->flags_.rule = _dbTechMinCutRule::kMinimumCutBelow;
  } else {
    _lsm->flags_.rule = _dbTechMinCutRule::kMinimumCut;
  }
}

bool dbTechMinCutRule::isAboveOnly() const
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;
  return (_lsm->flags_.rule == _dbTechMinCutRule::kMinimumCutAbove);
}

bool dbTechMinCutRule::isBelowOnly() const
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;
  return (_lsm->flags_.rule == _dbTechMinCutRule::kMinimumCutBelow);
}

bool dbTechMinCutRule::getLengthForCuts(uint32_t& length,
                                        uint32_t& distance) const
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;

  if ((_lsm->flags_.rule == _dbTechMinCutRule::kNone)
      || !(_lsm->flags_.cuts_length)) {
    return false;
  }

  length = _lsm->length_;
  distance = _lsm->distance_;
  return true;
}

bool dbTechMinCutRule::getCutDistance(uint32_t& cut_distance) const
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;
  if (_lsm->cut_distance_ < 0) {
    return false;
  }

  cut_distance = _lsm->cut_distance_;
  return true;
}

void dbTechMinCutRule::setCutDistance(uint32_t cut_distance)
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;
  _lsm->cut_distance_ = cut_distance;
}

//
//  NOTE: Assumes that the rule type has already been set.
//
void dbTechMinCutRule::setLengthForCuts(uint32_t length, uint32_t distance)
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;

  assert((_lsm->flags_.rule == _dbTechMinCutRule::kMinimumCut)
         || (_lsm->flags_.rule == _dbTechMinCutRule::kMinimumCutAbove)
         || (_lsm->flags_.rule == _dbTechMinCutRule::kMinimumCutBelow));

  _lsm->flags_.cuts_length = 1;
  _lsm->length_ = length;
  _lsm->distance_ = distance;
}

void dbTechMinCutRule::writeLef(lefout& writer) const
{
  uint32_t numcuts = 0;
  uint32_t cut_width = 0;
  getMinimumCuts(numcuts, cut_width);
  fmt::print(writer.out(),
             "    MINIMUMCUT {}  WIDTH {:g} ",
             numcuts,
             writer.lefdist(cut_width));

  uint32_t cut_distance;
  if (getCutDistance(cut_distance)) {
    fmt::print(writer.out(), "WITHIN {:g} ", writer.lefdist(cut_distance));
  }

  if (isAboveOnly()) {
    fmt::print(writer.out(), "{}", "FROMABOVE ");
  } else if (isBelowOnly()) {
    fmt::print(writer.out(), "{}", "FROMBELOW ");
  }

  uint32_t length, distance;
  if (getLengthForCuts(length, distance)) {
    fmt::print(writer.out(),
               "LENGTH {:g}  WITHIN {:g} ",
               writer.lefdist(length),
               writer.lefdist(distance));
  }
  fmt::print(writer.out(), ";\n");
}

dbTechMinCutRule* dbTechMinCutRule::create(dbTechLayer* inly)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  _dbTechMinCutRule* newrule = layer->min_cut_rules_tbl_->create();
  return ((dbTechMinCutRule*) newrule);
}

dbTechMinCutRule* dbTechMinCutRule::getMinCutRule(dbTechLayer* inly,
                                                  uint32_t dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechMinCutRule*) layer->min_cut_rules_tbl_->getPtr(dbid);
}

////////////////////////////////////////////////////////////////////
//
// dbTechMinEncRule - Methods
//
////////////////////////////////////////////////////////////////////

bool dbTechMinEncRule::getEnclosure(uint32_t& area) const
{
  _dbTechMinEncRule* _lsm = (_dbTechMinEncRule*) this;

  area = _lsm->area_;
  return true;
}

void dbTechMinEncRule::setEnclosure(uint32_t area)
{
  _dbTechMinEncRule* _lsm = (_dbTechMinEncRule*) this;

  _lsm->area_ = area;
}

bool dbTechMinEncRule::getEnclosureWidth(uint32_t& width) const
{
  _dbTechMinEncRule* _lsm = (_dbTechMinEncRule*) this;

  if (!(_lsm->flags_.has_width)) {
    return false;
  }

  width = _lsm->width_;
  return true;
}

void dbTechMinEncRule::setEnclosureWidth(uint32_t width)
{
  _dbTechMinEncRule* _lsm = (_dbTechMinEncRule*) this;

  _lsm->flags_.has_width = 1;
  _lsm->width_ = width;
}

void dbTechMinEncRule::writeLef(lefout& writer) const
{
  uint32_t enc_area, enc_width;
  getEnclosure(enc_area);
  fmt::print(
      writer.out(), "    MINENCLOSEDAREA {:g} ", writer.lefarea(enc_area));
  if (getEnclosureWidth(enc_width)) {
    fmt::print(writer.out(), "WIDTH {:g} ", writer.lefdist(enc_width));
  }
  fmt::print(writer.out(), "{}", ";\n");
}

dbTechMinEncRule* dbTechMinEncRule::create(dbTechLayer* inly)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  _dbTechMinEncRule* newrule = layer->min_enc_rules_tbl_->create();
  return ((dbTechMinEncRule*) newrule);
}

dbTechMinEncRule* dbTechMinEncRule::getMinEncRule(dbTechLayer* inly,
                                                  uint32_t dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechMinEncRule*) layer->min_enc_rules_tbl_->getPtr(dbid);
}

}  // namespace odb
