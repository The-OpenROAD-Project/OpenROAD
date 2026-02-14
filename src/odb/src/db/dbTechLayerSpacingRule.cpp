// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechLayerSpacingRule.h"

#include <cassert>
#include <cstdint>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "odb/db.h"
#include "odb/lefout.h"
#include "spdlog/fmt/ostr.h"

namespace odb {

template class dbTable<_dbTechLayerSpacingRule>;
template class dbTable<_dbTechV55InfluenceEntry>;

void _dbTechLayerSpacingRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

void _dbTechV55InfluenceEntry::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

bool _dbTechLayerSpacingRule::operator==(
    const _dbTechLayerSpacingRule& rhs) const
{
  if (flags_.rule != rhs.flags_.rule) {
    return false;
  }

  if (flags_.except_same_pgnet != rhs.flags_.except_same_pgnet) {
    return false;
  }

  if (flags_.cut_stacking != rhs.flags_.cut_stacking) {
    return false;
  }

  if (flags_.cut_center_to_center != rhs.flags_.cut_center_to_center) {
    return false;
  }

  if (flags_.cut_same_net != rhs.flags_.cut_same_net) {
    return false;
  }

  if (flags_.cut_parallel_overlap != rhs.flags_.cut_parallel_overlap) {
    return false;
  }

  if (spacing_ != rhs.spacing_) {
    return false;
  }

  if (length_or_influence_ != rhs.length_or_influence_) {
    return false;
  }

  if (r1min_ != rhs.r1min_) {
    return false;
  }

  if (r1max_ != rhs.r1max_) {
    return false;
  }

  if (r2min_ != rhs.r2min_) {
    return false;
  }

  if (r2max_ != rhs.r2max_) {
    return false;
  }

  if (cut_area_ != rhs.cut_area_) {
    return false;
  }

  if (layer_ != rhs.layer_) {
    return false;
  }

  if (cut_layer_below_ != rhs.cut_layer_below_) {
    return false;
  }

  return true;
}

bool _dbTechV55InfluenceEntry::operator==(
    const _dbTechV55InfluenceEntry& rhs) const
{
  if (width_ != rhs.width_) {
    return false;
  }

  if (within_ != rhs.within_) {
    return false;
  }

  if (spacing_ != rhs.spacing_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// _dbTechLayerSpacingRule - Methods
// NOTE: Streaming methods work with pointers.
//
////////////////////////////////////////////////////////////////////

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerSpacingRule& rule)
{
  uint32_t* bit_field = (uint32_t*) &rule.flags_;
  stream << *bit_field;
  stream << rule.spacing_;
  stream << rule.length_or_influence_;
  stream << rule.r1min_;
  stream << rule.r1max_;
  stream << rule.r2min_;
  stream << rule.r2max_;
  stream << rule.cut_area_;
  stream << rule.layer_;
  stream << rule.cut_layer_below_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerSpacingRule& rule)
{
  uint32_t* bit_field = (uint32_t*) &rule.flags_;
  stream >> *bit_field;
  stream >> rule.spacing_;
  stream >> rule.length_or_influence_;
  stream >> rule.r1min_;
  stream >> rule.r1max_;
  stream >> rule.r2min_;
  stream >> rule.r2max_;
  stream >> rule.cut_area_;
  stream >> rule.layer_;
  stream >> rule.cut_layer_below_;

  return stream;
}

////////////////////////////////////////////////////////////////////
//
// _dbTechV55InfluenceEntry - Methods
//
////////////////////////////////////////////////////////////////////

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechV55InfluenceEntry& infitem)
{
  stream << infitem.width_;
  stream << infitem.within_;
  stream << infitem.spacing_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechV55InfluenceEntry& infitem)
{
  stream >> infitem.width_;
  stream >> infitem.within_;
  stream >> infitem.spacing_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerSpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

uint32_t dbTechLayerSpacingRule::getSpacing() const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  return _lsp->spacing_;
}

void dbTechLayerSpacingRule::setSpacing(uint32_t spacing)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  _lsp->spacing_ = spacing;
}

