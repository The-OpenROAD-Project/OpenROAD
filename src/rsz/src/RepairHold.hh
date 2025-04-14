// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include "db_sta/dbSta.hh"
#include "sta/MinMax.hh"
#include "sta/StaState.hh"
#include "utl/Logger.h"

namespace rsz {

class Resizer;

using utl::Logger;

using odb::Point;

using sta::dbNetwork;
using sta::dbSta;
using sta::Delay;
using sta::LibertyCell;
using sta::MinMax;
using sta::Pin;
using sta::PinSeq;
using sta::RiseFall;
using sta::Slack;
using sta::StaState;
using sta::Vertex;
using sta::VertexSeq;

using Slacks = Slack[RiseFall::index_count][MinMax::index_count];

class RepairHold : public sta::dbStaState
{
 public:
  RepairHold(Resizer* resizer);
  bool repairHold(double setup_margin,
                  double hold_margin,
                  bool allow_setup_violations,
                  // Max buffer count as percent of design instance count.
                  float max_buffer_percent,
                  int max_passes,
                  bool verbose);
  void repairHold(const Pin* end_pin,
                  double setup_margin,
                  double hold_margin,
                  bool allow_setup_violations,
                  float max_buffer_percent,
                  int max_passes);
  int holdBufferCount() const { return inserted_buffer_count_; }

 private:
  void init();
  LibertyCell* findHoldBuffer();
  float bufferHoldDelay(LibertyCell* buffer);
  void bufferHoldDelays(LibertyCell* buffer,
                        // Return values.
                        Delay delays[RiseFall::index_count]);
  void findHoldViolations(VertexSeq& ends,
                          double hold_margin,
                          // Return values.
                          Slack& worst_slack,
                          VertexSeq& hold_violations);
  bool repairHold(VertexSeq& ends,
                  LibertyCell* buffer_cell,
                  double setup_margin,
                  double hold_margin,
                  bool allow_setup_violations,
                  int max_buffer_count,
                  int max_passes,
                  bool verbose);
  void repairHoldPass(VertexSeq& hold_failures,
                      LibertyCell* buffer_cell,
                      double setup_margin,
                      double hold_margin,
                      bool allow_setup_violations,
                      int max_buffer_count,
                      bool verbose,
                      int& pass);
  void repairEndHold(Vertex* end_vertex,
                     LibertyCell* buffer_cell,
                     double setup_margin,
                     double hold_margin,
                     bool allow_setup_violations);
  void makeHoldDelay(Vertex* drvr,
                     PinSeq& load_pins,
                     bool loads_have_out_port,
                     LibertyCell* buffer_cell,
                     const Point& loc);
  bool checkMaxSlewCap(const Pin* drvr_pin);
  void mergeInit(Slacks& slacks);
  void mergeInto(Slacks& from, Slacks& result);

  void printProgress(int iteration, bool force, bool end) const;

  Logger* logger_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  Resizer* resizer_;

  int resize_count_ = 0;
  int inserted_buffer_count_ = 0;
  int cloned_gate_count_ = 0;
  int swap_pin_count_ = 0;
  int removed_buffer_count_ = 0;
  double initial_design_area_ = 0;
  const MinMax* min_ = MinMax::min();
  const MinMax* max_ = MinMax::max();
  const int min_index_ = MinMax::minIndex();
  const int max_index_ = MinMax::maxIndex();
  const int rise_index_ = RiseFall::riseIndex();
  const int fall_index_ = RiseFall::fallIndex();

  static constexpr float hold_slack_limit_ratio_max_ = 0.2;
  static constexpr int print_interval_ = 10;
};

}  // namespace rsz
