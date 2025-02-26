///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#include "dbTechLayerAntennaRule.h"

#include <spdlog/fmt/ostr.h>

#include <vector>

#include "dbDatabase.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "odb/db.h"
#include "odb/lefout.h"

namespace odb {

template class dbTable<_dbTechLayerAntennaRule>;
template class dbTable<_dbTechAntennaPinModel>;

using std::vector;

bool _ARuleFactor::operator==(const _ARuleFactor& rhs) const
{
  if (_factor != rhs._factor) {
    return false;
  }

  if (_explicit != rhs._explicit) {
    return false;
  }

  if (_diff_use_only != rhs._diff_use_only) {
    return false;
  }

  return true;
}

bool _ARuleRatio::operator==(const _ARuleRatio& rhs) const
{
  if (_ratio != rhs._ratio) {
    return false;
  }

  if (_diff_idx != rhs._diff_idx) {
    return false;
  }

  if (_diff_ratio != rhs._diff_ratio) {
    return false;
  }

  return true;
}

bool _dbTechLayerAntennaRule::operator==(
    const _dbTechLayerAntennaRule& rhs) const
{
  if (_layer != rhs._layer) {
    return false;
  }

  if (_area_mult != rhs._area_mult) {
    return false;
  }

  if (_sidearea_mult != rhs._sidearea_mult) {
    return false;
  }

  if (_par_area_val != rhs._par_area_val) {
    return false;
  }

  if (_cum_area_val != rhs._cum_area_val) {
    return false;
  }

  if (_par_sidearea_val != rhs._par_sidearea_val) {
    return false;
  }

  if (_cum_sidearea_val != rhs._cum_sidearea_val) {
    return false;
  }

  if (_area_diff_reduce_val != rhs._area_diff_reduce_val) {
    return false;
  }

  if (_gate_plus_diff_factor != rhs._gate_plus_diff_factor) {
    return false;
  }

  if (_area_minus_diff_factor != rhs._area_minus_diff_factor) {
    return false;
  }

  if (_has_antenna_cumroutingpluscut != rhs._has_antenna_cumroutingpluscut) {
    return false;
  }

  return true;
}

bool _dbTechAntennaAreaElement::operator==(
    const _dbTechAntennaAreaElement& rhs) const
{
  if (_area != rhs._area) {
    return false;
  }

  if (_lyidx != rhs._lyidx) {
    return false;
  }

  return true;
}

bool _dbTechAntennaPinModel::operator==(const _dbTechAntennaPinModel& rhs) const
{
  if (_mterm != rhs._mterm) {
    return false;
  }

  if (_gate_area != rhs._gate_area) {
    return false;
  }

  if (_max_area_car != rhs._max_area_car) {
    return false;
  }

  if (_max_sidearea_car != rhs._max_sidearea_car) {
    return false;
  }

  if (_max_cut_car != rhs._max_cut_car) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// _ARuleFactor - Methods
//
////////////////////////////////////////////////////////////////////

void _ARuleFactor::setFactor(double factor, bool diffuse)
{
  assert(factor > 0.0);
  _factor = factor;
  _diff_use_only = diffuse;
  _explicit = true;
}

dbOStream& operator<<(dbOStream& stream, const _ARuleFactor& arf)
{
  stream << arf._factor;
  stream << arf._explicit;
  stream << arf._diff_use_only;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _ARuleFactor& arf)
{
  stream >> arf._factor;
  stream >> arf._explicit;
  stream >> arf._diff_use_only;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// _ARuleRatio - Methods
//
////////////////////////////////////////////////////////////////////

void _ARuleRatio::setRatio(double ratio)
{
  assert(ratio > 0);
  _ratio = ratio;
}

void _ARuleRatio::setDiff(double ratio)
{
  assert(ratio > 0);
  assert((_diff_idx.size() == 0) && (_diff_ratio.size() == 0));
  _diff_idx.assign(1, 0.0);
  _diff_ratio.assign(1, ratio);
}

void _ARuleRatio::setDiff(const vector<double>& diff_idx,
                          const vector<double>& ratios)
{
  assert((_diff_idx.size() == 0) && (_diff_ratio.size() == 0));
  _diff_idx = diff_idx;
  _diff_ratio = ratios;
}

dbOStream& operator<<(dbOStream& stream, const _ARuleRatio& arrt)
{
  stream << arrt._ratio;
  stream << arrt._diff_idx;
  stream << arrt._diff_ratio;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _ARuleRatio& arrt)
{
  stream >> arrt._ratio;
  stream >> arrt._diff_idx;
  stream >> arrt._diff_ratio;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// _dbTechLayerAntennaRule - Methods
//
////////////////////////////////////////////////////////////////////

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerAntennaRule& inrule)
{
  stream << inrule._layer;
  stream << inrule._area_mult;
  stream << inrule._sidearea_mult;
  stream << inrule._par_area_val;
  stream << inrule._cum_area_val;
  stream << inrule._par_sidearea_val;
  stream << inrule._cum_sidearea_val;
  stream << inrule._area_diff_reduce_val;
  stream << inrule._gate_plus_diff_factor;
  stream << inrule._area_minus_diff_factor;
  stream << inrule._has_antenna_cumroutingpluscut;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerAntennaRule& inrule)
{
  stream >> inrule._layer;
  stream >> inrule._area_mult;
  stream >> inrule._sidearea_mult;
  stream >> inrule._par_area_val;
  stream >> inrule._cum_area_val;
  stream >> inrule._par_sidearea_val;
  stream >> inrule._cum_sidearea_val;
  stream >> inrule._area_diff_reduce_val;
  stream >> inrule._gate_plus_diff_factor;
  stream >> inrule._area_minus_diff_factor;
  stream >> inrule._has_antenna_cumroutingpluscut;
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

  return ((ant_rule->_par_area_val._ratio > 0)
          || (ant_rule->_cum_area_val._ratio > 0)
          || (ant_rule->_par_sidearea_val._ratio > 0)
          || (ant_rule->_cum_sidearea_val._ratio > 0));
}

void dbTechLayerAntennaRule::writeLef(lefout& writer) const
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  if (ant_rule->_area_mult._explicit) {
    fmt::print(writer.out(),
               "    ANTENNAAREAFACTOR {:g} {};\n",
               ant_rule->_area_mult._factor,
               ant_rule->_area_mult._diff_use_only ? "DIFFUSEONLY " : "");
  }

  if (ant_rule->_has_antenna_cumroutingpluscut) {
    fmt::print(writer.out(), "    ANTENNACUMROUTINGPLUSCUT ;\n");
  }

  if (ant_rule->_gate_plus_diff_factor > 0.0) {
    fmt::print(writer.out(),
               "    ANTENNAGATEPLUSDIFF {:g} ;\n",
               ant_rule->_gate_plus_diff_factor);
  }

  if (ant_rule->_area_minus_diff_factor > 0.0) {
    fmt::print(writer.out(),
               "    ANTENNAAREAMINUSDIFF {:g} ;\n",
               ant_rule->_area_minus_diff_factor);
  }

  if (ant_rule->_sidearea_mult._explicit) {
    fmt::print(writer.out(),
               "    ANTENNASIDEAREAFACTOR {:g} {};\n",
               ant_rule->_sidearea_mult._factor,
               ant_rule->_sidearea_mult._diff_use_only ? "DIFFUSEONLY" : "");
  }

  dbVector<double>::const_iterator diffdx_itr;
  dbVector<double>::const_iterator ratio_itr;

  if (ant_rule->_par_area_val._ratio > 0) {
    fmt::print(writer.out(),
               "    ANTENNAAREARATIO {:g} ;\n",
               ant_rule->_par_area_val._ratio);
  }

  if ((ant_rule->_par_area_val._diff_ratio.size() == 1)
      && (ant_rule->_par_area_val._diff_ratio[0] > 0)) {
    fmt::print(writer.out(),
               "    ANTENNADIFFAREARATIO {:g} ;\n",
               ant_rule->_par_area_val._diff_ratio[0]);
  }

  if (ant_rule->_par_area_val._diff_ratio.size() > 1) {
    fmt::print(writer.out(), "    ANTENNADIFFAREARATIO  PWL ( ");
    for (diffdx_itr = ant_rule->_par_area_val._diff_idx.begin(),
        ratio_itr = ant_rule->_par_area_val._diff_ratio.begin();
         diffdx_itr != ant_rule->_par_area_val._diff_idx.end()
         && ratio_itr != ant_rule->_par_area_val._diff_ratio.end();
         diffdx_itr++, ratio_itr++) {
      fmt::print(writer.out(), "( {:g} {:g} ) ", *diffdx_itr, *ratio_itr);
    }
    fmt::print(writer.out(), ") ;\n");
  }

  if (ant_rule->_cum_area_val._ratio > 0) {
    fmt::print(writer.out(),
               "    ANTENNACUMAREARATIO {:g} ;\n",
               ant_rule->_cum_area_val._ratio);
  }

  if ((ant_rule->_cum_area_val._diff_ratio.size() == 1)
      && (ant_rule->_cum_area_val._diff_ratio[0] > 0)) {
    fmt::print(writer.out(),
               "    ANTENNACUMDIFFAREARATIO {:g} ;\n",
               ant_rule->_cum_area_val._diff_ratio[0]);
  }

  if (ant_rule->_cum_area_val._diff_ratio.size() > 1) {
    fmt::print(writer.out(), "    ANTENNACUMDIFFAREARATIO  PWL ( ");
    for (diffdx_itr = ant_rule->_cum_area_val._diff_idx.begin(),
        ratio_itr = ant_rule->_cum_area_val._diff_ratio.begin();
         diffdx_itr != ant_rule->_cum_area_val._diff_idx.end()
         && ratio_itr != ant_rule->_cum_area_val._diff_ratio.end();
         diffdx_itr++, ratio_itr++) {
      fmt::print(writer.out(), "( {:g} {:g} ) ", *diffdx_itr, *ratio_itr);
    }
    fmt::print(writer.out(), ") ;\n");
  }

  if (ant_rule->_par_sidearea_val._ratio > 0) {
    fmt::print(writer.out(),
               "    ANTENNASIDEAREARATIO {:g} ;\n",
               ant_rule->_par_sidearea_val._ratio);
  }

  if ((ant_rule->_par_sidearea_val._diff_ratio.size() == 1)
      && (ant_rule->_par_sidearea_val._diff_ratio[0] > 0)) {
    fmt::print(writer.out(),
               "    ANTENNADIFFSIDEAREARATIO {:g} ;\n",
               ant_rule->_par_sidearea_val._diff_ratio[0]);
  }

  if (ant_rule->_par_sidearea_val._diff_ratio.size() > 1) {
    fmt::print(writer.out(), "    ANTENNADIFFSIDEAREARATIO  PWL ( ");
    for (diffdx_itr = ant_rule->_par_sidearea_val._diff_idx.begin(),
        ratio_itr = ant_rule->_par_sidearea_val._diff_ratio.begin();
         diffdx_itr != ant_rule->_par_sidearea_val._diff_idx.end()
         && ratio_itr != ant_rule->_par_sidearea_val._diff_ratio.end();
         diffdx_itr++, ratio_itr++) {
      fmt::print(writer.out(), "( {:g} {:g} ) ", *diffdx_itr, *ratio_itr);
    }
    fmt::print(writer.out(), ") ;\n");
  }

  if (ant_rule->_cum_sidearea_val._ratio > 0) {
    fmt::print(writer.out(),
               "    ANTENNACUMSIDEAREARATIO {:g} ;\n",
               ant_rule->_cum_sidearea_val._ratio);
  }

  if ((ant_rule->_cum_sidearea_val._diff_ratio.size() == 1)
      && (ant_rule->_cum_sidearea_val._diff_ratio[0] > 0)) {
    fmt::print(writer.out(),
               "    ANTENNACUMDIFFSIDEAREARATIO {:g} ;\n",
               ant_rule->_cum_sidearea_val._diff_ratio[0]);
  }

  if (ant_rule->_cum_sidearea_val._diff_ratio.size() > 1) {
    fmt::print(writer.out(), "    ANTENNACUMDIFFSIDEAREARATIO  PWL ( ");
    for (diffdx_itr = ant_rule->_cum_sidearea_val._diff_idx.begin(),
        ratio_itr = ant_rule->_cum_sidearea_val._diff_ratio.begin();
         diffdx_itr != ant_rule->_cum_sidearea_val._diff_idx.end()
         && ratio_itr != ant_rule->_cum_sidearea_val._diff_ratio.end();
         diffdx_itr++, ratio_itr++) {
      fmt::print(writer.out(), "( {:g} {:g} ) ", *diffdx_itr, *ratio_itr);
    }
    fmt::print(writer.out(), ") ;\n");
  }

  if (ant_rule->_area_diff_reduce_val._diff_ratio.size() > 1) {
    fmt::print(writer.out(), "    ANTENNAAREADIFFREDUCEPWL ( ");
    for (diffdx_itr = ant_rule->_area_diff_reduce_val._diff_idx.begin(),
        ratio_itr = ant_rule->_area_diff_reduce_val._diff_ratio.begin();
         diffdx_itr != ant_rule->_area_diff_reduce_val._diff_idx.end()
         && ratio_itr != ant_rule->_area_diff_reduce_val._diff_ratio.end();
         diffdx_itr++, ratio_itr++) {
      fmt::print(writer.out(), "( {:g} {:g} ) ", *diffdx_itr, *ratio_itr);
    }
    fmt::print(writer.out(), ") ;\n");
  }
}

bool dbTechLayerAntennaRule::hasAreaFactor() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->_area_mult._explicit;
}

bool dbTechLayerAntennaRule::hasSideAreaFactor() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->_sidearea_mult._explicit;
}