bool dbTechLayerSpacingRule::getCutStacking() const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  return _lsp->flags_.cut_stacking;
}

void dbTechLayerSpacingRule::setCutStacking(bool stacking)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  _lsp->flags_.cut_stacking = stacking;
}

bool dbTechLayerSpacingRule::getCutCenterToCenter() const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  return _lsp->flags_.cut_center_to_center;
}

void dbTechLayerSpacingRule::setCutCenterToCenter(bool c2c)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  _lsp->flags_.cut_center_to_center = c2c;
}

bool dbTechLayerSpacingRule::getCutSameNet() const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  return _lsp->flags_.cut_same_net;
}

void dbTechLayerSpacingRule::setCutSameNet(bool same_net)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  _lsp->flags_.cut_same_net = same_net;
}

bool dbTechLayerSpacingRule::getCutParallelOverlap() const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  return _lsp->flags_.cut_parallel_overlap;
}

void dbTechLayerSpacingRule::setCutParallelOverlap(bool overlap)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  _lsp->flags_.cut_parallel_overlap = overlap;
}

uint32_t dbTechLayerSpacingRule::getCutArea() const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  return _lsp->cut_area_;
}

void dbTechLayerSpacingRule::setCutArea(uint32_t area)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  _lsp->cut_area_ = area;
}

bool dbTechLayerSpacingRule::isUnconditional() const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  return (_lsp->flags_.rule == _dbTechLayerSpacingRule::kDefault);
}

bool dbTechLayerSpacingRule::getLengthThreshold(uint32_t& threshold) const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  if ((_lsp->flags_.rule != _dbTechLayerSpacingRule::kLengthThreshold)
      && (_lsp->flags_.rule
          != _dbTechLayerSpacingRule::kLengthThresholdRange)) {
    return false;
  }

  threshold = _lsp->length_or_influence_;
  return true;
}

bool dbTechLayerSpacingRule::getLengthThresholdRange(uint32_t& rmin,
                                                     uint32_t& rmax) const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  if (_lsp->flags_.rule != _dbTechLayerSpacingRule::kLengthThresholdRange) {
    return false;
  }

  rmin = _lsp->r2min_;
  rmax = _lsp->r2max_;
  return true;
}
bool dbTechLayerSpacingRule::hasRange() const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  if ((_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeOnly)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeRange)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeUseLength)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluence)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluenceRange)) {
    return false;
  }
  return true;
}

bool dbTechLayerSpacingRule::hasLengthThreshold() const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  if ((_lsp->flags_.rule != _dbTechLayerSpacingRule::kLengthThreshold)
      && (_lsp->flags_.rule
          != _dbTechLayerSpacingRule::kLengthThresholdRange)) {
    return false;
  }
  return true;
}

void dbTechLayerSpacingRule::setSpacingNotchLengthValid(bool val)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  _lsp->flags_.notch_length = val;
}

void dbTechLayerSpacingRule::setSpacingEndOfNotchWidthValid(bool val)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  _lsp->flags_.end_of_notch_width = val;
}

bool dbTechLayerSpacingRule::hasSpacingNotchLength() const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  return _lsp->flags_.notch_length;
}

bool dbTechLayerSpacingRule::hasSpacingEndOfNotchWidth() const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  return _lsp->flags_.end_of_notch_width;
}

bool dbTechLayerSpacingRule::getRange(uint32_t& rmin, uint32_t& rmax) const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  if ((_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeOnly)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeRange)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeUseLength)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluence)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluenceRange)) {
    return false;
  }

  rmin = _lsp->r1min_;
  rmax = _lsp->r1max_;
  return true;
}

bool dbTechLayerSpacingRule::hasUseLengthThreshold() const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  return (_lsp->flags_.rule == _dbTechLayerSpacingRule::kRangeUseLength);
}

