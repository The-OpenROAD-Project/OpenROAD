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

#include "utl/Logger.h"
#include "db_sta/dbSta.hh"

#include "sta/StaState.hh"
#include "sta/MinMax.hh"
#include "sta/FuncExpr.hh"

namespace sta {
class PathExpanded;
}

namespace rsz {

class Resizer;

using std::vector;

using utl::Logger;

using sta::StaState;
using sta::dbSta;
using sta::dbNetwork;
using sta::Pin;
using sta::Net;
using sta::PathRef;
using sta::MinMax;
using sta::Slack;
using sta::PathExpanded;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::TimingArc;
using sta::DcalcAnalysisPt;
using sta::Vertex;
using sta::Corner;

class BufferedNet;
enum class BufferedNetType;
typedef std::shared_ptr<BufferedNet> BufferedNetPtr;
typedef vector<BufferedNetPtr> BufferedNetSeq;

class RepairSetup : StaState
{
public:
  RepairSetup(Resizer *resizer);
  void repairSetup(float setup_slack_margin,
                   // Percent of violating ends to repair to
                   // reduce tns (0.0-1.0).
                   double repair_tns_end_percent,
                   int max_passes,
                   bool skip_pin_swap,
                   bool skip_gate_cloning);
  // For testing.
  void repairSetup(const Pin *end_pin);
  // Rebuffer one net (for testing).
  // resizerPreamble() required.
  void rebufferNet(const Pin *drvr_pin);

private:
  void init();
  bool repairSetup(PathRef &path,
                   Slack path_slack,
                   bool skip_pin_swap,
                   bool skip_gate_cloning);
  void debugCheckMultipleBuffers(PathRef &path,
                                 PathExpanded *expanded);

  void getEquivPortList2(sta::FuncExpr *expr, sta::LibertyPortSet &ports,
                         sta::FuncExpr::Operator &status);
  void getEquivPortList(sta::FuncExpr *expr, sta::LibertyPortSet &ports);
  void equivCellPins(const LibertyCell *cell, sta::LibertyPortSet &ports);
  bool swapPins(PathRef *drvr_path, int drvr_index, PathExpanded *expanded);
  bool meetsSizeCriteria(LibertyCell *cell, LibertyCell *equiv, bool match_size);
  bool upsizeDrvr(PathRef *drvr_path,
                  int drvr_index,
                  PathExpanded *expanded,
                  bool only_same_size_swap);
  void splitLoads(PathRef *drvr_path,
                  int drvr_index,
                  Slack drvr_slack,
                  PathExpanded *expanded);
  LibertyCell *upsizeCell(LibertyPort *in_port,
                          LibertyPort *drvr_port,
                          float load_cap,
                          float prev_drive,
                          const DcalcAnalysisPt *dcalc_ap,
                          bool match_size);
  int fanout(Vertex *vertex);
  bool hasTopLevelOutputPort(Net *net);

  int rebuffer(const Pin *drvr_pin);
  BufferedNetSeq rebufferBottomUp(BufferedNetPtr bnet,
                                  int level);
  int rebufferTopDown(BufferedNetPtr choice,
                      Net *net,
                      int level);
  BufferedNetSeq
  addWireAndBuffer(BufferedNetSeq Z,
                   BufferedNetPtr bnet_wire,
                   int level);
  float bufferInputCapacitance(LibertyCell *buffer_cell,
                               const DcalcAnalysisPt *dcalc_ap);
  Slack slackPenalized(BufferedNetPtr bnet);
  Slack slackPenalized(BufferedNetPtr bnet,
                       int index);

  Logger *logger_;
  dbSta *sta_;
  dbNetwork *db_network_;
  Resizer *resizer_;
  const Corner *corner_;
  LibertyPort *drvr_port_;

  int resize_count_;
  int inserted_buffer_count_;
  int split_load_buffer_count_;
  int rebuffer_net_count_;
  int cloned_gate_count_;  
  int swap_pin_count_;
  // Map to block pins from being swapped more than twice for the
  // same instance. 
  std::unordered_map<const sta::Instance *, int> swap_pin_inst_map_;
  
  const MinMax *min_;
  const MinMax *max_;

  sta::UnorderedMap<LibertyCell *, sta::LibertyPortSet> equiv_pin_map_;

  static constexpr int decreasing_slack_max_passes_ = 50;
  static constexpr int rebuffer_max_fanout_ = 20;
  static constexpr int split_load_min_fanout_ = 8;
  static constexpr double rebuffer_buffer_penalty_ = .01;
};

} // namespace