double dbTechLayerAntennaRule::getAreaFactor() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->_area_mult._factor;
}

double dbTechLayerAntennaRule::getSideAreaFactor() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->_sidearea_mult._factor;
}

bool dbTechLayerAntennaRule::isAreaFactorDiffUseOnly() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->_area_mult._diff_use_only;
}

bool dbTechLayerAntennaRule::isSideAreaFactorDiffUseOnly() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->_sidearea_mult._diff_use_only;
}

void dbTechLayerAntennaRule::setAreaFactor(double factor, bool diffuse)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->_area_mult.setFactor(factor, diffuse);
}

void dbTechLayerAntennaRule::setSideAreaFactor(double factor, bool diffuse)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->_sidearea_mult.setFactor(factor, diffuse);
}

double dbTechLayerAntennaRule::getPAR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->_par_area_val._ratio;
}

double dbTechLayerAntennaRule::getCAR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->_cum_area_val._ratio;
}

double dbTechLayerAntennaRule::getPSR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->_par_sidearea_val._ratio;
}

double dbTechLayerAntennaRule::getCSR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  return ant_rule->_cum_sidearea_val._ratio;
}

dbTechLayerAntennaRule::pwl_pair dbTechLayerAntennaRule::getDiffPAR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  auto& rule = ant_rule->_par_area_val;
  return pwl_pair{rule._diff_idx, rule._diff_ratio};
}

