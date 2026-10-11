// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "AbstractGraphics.h"
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
class ClockBase;

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
                std::shared_ptr<ClockBase> cb,
                std::unique_ptr<AbstractGraphics> graphics,
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
  NesterovPlaceVars& getNpVars() { return npVars_; }
  float getAverageOverflow() const { return average_overflow_unscaled_; }

  void setTargetOverflow(float overflow) { npVars_.targetOverflow = overflow; }
  void setMaxIters(int limit) { npVars_.maxNesterovIter = limit; }
  // Enables the incremental density-penalty guard for the next
  // doNesterovPlace() call; consumed (cleared) at the start of that call.
  void enableIncrementalDensityPenaltyGuard();
  // Clears a divergence left over from a previous doNesterovPlace() call
  // (e.g. one the caller intends to retry from). Without this, the leftover
  // state trips the divergence check at the very start of the next
  // doNesterovPlace() call before it does any work.
  void clearDivergence();
  // When true, a divergence is reported as a warning and doNesterovPlace()
  // returns normally (check divergedLastRun()) instead of logging an ERROR
  // and throwing. Defaults to false, so a plain (non-incremental) placement
  // run still fails loudly and immediately on divergence, as before. The
  // caller is responsible for toggling this back off once the recoverable
  // window has passed (e.g. incremental placement's phase 2 has no further
  // fallback, so it should not set this).
  void setAllowDivergenceRecovery(bool allow)
  {
    allow_divergence_recovery_ = allow;
  }
  bool divergedLastRun() const { return num_region_diverged_ > 0; }

  void npUpdatePrevGradient(const std::shared_ptr<NesterovBase>& nb);
  void npUpdateCurGradient(const std::shared_ptr<NesterovBase>& nb);
  void npUpdateNextGradient(const std::shared_ptr<NesterovBase>& nb);

  void resizeGCell(odb::dbInst*);
  void moveGCell(odb::dbInst*);

  void createCbkGCell(odb::dbInst*);
  void createGNet(odb::dbNet*);
  void createCbkITerm(odb::dbITerm*);

  void destroyCbkGCell(odb::dbInst*);
  void destroyCbkGNet(odb::dbNet*);
  void destroyCbkITerm(odb::dbITerm*);

 private:
  void updateIterGraphics(int iter,
                          const std::string& reports_dir,
                          const std::string& routability_driven_dir,
                          int routability_driven_count,
                          int timing_driven_count,
                          bool& final_routability_image_saved);
  void runTimingDriven(int iter,
                       const std::string& timing_driven_dir,
                       int routability_driven_count,
                       int& timing_driven_count,
                       int64_t& td_accumulated_delta_area,
                       bool is_routability_gpl_iter,
                       int& virtual_cts_count);
  bool isDiverged(float& diverge_snapshot_WlCoefX,
                  float& diverge_snapshot_WlCoefY,
                  bool& is_diverge_snapshot_saved);
  void routabilitySnapshot(int iter,
                           float curA,
                           const std::string& routability_driven_dir,
                           int routability_driven_count,
                           int timing_driven_count,
                           bool& is_routability_snapshot_saved,
                           float& route_snapshot_WlCoefX,
                           float& route_snapshot_WlCoefY,
                           float& route_snapshotA);
  void runRoutability(int iter,
                      int timing_driven_count,
                      const std::string& routability_driven_dir,
                      float route_snapshotA,
                      float route_snapshot_WlCoefX,
                      float route_snapshot_WlCoefY,
                      int& routability_driven_count,
                      float& curA);
  // True when the per-iteration cell displacement has come off its peak in
  // every region, i.e. the placement is close to where it is going.
  bool isPlacementSettled() const;

  bool isConverged(int gpl_iter_count, int routability_gpl_iter_count);
  // Re-derives densityPenalty_ for every region via
  // NesterovBase::updateDensityPenaltyFromRatio() - the same formula
  // NesterovBase::initDensity2() uses at true init, just re-triggered
  // mid-run. Leaves wireLengthCoefX_/Y_ untouched, unlike calling init()
  // again.
  void applyDensityPenaltyFactor(float factor);
  // Checked once per outer iteration while the incremental density-penalty
  // guard is enabled. On any regression past best_overflow, escalates
  // current_factor in place (no revert - see the .cpp for why) and keeps
  // running; otherwise a no-op. The escalation multiplier is fixed at the
  // best value found by a guard-parameter sweep (see the .cpp).
  void guardIncrementalDensityPenalty(float& current_factor,
                                      float& best_overflow,
                                      int& retries);
  // The top-level (unfenced/full-die) region is always nbVec_[0].
  NesterovBase* getTopLevelNB() const;
  std::string getReportsDir() const;
  void cleanReportsDirs(const std::string& timing_driven_dir,
                        const std::string& routability_driven_dir) const;
  void doBackTracking(float coeff);
  void reportResults(int nesterov_iter,
                     int64_t original_area,
                     int64_t td_accumulated_delta_area);

  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::vector<std::shared_ptr<PlacerBase>> pbVec_;
  std::vector<std::shared_ptr<NesterovBase>> nbVec_;
  utl::Logger* log_ = nullptr;
  std::shared_ptr<RouteBase> rb_;
  std::shared_ptr<TimingBase> tb_;
  std::shared_ptr<ClockBase> cb_;
  NesterovPlaceVars npVars_;
  std::unique_ptr<AbstractGraphics> graphics_;

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
  utl::Gauge<double>* hpwl_gauge_ = nullptr;

  // half-parameter-wire-length
  int64_t prevHpwl_ = 0;

  int num_region_diverged_ = 0;
  bool is_routability_need_ = true;
  bool allow_divergence_recovery_ = false;

  std::string divergeMsg_;
  int divergeCode_ = 0;

  int recursionCntWlCoef_ = 0;
  int recursionCntInitSLPCoef_ = 0;

  int placement_gif_key_ = -1;
  int routability_gif_key_ = -1;

  bool incremental_penalty_guard_requested_ = false;

  void init();
  void reset();

  std::unique_ptr<nesterovDbCbk> db_cbk_;
};

class nesterovDbCbk : public odb::dbBlockCallBackObj
{
 public:
  nesterovDbCbk(NesterovPlace* nesterov_place_);

  void inDbInstCreate(odb::dbInst*) override;
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