bool dbTechLayerSpacingRule::getInfluence(uint32_t& influence) const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  if ((_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluence)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluenceRange)) {
    return false;
  }

  influence = _lsp->length_or_influence_;
  return true;
}

bool dbTechLayerSpacingRule::getInfluenceRange(uint32_t& rmin,
                                               uint32_t& rmax) const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  if (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluenceRange) {
    return false;
  }

  rmin = _lsp->r2min_;
  rmax = _lsp->r2max_;
  return true;
}

bool dbTechLayerSpacingRule::getRangeRange(uint32_t& rmin, uint32_t& rmax) const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  if (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeRange) {
    return false;
  }

  rmin = _lsp->r2min_;
  rmax = _lsp->r2max_;
  return true;
}

bool dbTechLayerSpacingRule::getAdjacentCuts(uint32_t& numcuts,
                                             uint32_t& within,
                                             uint32_t& spacing,
                                             bool& except_same_pgnet) const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;

  if (_lsp->flags_.rule != _dbTechLayerSpacingRule::kAdjacentCutsInfluence) {
    return false;
  }

  spacing = _lsp->spacing_;
  within = _lsp->length_or_influence_;
  numcuts = _lsp->r1min_;
  except_same_pgnet = _lsp->flags_.except_same_pgnet;
  return true;
}

bool dbTechLayerSpacingRule::getCutLayer4Spacing(dbTechLayer*& outly) const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  if (_lsp->flags_.rule != _dbTechLayerSpacingRule::kCutLayerBelow) {
    return false;
  }

  dbTechLayer* tmply = (dbTechLayer*) _lsp->getOwner();
  dbTech* tmptech = (dbTech*) tmply->getImpl()->getOwner();
  outly = odb::dbTechLayer::getTechLayer(tmptech, _lsp->cut_layer_below_);
  return true;
}

void dbTechLayerSpacingRule::setLengthThreshold(uint32_t threshold)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  assert((_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeOnly)
         && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeRange)
         && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeUseLength)
         && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluence)
         && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluenceRange)
         && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kCutLayerBelow)
         && (_lsp->flags_.rule
             != _dbTechLayerSpacingRule::kAdjacentCutsInfluence));

  // Not already LENGTHTHRESHOLD or LENGTHTHRESHOLD_RANGE
  if (_lsp->flags_.rule == _dbTechLayerSpacingRule::kDefault) {
    _lsp->flags_.rule = _dbTechLayerSpacingRule::kLengthThreshold;
  }

  _lsp->length_or_influence_ = threshold;
}

void dbTechLayerSpacingRule::setLengthThresholdRange(uint32_t rmin,
                                                     uint32_t rmax)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  assert(
      (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeOnly)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeRange)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeUseLength)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluence)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluenceRange));

  _lsp->flags_.rule = _dbTechLayerSpacingRule::kLengthThresholdRange;
  _lsp->r2min_ = rmin;
  _lsp->r2max_ = rmax;
}

void dbTechLayerSpacingRule::setRange(uint32_t rmin, uint32_t rmax)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  assert(
      (_lsp->flags_.rule != _dbTechLayerSpacingRule::kLengthThreshold)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kLengthThresholdRange));

  if (_lsp->flags_.rule == _dbTechLayerSpacingRule::kDefault) {
    _lsp->flags_.rule = _dbTechLayerSpacingRule::kRangeOnly;
  }

  _lsp->r1min_ = rmin;
  _lsp->r1max_ = rmax;
}

void dbTechLayerSpacingRule::setUseLengthThreshold()
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  assert(
      (_lsp->flags_.rule != _dbTechLayerSpacingRule::kLengthThreshold)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kLengthThresholdRange)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeRange)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluenceRange)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluence));

  _lsp->flags_.rule = _dbTechLayerSpacingRule::kRangeUseLength;
}