dbTechLayerAntennaRule::pwl_pair dbTechLayerAntennaRule::getDiffCAR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  auto& rule = ant_rule->_cum_area_val;
  return pwl_pair{rule._diff_idx, rule._diff_ratio};
}

dbTechLayerAntennaRule::pwl_pair dbTechLayerAntennaRule::getDiffPSR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  auto& rule = ant_rule->_par_sidearea_val;
  return pwl_pair{rule._diff_idx, rule._diff_ratio};
}

dbTechLayerAntennaRule::pwl_pair dbTechLayerAntennaRule::getDiffCSR() const
{
  auto ant_rule = (const _dbTechLayerAntennaRule*) this;
  auto& rule = ant_rule->_cum_sidearea_val;
  return pwl_pair{rule._diff_idx, rule._diff_ratio};
}

void dbTechLayerAntennaRule::setPAR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->_par_area_val.setRatio(ratio);
}

void dbTechLayerAntennaRule::setCAR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->_cum_area_val.setRatio(ratio);
}

void dbTechLayerAntennaRule::setPSR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->_par_sidearea_val.setRatio(ratio);
}

void dbTechLayerAntennaRule::setCSR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->_cum_sidearea_val.setRatio(ratio);
}

void dbTechLayerAntennaRule::setDiffPAR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->_par_area_val.setDiff(ratio);
}

