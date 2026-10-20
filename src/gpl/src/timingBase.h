// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace est {
class EstimateParasitics;
}

namespace odb {
class dbBlock;
class dbInst;
}  // namespace odb

namespace sta {
class dbSta;
}

namespace grt {
class GlobalRouter;
}

namespace rsz {
class Resizer;
}

namespace utl {
class Logger;
}

namespace gpl {

class NesterovBaseCommon;
class GNet;

class TimingBase
{
 public:
  TimingBase();
  TimingBase(std::shared_ptr<NesterovBaseCommon> nbc,
             grt::GlobalRouter* grt,
             rsz::Resizer* rs,
             utl::Logger* log);

  // check whether overflow reached the timingOverflow
  bool isTimingNetWeightOverflow(float overflow);
  void addTimingNetWeightOverflow(int overflow);
  void setTimingNetWeightOverflows(const std::vector<int>& overflows);
  void deleteTimingNetWeightOverflow(int overflow);
  void clearTimingNetWeightOverflow();
  size_t getTimingNetWeightOverflowSize() const;

  void setTimingNetWeightMax(float max);
  void setTimingNetsPercentage(float percentage);
  void setRepairTiming(bool run_repair_timing)
  {
    repair_timing_ = run_repair_timing;
  }
  void setRepairTnsEndPercent(float percent)
  {
    repair_tns_end_percent_ = percent / 100.0f;
  }

  // updateNetWeight.
  // True: successfully reweighted gnets
  // False: no slacks found
  bool executeTimingDriven(bool run_journal_restore,
                           bool enable_repair_timing = false);

  // Incremental placement must not change the existing buffer tree or sizes.
  void setIncrementalMode(sta::dbSta* sta) { incremental_sta_ = sta; }
  bool isIncrementalMode() const { return incremental_sta_ != nullptr; }

 private:
  grt::GlobalRouter* grt_ = nullptr;
  rsz::Resizer* rs_ = nullptr;
  utl::Logger* log_ = nullptr;
  std::shared_ptr<NesterovBaseCommon> nbc_;

  std::vector<int> timingNetWeightOverflow_;
  std::vector<int> timingOverflowChk_;
  float net_weight_max_ = 5;
  float nets_percentage_ = 10;
  bool repair_timing_ = false;
  float repair_tns_end_percent_ = 0.01;
  void initTimingOverflowChk();
  bool executeIncrementalTimingDriven();
  sta::dbSta* incremental_sta_ = nullptr;
};

struct IncrementalTimingMetrics
{
  float worst_slack;
  float total_negative_slack;
  bool isValid() const;
  bool degrades(const IncrementalTimingMetrics& baseline) const;
};

// Check the optional unlocked pass against the mandatory locked placement.
// Both measurements use placement parasitics, not potentially stale routes.
// These are design-wide setup metrics, not per-clock or per-endpoint bounds.
class IncrementalTimingGuard
{
 public:
  // sta is initialized and non-null. Once timing is usable, parasitics and
  // block must be non-null and belong to that live design. Missing Liberty
  // disables the guard before consulting those dependencies.
  IncrementalTimingGuard(sta::dbSta* sta,
                         est::EstimateParasitics* parasitics,
                         odb::dbBlock* block);
  bool checkpoint();
  bool restoreIfWorse();

 private:
  struct Location
  {
    odb::dbInst* inst;
    int x;
    int y;
  };
  IncrementalTimingMetrics measure();
  sta::dbSta* sta_;
  est::EstimateParasitics* parasitics_;
  odb::dbBlock* block_;
  std::vector<Location> locations_;
  IncrementalTimingMetrics baseline_{};
  bool active_ = false;
};

// Bottom of the timing weight ramp, as a percentage of all timing-constrained
// nets, to keep one outlier from rescaling every other net. Relative to the
// nets considered during timing-driven mode this is 2% at the default
// -timing_driven_nets_percentage of 10%, and 0.2% at 100%. Designs with too
// few constrained nets for 0.2% to reach one net anchor on the worst net.
inline constexpr double kSlackMinPercent = 0.2;

// Index into the worst-slack net list, which holds the worst nets_percentage %
// of the constrained nets sorted by increasing slack. net_count must be > 0.
size_t slackRampAnchorIndex(size_t net_count, float nets_percentage);

// net_weight_max at or below slack_min, 1.0 at or above slack_max, linear in
// between.
float computeTimingNetWeight(float net_slack,
                             float slack_min,
                             float slack_max,
                             float net_weight_max);

}  // namespace gpl
