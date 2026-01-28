// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechLayerAntennaRule.h"

#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbVector.h"
#include "odb/db.h"
#include "odb/lefout.h"
#include "spdlog/fmt/ostr.h"

namespace odb {

template class dbTable<_dbTechLayerAntennaRule>;
template class dbTable<_dbTechAntennaPinModel>;

using std::vector;

bool ARuleFactor::operator==(const ARuleFactor& rhs) const
{
  if (factor_ != rhs.factor_) {
    return false;
  }

  if (explicit_ != rhs.explicit_) {
    return false;
  }

  if (diff_use_only_ != rhs.diff_use_only_) {
    return false;
  }

  return true;
}

bool ARuleRatio::operator==(const ARuleRatio& rhs) const
{
  if (ratio_ != rhs.ratio_) {
    return false;
  }

  if (diff_idx_ != rhs.diff_idx_) {
    return false;
  }

  if (diff_ratio_ != rhs.diff_ratio_) {
    return false;
  }

  return true;
}

bool _dbTechLayerAntennaRule::operator==(
    const _dbTechLayerAntennaRule& rhs) const
{
  if (layer_ != rhs.layer_) {
    return false;
  }

  if (area_mult_ != rhs.area_mult_) {
    return false;
  }

  if (sidearea_mult_ != rhs.sidearea_mult_) {
    return false;
  }

  if (par_area_val_ != rhs.par_area_val_) {
    return false;
  }

  if (cum_area_val_ != rhs.cum_area_val_) {
    return false;
  }

  if (par_sidearea_val_ != rhs.par_sidearea_val_) {
    return false;
  }

  if (cum_sidearea_val_ != rhs.cum_sidearea_val_) {
    return false;
  }

  if (area_diff_reduce_val_ != rhs.area_diff_reduce_val_) {
    return false;
  }

  if (gate_plus_diff_factor_ != rhs.gate_plus_diff_factor_) {
    return false;
  }

  if (area_minus_diff_factor_ != rhs.area_minus_diff_factor_) {
    return false;
  }

  if (has_antenna_cumroutingpluscut_ != rhs.has_antenna_cumroutingpluscut_) {
    return false;
  }

  return true;
}

bool _dbTechAntennaAreaElement::operator==(
    const _dbTechAntennaAreaElement& rhs) const
{
  if (area_ != rhs.area_) {
    return false;
  }

  if (lyidx_ != rhs.lyidx_) {
    return false;
  }

  return true;
}

bool _dbTechAntennaPinModel::operator==(const _dbTechAntennaPinModel& rhs) const
{
  if (mterm_ != rhs.mterm_) {
    return false;
  }

  if (gate_area_ != rhs.gate_area_) {
    return false;
  }

  if (max_area_car_ != rhs.max_area_car_) {
    return false;
  }

  if (max_sidearea_car_ != rhs.max_sidearea_car_) {
    return false;
  }

  if (max_cut_car_ != rhs.max_cut_car_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// ARuleFactor - Methods
//
////////////////////////////////////////////////////////////////////

void ARuleFactor::setFactor(double factor, bool diffuse)
{
  assert(factor > 0.0);
  factor_ = factor;
  diff_use_only_ = diffuse;
  explicit_ = true;
}

dbOStream& operator<<(dbOStream& stream, const ARuleFactor& arf)
{
  stream << arf.factor_;
  stream << arf.explicit_;
  stream << arf.diff_use_only_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, ARuleFactor& arf)
{
  stream >> arf.factor_;
  stream >> arf.explicit_;
  stream >> arf.diff_use_only_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// ARuleRatio - Methods
//
////////////////////////////////////////////////////////////////////

void ARuleRatio::setRatio(double ratio)
{
  assert(ratio > 0);
  ratio_ = ratio;
}

void ARuleRatio::setDiff(double ratio)
{
  assert(ratio > 0);
  assert((diff_idx_.size() == 0) && (diff_ratio_.size() == 0));
  diff_idx_.assign(1, 0.0);
  diff_ratio_.assign(1, ratio);
}

void ARuleRatio::setDiff(const vector<double>& diff_idx,
                         const vector<double>& ratios)
{
  assert((diff_idx_.size() == 0) && (diff_ratio_.size() == 0));
  diff_idx_ = diff_idx;
  diff_ratio_ = ratios;
}

dbOStream& operator<<(dbOStream& stream, const ARuleRatio& arrt)
{
  stream << arrt.ratio_;
  stream << arrt.diff_idx_;
  stream << arrt.diff_ratio_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, ARuleRatio& arrt)
{
  stream >> arrt.ratio_;
  stream >> arrt.diff_idx_;
  stream >> arrt.diff_ratio_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// _dbTechLayerAntennaRule - Methods
//
////////////////////////////////////////////////////////////////////

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerAntennaRule& inrule)
{
  stream << inrule.layer_;
  stream << inrule.area_mult_;
  stream << inrule.sidearea_mult_;
  stream << inrule.par_area_val_;
  stream << inrule.cum_area_val_;
  stream << inrule.par_sidearea_val_;
  stream << inrule.cum_sidearea_val_;
  stream << inrule.area_diff_reduce_val_;
  stream << inrule.gate_plus_diff_factor_;
  stream << inrule.area_minus_diff_factor_;
  stream << inrule.has_antenna_cumroutingpluscut_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerAntennaRule& inrule)
{
  stream >> inrule.layer_;
  stream >> inrule.area_mult_;
  stream >> inrule.sidearea_mult_;
  stream >> inrule.par_area_val_;
  stream >> inrule.cum_area_val_;
  stream >> inrule.par_sidearea_val_;
  stream >> inrule.cum_sidearea_val_;
  stream >> inrule.area_diff_reduce_val_;
  stream >> inrule.gate_plus_diff_factor_;
  stream >> inrule.area_minus_diff_factor_;
  stream >> inrule.has_antenna_cumroutingpluscut_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerAntennaRule - Methods
//
////////////////////////////////////////////////////////////////////

bool dbTechLayerAntennaRule::isValid() const
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  return ((ant_rule->par_area_val_.ratio_ > 0)
          || (ant_rule->cum_area_val_.ratio_ > 0)
          || (ant_rule->par_sidearea_val_.ratio_ > 0)
          || (ant_rule->cum_sidearea_val_.ratio_ > 0));
}

void dbTechLayerAntennaRule::writeLef(lefout& writer) const
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  if (ant_rule->area_mult_.explicit_) {
    fmt::print(writer.out(),
               "    ANTENNAAREAFACTOR {:g} {};\n",
               ant_rule->area_mult_.factor_,
               ant_rule->area_mult_.diff_use_only_ ? "DIFFUSEONLY " : "");
  }

  if (ant_rule->has_antenna_cumroutingpluscut_) {
    fmt::print(writer.out(), "    ANTENNACUMROUTINGPLUSCUT ;\n");
  }

  if (ant_rule->gate_plus_diff_factor_ > 0.0) {
    fmt::print(writer.out(),
               "    ANTENNAGATEPLUSDIFF {:g} ;\n",
               ant_rule->gate_plus_diff_factor_);
  }

  if (ant_rule->area_minus_diff_factor_ > 0.0) {
    fmt::print(writer.out(),
               "    ANTENNAAREAMINUSDIFF {:g} ;\n",
               ant_rule->area_minus_diff_factor_);
  }

  if (ant_rule->sidearea_mult_.explicit_) {
    fmt::print(writer.out(),
               "    ANTENNASIDEAREAFACTOR {:g} {};\n",
               ant_rule->sidearea_mult_.factor_,
               ant_rule->sidearea_mult_.diff_use_only_ ? "DIFFUSEONLY" : "");
  }

  dbVector<double>::const_iterator diffdx_itr;
  dbVector<double>::const_iterator ratio_itr;

  if (ant_rule->par_area_val_.ratio_ > 0) {
    fmt::print(writer.out(),
               "    ANTENNAAREARATIO {:g} ;\n",
               ant_rule->par_area_val_.ratio_);
  }

  if ((ant_rule->par_area_val_.diff_ratio_.size() == 1)
      && (ant_rule->par_area_val_.diff_ratio_[0] > 0)) {
    fmt::print(writer.out(),
               "    ANTENNADIFFAREARATIO {:g} ;\n",
               ant_rule->par_area_val_.diff_ratio_[0]);
  }

  if (ant_rule->par_area_val_.diff_ratio_.size() > 1) {
    fmt::print(writer.out(), "    ANTENNADIFFAREARATIO  PWL ( ");
    for (diffdx_itr = ant_rule->par_area_val_.diff_idx_.begin(),
        ratio_itr = ant_rule->par_area_val_.diff_ratio_.begin();
         diffdx_itr != ant_rule->par_area_val_.diff_idx_.end()
         && ratio_itr != ant_rule->par_area_val_.diff_ratio_.end();
         diffdx_itr++, ratio_itr++) {
      fmt::print(writer.out(), "( {:g} {:g} ) ", *diffdx_itr, *ratio_itr);
    }
    fmt::print(writer.out(), ") ;\n");
  }

  if (ant_rule->cum_area_val_.ratio_ > 0) {
    fmt::print(writer.out(),
               "    ANTENNACUMAREARATIO {:g} ;\n",
               ant_rule->cum_area_val_.ratio_);
  }

  if ((ant_rule->cum_area_val_.diff_ratio_.size() == 1)
      && (ant_rule->cum_area_val_.diff_ratio_[0] > 0)) {
    fmt::print(writer.out(),
               "    ANTENNACUMDIFFAREARATIO {:g} ;\n",
               ant_rule->cum_area_val_.diff_ratio_[0]);
  }

  if (ant_rule->cum_area_val_.diff_ratio_.size() > 1) {
    fmt::print(writer.out(), "    ANTENNACUMDIFFAREARATIO  PWL ( ");
    for (diffdx_itr = ant_rule->cum_area_val_.diff_idx_.begin(),
        ratio_itr = ant_rule->cum_area_val_.diff_ratio_.begin();
         diffdx_itr != ant_rule->cum_area_val_.diff_idx_.end()
         && ratio_itr != ant_rule->cum_area_val_.diff_ratio_.end();
         diffdx_itr++, ratio_itr++) {
      fmt::print(writer.out(), "( {:g} {:g} ) ", *diffdx_itr, *ratio_itr);
    }
    fmt::print(writer.out(), ") ;\n");
  }

  if (ant_rule->par_sidearea_val_.ratio_ > 0) {
    fmt::print(writer.out(),
               "    ANTENNASIDEAREARATIO {:g} ;\n",
               ant_rule->par_sidearea_val_.ratio_);
  }

  if ((ant_rule->par_sidearea_val_.diff_ratio_.size() == 1)
      && (ant_rule->par_sidearea_val_.diff_ratio_[0] > 0)) {
    fmt::print(writer.out(),
               "    ANTENNADIFFSIDEAREARATIO {:g} ;\n",
               ant_rule->par_sidearea_val_.diff_ratio_[0]);
  }

  if (ant_rule->par_sidearea_val_.diff_ratio_.size() > 1) {
    fmt::print(writer.out(), "    ANTENNADIFFSIDEAREARATIO  PWL ( ");
    for (diffdx_itr = ant_rule->par_sidearea_val_.diff_idx_.begin(),
        ratio_itr = ant_rule->par_sidearea_val_.diff_ratio_.begin();
         diffdx_itr != ant_rule->par_sidearea_val_.diff_idx_.end()
         && ratio_itr != ant_rule->par_sidearea_val_.diff_ratio_.end();
         diffdx_itr++, ratio_itr++) {
      fmt::print(writer.out(), "( {:g} {:g} ) ", *diffdx_itr, *ratio_itr);
    }
    fmt::print(writer.out(), ") ;\n");
  }

  if (ant_rule->cum_sidearea_val_.ratio_ > 0) {
    fmt::print(writer.out(),
               "    ANTENNACUMSIDEAREARATIO {:g} ;\n",
               ant_rule->cum_sidearea_val_.ratio_);
  }

  if ((ant_rule->cum_sidearea_val_.diff_ratio_.size() == 1)
      && (ant_rule->cum_sidearea_val_.diff_ratio_[0] > 0)) {
    fmt::print(writer.out(),
               "    ANTENNACUMDIFFSIDEAREARATIO {:g} ;\n",
               ant_rule->cum_sidearea_val_.diff_ratio_[0]);
  }

  if (ant_rule->cum_sidearea_val_.diff_ratio_.size() > 1) {
    fmt::print(writer.out(), "    ANTENNACUMDIFFSIDEAREARATIO  PWL ( ");
    for (diffdx_itr = ant_rule->cum_sidearea_val_.diff_idx_.begin(),
        ratio_itr = ant_rule->cum_sidearea_val_.diff_ratio_.begin();
         diffdx_itr != ant_rule->cum_sidearea_val_.diff_idx_.end()
         && ratio_itr != ant_rule->cum_sidearea_val_.diff_ratio_.end();
         diffdx_itr++, ratio_itr++) {
      fmt::print(writer.out(), "( {:g} {:g} ) ", *diffdx_itr, *ratio_itr);
    }
    fmt::print(writer.out(), ") ;\n");
  }

  if (ant_rule->area_diff_reduce_val_.diff_ratio_.size() > 1) {
    fmt::print(writer.out(), "    ANTENNAAREADIFFREDUCEPWL ( ");
    for (diffdx_itr = ant_rule->area_diff_reduce_val_.diff_idx_.begin(),
        ratio_itr = ant_rule->area_diff_reduce_val_.diff_ratio_.begin();
         diffdx_itr != ant_rule->area_diff_reduce_val_.diff_idx_.end()
         && ratio_itr != ant_rule->area_diff_reduce_val_.diff_ratio_.end();
         diffdx_itr++, ratio_itr++) {
      fmt::print(writer.out(), "( {:g} {:g} ) ", *diffdx_itr, *ratio_itr);
    }
    fmt::print(writer.out(), ") ;\n");
  }
}

bool dbTechLayerAntennaRule::hasAreaFactor() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->area_mult_.explicit_;
}

bool dbTechLayerAntennaRule::hasSideAreaFactor() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->sidearea_mult_.explicit_;
}

double dbTechLayerAntennaRule::getAreaFactor() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->area_mult_.factor_;
}

