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

#include "RecoverPower.hh"
#include "rsz/Resizer.hh"

#include "utl/Logger.h"
#include "db_sta/dbNetwork.hh"

#include "sta/Units.hh"
#include "sta/Liberty.hh"
#include "sta/TimingArc.hh"
#include "sta/Graph.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Parasitics.hh"
#include "sta/Sdc.hh"
#include "sta/InputDrive.hh"
#include "sta/Corner.hh"
#include "sta/PathVertex.hh"
#include "sta/PathRef.hh"
#include "sta/PathExpanded.hh"
#include "sta/Fuzzy.hh"
#include "sta/PortDirection.hh"


namespace rsz {

using std::string;
using std::vector;
using std::pair;

using utl::RSZ;

using sta::VertexOutEdgeIterator;
using sta::Edge;
using sta::PathExpanded;

RecoverPower::RecoverPower(Resizer* resizer)
    : logger_(nullptr),
      sta_(nullptr),
      db_network_(nullptr),
      resizer_(resizer),
      corner_(nullptr),
      resize_count_(0),
      max_(MinMax::max())
{
}

void
RecoverPower::init()
{
  logger_ = resizer_->logger_;
  sta_ = resizer_->sta_;
  db_network_ = resizer_->db_network_;
  copyState(sta_);
}

void
RecoverPower::recoverPower(float recover_power_percent)
{
  init();
  constexpr int digits = 3;
  int failed_move_threshold = 0;
  resize_count_ = 0;
  resizer_->buffer_moved_into_core_ = false;

  // Sort failing endpoints by slack.
  VertexSet *endpoints = sta_->endpoints();
  VertexSeq ends_with_slack;
  for (Vertex *end : *endpoints) {
    Slack end_slack = sta_->vertexSlack(end, max_);
    if (end_slack > setup_slack_margin_ && end_slack < setup_slack_max_margin_)  {
      ends_with_slack.push_back(end);
    }
  }

  sort(ends_with_slack, [=](Vertex *end1, Vertex *end2) {
    return sta_->vertexSlack(end1, max_) > sta_->vertexSlack(end2, max_);
  });

  debugPrint(logger_, RSZ, "recover_power", 1, "Candidate paths {}/{} {}%",
             ends_with_slack.size(),
             endpoints->size(),
             int(ends_with_slack.size() / double(endpoints->size()) * 100));

  int end_index = 0;
  int max_end_count = ends_with_slack.size() * recover_power_percent;
  // As long as we are here fix at least one path
  if (max_end_count == 0) {
    max_end_count = 1;
  }


  Slack worst_slack_before;
  Vertex *worst_vertex;
  resizer_->incrementalParasiticsBegin();
  resizer_->updateParasitics();
  sta_->findRequireds();
  sta_->worstSlack(max_, worst_slack_before, worst_vertex);

  for (Vertex *end : ends_with_slack) {
    Slack end_slack_before = sta_->vertexSlack(end, max_);
    Slack worst_slack_after;
    //=====================================================================
    // Just a counter to know when to break out
    end_index++;
    debugPrint(logger_, RSZ, "recover_power", 2, "Doing {} /{}", end_index,
               max_end_count);
    if (end_index > max_end_count) {
      break;
    }
    //=====================================================================
    resizer_->journalBegin();
    PathRef end_path = sta_->vertexWorstSlackPath(end, max_);
    bool changed = recoverPower(end_path, end_slack_before);
    if (changed) {
      resizer_->updateParasitics(true);
      sta_->findRequireds();
      Slack end_slack_after = sta_->vertexSlack(end, max_);

      sta_->worstSlack(max_, worst_slack_after, worst_vertex);
      // tns_slack_after = sta_->totalNegativeSlack(max_);

      float worst_slack_percent = fabs((worst_slack_before - worst_slack_after) / worst_slack_before * 100);
      bool better = (worst_slack_percent < 0.0001 ||
              (worst_slack_before > 0 && worst_slack_after/worst_slack_before > 0.5));

      debugPrint(logger_, RSZ, "recover_power", 2,
                 "slack = {} worst_slack = {} better = {}",
                 delayAsString(end_slack_after, sta_, digits), delayAsString(worst_slack_after, sta_, digits),
                 better ? "save" : "");

      if (better) {
        failed_move_threshold = 0;
        resizer_->journalBegin();
        debugPrint(logger_, RSZ, "recover_power", 2, "{}/{} Resize for power Slack change {} -> {}",
                   end_index, ends_with_slack.size(), worst_slack_before, worst_slack_after);
      }
      else {
	    // Undo the change here.
        ++failed_move_threshold;
        if (failed_move_threshold > failed_move_threshold_limit_) {
          logger_->info(RSZ, 142, "{} successive tries yielded negative slack. Ending power recovery",
                     failed_move_threshold_limit_);

          break;
        }
        int resize_count = 100, inserted_buffer_count = 100, cloned_gate_count = 100;
        resizer_->journalRestore(resize_count, inserted_buffer_count, cloned_gate_count);
        resizer_->updateParasitics();
        sta_->findRequireds();
        debugPrint(logger_, RSZ, "recover_power", 2, "{}/{} Undo resize for power Slack change {} -> {}",
                   end_index, ends_with_slack.size(), worst_slack_before, worst_slack_after);
      }
      if (resizer_->overMaxArea()) {
        break;
      }
    }
  }

  resizer_->incrementalParasiticsEnd();
  // TODO: Add the appropriate metric here
  // logger_->metric("design__instance__count__setup_buffer", inserted_buffer_count_);
  if (resize_count_ > 0) {
    logger_->info(RSZ, 141, "Resized {} instances.", resize_count_);
  }
  if (resizer_->overMaxArea()) {
    logger_->error(RSZ, 125, "max utilization reached.");
  }
}

// For testing.
void
RecoverPower::recoverPower(const Pin *end_pin)
{
  init();
  resize_count_ = 0;

  Vertex *vertex = graph_->pinLoadVertex(end_pin);
  Slack slack = sta_->vertexSlack(vertex, max_);
  PathRef path = sta_->vertexWorstSlackPath(vertex, max_);
  resizer_->incrementalParasiticsBegin();
  recoverPower(path, slack);
  // Leave the parasitices up to date.
  resizer_->updateParasitics();
  resizer_->incrementalParasiticsEnd();

  if (resize_count_ > 0) {
    logger_->info(RSZ, 3111, "Resized {} instances.", resize_count_);
  }
}

// This is the main routine for recovering power.
bool
RecoverPower::recoverPower(PathRef &path, Slack path_slack)
{
  PathExpanded expanded(&path, sta_);
  bool changed = false;

  if (expanded.size() > 1) {
    int path_length = expanded.size();
    vector<pair<int, Delay>> load_delays;
    int start_index = expanded.startIndex();
    const DcalcAnalysisPt *dcalc_ap = path.dcalcAnalysisPt(sta_);
    int lib_ap = dcalc_ap->libertyIndex();
    // Find load delay for each gate in the path.
    for (int i = start_index; i < path_length; i++) {
      PathRef *path = expanded.path(i);
      Vertex *path_vertex = path->vertex(sta_);
      const Pin *path_pin = path->pin(sta_);
      if (i > 0 && network_->isDriver(path_pin) &&
          !network_->isTopLevelPort(path_pin)) {
        TimingArc *prev_arc = expanded.prevArc(i);
        const TimingArc *corner_arc = prev_arc->cornerArc(lib_ap);
        Edge *prev_edge = path->prevEdge(prev_arc, sta_);
        Delay load_delay = graph_->arcDelay(prev_edge, prev_arc, dcalc_ap->index())
          // Remove intrinsic delay to find load dependent delay.
          - corner_arc->intrinsicDelay();
        load_delays.emplace_back(i, load_delay);
        debugPrint(logger_, RSZ, "recover_power", 3, "{} load_delay = {}",
                   path_vertex->name(network_),
                   delayAsString(load_delay, sta_, 3));
      }
    }

    // Sort the delays for any specific path. This way we can pick the fastest
    // delay and downsize that cell to achieve our goal instead of messing with
    // too many cells.
    sort(load_delays.begin(), load_delays.end(),
         [](pair<int, Delay> pair1,
            pair<int, Delay> pair2) {
           return pair1.second > pair2.second
             || (pair1.second == pair2.second
                 && pair1.first < pair2.first);
         });
    for (const auto& [drvr_index, ignored] : load_delays) {
      PathRef *drvr_path = expanded.path(drvr_index);
      Vertex *drvr_vertex = drvr_path->vertex(sta_);
      const Pin *drvr_pin = drvr_vertex->pin();
      LibertyPort *drvr_port = network_->libertyPort(drvr_pin);
      LibertyCell *drvr_cell = drvr_port ? drvr_port->libertyCell() : nullptr;
      int fanout = this->fanout(drvr_vertex);
      debugPrint(logger_, RSZ, "recover_power", 3, "{} {} fanout = {}",
                 network_->pathName(drvr_pin),
                 drvr_cell ? drvr_cell->name() : "none",
                 fanout);
      if (downsizeDrvr(drvr_path, drvr_index, &expanded, true, path_slack)) {
        changed = true;
        break;
      }
    }
  }
  return changed;
}

bool
RecoverPower::downsizeDrvr(PathRef *drvr_path,
                        int drvr_index,
                        PathExpanded *expanded,
                        bool only_same_size_swap,
                        Slack path_slack)
{
  Pin *drvr_pin = drvr_path->pin(this);
  Instance *drvr = network_->instance(drvr_pin);
  const DcalcAnalysisPt *dcalc_ap = drvr_path->dcalcAnalysisPt(sta_);
  float load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap);
  int in_index = drvr_index - 1;
  PathRef *in_path = expanded->path(in_index);
  Pin *in_pin = in_path->pin(sta_);
  LibertyPort *in_port = network_->libertyPort(in_pin);
  if (!resizer_->dontTouch(drvr)) {
    float prev_drive;
    if (drvr_index >= 2) {
      int prev_drvr_index = drvr_index - 2;
      PathRef *prev_drvr_path = expanded->path(prev_drvr_index);
      Pin *prev_drvr_pin = prev_drvr_path->pin(sta_);
      prev_drive = 0.0;
      LibertyPort *prev_drvr_port = network_->libertyPort(prev_drvr_pin);
      if (prev_drvr_port) {
        prev_drive = prev_drvr_port->driveResistance();
      }
    }
    else {
      prev_drive = 0.0;
    }
    LibertyPort *drvr_port = network_->libertyPort(drvr_pin);
    LibertyCell *downsize = downsizeCell(in_port, drvr_port, load_cap,
                                         prev_drive, dcalc_ap,
                                         only_same_size_swap, path_slack);
    if (downsize != nullptr) {
      debugPrint(logger_, RSZ, "recover_power", 3, "resize {} {} -> {}",
                 network_->pathName(drvr_pin),
                 drvr_port->libertyCell()->name(),
                 downsize->name());
      if (resizer_->replaceCell(drvr, downsize, true)) {
        resize_count_++;
        return true;
      }
    }
  }
  return false;
}

