/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2023, Precision Innovations Inc.
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
using BufferedNetPtr = std::shared_ptr<BufferedNet>;
using BufferedNetSeq = vector<BufferedNetPtr>;

class RecoverPower : StaState
{
public:
  RecoverPower(Resizer *resizer);
  void recoverPower(float recover_power_percent);
  // For testing.
  void recoverPower(const Pin *end_pin);

private:
  void init();
  bool recoverPower(PathRef &path,
                   Slack path_slack);
  bool meetsSizeCriteria(LibertyCell *cell, LibertyCell *equiv,                           bool match_size);
  bool downsizeDrvr(PathRef *drvr_path, int drvr_index,
                    PathExpanded *expanded, bool only_same_size_swap,
                    Slack path_slack);

  LibertyCell *downsizeCell(LibertyPort *in_port, LibertyPort *drvr_port,
                          float load_cap, float prev_drive,
                          const DcalcAnalysisPt *dcalc_ap,
                          bool match_size,  Slack path_slack);
  int fanout(Vertex *vertex);
  bool hasTopLevelOutputPort(Net *net);

  BufferedNetSeq
  addWireAndBuffer(BufferedNetSeq Z,
                   BufferedNetPtr bnet_wire,
                   int level);
  float pinCapacitance(const Pin *pin,
                       const DcalcAnalysisPt *dcalc_ap);
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
  int resize_count_;
  const MinMax *max_;

  // Paths with slack more than this would be considered for power recovery
  static constexpr float setup_slack_margin_ = 1e-11;
  // For paths with no timing the max margin is INT_MAX. We need to filter those out (using 1e-4)
  static constexpr float setup_slack_max_margin_ = 1e-4;
  // Threshold for failed successive moves for power recovery before we stop trying
  static constexpr int failed_move_threshold_limit_ = 500;

  sta::UnorderedMap<LibertyCell *, sta::LibertyPortSet> equiv_pin_map_;

  static constexpr int decreasing_slack_max_passes_ = 50;
  static constexpr int rebuffer_max_fanout_ = 20;
  static constexpr int split_load_min_fanout_ = 8;
  static constexpr double rebuffer_buffer_penalty_ = .01;
};

} // namespace rsz