double dbTechLayerAntennaRule::getSideAreaFactor() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->sidearea_mult_.factor_;
}

bool dbTechLayerAntennaRule::isAreaFactorDiffUseOnly() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->area_mult_.diff_use_only_;
}

bool dbTechLayerAntennaRule::isSideAreaFactorDiffUseOnly() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->sidearea_mult_.diff_use_only_;
}

void dbTechLayerAntennaRule::setAreaFactor(double factor, bool diffuse)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->area_mult_.setFactor(factor, diffuse);
}

void dbTechLayerAntennaRule::setSideAreaFactor(double factor, bool diffuse)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->sidearea_mult_.setFactor(factor, diffuse);
}

double dbTechLayerAntennaRule::getPAR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->par_area_val_.ratio_;
}

double dbTechLayerAntennaRule::getCAR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->cum_area_val_.ratio_;
}

double dbTechLayerAntennaRule::getPSR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->par_sidearea_val_.ratio_;
}

double dbTechLayerAntennaRule::getCSR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->cum_sidearea_val_.ratio_;
}

dbTechLayerAntennaRule::pwl_pair dbTechLayerAntennaRule::getDiffPAR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  auto& rule = ant_rule->par_area_val_;
  return {.indices = rule.diff_idx_, .ratios = rule.diff_ratio_};
}

