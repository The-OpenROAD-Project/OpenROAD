// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once
#include <boost/functional/hash.hpp>
#include <unordered_set>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/FuncExpr.hh"
#include "sta/MinMax.hh"
#include "sta/StaState.hh"
#include "utl/Logger.h"

namespace sta {
class PathExpanded;
}

namespace rsz {

class Resizer;
class RemoveBuffer;

using odb::Point;
using utl::Logger;

using sta::Corner;
using sta::dbNetwork;
using sta::dbSta;
using sta::DcalcAnalysisPt;
using sta::Instance;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::MinMax;
using sta::Net;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::RiseFall;
using sta::Slack;
using sta::Slew;
using sta::StaState;
using sta::TimingArc;
using sta::Vertex;

class BufferedNet;
enum class BufferedNetType;
using BufferedNetPtr = std::shared_ptr<BufferedNet>;
using BufferedNetSeq = std::vector<BufferedNetPtr>;
struct SlackEstimatorParams
{
  Pin* driver_pin;
  Pin* prev_driver_pin;
  Pin* driver_input_pin;
  Instance* driver;
  const Path* driver_path;
  const Path* prev_driver_path;
  LibertyCell* driver_cell;
  const float setup_slack_margin;
  const Corner* corner;

  SlackEstimatorParams(const float margin, const Corner* corner)
      : setup_slack_margin(margin), corner(corner)
  {
    driver_pin = nullptr;
    prev_driver_pin = nullptr;
    driver_input_pin = nullptr;
    driver = nullptr;
    driver_path = nullptr;
    prev_driver_path = nullptr;
    driver_cell = nullptr;
  }
};
struct OptoParams
{
  int iteration;
  float initial_tns;
  const float setup_slack_margin;
  const bool verbose;

  OptoParams(const float margin, const bool verbose)
      : setup_slack_margin(margin), verbose(verbose)
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
                   int max_repairs_per_pass,
                   bool verbose,
                   bool skip_pin_swap,
                   bool skip_gate_cloning,
                   bool skip_buffering,
                   bool skip_buffer_removal,
                   bool skip_last_gasp);
  // For testing.
  void repairSetup(const Pin* end_pin);
  // For testing.
  void reportSwappablePins();
  // Rebuffer one net (for testing).
  // resizerPreamble() required.
  void rebufferNet(const Pin* drvr_pin);

 private:
  void init();
  bool repairPath(Path* path,
                  Slack path_slack,
                  bool skip_pin_swap,
                  bool skip_gate_cloning,
                  bool skip_buffering,
                  bool skip_buffer_removal,
                  float setup_slack_margin);
  void debugCheckMultipleBuffers(Path* path, PathExpanded* expanded);
  bool simulateExpr(
      sta::FuncExpr* expr,
      sta::UnorderedMap<const LibertyPort*, std::vector<bool>>& port_stimulus,
      size_t table_index);
  std::vector<bool> simulateExpr(
      sta::FuncExpr* expr,
      sta::UnorderedMap<const LibertyPort*, std::vector<bool>>& port_stimulus);
  bool isPortEqiv(sta::FuncExpr* expr,
                  const LibertyCell* cell,
                  const LibertyPort* port_a,
                  const LibertyPort* port_b);
  void equivCellPins(const LibertyCell* cell,
                     LibertyPort* input_port,
                     sta::LibertyPortSet& ports);
  bool swapPins(const Path* drvr_path, int drvr_index, PathExpanded* expanded);
  bool removeDrvr(const Path* drvr_path,
                  LibertyCell* drvr_cell,
                  int drvr_index,
                  PathExpanded* expanded,
                  float setup_slack_margin);
  bool estimatedSlackOK(const SlackEstimatorParams& params);
  bool estimateInputSlewImpact(Instance* instance,
                               const DcalcAnalysisPt* dcalc_ap,
                               Slew old_in_slew[RiseFall::index_count],
                               Slew new_in_slew[RiseFall::index_count],
                               // delay adjustment from prev stage
                               float delay_adjust,
                               SlackEstimatorParams params,
                               bool accept_if_slack_improves);
  bool upsizeDrvr(const Path* drvr_path,
                  int drvr_index,
                  PathExpanded* expanded);
  Point computeCloneGateLocation(
      const Pin* drvr_pin,
      const std::vector<std::pair<Vertex*, Slack>>& fanout_slacks);
  bool cloneDriver(const Path* drvr_path,
                   int drvr_index,
                   Slack drvr_slack,
                   PathExpanded* expanded);
  void splitLoads(const Path* drvr_path,
                  int drvr_index,
                  Slack drvr_slack,
                  PathExpanded* expanded);
  LibertyCell* upsizeCell(LibertyPort* in_port,
                          LibertyPort* drvr_port,
                          float load_cap,
                          float prev_drive,
                          const DcalcAnalysisPt* dcalc_ap);
  int fanout(Vertex* vertex);
  bool hasTopLevelOutputPort(Net* net);

