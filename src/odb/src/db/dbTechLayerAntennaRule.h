// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <utility>
#include <vector>

#include "dbCore.h"
#include "dbTechLayer.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"

namespace odb {

class _dbDatabase;
class _dbMTerm;
class _dbTechLayer;
class dbTech;
class dbTechLayer;
class dbIStream;
class dbOStream;
class lefout;

//
// An antenna multiplier factor is applied to metal. A separate factor may
// be used for diffusion connected metal.
//
class ARuleFactor
{
 public:
  ARuleFactor();
  void setFactor(double factor, bool diffuse);
  bool operator==(const ARuleFactor& rhs) const;
  bool operator!=(const ARuleFactor& rhs) const { return !operator==(rhs); }

  double factor_;
  bool explicit_;
  bool diff_use_only_;
};

inline ARuleFactor::ARuleFactor()
{
  factor_ = 1.0;
  explicit_ = false;
  diff_use_only_ = false;
}

dbOStream& operator<<(dbOStream& stream, const ARuleFactor& arf);
dbIStream& operator>>(dbIStream& stream, ARuleFactor& arf);

//
// An antenna rule ratio is a single ratio for non-diffusion connected segments
// or a piecewise linear function of diffusion area for diffusion connected
// segments.
//
class ARuleRatio
{
 public:
  void setRatio(double ratio);
  void setDiff(const std::vector<double>& diff_idx,
               const std::vector<double>& ratios);
  void setDiff(double ratio);  // single value stored as PWL

  bool operator==(const ARuleRatio& rhs) const;
  bool operator!=(const ARuleRatio& rhs) const { return !operator==(rhs); }

  double ratio_{0.0};
  dbVector<double> diff_idx_;
  dbVector<double> diff_ratio_;
};

dbOStream& operator<<(dbOStream& stream, const ARuleRatio& arrt);
dbIStream& operator>>(dbIStream& stream, ARuleRatio& arrt);

///  An antenna rule comprises a multiplier factor for area and sidearea
///  (perimeter), as well as ratios for the area and sidearea for both
///  a partial (single layer based) and cumulative (all layer) models.

class _dbTechLayerAntennaRule : public _dbObject
{
 public:
  _dbTechLayerAntennaRule(_dbDatabase*)
      : gate_plus_diff_factor_(0),
        area_minus_diff_factor_(0),
        has_antenna_cumroutingpluscut_(false)
  {
  }
  _dbTechLayerAntennaRule(_dbDatabase*, const _dbTechLayerAntennaRule& r)
      : layer_(r.layer_),
        area_mult_(r.area_mult_),
        sidearea_mult_(r.sidearea_mult_),
        par_area_val_(r.par_area_val_),
        cum_area_val_(r.cum_area_val_),
        par_sidearea_val_(r.par_sidearea_val_),
        cum_sidearea_val_(r.cum_sidearea_val_),
        area_diff_reduce_val_(r.area_diff_reduce_val_),
        gate_plus_diff_factor_(r.gate_plus_diff_factor_),
        area_minus_diff_factor_(r.area_minus_diff_factor_)
  {
  }

  bool operator==(const _dbTechLayerAntennaRule& rhs) const;
  bool operator!=(const _dbTechLayerAntennaRule& rhs) const
  {
    return !operator==(rhs);
  }
  void collectMemInfo(MemInfo& info);

  dbId<_dbTechLayer> layer_;
  ARuleFactor area_mult_;
  ARuleFactor sidearea_mult_;
  ARuleRatio par_area_val_;
  ARuleRatio cum_area_val_;
  ARuleRatio par_sidearea_val_;
  ARuleRatio cum_sidearea_val_;
  ARuleRatio area_diff_reduce_val_;
  double gate_plus_diff_factor_;
  double area_minus_diff_factor_;
  bool has_antenna_cumroutingpluscut_;
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
  static void create(
      dbVector<_dbTechAntennaAreaElement*>& incon,
      double inarea,
      dbTechLayer* inly
      = nullptr);  // Allocate a new element and add to container.
  void writeLef(const char* header, dbTech* tech, lefout& writer) const;

  friend dbOStream& operator<<(dbOStream& stream,
                               const _dbTechAntennaAreaElement* aae);
  friend dbIStream& operator>>(dbIStream& stream,
                               _dbTechAntennaAreaElement*& aae);

  _dbTechAntennaAreaElement(const _dbTechAntennaAreaElement& e) = default;

  bool operator==(const _dbTechAntennaAreaElement& rhs) const;
  bool operator!=(const _dbTechAntennaAreaElement& rhs) const
  {
    return !operator==(rhs);
  }

  double getArea() const { return area_; }
  dbId<_dbTechLayer> getLayerId() const { return lyidx_; }

 private:
  _dbTechAntennaAreaElement() = default;
  double area_{-1.0};
  dbId<_dbTechLayer> lyidx_;
};

//
// An antenna pin model stores the model specific antenna info for a pin.
//
class _dbTechAntennaPinModel : public _dbObject
{
 public:
  _dbTechAntennaPinModel(_dbDatabase*, const _dbTechAntennaPinModel& m);
  _dbTechAntennaPinModel(_dbDatabase*) {}
  ~_dbTechAntennaPinModel();

  bool operator==(const _dbTechAntennaPinModel& rhs) const;
  bool operator!=(const _dbTechAntennaPinModel& rhs) const
  {
    return !operator==(rhs);
  }
  void collectMemInfo(MemInfo& info);

  static void getAntennaValues(
      _dbDatabase* db,
      const dbVector<_dbTechAntennaAreaElement*>& elements,
      std::vector<std::pair<double, dbTechLayer*>>& result);

  dbId<_dbMTerm> mterm_;
  dbVector<_dbTechAntennaAreaElement*> gate_area_;
  dbVector<_dbTechAntennaAreaElement*> max_area_car_;
  dbVector<_dbTechAntennaAreaElement*> max_sidearea_car_;
  dbVector<_dbTechAntennaAreaElement*> max_cut_car_;
};

dbOStream& operator<<(dbOStream& stream, const _dbTechAntennaPinModel& inmod);
dbIStream& operator>>(dbIStream& stream, _dbTechAntennaPinModel& inmod);

}  // namespace odb