dbTechLayerAntennaRule::pwl_pair dbTechLayerAntennaRule::getDiffCAR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  auto& rule = ant_rule->cum_area_val_;
  return {.indices = rule.diff_idx_, .ratios = rule.diff_ratio_};
}

dbTechLayerAntennaRule::pwl_pair dbTechLayerAntennaRule::getDiffPSR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  auto& rule = ant_rule->par_sidearea_val_;
  return {.indices = rule.diff_idx_, .ratios = rule.diff_ratio_};
}

dbTechLayerAntennaRule::pwl_pair dbTechLayerAntennaRule::getDiffCSR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  auto& rule = ant_rule->cum_sidearea_val_;
  return {.indices = rule.diff_idx_, .ratios = rule.diff_ratio_};
}

void dbTechLayerAntennaRule::setPAR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->par_area_val_.setRatio(ratio);
}

void dbTechLayerAntennaRule::setCAR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->cum_area_val_.setRatio(ratio);
}

void dbTechLayerAntennaRule::setPSR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->par_sidearea_val_.setRatio(ratio);
}

void dbTechLayerAntennaRule::setCSR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->cum_sidearea_val_.setRatio(ratio);
}

void dbTechLayerAntennaRule::setDiffPAR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->par_area_val_.setDiff(ratio);
}

