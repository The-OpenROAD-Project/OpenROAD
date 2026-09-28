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

  void setTargetOverflow(float overflow) { npVars_.targetOverflow = overflow; }
  void setMaxIters(int limit) { npVars_.maxNesterovIter = limit; }

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
  bool isDiverged(float& curA);
  void routabilitySnapshot(int iter,
                           float curA,
                           const std::string& routability_driven_dir,
                           int routability_driven_count,
                           int timing_driven_count);
  void runRoutability(int iter,
                      int timing_driven_count,
                      const std::string& routability_driven_dir,
                      int& routability_driven_count,
                      float& curA);
  // Recover from a divergence by rolling back to the routability snapshot with
  // a different inflation in place. Returns false when no attempt is left or
  // there is no routability snapshot to go to.
  bool tryRoutabilityDivergeRecovery(float& curA);
  // True when the per-iteration cell displacement has come off its peak in
  // every region, i.e. the placement is close to where it is going.
  bool isPlacementSettled() const;
  float getWorstSettleRatio() const;

  bool isConverged(int gpl_iter_count, int routability_gpl_iter_count);
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
  bool is_diverge_snapshot_saved_ = false;
  float diverge_snapshot_wl_coef_x_ = 0;
  float diverge_snapshot_wl_coef_y_ = 0;
  // min_hpwl_ as it stood when the snapshot was taken. Kept separately because
  // a routability pass restarts the search for a minimum on the newly inflated
  // design, which clears min_hpwl_ while the snapshot it produced is still the
  // best thing to fall back on.
  int64_t diverge_snapshot_hpwl_ = 0;

  // A revert resumes placement from the snapshot instead of ending the run, so
  // a design that keeps diverging would otherwise bounce off the same snapshot
  // until the iteration budget runs out and report a max-iteration warning
  // rather than the divergence that actually happened. Past this many reverts
  // the run is treated as genuinely divergent and raises GPL-0307.
  //
  // Reverting is deterministic: the same snapshot, the same cell sizes and the
  // same momentum reset replay the same trajectory and diverge in the same
  // place. What makes a later attempt worth running is that the snapshot moves
  // - the resumed descent records a new minimum HPWL before it diverges again.
  static constexpr int kMaxDivergeReverts = 3;
  int diverge_revert_count_ = 0;

  // Snapshot saving for routability
  bool is_routability_snapshot_saved_ = false;
  float route_snapshot_a_ = 0;
  float route_snapshot_wl_coef_x_ = 0;
  float route_snapshot_wl_coef_y_ = 0;

  // Divergence recovery attempts that roll back to the routability snapshot.
  // Each one puts a different inflation in place before resuming, so the count
  // is bounded by how many distinct states there are to try: attempt 1 goes
  // back to the least congested inflation and lets routability carry on,
  // attempt 2 does the same but stops it inflating any further. A third would
  // only repeat the second.
  static constexpr int kMaxRoutabilityDivergeAttempts = 2;
  int routability_diverge_attempt_count_ = 0;

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
  int routability_settle_wait_start_iter_ = -1;

  std::string divergeMsg_;
  int divergeCode_ = 0;

  int recursionCntWlCoef_ = 0;
  int recursionCntInitSLPCoef_ = 0;

  int placement_gif_key_ = -1;
  int routability_gif_key_ = -1;

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