bool
RecoverPower::meetsSizeCriteria(LibertyCell *cell, LibertyCell *equiv,
                               bool match_size)
{
    if (!match_size) {
      return true;
    }
    dbMaster* equivalent_cell = db_network_->staToDb(equiv);
    dbMaster* curr_cell = db_network_->staToDb(cell);
    if (equivalent_cell->getWidth() <= curr_cell->getWidth() &&
        equivalent_cell->getHeight() == curr_cell->getHeight()) {
        return true;
    }
    return false;
}

LibertyCell *
RecoverPower::downsizeCell(LibertyPort *in_port,
                           LibertyPort *drvr_port,
                           float load_cap,
                           float prev_drive,
                           const DcalcAnalysisPt *dcalc_ap,
                           bool match_size,
                           Slack path_slack)
{
  int lib_ap = dcalc_ap->libertyIndex();
  LibertyCell *cell = drvr_port->libertyCell();
  LibertyCellSeq *equiv_cells = sta_->equivCells(cell);
  constexpr double delay_margin = 1.5; // Prevent overly aggressive downsizing

  if (equiv_cells) {
    const char *in_port_name = in_port->name();
    const char *drvr_port_name = drvr_port->name();
    sort(equiv_cells,
         [=] (const LibertyCell *cell1,
              const LibertyCell *cell2) {
           LibertyPort *port1=cell1->findLibertyPort(drvr_port_name)->cornerPort(lib_ap);
           LibertyPort *port2=cell2->findLibertyPort(drvr_port_name)->cornerPort(lib_ap);
           float drive1 = port1->driveResistance();
           float drive2 = port2->driveResistance();
           ArcDelay intrinsic1 = port1->intrinsicDelay(this);
           ArcDelay intrinsic2 = port2->intrinsicDelay(this);
           return (std::tie(drive1, intrinsic2) <
		   std::tie(drive2, intrinsic1));
         });
    float drive = drvr_port->cornerPort(lib_ap)->driveResistance();
    float delay = resizer_->gateDelay(drvr_port, load_cap, resizer_->tgt_slew_dcalc_ap_)
      + prev_drive * in_port->cornerPort(lib_ap)->capacitance();

    float current_drive, current_delay;
    LibertyCell *best_cell = nullptr;
    for (LibertyCell *equiv : *equiv_cells) {
      LibertyCell *equiv_corner = equiv->cornerCell(lib_ap);
      LibertyPort *equiv_drvr = equiv_corner->findLibertyPort(drvr_port_name);
      LibertyPort *equiv_input = equiv_corner->findLibertyPort(in_port_name);
      current_drive = equiv_drvr->driveResistance();
      // Include delay of previous driver into equiv gate.
      current_delay = resizer_->gateDelay(equiv_drvr, load_cap, dcalc_ap)
        + prev_drive * equiv_input->capacitance();

      if (!resizer_->dontUse(equiv)
          && current_drive > drive
          && current_delay > delay
          && (current_delay-delay)* delay_margin < path_slack // add margin
          && meetsSizeCriteria(cell, equiv, match_size)) {
        best_cell = equiv;
        // printf("Swapping %s for %s\n", equiv->name(), cell->name());
      }
    }
    if (best_cell != nullptr) {
      return best_cell;
    }
  }
  return nullptr;
}

int
RecoverPower::fanout(Vertex *vertex)
{
  int fanout = 0;
  VertexOutEdgeIterator edge_iter(vertex, graph_);
  while (edge_iter.hasNext()) {
    edge_iter.next();
    fanout++;
  }
  return fanout;
}

}  // namespace rsz