void dbTechLayerAntennaRule::setDiffCAR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->cum_area_val_.setDiff(ratio);
}

void dbTechLayerAntennaRule::setDiffPSR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->par_sidearea_val_.setDiff(ratio);
}

void dbTechLayerAntennaRule::setDiffCSR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->cum_sidearea_val_.setDiff(ratio);
}

void dbTechLayerAntennaRule::setDiffPAR(const vector<double>& diff_idx,
                                        const vector<double>& ratios)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->par_area_val_.setDiff(diff_idx, ratios);
}

void dbTechLayerAntennaRule::setDiffCAR(const vector<double>& diff_idx,
                                        const vector<double>& ratios)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->cum_area_val_.setDiff(diff_idx, ratios);
}

void dbTechLayerAntennaRule::setDiffPSR(const vector<double>& diff_idx,
                                        const vector<double>& ratios)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->par_sidearea_val_.setDiff(diff_idx, ratios);
}

void dbTechLayerAntennaRule::setDiffCSR(const vector<double>& diff_idx,
                                        const vector<double>& ratios)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->cum_sidearea_val_.setDiff(diff_idx, ratios);
}

dbTechLayerAntennaRule* dbTechLayerAntennaRule::getAntennaRule(dbTech* _tech,
                                                               uint32_t dbid)
{
  _dbTech* tech = (_dbTech*) _tech;
  return (dbTechLayerAntennaRule*) tech->antenna_rule_tbl_->getPtr(dbid);
}

