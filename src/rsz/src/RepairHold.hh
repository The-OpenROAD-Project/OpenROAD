// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace est {
class EstimateParasitics;
}

namespace rsz {

class Resizer;

using Slacks = sta::Slack[sta::RiseFall::index_count][sta::MinMax::index_count];

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
                  int max_iterations,
                  bool verbose);
  void repairHold(const sta::Pin* end_pin,
                  double setup_margin,
                  double hold_margin,
                  bool allow_setup_violations,
                  float max_buffer_percent,
                  int max_passes);
  int holdBufferCount() const { return inserted_buffer_count_; }
  sta::LibertyCell* reportHoldBuffer();

 private:
  void init();
  sta::LibertyCell* findHoldBuffer();
  void filterHoldBuffers(sta::LibertyCellSeq& hold_buffers);
  bool addMatchingBuffers(const sta::LibertyCellSeq& buffer_list,
                          sta::LibertyCellSeq& hold_buffers,
                          int best_vt_index,
                          odb::dbSite* best_site,
                          bool lib_has_footprints,
                          bool match_site,
                          bool match_vt,
                          bool match_footprint);
  float bufferHoldDelay(sta::LibertyCell* buffer);
  void bufferHoldDelays(sta::LibertyCell* buffer,
                        // Return values.
                        sta::Delay delays[sta::RiseFall::index_count]);
  void findHoldViolations(sta::VertexSeq& ends,
                          double hold_margin,
                          // Return values.
                          sta::Slack& worst_slack,
                          sta::VertexSeq& hold_violations);
  bool repairHold(sta::VertexSeq& ends,
                  sta::LibertyCell* buffer_cell,
                  double setup_margin,
                  double hold_margin,
                  bool allow_setup_violations,
                  int max_buffer_count,
                  int max_passes,
                  int max_iterations,
                  bool verbose);
  void repairHoldPass(sta::VertexSeq& hold_failures,
                      sta::LibertyCell* buffer_cell,
                      double setup_margin,
                      double hold_margin,
                      bool allow_setup_violations,
                      int max_buffer_count,
                      bool verbose,
                      int& pass);
  void repairEndHold(sta::Vertex* end_vertex,
                     sta::LibertyCell* buffer_cell,
                     double setup_margin,
                     double hold_margin,
                     bool allow_setup_violations);
  void makeHoldDelay(sta::Vertex* drvr,
                     sta::PinSeq& load_pins,
                     bool loads_have_out_port,
                     sta::LibertyCell* buffer_cell,
                     const odb::Point& loc);
  bool checkMaxSlewCap(const sta::Pin* drvr_pin);
  void mergeInit(Slacks& slacks);
  void mergeInto(Slacks& from, Slacks& result);

  void printProgress(int iteration, bool force, bool end) const;

  utl::Logger* logger_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;
  Resizer* resizer_;
  est::EstimateParasitics* estimate_parasitics_;

  int resize_count_ = 0;
  int inserted_buffer_count_ = 0;
  int cloned_gate_count_ = 0;
  int swap_pin_count_ = 0;
  int removed_buffer_count_ = 0;
  double initial_design_area_ = 0;
  const sta::MinMax* min_ = sta::MinMax::min();
  const sta::MinMax* max_ = sta::MinMax::max();
  const int min_index_ = sta::MinMax::minIndex();
  const int max_index_ = sta::MinMax::maxIndex();
  const int rise_index_ = sta::RiseFall::riseIndex();
  const int fall_index_ = sta::RiseFall::fallIndex();

  static constexpr float hold_slack_limit_ratio_max_ = 0.2;
  static constexpr int print_interval_ = 10;
};

}  // namespace rsz