void dbTechLayerAntennaRule::setDiffCAR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->_cum_area_val.setDiff(ratio);
}

void dbTechLayerAntennaRule::setDiffPSR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->_par_sidearea_val.setDiff(ratio);
}

void dbTechLayerAntennaRule::setDiffCSR(double ratio)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->_cum_sidearea_val.setDiff(ratio);
}

void dbTechLayerAntennaRule::setDiffPAR(const vector<double>& diff_idx,
                                        const vector<double>& ratios)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->_par_area_val.setDiff(diff_idx, ratios);
}

void dbTechLayerAntennaRule::setDiffCAR(const vector<double>& diff_idx,
                                        const vector<double>& ratios)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->_cum_area_val.setDiff(diff_idx, ratios);
}

void dbTechLayerAntennaRule::setDiffPSR(const vector<double>& diff_idx,
                                        const vector<double>& ratios)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->_par_sidearea_val.setDiff(diff_idx, ratios);
}

void dbTechLayerAntennaRule::setDiffCSR(const vector<double>& diff_idx,
                                        const vector<double>& ratios)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;

  ant_rule->_cum_sidearea_val.setDiff(diff_idx, ratios);
}

dbTechLayerAntennaRule* dbTechLayerAntennaRule::getAntennaRule(dbTech* _tech,
                                                               uint dbid)
{
  _dbTech* tech = (_dbTech*) _tech;
  return (dbTechLayerAntennaRule*) tech->_antenna_rule_tbl->getPtr(dbid);
}

