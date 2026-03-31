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
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
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
  void repairSetup(const sta::Pin* end_pin);
  // For testing.
  void reportSwappablePins();
  // Rebuffer one net (for testing).
  // resizerPreamble() required.

 private:
  void init();
  bool repairEndpoint(sta::Pin* end_pin,
                      sta::Slack path_slack,
                      float setup_slack_margin);
  bool repairPins(
      const std::vector<const sta::Pin*>& pins,
      float setup_slack_margin,
      const std::map<const sta::Pin*, std::set<BaseMove*>>* rejected_moves
      = nullptr,
      std::vector<std::pair<const sta::Pin*, BaseMove*>>* chosen_moves
      = nullptr);
  int fanout(sta::Vertex* vertex);
  bool hasTopLevelOutputPort(sta::Net* net);

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
  void traverseFaninCone(sta::Vertex* endpoint,
                         std::unordered_map<sta::Instance*, float>& crit_insts,
                         std::unordered_set<sta::Vertex*>& visited,
                         std::unordered_set<sta::Instance*>& notSwappable,
                         const OptoParams& params);
  sta::Slack getInstanceSlack(sta::Instance* inst);

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
  void repairSetup_Wns(float setup_slack_margin,
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
  void repairSetup_Tns(float setup_slack_margin,
                       int max_passes_per_endpoint,
                       int max_repairs_per_pass,
                       bool verbose,
                       int& opto_iteration,
                       float initial_tns,
                       float& prev_tns,
                       char phase_marker = '*',  // Character for progress table
                       ViolatorSortType sort_type
                       = ViolatorSortType::SORT_BY_LOAD_DELAY);
  void repairSetup_EndpointFanin(float setup_slack_margin,
                                 int max_passes_per_endpoint,
                                 bool verbose,
                                 int& opto_iteration,
                                 char phase_marker = '!');
  void repairSetup_StartpointFanout(float setup_slack_margin,
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

  utl::Logger* logger_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;
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
  std::map<const sta::Pin*, int> endpoint_pass_counts_phase1_;
  int wns_no_progress_count_ = 0;
  // Track which (pin, move) combinations have been tried and rejected for
  // current endpoint
  std::map<const sta::Pin*, std::set<BaseMove*>>
      rejected_pin_moves_current_endpoint_;

  // TNS Phase tracking: TNS-focused optimization
  int overall_no_progress_count_ = 0;

  // Used to early-exit some unit tests
  int max_end_repairs_ = -1;

  const sta::MinMax* min_ = sta::MinMax::min();
  const sta::MinMax* max_ = sta::MinMax::max();

  std::unordered_map<sta::LibertyPort*, sta::LibertyPortSet> equiv_pin_map_;

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
