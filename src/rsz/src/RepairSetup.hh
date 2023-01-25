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
                   int max_passes);
  // For testing.
  void repairSetup(Pin *drvr_pin);
  // Rebuffer one net (for testing).
  // resizerPreamble() required.
  void rebufferNet(const Pin *drvr_pin);

private:
  void init();
  bool repairSetup(PathRef &path,
                   Slack path_slack);
  bool upsizeDrvr(PathRef *drvr_path,
                  int drvr_index,
                  PathExpanded *expanded);
  void splitLoads(PathRef *drvr_path,
                  int drvr_index,
                  Slack drvr_slack,
                  PathExpanded *expanded);
  LibertyCell *upsizeCell(LibertyPort *in_port,
                          LibertyPort *drvr_port,
                          float load_cap,
                          float prev_drive,
                          const DcalcAnalysisPt *dcalc_ap);
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
  LibertyPort *drvr_port_;

  int resize_count_;
  int inserted_buffer_count_;
  int rebuffer_net_count_;
  const MinMax *min_;
  const MinMax *max_;

  static constexpr int decreasing_slack_max_passes_ = 50;
  static constexpr int rebuffer_max_fanout_ = 20;
  static constexpr int split_load_min_fanout_ = 8;
  static constexpr double rebuffer_buffer_penalty_ = .01;
};

} // namespace