void dbTechLayerSpacingRule::setInfluence(uint32_t influence)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  assert(
      (_lsp->flags_.rule != _dbTechLayerSpacingRule::kLengthThreshold)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kLengthThresholdRange)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeUseLength)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kCutLayerBelow)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kAdjacentCutsInfluence)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeRange));

  // Not already RANGE_INFLUENCE or RANGE_INFLUENCE_RANGE
  if ((_lsp->flags_.rule == _dbTechLayerSpacingRule::kDefault)
      || (_lsp->flags_.rule == _dbTechLayerSpacingRule::kRangeOnly)) {
    _lsp->flags_.rule = _dbTechLayerSpacingRule::kRangeInfluence;
  }

  _lsp->length_or_influence_ = influence;
}

void dbTechLayerSpacingRule::setInfluenceRange(uint32_t rmin, uint32_t rmax)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  assert(
      (_lsp->flags_.rule != _dbTechLayerSpacingRule::kLengthThreshold)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kLengthThresholdRange)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeRange)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeUseLength));

  _lsp->flags_.rule = _dbTechLayerSpacingRule::kRangeInfluenceRange;
  _lsp->r2min_ = rmin;
  _lsp->r2max_ = rmax;
}

void dbTechLayerSpacingRule::setRangeRange(uint32_t rmin, uint32_t rmax)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  assert(
      (_lsp->flags_.rule != _dbTechLayerSpacingRule::kLengthThreshold)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kLengthThresholdRange)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluenceRange)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluence)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeUseLength));

  _lsp->flags_.rule = _dbTechLayerSpacingRule::kRangeRange;
  _lsp->r2min_ = rmin;
  _lsp->r2max_ = rmax;
}

void dbTechLayerSpacingRule::setEol(uint32_t width,
                                    uint32_t within,
                                    bool parallelEdge,
                                    uint32_t parallelSpace,
                                    uint32_t parallelWithin,
                                    bool twoEdges)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  assert(
      (_lsp->flags_.rule != _dbTechLayerSpacingRule::kLengthThreshold)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kLengthThresholdRange)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeRange)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluenceRange)
      && (_lsp->flags_.rule != _dbTechLayerSpacingRule::kRangeInfluence));

  _lsp->r1min_ = width;
  _lsp->r1max_ = within;
  if (!parallelEdge) {
    _lsp->flags_.rule = _dbTechLayerSpacingRule::kEndOfLine;
  } else {
    _lsp->r2min_ = parallelSpace;
    _lsp->r2max_ = parallelWithin;
    if (!twoEdges) {
      _lsp->flags_.rule = _dbTechLayerSpacingRule::kEndOfLineParallel;
    } else {
      _lsp->flags_.rule = _dbTechLayerSpacingRule::kEndOfLineParallelTwoEdges;
    }
  }
}

bool dbTechLayerSpacingRule::getEol(uint32_t& width,
                                    uint32_t& within,
                                    bool& parallelEdge,
                                    uint32_t& parallelSpace,
                                    uint32_t& parallelWithin,
                                    bool& twoEdges) const
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;

  if (_lsp->flags_.rule != _dbTechLayerSpacingRule::kEndOfLine
      && _lsp->flags_.rule != _dbTechLayerSpacingRule::kEndOfLineParallel
      && _lsp->flags_.rule
             != _dbTechLayerSpacingRule::kEndOfLineParallelTwoEdges) {
    return false;
  }

  parallelSpace = 0;
  parallelWithin = 0;
  twoEdges = false;
  width = _lsp->r1min_;
  within = _lsp->r1max_;
  if (_lsp->flags_.rule == _dbTechLayerSpacingRule::kEndOfLine) {
    parallelSpace = false;
    return true;
  }
  parallelEdge = true;
  parallelSpace = _lsp->r2min_;
  parallelWithin = _lsp->r2max_;
  twoEdges = _lsp->flags_.rule
             == _dbTechLayerSpacingRule::kEndOfLineParallelTwoEdges;
  return true;
}