bool dbTechLayerAntennaRule::hasAntennaCumRoutingPlusCut() const
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  return ant_rule->_has_antenna_cumroutingpluscut;
}

void dbTechLayerAntennaRule::setAntennaCumRoutingPlusCut(bool value)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  ant_rule->_has_antenna_cumroutingpluscut = value;
}

double dbTechLayerAntennaRule::getGatePlusDiffFactor() const
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  return ant_rule->_gate_plus_diff_factor;
}

void dbTechLayerAntennaRule::setGatePlusDiffFactor(double factor)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  ant_rule->_gate_plus_diff_factor = factor;
}

double dbTechLayerAntennaRule::getAreaMinusDiffFactor() const
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  return ant_rule->_area_minus_diff_factor;
}

void dbTechLayerAntennaRule::setAreaMinusDiffFactor(double factor)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  ant_rule->_area_minus_diff_factor = factor;
}

dbTechLayerAntennaRule::pwl_pair dbTechLayerAntennaRule::getAreaDiffReduce()
    const
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  auto& rule = ant_rule->_area_diff_reduce_val;
  return pwl_pair{rule._diff_idx, rule._diff_ratio};
}

void dbTechLayerAntennaRule::setAreaDiffReduce(
    const std::vector<double>& areas,
    const std::vector<double>& factors)
{
  _dbTechLayerAntennaRule* ant_rule = (_dbTechLayerAntennaRule*) this;
  ant_rule->_area_diff_reduce_val.setDiff(areas, factors);
}

////////////////////////////////////////////////////////////////////
//
// _dbTechAntennaAreaElement - Methods
//
////////////////////////////////////////////////////////////////////

_dbTechAntennaAreaElement::_dbTechAntennaAreaElement(
    const _dbTechAntennaAreaElement& e)
    : _area(e._area), _lyidx(e._lyidx)
{
}

dbOStream& operator<<(dbOStream& stream, const _dbTechAntennaAreaElement* aae)
{
  stream << aae->_area;
  stream << aae->_lyidx;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechAntennaAreaElement*& aae)
{
  aae = new _dbTechAntennaAreaElement;
  stream >> aae->_area;
  stream >> aae->_lyidx;
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
  aae->_area = inarea;
  if (inly) {
    aae->_lyidx = inly->getId();
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
  fmt::print(writer.out(), "        {} {:g} ", header, _area);
  if (_lyidx.isValid()) {
    fmt::print(writer.out(),
               "LAYER {} ",
               dbTechLayer::getTechLayer(tech, _lyidx)->getName().c_str());
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
    : _mterm(m._mterm)
{
  dbVector<_dbTechAntennaAreaElement*>::const_iterator itr;

  for (itr = m._gate_area.begin(); itr != m._gate_area.end(); ++itr) {
    _dbTechAntennaAreaElement* e = new _dbTechAntennaAreaElement(*(*itr));
    _gate_area.push_back(e);
  }

  for (itr = m._max_area_car.begin(); itr != m._max_area_car.end(); ++itr) {
    _dbTechAntennaAreaElement* e = new _dbTechAntennaAreaElement(*(*itr));
    _max_area_car.push_back(e);
  }

  for (itr = m._max_sidearea_car.begin(); itr != m._max_sidearea_car.end();
       ++itr) {
    _dbTechAntennaAreaElement* e = new _dbTechAntennaAreaElement(*(*itr));
    _max_sidearea_car.push_back(e);
  }

  for (itr = m._max_cut_car.begin(); itr != m._max_cut_car.end(); ++itr) {
    _dbTechAntennaAreaElement* e = new _dbTechAntennaAreaElement(*(*itr));
    _max_cut_car.push_back(e);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbTechAntennaPinModel& inmod)
{
  stream << inmod._mterm;
  stream << inmod._gate_area;
  stream << inmod._max_area_car;
  stream << inmod._max_sidearea_car;
  stream << inmod._max_cut_car;

  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechAntennaPinModel& inmod)
{
  stream >> inmod._mterm;
  stream >> inmod._gate_area;
  stream >> inmod._max_area_car;
  stream >> inmod._max_sidearea_car;
  stream >> inmod._max_cut_car;
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
  _dbTechAntennaAreaElement::create(xmod->_gate_area, inval, refly);
}

void dbTechAntennaPinModel::addMaxAreaCAREntry(double inval, dbTechLayer* refly)
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  _dbTechAntennaAreaElement::create(xmod->_max_area_car, inval, refly);
}

void dbTechAntennaPinModel::addMaxSideAreaCAREntry(double inval,
                                                   dbTechLayer* refly)
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  _dbTechAntennaAreaElement::create(xmod->_max_sidearea_car, inval, refly);
}

void dbTechAntennaPinModel::addMaxCutCAREntry(double inval, dbTechLayer* refly)
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  _dbTechAntennaAreaElement::create(xmod->_max_cut_car, inval, refly);
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
      layer = (dbTechLayer*) tech->_layer_tbl->getPtr(layerId);
    }
    result.emplace_back(elem->getArea(), layer);
  }
}