bool dbTechLayerAntennaRule::hasAntennaCumRoutingPlusCut() const
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  return ant_rule->has_antenna_cumroutingpluscut_;
}

void dbTechLayerAntennaRule::setAntennaCumRoutingPlusCut(bool value)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  ant_rule->has_antenna_cumroutingpluscut_ = value;
}

double dbTechLayerAntennaRule::getGatePlusDiffFactor() const
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  return ant_rule->gate_plus_diff_factor_;
}

void dbTechLayerAntennaRule::setGatePlusDiffFactor(double factor)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  ant_rule->gate_plus_diff_factor_ = factor;
}

double dbTechLayerAntennaRule::getAreaMinusDiffFactor() const
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  return ant_rule->area_minus_diff_factor_;
}

void dbTechLayerAntennaRule::setAreaMinusDiffFactor(double factor)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  ant_rule->area_minus_diff_factor_ = factor;
}

dbTechLayerAntennaRule::pwl_pair dbTechLayerAntennaRule::getAreaDiffReduce()
    const
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  auto& rule = ant_rule->area_diff_reduce_val_;
  return {.indices = rule.diff_idx_, .ratios = rule.diff_ratio_};
}

void dbTechLayerAntennaRule::setAreaDiffReduce(
    const std::vector<double>& areas,
    const std::vector<double>& factors)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  ant_rule->area_diff_reduce_val_.setDiff(areas, factors);
}

////////////////////////////////////////////////////////////////////
//
// _dbTechAntennaAreaElement - Methods
//
////////////////////////////////////////////////////////////////////

dbOStream& operator<<(dbOStream& stream, const _dbTechAntennaAreaElement* aae)
{
  stream << aae->area_;
  stream << aae->lyidx_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechAntennaAreaElement*& aae)
{
  aae = new _dbTechAntennaAreaElement;
  stream >> aae->area_;
  stream >> aae->lyidx_;
  return stream;
}