void dbTechLayerSpacingRule::setSameNetPgOnly(bool pgonly)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  _lsp->flags_.except_same_pgnet = pgonly;
}

bool dbTechLayerSpacingRule::getSameNetPgOnly()
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  return _lsp->flags_.except_same_pgnet;
}

void dbTechLayerSpacingRule::setAdjacentCuts(uint32_t numcuts,
                                             uint32_t within,
                                             uint32_t spacing,
                                             bool except_same_pgnet)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;

  assert((_lsp->flags_.rule == _dbTechLayerSpacingRule::kDefault)
         || (_lsp->flags_.rule
             == _dbTechLayerSpacingRule::kAdjacentCutsInfluence));

  _lsp->flags_.rule = _dbTechLayerSpacingRule::kAdjacentCutsInfluence;
  _lsp->flags_.except_same_pgnet = except_same_pgnet;
  _lsp->spacing_ = spacing;
  _lsp->length_or_influence_ = within;
  _lsp->r1min_ = numcuts;
}

void dbTechLayerSpacingRule::setCutLayer4Spacing(dbTechLayer* cutly)
{
  _dbTechLayerSpacingRule* _lsp = (_dbTechLayerSpacingRule*) this;
  assert((_lsp->flags_.rule == _dbTechLayerSpacingRule::kDefault)
         || (_lsp->flags_.rule == _dbTechLayerSpacingRule::kCutLayerBelow));

  [[maybe_unused]] dbTechLayer* tmply = (dbTechLayer*) _lsp->getOwner();
  assert(cutly->getNumber() < tmply->getNumber());
  _dbTechLayer* ct_ly = (_dbTechLayer*) cutly;

  _lsp->flags_.rule = _dbTechLayerSpacingRule::kCutLayerBelow;
  _lsp->cut_layer_below_ = ct_ly->getOID();
}

dbTechLayerSpacingRule* dbTechLayerSpacingRule::create(dbTechLayer* inly)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  _dbTechLayerSpacingRule* newrule = layer->spacing_rules_tbl_->create();
  newrule->layer_ = inly->getImpl()->getOID();

  return ((dbTechLayerSpacingRule*) newrule);
}

dbTechLayerSpacingRule* dbTechLayerSpacingRule::getTechLayerSpacingRule(
    dbTechLayer* inly,
    uint32_t dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerSpacingRule*) layer->spacing_rules_tbl_->getPtr(dbid);
}

