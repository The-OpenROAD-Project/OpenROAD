// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "db_sta/dbSta.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/StaState.hh"
#include "utl/Logger.h"

namespace est {
class EstimateParasitics;
}

namespace rsz {

class Resizer;

using utl::Logger;

using sta::Corner;
using sta::dbNetwork;
using sta::dbSta;
using sta::DcalcAnalysisPt;
using sta::LibertyCell;
using sta::MinMax;
using sta::Net;
using sta::Pin;
using sta::Slack;
using sta::StaState;
using sta::TimingArc;
using sta::Vertex;

class RecoverPowerMore : public sta::dbStaState
{
 public:
  explicit RecoverPowerMore(Resizer* resizer);
  bool recoverPower(float recover_power_percent, bool verbose);

 private:
  enum class ActionType
  {
    kSwap,
    kRemoveBuffer,
  };

  struct ActionRecord
  {
    ActionType type = ActionType::kSwap;
    std::string inst_name;
    LibertyCell* prev_cell = nullptr;  // swap: original
    LibertyCell* new_cell = nullptr;   // swap: replacement, remove: buffer cell
  };

  struct CandidateInstance
  {
    sta::Instance* inst = nullptr;
    Slack slack = 0.0;
    Slack headroom = 0.0;
    // Total instance power provides a better cross-library ranking.
    float power = 0.0f;
    int64_t area = 0;
    bool drives_clock = false;
  };

  void init();
  std::vector<CandidateInstance> collectCandidates(
      Slack current_setup_wns) const;
  Slack instanceWorstSlack(sta::Instance* inst) const;
  bool instanceDrivesClock(sta::Instance* inst) const;

  float minClockPeriod() const;
  Slack computeWnsFloor(Slack worst_setup_before, float clock_period) const;

  const Corner* selectPowerCorner() const;
  float designTotalPower(const Corner* corner) const;

  // Collect signal (non-P/G) nets connected to an instance.
  std::vector<const Net*> instanceSignalNets(sta::Instance* inst) const;
  // Expand a net cone to cover potential slew propagation (limited depth).
  std::vector<const Net*> slewCheckNetCone(
      const std::vector<const Net*>& seed_nets) const;

  size_t countSlewViolations(const std::vector<const Net*>& nets) const;
  size_t countCapViolations(const std::vector<const Net*>& nets) const;
  size_t countFanoutViolations(const std::vector<const Net*>& nets) const;

  std::vector<LibertyCell*> nextSmallerCells(LibertyCell* cell) const;
  LibertyCell* nextLowerLeakageVtCell(LibertyCell* cell) const;

  bool trySwapCell(sta::Instance* inst,
                   LibertyCell* replacement,
                   Slack wns_floor,
                   Slack hold_floor,
                   size_t max_slew_vio,
                   size_t max_cap_vio,
                   size_t max_fanout_vio,
                   bool verbose);
  bool tryRemoveBuffer(sta::Instance* inst,
                       Slack wns_floor,
                       Slack hold_floor,
                       size_t max_slew_vio,
                       size_t max_cap_vio,
                       size_t max_fanout_vio,
                       bool verbose);
  bool optimizeInstanceSwaps(sta::Instance* inst,
                             Slack wns_floor,
                             Slack hold_floor,
                             size_t max_slew_vio,
                             size_t max_cap_vio,
                             size_t max_fanout_vio,
                             bool verbose);
  bool optimizeInstanceBufferRemoval(sta::Instance* inst,
                                     Slack wns_floor,
                                     Slack hold_floor,
                                     size_t max_slew_vio,
                                     size_t max_cap_vio,
                                     size_t max_fanout_vio,
                                     bool verbose);
  void backoffToTimingFloor(Slack wns_floor, Slack hold_floor, bool verbose);

  bool meetsSizeCriteria(const LibertyCell* cell,
                         const LibertyCell* candidate) const;
  int fanout(Vertex* vertex) const;

  void printProgress(int iteration,
                     int total_iterations,
                     bool force,
                     bool end) const;

  Logger* logger_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  Resizer* resizer_ = nullptr;
  est::EstimateParasitics* estimate_parasitics_ = nullptr;
  const Corner* corner_ = nullptr;
  int resize_count_ = 0;
  const MinMax* max_ = MinMax::max();
  const MinMax* min_ = MinMax::min();

  // Instances with slack headroom more than this are considered.
  static constexpr float setup_slack_margin_ = 1e-11;
  // For unconstrained paths slacks can be extremely large; filter those out.
  static constexpr float setup_slack_max_margin_ = 1e-4;

  // Slew can propagate through timing arcs (input slew affects output slew).
  // Limit how far the DRV check cone expands to keep runtime bounded.
  static constexpr int slew_check_depth_ = 2;

  static constexpr int max_passes_ = 10;
  static constexpr int max_swaps_per_instance_ = 16;

  // Limit performance degradation from recover_power by bounding the increase
  // in effective clock period (target clock period - worst setup slack).
  static constexpr float max_eff_period_degrade_frac_ = 0.01f;
  // Keep a small timing guard-band above the computed floor to absorb routing
  // noise from rejected candidates.
  static constexpr float timing_floor_guard_frac_ = 0.06f;
  static constexpr Slack timing_floor_guard_cap_ = 2e-12;

  double initial_design_area_ = 0;
  float initial_power_total_ = -1.0f;
  size_t initial_max_slew_violations_ = 0;
  size_t initial_max_cap_violations_ = 0;
  size_t initial_max_fanout_violations_ = 0;
  size_t curr_max_slew_violations_ = 0;
  size_t curr_max_cap_violations_ = 0;
  size_t curr_max_fanout_violations_ = 0;
  int print_interval_ = 0;

  static constexpr int min_print_interval_ = 10;
  static constexpr int max_print_interval_ = 5000;

  Slack wns_floor_ = 0.0;
  Slack hold_floor_ = 0.0;
  int buffer_remove_count_ = 0;
  std::vector<ActionRecord> action_history_;
};

}  // namespace rsz