//
// Allocate a new element and add to container. Layer argument is optional
//
void _dbTechAntennaAreaElement::create(
    dbVector<_dbTechAntennaAreaElement*>& incon,
    double inarea,
    dbTechLayer* inly)
{
  if (inarea < 0.0) {
    return;
  }

  _dbTechAntennaAreaElement* aae = new _dbTechAntennaAreaElement;
  aae->area_ = inarea;
  if (inly) {
    aae->lyidx_ = inly->getId();
  }

  incon.push_back(aae);
}

//
// Write out antenna element info given header string and file.
//
void _dbTechAntennaAreaElement::writeLef(const char* header,
                                         dbTech* tech,
                                         lefout& writer) const
{
  fmt::print(writer.out(), "        {} {:g} ", header, area_);
  if (lyidx_.isValid()) {
    fmt::print(writer.out(),
               "LAYER {} ",
               dbTechLayer::getTechLayer(tech, lyidx_)->getName().c_str());
  }
  fmt::print(writer.out(), ";\n");
}

////////////////////////////////////////////////////////////////////
//
// _dbTechAntennaPinModel methods here.
//
////////////////////////////////////////////////////////////////////

_dbTechAntennaPinModel::_dbTechAntennaPinModel(_dbDatabase*,
                                               const _dbTechAntennaPinModel& m)
    : mterm_(m.mterm_)
{
  dbVector<_dbTechAntennaAreaElement*>::const_iterator itr;

  for (itr = m.gate_area_.begin(); itr != m.gate_area_.end(); ++itr) {
    _dbTechAntennaAreaElement* e = new _dbTechAntennaAreaElement(*(*itr));
    gate_area_.push_back(e);
  }

  for (itr = m.max_area_car_.begin(); itr != m.max_area_car_.end(); ++itr) {
    _dbTechAntennaAreaElement* e = new _dbTechAntennaAreaElement(*(*itr));
    max_area_car_.push_back(e);
  }

  for (itr = m.max_sidearea_car_.begin(); itr != m.max_sidearea_car_.end();
       ++itr) {
    _dbTechAntennaAreaElement* e = new _dbTechAntennaAreaElement(*(*itr));
    max_sidearea_car_.push_back(e);
  }

  for (itr = m.max_cut_car_.begin(); itr != m.max_cut_car_.end(); ++itr) {
    _dbTechAntennaAreaElement* e = new _dbTechAntennaAreaElement(*(*itr));
    max_cut_car_.push_back(e);
  }
}

_dbTechAntennaPinModel::~_dbTechAntennaPinModel()
{
  for (auto elem : gate_area_) {
    delete elem;
  }
  gate_area_.clear();

  for (auto elem : max_area_car_) {
    delete elem;
  }
  max_area_car_.clear();

  for (auto elem : max_sidearea_car_) {
    delete elem;
  }
  max_sidearea_car_.clear();

  for (auto elem : max_cut_car_) {
    delete elem;
  }
  max_cut_car_.clear();
}

dbOStream& operator<<(dbOStream& stream, const _dbTechAntennaPinModel& inmod)
{
  stream << inmod.mterm_;
  stream << inmod.gate_area_;
  stream << inmod.max_area_car_;
  stream << inmod.max_sidearea_car_;
  stream << inmod.max_cut_car_;

  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechAntennaPinModel& inmod)
{
  stream >> inmod.mterm_;
  stream >> inmod.gate_area_;
  stream >> inmod.max_area_car_;
  stream >> inmod.max_sidearea_car_;
  stream >> inmod.max_cut_car_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechAntennaPinModel methods here.
//
////////////////////////////////////////////////////////////////////

void dbTechAntennaPinModel::addGateAreaEntry(double inval, dbTechLayer* refly)
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  _dbTechAntennaAreaElement::create(xmod->gate_area_, inval, refly);
}

void dbTechAntennaPinModel::addMaxAreaCAREntry(double inval, dbTechLayer* refly)
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  _dbTechAntennaAreaElement::create(xmod->max_area_car_, inval, refly);
}

