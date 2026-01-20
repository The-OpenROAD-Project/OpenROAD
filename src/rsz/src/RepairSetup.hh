// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ViolatorCollector.hh"
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
class ViolatorCollector;
class MoveTracker;

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
  ~RepairSetup() override;
  void setupMoveSequence(const std::vector<MoveType>& sequence,
                         bool skip_pin_swap,
                         bool skip_gate_cloning,
                         bool skip_size_down,
                         bool skip_buffering,
                         bool skip_buffer_removal,
                         bool skip_vt_swap);
  bool repairSetup(float setup_slack_margin,
                   // Percent of violating ends to repair to
                   // reduce tns (0.0-1.0).
                   double repair_tns_end_percent,
                   int max_passes,
                   int max_iterations,
                   int max_repairs_per_pass,
                   bool verbose,
                   const std::vector<MoveType>& sequence,
                   const char* phases,
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
  bool repairPins(
      const std::vector<const Pin*>& pins,
      float setup_slack_margin,
      const std::map<const Pin*, std::set<BaseMove*>>* rejected_moves = nullptr,
      std::vector<std::pair<const Pin*, BaseMove*>>* chosen_moves = nullptr);
  int fanout(Vertex* vertex);
  bool hasTopLevelOutputPort(Net* net);

  void printProgress(int iteration,
                     bool force,
                     bool end,
                     char phase_marker = ' ',
                     bool use_startpoint_metrics = false) const;
  void printProgressHeader() const;
  void printProgressFooter() const;
  bool terminateProgress(int iteration,
                         float initial_tns,
                         float& prev_tns,
                         float& fix_rate_threshold,
                         int endpt_index,
                         int num_endpts,
                         const char* phase_name,
                         char phase_marker);
  bool swapVTCritCells(const OptoParams& params, int& num_viols);
  void traverseFaninCone(Vertex* endpoint,
                         std::unordered_map<Instance*, float>& crit_insts,
                         std::unordered_set<Vertex*>& visited,
                         std::unordered_set<Instance*>& notSwappable,
                         const OptoParams& params);
  Slack getInstanceSlack(Instance* inst);

  // Different repair setup methods
  void repairSetup_Legacy(float setup_slack_margin,
                          int max_passes,
                          int max_iterations,
                          int max_repairs_per_pass,
                          bool verbose,
                          int& opto_iteration,
                          float initial_tns,
                          float& prev_tns,
                          char phase_marker = 'L');
  void repairSetup_WNS(float setup_slack_margin,
                       int max_passes_per_endpoint,
                       int max_repairs_per_pass,
                       bool verbose,
                       int& opto_iteration,
                       float initial_tns,
                       float& prev_tns,
                       bool use_cone_collection
                       = false,  // true for WNS_CONE, false for WNS
                       char phase_marker = '1',  // Character for progress table
                       ViolatorSortType sort_type
                       = ViolatorSortType::SORT_BY_LOAD_DELAY);
  void repairSetup_TNS(float setup_slack_margin,
                       int max_passes_per_endpoint,
                       int max_repairs_per_pass,
                       bool verbose,
                       int& opto_iteration,
                       float initial_tns,
                       float& prev_tns,
                       char phase_marker = '*',  // Character for progress table
                       ViolatorSortType sort_type
                       = ViolatorSortType::SORT_BY_LOAD_DELAY);
  void repairSetup_EP_FI(float setup_slack_margin,
                         int max_passes_per_endpoint,
                         bool verbose,
                         int& opto_iteration,
                         char phase_marker = '!');
  void repairSetup_SP_FO(float setup_slack_margin,
                         int max_passes_per_startpoint,
                         bool verbose,
                         int& opto_iteration,
                         char phase_marker = '@');
  void repairSetup_Directional(bool use_startpoints,
                               float setup_slack_margin,
                               int max_passes_per_point,
                               bool verbose,
                               int& opto_iteration,
                               char phase_marker);
  void repairSetup_LastGasp(const OptoParams& params,
                            int& num_viols,
                            int max_iterations,
                            char phase_marker = 'G');

  Logger* logger_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  Resizer* resizer_;
  est::EstimateParasitics* estimate_parasitics_;
  std::unique_ptr<ViolatorCollector> violator_collector_;
  std::unique_ptr<MoveTracker> move_tracker_;

  bool fallback_ = false;
  float min_viol_ = 0.0;
  float max_viol_ = 0.0;
  int max_repairs_per_pass_ = 1;
  int removed_buffer_count_ = 0;
  double initial_design_area_ = 0;

  std::vector<BaseMove*> move_sequence_;

  // WNS Phase tracking: WNS-focused optimization
  std::map<const Pin*, int> endpoint_pass_counts_phase1_;
  int wns_no_progress_count_ = 0;
  // Track which (pin, move) combinations have been tried and rejected for
  // current endpoint
  std::map<const Pin*, std::set<BaseMove*>>
      rejected_pin_moves_current_endpoint_;

  // TNS Phase tracking: TNS-focused optimization
  int overall_no_progress_count_ = 0;

  // Used to early-exit some unit tests
  int max_end_repairs_ = -1;

  const MinMax* min_ = MinMax::min();
  const MinMax* max_ = MinMax::max();

  sta::UnorderedMap<LibertyPort*, sta::LibertyPortSet> equiv_pin_map_;

  static constexpr int initial_decreasing_slack_max_passes_ = 6;
  static constexpr int pass_limit_increment_ = 5;
  static constexpr int print_interval_ = 10;
  static constexpr int opto_small_interval_ = 100;
  static constexpr int opto_large_interval_ = 1000;
  static constexpr float inc_fix_rate_threshold_
      = 0.0001;  // default fix rate threshold = 0.01%
  static constexpr int max_last_gasp_passes_ = 10;
};

}  // namespace rsz
