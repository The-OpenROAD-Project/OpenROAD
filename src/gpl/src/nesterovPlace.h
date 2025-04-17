// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "nesterovBase.h"
#include "odb/dbBlockCallBackObj.h"
#include "point.h"
#include "utl/prometheus/gauge.h"

namespace utl {
class Logger;
}

namespace odb {
class dbInst;
}

namespace gpl {

class PlacerBase;
class PlacerBaseCommon;
class Instance;
class RouteBase;
class TimingBase;
class Graphics;

class NesterovPlace
{
 public:
  NesterovPlace();
  NesterovPlace(const NesterovPlaceVars& npVars,
                const std::shared_ptr<PlacerBaseCommon>& pbc,
                const std::shared_ptr<NesterovBaseCommon>& nbc,
                std::vector<std::shared_ptr<PlacerBase>>& pbVec,
                std::vector<std::shared_ptr<NesterovBase>>& nbVec,
                std::shared_ptr<RouteBase> rb,
                std::shared_ptr<TimingBase> tb,
                utl::Logger* log);
  ~NesterovPlace();

  // return iteration count
  int doNesterovPlace(int start_iter = 0);

  void updateWireLengthCoef(float overflow);

  void updateNextIter(int iter);

  void updateDb();

  void checkInvalidValues(float wireLengthGradSum, float densityGradSum);

  float getWireLengthCoefX() const { return wireLengthCoefX_; }
  float getWireLengthCoefY() const { return wireLengthCoefY_; }

  void setTargetOverflow(float overflow) { npVars_.targetOverflow = overflow; }
  void setMaxIters(int limit) { npVars_.maxNesterovIter = limit; }

  void updatePrevGradient(const std::shared_ptr<NesterovBase>& nb);
  void updateCurGradient(const std::shared_ptr<NesterovBase>& nb);
  void updateNextGradient(const std::shared_ptr<NesterovBase>& nb);

  void resizeGCell(odb::dbInst*);
  void moveGCell(odb::dbInst*);

  void createGCell(odb::dbInst*);
  void createGNet(odb::dbNet*);
  void createITerm(odb::dbITerm*);

  void destroyGCell(odb::dbInst*);
  void destroyGNet(odb::dbNet*);
  void destroyITerm(odb::dbITerm*);

 private:
  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::vector<std::shared_ptr<PlacerBase>> pbVec_;
  std::vector<std::shared_ptr<NesterovBase>> nbVec_;
  utl::Logger* log_ = nullptr;
  std::shared_ptr<RouteBase> rb_;
  std::shared_ptr<TimingBase> tb_;
  NesterovPlaceVars npVars_;
  std::unique_ptr<Graphics> graphics_;

  float total_sum_overflow_ = 0;
  float total_sum_overflow_unscaled_ = 0;
  // The average here is between regions (NB objects)
  float average_overflow_ = 0;
  float average_overflow_unscaled_ = 0;

  // Snapshot saving for revert if diverge
  float diverge_snapshot_average_overflow_unscaled_ = 0;
  int64_t min_hpwl_ = INT64_MAX;
  int diverge_snapshot_iter_ = 0;
  bool is_min_hpwl_ = false;

  // densityPenalty stor
  std::vector<float> densityPenaltyStor_;

  // base_wcof
  float baseWireLengthCoef_ = 0;

  // wlen_cof
  float wireLengthCoefX_ = 0;
  float wireLengthCoefY_ = 0;

  // observability metrics
  utl::Gauge<double>* hpwl_gauge_;

  // half-parameter-wire-length
  int64_t prevHpwl_ = 0;

  int num_region_diverged_ = 0;
  bool is_routability_need_ = true;
  float routability_save_snapshot_ = 0.6;

  std::string divergeMsg_;
  int divergeCode_ = 0;

  int recursionCntWlCoef_ = 0;
  int recursionCntInitSLPCoef_ = 0;

  void cutFillerCoordinates();

  void init();
  void reset();

  std::unique_ptr<nesterovDbCbk> db_cbk_;
};

class nesterovDbCbk : public odb::dbBlockCallBackObj
{
 public:
  nesterovDbCbk(NesterovPlace* nesterov_place_);

  void inDbInstCreate(odb::dbInst*) override;
  void inDbInstCreate(odb::dbInst*, odb::dbRegion*) override;
  void inDbInstDestroy(odb::dbInst*) override;

  void inDbITermCreate(odb::dbITerm*) override;
  void inDbITermDestroy(odb::dbITerm*) override;

  void inDbNetCreate(odb::dbNet*) override;
  void inDbNetDestroy(odb::dbNet*) override;

  void inDbInstSwapMasterAfter(odb::dbInst*) override;
  void inDbPostMoveInst(odb::dbInst*) override;

 private:
  NesterovPlace* nesterov_place_;
};

}  // namespace gpl