void dbTechAntennaPinModel::addMaxSideAreaCAREntry(double inval,
                                                   dbTechLayer* refly)
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  _dbTechAntennaAreaElement::create(xmod->max_sidearea_car_, inval, refly);
}

void dbTechAntennaPinModel::addMaxCutCAREntry(double inval, dbTechLayer* refly)
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  _dbTechAntennaAreaElement::create(xmod->max_cut_car_, inval, refly);
}

void _dbTechAntennaPinModel::getAntennaValues(
    _dbDatabase* db,
    const dbVector<_dbTechAntennaAreaElement*>& elements,
    std::vector<std::pair<double, dbTechLayer*>>& result)
{
  _dbTech* tech = (_dbTech*) ((dbDatabase*) db)->getTech();

  for (auto elem : elements) {
    dbTechLayer* layer = nullptr;
    dbId<_dbTechLayer> layerId = elem->getLayerId();
    if (layerId.isValid()) {
      layer = (dbTechLayer*) tech->layer_tbl_->getPtr(layerId);
    }
    result.emplace_back(elem->getArea(), layer);
  }
}

void dbTechAntennaPinModel::getGateArea(
    std::vector<std::pair<double, dbTechLayer*>>& data)
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  _dbTechAntennaPinModel::getAntennaValues(
      getImpl()->getDatabase(), xmod->gate_area_, data);
}

void dbTechAntennaPinModel::getMaxAreaCAR(
    std::vector<std::pair<double, dbTechLayer*>>& data)
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  _dbTechAntennaPinModel::getAntennaValues(
      getImpl()->getDatabase(), xmod->max_area_car_, data);
}

void dbTechAntennaPinModel::getMaxSideAreaCAR(
    std::vector<std::pair<double, dbTechLayer*>>& data)
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  _dbTechAntennaPinModel::getAntennaValues(
      getImpl()->getDatabase(), xmod->max_sidearea_car_, data);
}

void dbTechAntennaPinModel::getMaxCutCAR(
    std::vector<std::pair<double, dbTechLayer*>>& data)
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  _dbTechAntennaPinModel::getAntennaValues(
      getImpl()->getDatabase(), xmod->max_cut_car_, data);
}

void dbTechAntennaPinModel::writeLef(dbTech* tech, lefout& writer) const
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;

  for (auto ant : xmod->gate_area_) {
    ant->writeLef("ANTENNAGATEAREA", tech, writer);
  }

  for (auto ant : xmod->max_area_car_) {
    ant->writeLef("ANTENNAMAXAREACAR", tech, writer);
  }

  for (auto ant : xmod->max_sidearea_car_) {
    ant->writeLef("ANTENNAMAXSIDEAREACAR", tech, writer);
  }

  for (auto ant : xmod->max_cut_car_) {
    ant->writeLef("ANTENNAMAXCUTCAR", tech, writer);
  }
}

dbTechAntennaPinModel* dbTechAntennaPinModel::getAntennaPinModel(
    dbMaster* _master,
    uint32_t dbid)
{
  _dbMaster* master = (_dbMaster*) _master;
  return (dbTechAntennaPinModel*) master->antenna_pin_model_tbl_->getPtr(dbid);
}

void _dbTechLayerAntennaRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

void _dbTechAntennaPinModel::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["_gate_area"].add(gate_area_);
  info.children["_gate_area"].size
      += gate_area_.size() * sizeof(_dbTechAntennaAreaElement);
  info.children["_max_area_car"].add(max_area_car_);
  info.children["_max_area_car"].size
      += max_area_car_.size() * sizeof(_dbTechAntennaAreaElement);
  info.children["_max_sidearea_car"].add(max_sidearea_car_);
  info.children["_max_sidearea_car"].size
      += max_sidearea_car_.size() * sizeof(_dbTechAntennaAreaElement);
  info.children["_max_cut_car"].add(max_cut_car_);
  info.children["_max_cut_car"].size
      += max_cut_car_.size() * sizeof(_dbTechAntennaAreaElement);
}

}  // namespace odb