  int rebuffer(const Pin* drvr_pin);

  BufferedNetPtr rebufferForTiming(const BufferedNetPtr& bnet);
  BufferedNetPtr recoverArea(const BufferedNetPtr& bnet,
                             sta::Delay slack_target,
                             float alpha);

  int rebufferTopDown(const BufferedNetPtr& choice,
                      Net* net,
                      int level,
                      Instance* parent,
                      odb::dbITerm* mod_net_drvr,
                      odb::dbModNet* mod_net);
  BufferedNetPtr addWire(const BufferedNetPtr& p,
                         const Point& wire_end,
                         int wire_layer,
                         int level);
  void addBuffers(BufferedNetSeq& Z1,
                  int level,
                  bool area_oriented = false,
                  sta::Delay threshold = 0);
  float bufferInputCapacitance(LibertyCell* buffer_cell,
                               const DcalcAnalysisPt* dcalc_ap);
  std::tuple<const Path*, sta::Delay> drvrPinTiming(const BufferedNetPtr& bnet);
  Slack slackAtDriverPin(const BufferedNetPtr& bnet);
  Slack slackAtDriverPin(const BufferedNetPtr& bnet, int index);

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
  void repairSetupLastGasp(const OptoParams& params, int& num_viols);

  std::vector<Instance*> buf_to_remove_;
  Logger* logger_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  Resizer* resizer_;
  const Corner* corner_ = nullptr;
  LibertyPort* drvr_port_ = nullptr;

  bool fallback_ = false;
  float min_viol_ = 0.0;
  float max_viol_ = 0.0;
  int max_repairs_per_pass_ = 1;
  int resize_count_ = 0;
  int inserted_buffer_count_ = 0;
  int split_load_buffer_count_ = 0;
  int rebuffer_net_count_ = 0;
  int cloned_gate_count_ = 0;
  int swap_pin_count_ = 0;
  int removed_buffer_count_ = 0;
  double initial_design_area_ = 0;
  // Map to block pins from being swapped more than twice for the
  // same instance.
  std::unordered_set<const sta::Instance*> swap_pin_inst_set_;

  const MinMax* min_ = MinMax::min();
  const MinMax* max_ = MinMax::max();

  sta::UnorderedMap<LibertyPort*, sta::LibertyPortSet> equiv_pin_map_;

  static constexpr int decreasing_slack_max_passes_ = 50;
  static constexpr int rebuffer_max_fanout_ = 20;
  static constexpr int split_load_min_fanout_ = 8;
  static constexpr int print_interval_ = 10;
  static constexpr int opto_small_interval_ = 100;
  static constexpr int opto_large_interval_ = 1000;
  static constexpr int buffer_removal_max_fanout_ = 10;
  static constexpr float inc_fix_rate_threshold_
      = 0.0001;  // default fix rate threshold = 0.01%
  static constexpr int max_last_gasp_passes_ = 10;
  static constexpr float rebuffer_relaxation_factor_ = 0.03;
};

}  // namespace rsz