void dbTechLayerSpacingRule::writeLef(lefout& writer) const
{
  uint32_t rmin, rmax, length_or_influence, cut_spacing, numcuts;
  bool except_same_pgnet;
  dbTechLayer* rulely;

  fmt::print(writer.out(), "    SPACING {:g} ", writer.lefdist(getSpacing()));

  if (getCutCenterToCenter()) {
    fmt::print(writer.out(), "    CENTERTOCENTER ");
  }

  if (getCutSameNet()) {
    fmt::print(writer.out(), "    SAMENET ");
  }

  if (getCutParallelOverlap()) {
    fmt::print(writer.out(), "    PARALLELOVERLAP ");
  }

  if (getCutArea() > 0) {
    fmt::print(writer.out(), "    AREA {:g} ", writer.lefdist(getCutArea()));
  }

  if (getRange(rmin, rmax)) {
    fmt::print(writer.out(),
               "RANGE {:g} {:g} ",
               writer.lefdist(rmin),
               writer.lefdist(rmax));
    if (hasUseLengthThreshold()) {
      fmt::print(writer.out(), "USELENGTHTHRESHOLD ");
    } else if (getInfluence(length_or_influence)) {
      fmt::print(
          writer.out(), "INFLUENCE {:g} ", writer.lefdist(length_or_influence));
      if (getInfluenceRange(rmin, rmax)) {
        fmt::print(writer.out(),
                   "RANGE {:g} {:g} ",
                   writer.lefdist(rmin),
                   writer.lefdist(rmax));
      }
    } else if (getRangeRange(rmin, rmax)) {
      fmt::print(writer.out(),
                 "RANGE {:g} {:g} ",
                 writer.lefdist(rmin),
                 writer.lefdist(rmax));
    }
  } else if (getLengthThreshold(length_or_influence)) {
    fmt::print(writer.out(),
               "LENGTHTHRESHOLD {:g} ",
               writer.lefdist(length_or_influence));
    if (getLengthThresholdRange(rmin, rmax)) {
      fmt::print(writer.out(),
                 "RANGE {:g} {:g} ",
                 writer.lefdist(rmin),
                 writer.lefdist(rmax));
    }
  } else if (getCutLayer4Spacing(rulely)) {
    fmt::print(writer.out(), "LAYER {} ", rulely->getName().c_str());
  } else if (getAdjacentCuts(numcuts,
                             length_or_influence,
                             cut_spacing,
                             except_same_pgnet)) {
    fmt::print(writer.out(),
               "ADJACENTCUTS {} WITHIN {:g} ",
               numcuts,
               writer.lefdist(length_or_influence));
    if (except_same_pgnet) {
      fmt::print(writer.out(), "EXCEPTSAMEPGNET ");
    }
  } else {
    uint32_t width, within, parallelSpace, parallelWithin;
    bool parallelEdge, twoEdges;
    if (getEol(width,
               within,
               parallelEdge,
               parallelSpace,
               parallelWithin,
               twoEdges)) {
      fmt::print(writer.out(),
                 "ENDOFLINE {:g} WITHIN {:g} ",
                 writer.lefdist(width),
                 writer.lefdist(within));
      if (parallelEdge) {
        fmt::print(writer.out(),
                   "PARALLELEDGE {:g} WITHIN {:g} ",
                   writer.lefdist(parallelSpace),
                   writer.lefdist(parallelWithin));
        if (twoEdges) {
          fmt::print(writer.out(), " TWOEDGES ");
        }
      }
    }
  }
  fmt::print(writer.out(), " ;\n");
}

////////////////////////////////////////////////////////////////////
//
// dbTechV55InfluenceEntry - Methods
//
////////////////////////////////////////////////////////////////////

bool dbTechV55InfluenceEntry::getV55InfluenceEntry(uint32_t& width,
                                                   uint32_t& within,
                                                   uint32_t& spacing) const
{
  _dbTechV55InfluenceEntry* _v55ie = (_dbTechV55InfluenceEntry*) this;
  width = _v55ie->width_;
  within = _v55ie->within_;
  spacing = _v55ie->spacing_;
  return true;
}

void dbTechV55InfluenceEntry::setV55InfluenceEntry(const uint32_t& width,
                                                   const uint32_t& within,
                                                   const uint32_t& spacing)
{
  _dbTechV55InfluenceEntry* _v55ie = (_dbTechV55InfluenceEntry*) this;
  _v55ie->width_ = width;
  _v55ie->within_ = within;
  _v55ie->spacing_ = spacing;
}

void dbTechV55InfluenceEntry::writeLef(lefout& writer) const
{
  uint32_t inf_width, inf_within, inf_spacing;

  getV55InfluenceEntry(inf_width, inf_within, inf_spacing);
  fmt::print(writer.out(),
             "\n   WIDTH {:g} WITHIN {:g} SPACING {:g}",
             writer.lefdist(inf_width),
             writer.lefdist(inf_within),
             writer.lefdist(inf_spacing));
}

dbTechV55InfluenceEntry* dbTechV55InfluenceEntry::create(dbTechLayer* inly)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  _dbTechV55InfluenceEntry* newitem = layer->v55inf_tbl_->create();
  return ((dbTechV55InfluenceEntry*) newitem);
}

dbTechV55InfluenceEntry* dbTechV55InfluenceEntry::getV55InfluenceEntry(
    dbTechLayer* inly,
    uint32_t oid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechV55InfluenceEntry*) layer->v55inf_tbl_->getPtr(oid);
}

}  // namespace odb
