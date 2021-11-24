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

#pragma once

#include "dbCore.h"
#include "dbTechLayer.h"
#include "dbTypes.h"
#include "odb.h"

namespace odb {

class _dbDatabase;
class _dbMTerm;
class _dbTechLayer;
class dbTech;
class dbTechLayer;
class dbIStream;
class dbOStream;
class dbDiff;
class lefout;

//
// An antenna multiplier factor is applied to metal. A separate factor may
// be used for diffusion connected metal.
//
class _ARuleFactor
{
 public:
  double _factor;
  bool _explicit;
  bool _diff_use_only;

  _ARuleFactor();
  void setFactor(double factor, bool diffuse);
  bool operator==(const _ARuleFactor& rhs) const;
  bool operator!=(const _ARuleFactor& rhs) const { return !operator==(rhs); }
  void differences(dbDiff& diff,
                   const char* field,
                   const _ARuleFactor& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

inline _ARuleFactor::_ARuleFactor()
{
  _factor = 1.0;
  _explicit = false;
  _diff_use_only = false;
}

dbOStream& operator<<(dbOStream& stream, const _ARuleFactor& arf);
dbIStream& operator>>(dbIStream& stream, _ARuleFactor& arf);

//
// An antenna rule ratio is a single ratio for non-diffusion connected segments
// or a piecewise linear function of diffusion area for diffusion connected
// segments.
//
class _ARuleRatio
{
 public:
  double _ratio;
  dbVector<double> _diff_idx;
  dbVector<double> _diff_ratio;

  _ARuleRatio();

  void setRatio(double ratio);
  void setDiff(const std::vector<double>& diff_idx,
               const std::vector<double>& ratios);
  void setDiff(double ratio);  // single value stored as PWL

  bool operator==(const _ARuleRatio& rhs) const;
  bool operator!=(const _ARuleRatio& rhs) const { return !operator==(rhs); }
  void differences(dbDiff& diff,
                   const char* field,
                   const _ARuleRatio& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

inline _ARuleRatio::_ARuleRatio() : _ratio(0.0)
{
}

dbOStream& operator<<(dbOStream& stream, const _ARuleRatio& arrt);
dbIStream& operator>>(dbIStream& stream, _ARuleRatio& arrt);

///  An antenna rule comprises a multiplier factor for area and sidearea
///  (perimeter), as well as ratios for the area and sidearea for both
///  a partial (single layer based) and cumulative (all layer) models.

class _dbTechLayerAntennaRule : public _dbObject
{
 public:
  dbId<_dbTechLayer> _layer;
  _ARuleFactor _area_mult;
  _ARuleFactor _sidearea_mult;
  _ARuleRatio _par_area_val;
  _ARuleRatio _cum_area_val;
  _ARuleRatio _par_sidearea_val;
  _ARuleRatio _cum_sidearea_val;
  _ARuleRatio _area_diff_reduce_val;
  double _gate_plus_diff_factor;
  double _area_minus_diff_factor;
  bool _has_antenna_cumroutingpluscut;

  _dbTechLayerAntennaRule(_dbDatabase*) {}
  _dbTechLayerAntennaRule(_dbDatabase*, const _dbTechLayerAntennaRule& r)
      : _layer(r._layer),
        _area_mult(r._area_mult),
        _sidearea_mult(r._sidearea_mult),
        _par_area_val(r._par_area_val),
        _cum_area_val(r._cum_area_val),
        _par_sidearea_val(r._par_sidearea_val),
        _cum_sidearea_val(r._cum_sidearea_val),
        _area_diff_reduce_val(r._area_diff_reduce_val),
        _gate_plus_diff_factor(r._gate_plus_diff_factor),
        _area_minus_diff_factor(r._area_minus_diff_factor)
  {
  }

  ~_dbTechLayerAntennaRule() {}
  bool operator==(const _dbTechLayerAntennaRule& rhs) const;
  bool operator!=(const _dbTechLayerAntennaRule& rhs) const
  {
    return !operator==(rhs);
  }
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbTechLayerAntennaRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerAntennaRule& inrule);
dbIStream& operator>>(dbIStream& stream, _dbTechLayerAntennaRule& inrule);

//
// An antenna area element comprises an area value and an optional layer.
// It stores area in squm.
//
class _dbTechAntennaAreaElement
{
 public:
  ~_dbTechAntennaAreaElement(){};

  static void create(dbVector<_dbTechAntennaAreaElement*>& incon,
                     double inarea,
                     dbTechLayer* inly
                     = NULL);  // Allocate a new element and add to container.
  void writeLef(const char* header, dbTech* tech, lefout& writer) const;

  friend dbOStream& operator<<(dbOStream& stream,
                               const _dbTechAntennaAreaElement* aae);
  friend dbIStream& operator>>(dbIStream& stream,
                               _dbTechAntennaAreaElement*& aae);

  _dbTechAntennaAreaElement(const _dbTechAntennaAreaElement& e);

  bool operator==(const _dbTechAntennaAreaElement& rhs) const;
  bool operator!=(const _dbTechAntennaAreaElement& rhs) const
  {
    return !operator==(rhs);
  }
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbTechAntennaAreaElement& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  double getArea() const { return _area; }
  dbId<_dbTechLayer> getLayerId() const { return _lyidx; }

 private:
  _dbTechAntennaAreaElement();
  double _area;
  dbId<_dbTechLayer> _lyidx;
};

//
// An antenna pin model stores the model specific antenna info for a pin.
//
class _dbTechAntennaPinModel : public _dbObject
{
 public:
  dbId<_dbMTerm> _mterm;
  dbVector<_dbTechAntennaAreaElement*> _gate_area;
  dbVector<_dbTechAntennaAreaElement*> _max_area_car;
  dbVector<_dbTechAntennaAreaElement*> _max_sidearea_car;
  dbVector<_dbTechAntennaAreaElement*> _max_cut_car;

  _dbTechAntennaPinModel(_dbDatabase*, const _dbTechAntennaPinModel& m);
  _dbTechAntennaPinModel(_dbDatabase*) {}
  ~_dbTechAntennaPinModel() {}

  bool operator==(const _dbTechAntennaPinModel& rhs) const;
  bool operator!=(const _dbTechAntennaPinModel& rhs) const
  {
    return !operator==(rhs);
  }
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbTechAntennaPinModel& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  static void getAntennaValues(
      _dbDatabase* db,
      const dbVector<_dbTechAntennaAreaElement*>& elements,
      std::vector<std::pair<double, dbTechLayer*>>& result);
};

dbOStream& operator<<(dbOStream& stream, const _dbTechAntennaPinModel& inmod);
dbIStream& operator>>(dbIStream& stream, _dbTechAntennaPinModel& inmod);

}  // namespace odb
