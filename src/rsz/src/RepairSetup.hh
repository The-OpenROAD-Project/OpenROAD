/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

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
using std::pair;
using std::vector;
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
using sta::PathExpanded;
using sta::PathRef;
using sta::Pin;
using sta::Slack;
using sta::StaState;
using sta::TimingArc;
using sta::Vertex;

class BufferedNet;
enum class BufferedNetType;
using BufferedNetPtr = std::shared_ptr<BufferedNet>;
using BufferedNetSeq = vector<BufferedNetPtr>;
struct SlackEstimatorParams
{
  Pin* driver_pin;
  Pin* prev_driver_pin;
  Pin* driver_input_pin;
  Instance* driver;
  PathRef* driver_path;
  PathRef* prev_driver_path;
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

class RepairSetup : public sta::dbStaState
{
 public:
  RepairSetup(Resizer* resizer);
  void repairSetup(float setup_slack_margin,
                   // Percent of violating ends to repair to
                   // reduce tns (0.0-1.0).
                   double repair_tns_end_percent,
                   int max_passes,
                   bool verbose,
                   bool skip_pin_swap,
                   bool skip_gate_cloning,
                   bool skip_buffer_removal);
  // For testing.
  void repairSetup(const Pin* end_pin);
  // For testing.
  void reportSwappablePins();
  // Rebuffer one net (for testing).
  // resizerPreamble() required.
  void rebufferNet(const Pin* drvr_pin);

 private:
  void init();
  bool repairPath(PathRef& path,
                  const Slack path_slack,
                  const bool skip_pin_swap,
                  const bool skip_gate_cloning,
                  const bool skip_buffer_removal,
                  const float setup_slack_margin);
  void debugCheckMultipleBuffers(PathRef& path, PathExpanded* expanded);
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
  bool swapPins(PathRef* drvr_path, int drvr_index, PathExpanded* expanded);
  bool removeDrvr(PathRef* drvr_path,
                  LibertyCell* drvr_cell,
                  const int drvr_index,
                  PathExpanded* expanded,
                  const float setup_slack_margin);
  bool estimatedSlackOK(const SlackEstimatorParams& params);
  bool upsizeDrvr(PathRef* drvr_path, int drvr_index, PathExpanded* expanded);
  Point computeCloneGateLocation(
      const Pin* drvr_pin,
      const vector<pair<Vertex*, Slack>>& fanout_slacks);
  bool cloneDriver(PathRef* drvr_path,
                   int drvr_index,
                   Slack drvr_slack,
                   PathExpanded* expanded);
  void splitLoads(PathRef* drvr_path,
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
  BufferedNetSeq rebufferBottomUp(const BufferedNetPtr& bnet, int level);
  int rebufferTopDown(const BufferedNetPtr& choice, Net* net, int level);
  BufferedNetSeq addWireAndBuffer(const BufferedNetSeq& Z,
                                  const BufferedNetPtr& bnet_wire,
                                  int level);
  float bufferInputCapacitance(LibertyCell* buffer_cell,
                               const DcalcAnalysisPt* dcalc_ap);
  Slack slackPenalized(const BufferedNetPtr& bnet);
  Slack slackPenalized(const BufferedNetPtr& bnet, int index);

  void printProgress(int iteration, bool force, bool end) const;

  Logger* logger_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  Resizer* resizer_;
  const Corner* corner_ = nullptr;
  LibertyPort* drvr_port_ = nullptr;

  int resize_count_ = 0;
  int inserted_buffer_count_ = 0;
  int split_load_buffer_count_ = 0;
  int rebuffer_net_count_ = 0;
  int cloned_gate_count_ = 0;
  int swap_pin_count_ = 0;
  int removed_buffer_count_ = 0;
  // Map to block pins from being swapped more than twice for the
  // same instance.
  std::unordered_set<const sta::Instance*> swap_pin_inst_set_;

  const MinMax* min_ = MinMax::min();
  const MinMax* max_ = MinMax::max();

  sta::UnorderedMap<LibertyPort*, sta::LibertyPortSet> equiv_pin_map_;

  static constexpr int decreasing_slack_max_passes_ = 50;
  static constexpr int rebuffer_max_fanout_ = 20;
  static constexpr int split_load_min_fanout_ = 8;
  static constexpr double rebuffer_buffer_penalty_ = .01;
  static constexpr int print_interval_ = 10;
  static constexpr int buffer_removal_max_fanout_ = 10;
};

}  // namespace rsz