void dbTechAntennaPinModel::getGateArea(
    std::vector<std::pair<double, dbTechLayer*>>& data)
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  xmod->getAntennaValues(getImpl()->getDatabase(), xmod->_gate_area, data);
}

void dbTechAntennaPinModel::getMaxAreaCAR(
    std::vector<std::pair<double, dbTechLayer*>>& data)
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  xmod->getAntennaValues(getImpl()->getDatabase(), xmod->_max_area_car, data);
}

void dbTechAntennaPinModel::getMaxSideAreaCAR(
    std::vector<std::pair<double, dbTechLayer*>>& data)
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  xmod->getAntennaValues(
      getImpl()->getDatabase(), xmod->_max_sidearea_car, data);
}

void dbTechAntennaPinModel::getMaxCutCAR(
    std::vector<std::pair<double, dbTechLayer*>>& data)
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  xmod->getAntennaValues(getImpl()->getDatabase(), xmod->_max_cut_car, data);
}

void dbTechAntennaPinModel::writeLef(dbTech* tech, lefout& writer) const
{
  _dbTechAntennaPinModel* xmod = (_dbTechAntennaPinModel*) this;
  dbVector<_dbTechAntennaAreaElement*>::iterator ant_iter;

  for (ant_iter = xmod->_gate_area.begin(); ant_iter != xmod->_gate_area.end();
       ant_iter++) {
    (*ant_iter)->writeLef("ANTENNAGATEAREA", tech, writer);
  }

  for (ant_iter = xmod->_max_area_car.begin();
       ant_iter != xmod->_max_area_car.end();
       ant_iter++) {
    (*ant_iter)->writeLef("ANTENNAMAXAREACAR", tech, writer);
  }

  for (ant_iter = xmod->_max_sidearea_car.begin();
       ant_iter != xmod->_max_sidearea_car.end();
       ant_iter++) {
    (*ant_iter)->writeLef("ANTENNAMAXSIDEAREACAR", tech, writer);
  }

  for (ant_iter = xmod->_max_cut_car.begin();
       ant_iter != xmod->_max_cut_car.end();
       ant_iter++) {
    (*ant_iter)->writeLef("ANTENNAMAXCUTCAR", tech, writer);
  }
}

dbTechAntennaPinModel* dbTechAntennaPinModel::getAntennaPinModel(
    dbMaster* _master,
    uint dbid)
{
  _dbMaster* master = (_dbMaster*) _master;
  return (dbTechAntennaPinModel*) master->_antenna_pin_model_tbl->getPtr(dbid);
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

  info.children_["_gate_area"].add(_gate_area);
  info.children_["_gate_area"].size
      += _gate_area.size() * sizeof(_dbTechAntennaAreaElement);
  info.children_["_max_area_car"].add(_max_area_car);
  info.children_["_max_area_car"].size
      += _max_area_car.size() * sizeof(_dbTechAntennaAreaElement);
  info.children_["_max_sidearea_car"].add(_max_sidearea_car);
  info.children_["_max_sidearea_car"].size
      += _max_sidearea_car.size() * sizeof(_dbTechAntennaAreaElement);
  info.children_["_max_cut_car"].add(_max_cut_car);
  info.children_["_max_cut_car"].size
      += _max_cut_car.size() * sizeof(_dbTechAntennaAreaElement);
}

}  // namespace odb
