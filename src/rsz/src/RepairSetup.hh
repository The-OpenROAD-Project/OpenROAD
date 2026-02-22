// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "boost/functional/hash.hpp"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "utl/Logger.h"

namespace sta {
class PathExpanded;
}

namespace est {
class EstimateParasitics;
}

namespace rsz {

class Resizer;
class RemoveBuffer;
class BaseMove;

struct OptoParams
{
  int iteration;
  float initial_tns;
  const float setup_slack_margin;
  const bool verbose;
  const bool skip_pin_swap;
  const bool skip_gate_cloning;
  const bool skip_size_down;
  const bool skip_buffering;
  const bool skip_buffer_removal;
  const bool skip_vt_swap;

  OptoParams(const float margin,
             const bool verbose,
             const bool skip_pin_swap,
             const bool skip_gate_cloning,
             const bool skip_size_down,
             const bool skip_buffering,
             const bool skip_buffer_removal,
             const bool skip_vt_swap)
      : setup_slack_margin(margin),
        verbose(verbose),
        skip_pin_swap(skip_pin_swap),
        skip_gate_cloning(skip_gate_cloning),
        skip_size_down(skip_size_down),
        skip_buffering(skip_buffering),
        skip_buffer_removal(skip_buffer_removal),
        skip_vt_swap(skip_vt_swap)
  {
    iteration = 0;
    initial_tns = 0.0;
  }
};

class RepairSetup : public sta::dbStaState
{
 public:
  RepairSetup(Resizer* resizer);
  bool repairSetup(float setup_slack_margin,
                   // Percent of violating ends to repair to
                   // reduce tns (0.0-1.0).
                   double repair_tns_end_percent,
                   int max_passes,
                   int max_iterations,
                   int max_repairs_per_pass,
                   bool verbose,
                   const std::vector<MoveType>& sequence,
                   bool skip_pin_swap,
                   bool skip_gate_cloning,
                   bool skip_size_down,
                   bool skip_buffering,
                   bool skip_buffer_removal,
                   bool skip_last_gasp,
                   bool skip_vt_swap,
                   bool skip_crit_vt_swap);
  // For testing.
  void repairSetup(const sta::Pin* end_pin);
  // For testing.
  void reportSwappablePins();
  // Rebuffer one net (for testing).
  // resizerPreamble() required.

 private:
  void init();
  bool repairPath(sta::Path* path,
                  sta::Slack path_slack,
                  float setup_slack_margin);
  int fanout(sta::Vertex* vertex);
  bool hasTopLevelOutputPort(sta::Net* net);

  void printProgress(int iteration,
                     bool force,
                     bool end,
                     bool last_gasp,
                     int num_viols) const;
  bool terminateProgress(int iteration,
                         float initial_tns,
                         float& prev_tns,
                         float& fix_rate_threshold,
                         int endpt_index,
                         int num_endpts);
  void repairSetupLastGasp(const OptoParams& params,
                           int& num_viols,
                           int max_iterations);
  bool swapVTCritCells(const OptoParams& params, int& num_viols);
  void traverseFaninCone(sta::Vertex* endpoint,
                         std::unordered_map<sta::Instance*, float>& crit_insts,
                         std::unordered_set<sta::Vertex*>& visited,
                         std::unordered_set<sta::Instance*>& notSwappable,
                         const OptoParams& params);
  sta::Slack getInstanceSlack(sta::Instance* inst);

  utl::Logger* logger_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;
  Resizer* resizer_;
  est::EstimateParasitics* estimate_parasitics_;

  bool fallback_ = false;
  float min_viol_ = 0.0;
  float max_viol_ = 0.0;
  int max_repairs_per_pass_ = 1;
  int removed_buffer_count_ = 0;
  double initial_design_area_ = 0;

  std::vector<BaseMove*> move_sequence_;

  const sta::MinMax* min_ = sta::MinMax::min();
  const sta::MinMax* max_ = sta::MinMax::max();

  std::unordered_map<sta::LibertyPort*, sta::LibertyPortSet> equiv_pin_map_;

  static constexpr int decreasing_slack_max_passes_ = 50;
  static constexpr int print_interval_ = 10;
  static constexpr int opto_small_interval_ = 100;
  static constexpr int opto_large_interval_ = 1000;
  static constexpr float inc_fix_rate_threshold_
      = 0.0001;  // default fix rate threshold = 0.01%
  static constexpr int max_last_gasp_passes_ = 10;
};

}  // namespace rsz
