// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once
#include <unordered_set>
#include <vector>

#include "boost/functional/hash.hpp"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/FuncExpr.hh"
#include "sta/MinMax.hh"
#include "sta/StaState.hh"
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

using odb::Point;
using utl::Logger;

using sta::Corner;
using sta::dbNetwork;
using sta::dbSta;
using sta::DcalcAnalysisPt;
using sta::Delay;
using sta::Instance;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::MinMax;
using sta::Net;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::RiseFall;
using sta::RiseFallBoth;
using sta::Slack;
using sta::Slew;
using sta::StaState;
using sta::TimingArc;
using sta::Vertex;

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
  void repairSetup(const Pin* end_pin);
  // For testing.
  void reportSwappablePins();
  // Rebuffer one net (for testing).
  // resizerPreamble() required.

 private:
  void init();
  bool repairPath(Path* path, Slack path_slack, float setup_slack_margin);
  int fanout(Vertex* vertex);
  bool hasTopLevelOutputPort(Net* net);

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
  void traverseFaninCone(Vertex* endpoint,
                         std::unordered_map<Instance*, float>& crit_insts,
                         std::unordered_set<Vertex*>& visited,
                         std::unordered_set<Instance*>& notSwappable,
                         const OptoParams& params);
  Slack getInstanceSlack(Instance* inst);

  Logger* logger_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  Resizer* resizer_;
  est::EstimateParasitics* estimate_parasitics_;

  bool fallback_ = false;
  float min_viol_ = 0.0;
  float max_viol_ = 0.0;
  int max_repairs_per_pass_ = 1;
  int removed_buffer_count_ = 0;
  double initial_design_area_ = 0;

  std::vector<BaseMove*> move_sequence_;

  const MinMax* min_ = MinMax::min();
  const MinMax* max_ = MinMax::max();

  sta::UnorderedMap<LibertyPort*, sta::LibertyPortSet> equiv_pin_map_;

  static constexpr int decreasing_slack_max_passes_ = 50;
  static constexpr int print_interval_ = 10;
  static constexpr int opto_small_interval_ = 100;
  static constexpr int opto_large_interval_ = 1000;
  static constexpr float inc_fix_rate_threshold_
      = 0.0001;  // default fix rate threshold = 0.01%
  static constexpr int max_last_gasp_passes_ = 10;
};

}  // namespace rsz
