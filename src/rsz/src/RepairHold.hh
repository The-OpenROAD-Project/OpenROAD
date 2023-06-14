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

namespace rsz {

class Resizer;

using utl::Logger;

using odb::Point;

using sta::StaState;
using sta::dbSta;
using sta::dbNetwork;
using sta::MinMax;
using sta::Pin;
using sta::LibertyCell;
using sta::RiseFall;
using sta::Slack;
using sta::Delay;
using sta::Vertex;
using sta::PinSeq;
using sta::VertexSeq;

typedef Slack Slacks[RiseFall::index_count][MinMax::index_count];

class RepairHold : StaState
{
public:
  RepairHold(Resizer *resizer);
  void repairHold(double setup_margin,
                  double hold_margin,
                  bool allow_setup_violations,
                  // Max buffer count as percent of design instance count.
                  float max_buffer_percent,
                  int max_passes);
  void repairHold(const Pin *end_pin,
                  double setup_margin,
                  double hold_margin,
                  bool allow_setup_violations,
                  float max_buffer_percent,
                  int max_passes);
  int holdBufferCount() const { return inserted_buffer_count_; }

private:
  void init();
  LibertyCell *findHoldBuffer();
  float bufferHoldDelay(LibertyCell *buffer);
  void bufferHoldDelays(LibertyCell *buffer,
                        // Return values.
                        Delay delays[RiseFall::index_count]);
  void findHoldViolations(VertexSeq &ends,
                          double hold_margin,
                          // Return values.
                          Slack &worst_slack,
                          VertexSeq &hold_violations);
  void repairHold(VertexSeq &ends,
                  LibertyCell *buffer_cell,
                  double setup_margin,
                  double hold_margin,
                  bool allow_setup_violations,
                  int max_buffer_count,
                  int max_passes);
  void repairHoldPass(VertexSeq &ends,
                      LibertyCell *buffer_cell,
                      double setup_margin,
                      double hold_margin,
                      bool allow_setup_violations,
                      int max_buffer_count);
  void repairEndHold(Vertex *worst_vertex,
                     LibertyCell *buffer_cell,
                     double setup_margin,
                     double hold_margin,
                     bool allow_setup_violations,
                     int max_buffer_count);
  void makeHoldDelay(Vertex *drvr,
                     PinSeq &load_pins,
                     bool loads_have_out_port,
                     LibertyCell *buffer_cell,
                     Point loc);
  bool checkMaxSlewCap(const Pin *drvr_pin);
  void mergeInit(Slacks &slacks);
  void mergeInto(Slacks &slacks,
                 Slacks &result);

  Logger *logger_;
  dbSta *sta_;
  dbNetwork *db_network_;
  Resizer *resizer_;

  int resize_count_;
  int inserted_buffer_count_;
  int cloned_gate_count_;
  const MinMax *min_;
  const MinMax *max_;
  const int min_index_;
  const int max_index_;
  const int rise_index_;
  const int fall_index_;

  static constexpr float hold_slack_limit_ratio_max_ = 0.2;
};

} // namespace
