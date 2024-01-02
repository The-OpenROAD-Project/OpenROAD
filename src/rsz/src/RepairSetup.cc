/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
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

#include "RepairSetup.hh"
#include "rsz/Resizer.hh"

#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InputDrive.hh"
#include "sta/Liberty.hh"
#include "sta/Parasitics.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/PathVertex.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/TimingArc.hh"
#include "sta/Units.hh"

#include "utl/Logger.h"

namespace rsz {

using std::max;
using std::string;
using std::vector;
using std::pair;
using utl::RSZ;

using sta::VertexOutEdgeIterator;
using sta::Edge;
using sta::PathExpanded;
using sta::fuzzyEqual;
using sta::fuzzyLess;
using sta::fuzzyGreater;
using sta::InstancePinIterator;

RepairSetup::RepairSetup(Resizer* resizer)
    : logger_(nullptr),
      sta_(nullptr),
      db_network_(nullptr),
      resizer_(resizer),
      corner_(nullptr),
      drvr_port_(nullptr),
      resize_count_(0),
      inserted_buffer_count_(0),
      split_load_buffer_count_(0),
      rebuffer_net_count_(0),
      cloned_gate_count_(0),
      swap_pin_count_(0),
      min_(MinMax::min()),
      max_(MinMax::max())
{
}

void
RepairSetup::init()
{
  logger_ = resizer_->logger_;
  sta_ = resizer_->sta_;
  db_network_ = resizer_->db_network_;
  copyState(sta_);
}

void
RepairSetup::repairSetup(const float setup_slack_margin,
                         const double repair_tns_end_percent,
                         const int max_passes,
                         const bool verbose,
                         const bool skip_pin_swap,
                         const bool skip_gate_cloning)
{
  init();
  constexpr int digits = 3;
  inserted_buffer_count_ = 0;
  split_load_buffer_count_ = 0;
  resize_count_ = 0;
  cloned_gate_count_ = 0;
  resizer_->buffer_moved_into_core_ = false;

  // Sort failing endpoints by slack.
  const VertexSet *endpoints = sta_->endpoints();
  VertexSeq violating_ends;
  // logger_->setDebugLevel(RSZ, "repair_setup", 2);
  // Should check here whether we can figure out the clock domain for each
  // vertex. This may be the place where we can do some round robin fun to
  // individually control each clock domain instead of just fixating on fixing one.
  for (Vertex *end : *endpoints) {
    const Slack end_slack = sta_->vertexSlack(end, max_);
    if (end_slack < setup_slack_margin) {
      violating_ends.push_back(end);
    }
  }
  sort(violating_ends, [=](Vertex *end1, Vertex *end2) {
    return sta_->vertexSlack(end1, max_) < sta_->vertexSlack(end2, max_);
  });
  debugPrint(logger_, RSZ, "repair_setup", 1, "Violating endpoints {}/{} {}%",
             violating_ends.size(),
             endpoints->size(),
             int(violating_ends.size() / double(endpoints->size()) * 100));

  if (!violating_ends.empty()) {
    logger_->info(RSZ, 94, "Found {} endpoints with setup violations.",
                  violating_ends.size());
  } else {
    // nothing to repair
    logger_->metric("design__instance__count__setup_buffer", 0);
    return;
  }

  int end_index = 0;
  int max_end_count = violating_ends.size() * repair_tns_end_percent;
  // Always repair the worst endpoint, even if tns percent is zero.
  max_end_count = max(max_end_count, 1);
  swap_pin_inst_set_.clear(); // Make sure we do not swap the same pin twice.
  resizer_->incrementalParasiticsBegin();
  int print_iteration = 0;
  if (verbose) {
    printProgress(print_iteration, false, false);
  }
  for (Vertex *end : violating_ends) {
    resizer_->updateParasitics();
    sta_->findRequireds();
    Slack end_slack = sta_->vertexSlack(end, max_);
    Slack worst_slack;
    Vertex *worst_vertex;
    sta_->worstSlack(max_, worst_slack, worst_vertex);
    debugPrint(logger_, RSZ, "repair_setup", 1, "{} slack = {} worst_slack = {}",
               end->name(network_),
               delayAsString(end_slack, sta_, digits),
               delayAsString(worst_slack, sta_, digits));
    end_index++;
    debugPrint(logger_, RSZ, "repair_setup", 1, "Doing {} /{}", end_index, max_end_count);
    if (end_index > max_end_count) {
      break;
    }
    Slack prev_end_slack = end_slack;
    Slack prev_worst_slack = worst_slack;
    int pass = 1;
    int decreasing_slack_passes = 0;
    resizer_->journalBegin();
    while (pass <= max_passes) {
      print_iteration++;
      if (verbose) {
        printProgress(print_iteration, false, false);
      }

      if (end_slack > setup_slack_margin) {
        debugPrint(logger_, RSZ, "repair_setup", 2,
                   "Restoring best slack end slack {} worst slack {}",
                   delayAsString(prev_end_slack, sta_, digits),
                   delayAsString(prev_worst_slack, sta_, digits));
        resizer_->journalRestore(resize_count_, inserted_buffer_count_,
				 cloned_gate_count_);
        break;
      }
      PathRef end_path = sta_->vertexWorstSlackPath(end, max_);
      const bool changed = repairPath(end_path, end_slack, skip_pin_swap,
                                     skip_gate_cloning);
      if (!changed) {
        debugPrint(logger_, RSZ, "repair_setup", 2,
                   "No change after {} decreasing slack passes.",
                   decreasing_slack_passes);
        debugPrint(logger_, RSZ, "repair_setup", 2,
                   "Restoring best slack end slack {} worst slack {}",
                   delayAsString(prev_end_slack, sta_, digits),
                   delayAsString(prev_worst_slack, sta_, digits));
        resizer_->journalRestore(resize_count_, inserted_buffer_count_,
                                 cloned_gate_count_);
        break;
      }
      resizer_->updateParasitics();
      sta_->findRequireds();
      end_slack = sta_->vertexSlack(end, max_);
      sta_->worstSlack(max_, worst_slack, worst_vertex);
      const bool better = (fuzzyGreater(worst_slack, prev_worst_slack)
                           || (end_index != 1
                               && fuzzyEqual(worst_slack, prev_worst_slack)
                               && fuzzyGreater(end_slack, prev_end_slack)));
      debugPrint(logger_, RSZ,
                 "repair_setup", 2, "pass {} slack = {} worst_slack = {} {}",
                 pass,
                 delayAsString(end_slack, sta_, digits),
                 delayAsString(worst_slack, sta_, digits),
                 better ? "save" : "");
      if (better) {
        prev_end_slack = end_slack;
        prev_worst_slack = worst_slack;
        decreasing_slack_passes = 0;
        // Progress, Save checkpoint so we can back up to here.
        resizer_->journalBegin();
      }
      else {
        // Allow slack to increase to get out of local minima.
        // Do not update prev_end_slack so it saves the high water mark.
        decreasing_slack_passes++;
        if (decreasing_slack_passes > decreasing_slack_max_passes_) {
          // Undo changes that reduced slack.
          debugPrint(logger_, RSZ, "repair_setup", 2,
                     "decreasing slack for {} passes.",
                     decreasing_slack_passes);
          debugPrint(logger_, RSZ, "repair_setup", 2,
                     "Restoring best end slack {} worst slack {}",
                     delayAsString(prev_end_slack, sta_, digits),
                     delayAsString(prev_worst_slack, sta_, digits));
          resizer_->journalRestore(resize_count_,
                                   inserted_buffer_count_,
                                   cloned_gate_count_);
          break;
        }
      }

      if (resizer_->overMaxArea()) {
        break;
      }
      if (end_index == 1) {
        end = worst_vertex;
      }
      pass++;
    }
    if (verbose) {
      printProgress(print_iteration, true, false);
    }
  }
  if (verbose) {
    printProgress(print_iteration, true, true);
  }
  // Leave the parasitics up to date.
  resizer_->updateParasitics();
  resizer_->incrementalParasiticsEnd();

  if (inserted_buffer_count_ > 0 && split_load_buffer_count_ == 0) {
    logger_->info(RSZ, 40, "Inserted {} buffers.", inserted_buffer_count_);
  }
  else if (inserted_buffer_count_ > 0 && split_load_buffer_count_ > 0) {
        logger_->info(RSZ, 45, "Inserted {} buffers, {} to split loads.",
                          inserted_buffer_count_, split_load_buffer_count_);
  }
  logger_->metric("design__instance__count__setup_buffer", inserted_buffer_count_);
  if (resize_count_ > 0) {
    logger_->info(RSZ, 41, "Resized {} instances.", resize_count_);
  }
  if (swap_pin_count_ > 0) {
    logger_->info(RSZ, 43, "Swapped pins on {} instances.", swap_pin_count_);
  }
  if (cloned_gate_count_ > 0) {
    logger_->info(RSZ, 49, "Cloned {} instances.", cloned_gate_count_);
  }
  const Slack worst_slack = sta_->worstSlack(max_);
  if (fuzzyLess(worst_slack, setup_slack_margin)) {
    logger_->warn(RSZ, 62, "Unable to repair all setup violations.");
  }
  if (resizer_->overMaxArea()) {
    logger_->error(RSZ, 25, "max utilization reached.");
  }
}

// For testing.
void
RepairSetup::repairSetup(const Pin *end_pin)
{
  init();
  inserted_buffer_count_ = 0;
  resize_count_ = 0;
  swap_pin_count_ = 0;
  cloned_gate_count_ = 0;

  Vertex *vertex = graph_->pinLoadVertex(end_pin);
  const Slack slack = sta_->vertexSlack(vertex, max_);
  PathRef path = sta_->vertexWorstSlackPath(vertex, max_);
  resizer_->incrementalParasiticsBegin();
  repairPath(path, slack, false, false);
  // Leave the parasitices up to date.
  resizer_->updateParasitics();
  resizer_->incrementalParasiticsEnd();

  if (inserted_buffer_count_ > 0) {
    logger_->info(RSZ, 30, "Inserted {} buffers.", inserted_buffer_count_);
  }
  if (resize_count_ > 0) {
    logger_->info(RSZ, 31, "Resized {} instances.", resize_count_);
  }
  if (swap_pin_count_ > 0) { 
    logger_->info(RSZ, 44, "Swapped pins on {} instances.", swap_pin_count_);
  }
}

/* This is the main routine for repairing setup violations. We have
 - upsize driver (step 1)
 - rebuffer (step 2)
 - swap pin (step 3)
 - split loads
 And they are always done in the same order. Not clear whether
 this order is the best way at all times. Also need to worry about
 actually using global routes... 
 Things that can be added:
 - Intelligent rebuffering .... so if we added 2 buffers then maybe add
   two inverters instead.
 - pin swap (V0 is done) 
 - Logic cloning
 - VT swap (already there via the normal resize code.... but we need to
   figure out how to deal with min implant rules to make it production
   ready) 
 */
bool
RepairSetup::repairPath(PathRef &path,
                        const Slack path_slack,
                        const bool skip_pin_swap,
                        const bool skip_gate_cloning)
{
  PathExpanded expanded(&path, sta_);
  bool changed = false;

  if (expanded.size() > 1) {
    const int path_length = expanded.size();
    vector<pair<int, Delay>> load_delays;
    const int start_index = expanded.startIndex();
    const DcalcAnalysisPt *dcalc_ap = path.dcalcAnalysisPt(sta_);
    const int lib_ap = dcalc_ap->libertyIndex();
    // Find load delay for each gate in the path.
    for (int i = start_index; i < path_length; i++) {
      PathRef *path = expanded.path(i);
      Vertex *path_vertex = path->vertex(sta_);
      const Pin *path_pin = path->pin(sta_);
      if (i > 0
          && network_->isDriver(path_pin)
          && !network_->isTopLevelPort(path_pin)) {
        TimingArc *prev_arc = expanded.prevArc(i);
        const TimingArc *corner_arc = prev_arc->cornerArc(lib_ap);
        Edge *prev_edge = path->prevEdge(prev_arc, sta_);
        const Delay load_delay = graph_->arcDelay(prev_edge, prev_arc,
                                                  dcalc_ap->index())
          // Remove intrinsic delay to find load dependent delay.
          - corner_arc->intrinsicDelay();
        load_delays.emplace_back(i, load_delay);
        debugPrint(logger_, RSZ, "repair_setup", 3, "{} load_delay = {}",
                   path_vertex->name(network_),
                   delayAsString(load_delay, sta_, 3));
      }
    }

    sort(load_delays.begin(), load_delays.end(),
         [](pair<int, Delay> pair1,
            pair<int, Delay> pair2) {
           return pair1.second > pair2.second
             || (pair1.second == pair2.second
                 && pair1.first > pair2.first);
         });
    // Attack gates with largest load delays first.
    for (const auto& [drvr_index, ignored] : load_delays) {
      PathRef *drvr_path = expanded.path(drvr_index);
      Vertex *drvr_vertex = drvr_path->vertex(sta_);
      const Pin *drvr_pin = drvr_vertex->pin();
      const Net *net = network_->net(drvr_pin);
      LibertyPort *drvr_port = network_->libertyPort(drvr_pin);
      LibertyCell *drvr_cell = drvr_port ? drvr_port->libertyCell() : nullptr;
      const int fanout = this->fanout(drvr_vertex);
      debugPrint(logger_, RSZ, "repair_setup", 3, "{} {} fanout = {}",
                 network_->pathName(drvr_pin),
                 drvr_cell ? drvr_cell->name() : "none",
                 fanout);

      if (upsizeDrvr(drvr_path, drvr_index, &expanded)) {
        changed = true;
        break;
      }

      // Pin swapping 
      if (!skip_pin_swap) {
        if (swapPins(drvr_path, drvr_index, &expanded)) {
          changed = true;
          break;
        }
      }

      // For tristate nets all we can do is resize the driver.
      const bool tristate_drvr = resizer_->isTristateDriver(drvr_pin);
      dbNet* db_net = db_network_->staToDb(net);
      if (fanout > 1
          // Rebuffer blows up on large fanout nets.
          && fanout < rebuffer_max_fanout_
          && !tristate_drvr
          && !resizer_->dontTouch(net)
          && !db_net->isConnectedByAbutment()) {
        const int rebuffer_count = rebuffer(drvr_pin);
        if (rebuffer_count > 0) {
          debugPrint(logger_, RSZ, "repair_setup", 3, "rebuffer {} inserted {}",
                     network_->pathName(drvr_pin),
                     rebuffer_count);
          inserted_buffer_count_ += rebuffer_count;
          changed = true;
          break;
        }
      }

      // Gate cloning
      if (!skip_gate_cloning && fanout > split_load_min_fanout_ &&
          !tristate_drvr && !resizer_->dontTouch(net) &&
          resizer_->inserted_buffer_set_.find(db_network_->instance(drvr_pin)) == resizer_->inserted_buffer_set_.end() &&
          cloneDriver(drvr_path, drvr_index, path_slack, &expanded)) {
          changed = true;
          break;
      }
      
      // Don't split loads on low fanout nets.
      if (fanout > split_load_min_fanout_
          && !tristate_drvr
          && !resizer_->dontTouch(net)
          && !db_net->isConnectedByAbutment()) {
        const int init_buffer_count = inserted_buffer_count_;
        splitLoads(drvr_path, drvr_index, path_slack, &expanded);
        split_load_buffer_count_ = inserted_buffer_count_ - init_buffer_count;
        changed = true;
        break;
      }
    }
  }
  return changed;
}

void RepairSetup::debugCheckMultipleBuffers(PathRef &path,
                                            PathExpanded *expanded)
{
    if (expanded->size() > 1) {
        const int path_length = expanded->size();
        const int start_index = expanded->startIndex();
        for (int i = start_index; i < path_length; i++) {
            PathRef* path = expanded->path(i);
            const Pin* path_pin = path->pin(sta_);
            if (i > 0 && network_->isDriver(path_pin)
                && !network_->isTopLevelPort(path_pin)) {
                TimingArc* prev_arc = expanded->prevArc(i);
                printf("repair_setup %s: %s ---> %s \n",
                       prev_arc->from()->libertyCell()->name(),
                       prev_arc->from()->name(),
                       prev_arc->to()->name());
            }
        }
    }
    printf("done\n");
}

bool RepairSetup::swapPins(PathRef *drvr_path,
                           const int drvr_index,
                           PathExpanded *expanded)
{
    Pin *drvr_pin = drvr_path->pin(this);
    Instance *drvr = network_->instance(drvr_pin);
    const DcalcAnalysisPt *dcalc_ap = drvr_path->dcalcAnalysisPt(sta_);
    // int lib_ap = dcalc_ap->libertyIndex(); : check cornerPort
    const float load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap);
    const int in_index = drvr_index - 1;
    PathRef *in_path = expanded->path(in_index);
    Pin *in_pin = in_path->pin(sta_);


    if (!resizer_->dontTouch(drvr)) {
        // We get the driver port and the cell for that port.
        LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
        LibertyPort* input_port = network_->libertyPort(in_pin);
        LibertyCell* cell = drvr_port->libertyCell();
        LibertyPort *swap_port = input_port;
        sta::LibertyPortSet ports;

        // Results for > 2 input gates are unpredictable. Only swap pins for
        // 2 input gates for now.
        int input_port_count = 0;
        sta::LibertyCellPortIterator port_iter(cell);
        while (port_iter.hasNext()) {
            LibertyPort *port = port_iter.next();
            if (port->direction()->isInput()) {
                ++input_port_count;
            }
        }
        if (input_port_count > 2) {
            return false;
        }

        // Check if we have already dealt with this instance
        // and prevent any further swaps.
        if (swap_pin_inst_set_.find(drvr) == swap_pin_inst_set_.end()) {
            swap_pin_inst_set_.insert(drvr);
        }
        else {
            return false;
        }

        // Find the equivalent pins for a cell (simple implementation for now)
        // stash them
        if (equiv_pin_map_.find(cell) == equiv_pin_map_.end()) {
            equivCellPins(cell, ports);
            equiv_pin_map_.insert(cell, ports);
        }
        ports = equiv_pin_map_[cell];
        if (ports.size() > 1) {
            resizer_->findSwapPinCandidate(input_port, drvr_port, load_cap,
                                           dcalc_ap, &swap_port);
            if (!sta::LibertyPort::equiv(swap_port, input_port)) {
                debugPrint(logger_, RSZ, "repair_setup", 3,
                           "Swap {} ({}) {} {}",
                           network_->name(drvr), cell->name(),
                           input_port->name(), swap_port->name());
                resizer_->swapPins(drvr, input_port, swap_port, true);
                swap_pin_count_++;
                return true;
            }
        }
    }
    return false;
}

bool
RepairSetup::upsizeDrvr(PathRef *drvr_path,
                        const int drvr_index,
                        PathExpanded *expanded)
{
  Pin *drvr_pin = drvr_path->pin(this);
  Instance *drvr = network_->instance(drvr_pin);
  const DcalcAnalysisPt *dcalc_ap = drvr_path->dcalcAnalysisPt(sta_);
  const float load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap);
  const int in_index = drvr_index - 1;
  PathRef *in_path = expanded->path(in_index);
  Pin *in_pin = in_path->pin(sta_);
  LibertyPort *in_port = network_->libertyPort(in_pin);
  if (!resizer_->dontTouch(drvr) ||
      resizer_->cloned_inst_set_.find(drvr) != resizer_->cloned_inst_set_.end()) {
    float prev_drive;
    if (drvr_index >= 2) {
      const int prev_drvr_index = drvr_index - 2;
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
    LibertyCell *upsize = upsizeCell(in_port, drvr_port, load_cap,
                                     prev_drive, dcalc_ap);
    if (upsize) {
      debugPrint(logger_, RSZ, "repair_setup", 3, "resize {} {} -> {}",
                 network_->pathName(drvr_pin),
                 drvr_port->libertyCell()->name(),
                 upsize->name());
      if (!resizer_->dontTouch(drvr)
          && resizer_->replaceCell(drvr, upsize, true)) {
        resize_count_++;
        return true;
      }
    }
  }
  return false;
}

LibertyCell *
RepairSetup::upsizeCell(LibertyPort *in_port,
                        LibertyPort *drvr_port,
                        const float load_cap,
                        const float prev_drive,
                        const DcalcAnalysisPt *dcalc_ap)
{
  const int lib_ap = dcalc_ap->libertyIndex();
  LibertyCell *cell = drvr_port->libertyCell();
  LibertyCellSeq *equiv_cells = sta_->equivCells(cell);
  if (equiv_cells) {
    const char *in_port_name = in_port->name();
    const char *drvr_port_name = drvr_port->name();
    sort(equiv_cells,
         [=] (const LibertyCell *cell1,
              const LibertyCell *cell2) {
           LibertyPort *port1=cell1->findLibertyPort(drvr_port_name)->cornerPort(lib_ap);
           LibertyPort *port2=cell2->findLibertyPort(drvr_port_name)->cornerPort(lib_ap);
           const float drive1 = port1->driveResistance();
           const float drive2 = port2->driveResistance();
           const ArcDelay intrinsic1 = port1->intrinsicDelay(this);
           const ArcDelay intrinsic2 = port2->intrinsicDelay(this);
           return drive1 > drive2
             || ((drive1 == drive2
                  && intrinsic1 < intrinsic2)
                 || (intrinsic1 == intrinsic2
                     && port1->capacitance() < port2->capacitance()));
         });
    const float drive = drvr_port->cornerPort(lib_ap)->driveResistance();
    const float delay = resizer_->gateDelay(drvr_port, load_cap,
                                            resizer_->tgt_slew_dcalc_ap_)
      + prev_drive * in_port->cornerPort(lib_ap)->capacitance();

    for (LibertyCell *equiv : *equiv_cells) {
      LibertyCell *equiv_corner = equiv->cornerCell(lib_ap);
      LibertyPort *equiv_drvr = equiv_corner->findLibertyPort(drvr_port_name);
      LibertyPort *equiv_input = equiv_corner->findLibertyPort(in_port_name);
      const float equiv_drive = equiv_drvr->driveResistance();
      // Include delay of previous driver into equiv gate.
      const float equiv_delay = resizer_->gateDelay(equiv_drvr, load_cap,
                                                    dcalc_ap)
        + prev_drive * equiv_input->capacitance();
      if (!resizer_->dontUse(equiv)
          && equiv_drive < drive
          && equiv_delay < delay) {
        return equiv;
      }
    }
  }
  return nullptr;
}

Point RepairSetup::computeCloneGateLocation(const Pin *drvr_pin,
                              const vector<pair<Vertex*, Slack>> &fanout_slacks)
{
  int count(1); // driver_pin counts as one

  int centroid_x = db_network_->location(drvr_pin).getX();
  int centroid_y = db_network_->location(drvr_pin).getY();

  const int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; i++) {
    const pair<Vertex*, Slack>& fanout_slack = fanout_slacks[i];
    const Vertex *load_vertex = fanout_slack.first;
    const Pin *load_pin = load_vertex->pin();
    centroid_x += db_network_->location(load_pin).getX();
    centroid_y += db_network_->location(load_pin).getY();
    ++count;
  }
  return {centroid_x/count, centroid_y/count};
}

bool
RepairSetup::cloneDriver(PathRef* drvr_path,
                         const int drvr_index,
                         const Slack drvr_slack,
                         PathExpanded *expanded)
{
  Pin *drvr_pin = drvr_path->pin(this);
  PathRef *load_path = expanded->path(drvr_index + 1);
  Vertex *load_vertex = load_path->vertex(sta_);
  Pin *load_pin = load_vertex->pin();
  // Divide and conquer.
  debugPrint(logger_, RSZ, "repair_setup", 3, "clone driver {} -> {}",
             network_->pathName(drvr_pin),
             network_->pathName(load_pin));

  Vertex *drvr_vertex = drvr_path->vertex(sta_);
  const RiseFall *rf = drvr_path->transition(sta_);
  // Sort fanouts of the drvr on the critical path by slack margin
  // wrt the critical path slack.
  vector<pair<Vertex*, Slack>> fanout_slacks;
  VertexOutEdgeIterator edge_iter(drvr_vertex, graph_);
  while (edge_iter.hasNext()) {
    Edge *edge = edge_iter.next();
    Vertex *fanout_vertex = edge->to(graph_);
    const Slack fanout_slack = sta_->vertexSlack(fanout_vertex, rf, max_);
    const Slack slack_margin = fanout_slack - drvr_slack;
    debugPrint(logger_, RSZ, "repair_setup", 4, " fanin {} slack_margin = {}",
               network_->pathName(fanout_vertex->pin()),
               delayAsString(slack_margin, sta_, 3));
    fanout_slacks.emplace_back(fanout_vertex, slack_margin);
  }

  sort(fanout_slacks.begin(), fanout_slacks.end(),
       [=](const pair<Vertex*, Slack>& pair1,
           const pair<Vertex*, Slack>& pair2) {
         return (pair1.second > pair2.second
                 || (pair1.second == pair2.second
                     && network_->pathNameLess(pair1.first->pin(),
                                               pair2.first->pin())));
       });

  Instance *drvr_inst = db_network_->instance(drvr_pin);

  if (!resizer_->isSingleOutputCombinational(drvr_inst)) {
    return false;
  }

  const string buffer_name = resizer_->makeUniqueInstName("clone");
  Instance *parent = db_network_->topInstance();

  // This is the meat of the gate cloning code.
  // We need to downsize the current driver AND we need to insert another drive
  // that splits the load
  // For now we will defer the downsize to a later juncture.

  LibertyCell *original_cell = network_->libertyCell(drvr_inst);
  LibertyCell *clone_cell = resizer_->halfDrivingPowerCell(original_cell);

  if (clone_cell == nullptr) {
    clone_cell = original_cell;  // no clone available use original
  }

  Point drvr_loc = computeCloneGateLocation(drvr_pin, fanout_slacks);
  Instance *clone_inst = resizer_->journalCloneInstance(clone_cell, buffer_name.c_str(),
                                                        network_->instance(drvr_pin), parent, drvr_loc);

  cloned_gate_count_++;

  debugPrint(logger_, RSZ, "repair_setup", 3, "clone {} ({}) -> {} ({})",
             network_->pathName(drvr_pin), original_cell->name(),
             network_->pathName(clone_inst), clone_cell->name());


  Net *out_net = resizer_->makeUniqueNet();
  std::unique_ptr<InstancePinIterator> inst_pin_iter{network_->pinIterator(drvr_inst)};
  while (inst_pin_iter->hasNext()) {
    Pin *pin = inst_pin_iter->next();
    if (network_->direction(pin)->isInput()) {
      // Connect to all the inputs of the original cell.
      auto libPort = network_->libertyPort(pin); // get the liberty port of the original inst/pin
      auto net = network_->net(pin);
      sta_->connectPin(clone_inst, libPort, net);  // connect the same liberty port of the new instance
      resizer_->parasiticsInvalid(net);
    }
  }

  // Get the output pin
  Pin* clone_output_pin = nullptr;
  std::unique_ptr<InstancePinIterator> clone_pin_iter{network_->pinIterator(clone_inst)};
  while (clone_pin_iter->hasNext()) {
    Pin* pin = clone_pin_iter->next();
    // If output pin then cache for later use.
    if (network_->direction(pin)->isOutput()) {
      clone_output_pin = pin;
      break;
    }
  }
  // Connect to the new output net we just created
  auto *clone_output_port = network_->port(clone_output_pin);
  sta_->connectPin(clone_inst, clone_output_port, out_net);

  // Divide the list of pins in half and connect them to the new net we
  // created as part of gate cloning. Skip ports connected to the original net
  int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; i++) {
    pair<Vertex*, Slack> fanout_slack = fanout_slacks[i];
    Vertex *load_vertex = fanout_slack.first;
    Pin *load_pin = load_vertex->pin();
    // Leave ports connected to original net so verilog port names are preserved.
    if (!network_->isTopLevelPort(load_pin)) {
      auto *load_port = network_->port(load_pin);
      Instance *load = network_->instance(load_pin);
      sta_->disconnectPin(load_pin);
      sta_->connectPin(load, load_port, out_net);
    }
  }
  resizer_->parasiticsInvalid(out_net);
  return true;
}

void
RepairSetup::splitLoads(PathRef *drvr_path,
                        const int drvr_index,
                        const Slack drvr_slack,
                        PathExpanded *expanded)
{
  Pin *drvr_pin = drvr_path->pin(this);
  PathRef *load_path = expanded->path(drvr_index + 1);
  Vertex *load_vertex = load_path->vertex(sta_);
  Pin *load_pin = load_vertex->pin();
  // Divide and conquer.
  debugPrint(logger_, RSZ, "repair_setup", 3, "split loads {} -> {}",
             network_->pathName(drvr_pin),
             network_->pathName(load_pin));

  Vertex *drvr_vertex = drvr_path->vertex(sta_);
  const RiseFall *rf = drvr_path->transition(sta_);
  // Sort fanouts of the drvr on the critical path by slack margin
  // wrt the critical path slack.
  vector<pair<Vertex*, Slack>> fanout_slacks;
  VertexOutEdgeIterator edge_iter(drvr_vertex, graph_);
  while (edge_iter.hasNext()) {
    Edge *edge = edge_iter.next();
    // Watch out for problematic asap7 output->output timing arcs.
    if (edge->isWire()) {
      Vertex *fanout_vertex = edge->to(graph_);
      const Slack fanout_slack = sta_->vertexSlack(fanout_vertex, rf, max_);
      const Slack slack_margin = fanout_slack - drvr_slack;
      debugPrint(logger_, RSZ, "repair_setup", 4, " fanin {} slack_margin = {}",
                 network_->pathName(fanout_vertex->pin()),
                 delayAsString(slack_margin, sta_, 3));
      fanout_slacks.emplace_back(fanout_vertex, slack_margin);
    }
  }

  sort(fanout_slacks.begin(), fanout_slacks.end(),
       [=](const pair<Vertex*, Slack>& pair1,
           const pair<Vertex*, Slack>& pair2) {
         return (pair1.second > pair2.second
                 || (pair1.second == pair2.second
                     && network_->pathNameLess(pair1.first->pin(),
                                               pair2.first->pin())));
       });

  Net *net = network_->net(drvr_pin);
  const string buffer_name = resizer_->makeUniqueInstName("split");
  Instance *parent = db_network_->topInstance();
  LibertyCell *buffer_cell = resizer_->buffer_lowest_drive_;
  const Point drvr_loc = db_network_->location(drvr_pin);
  Instance *buffer = resizer_->makeBuffer(buffer_cell,
                                          buffer_name.c_str(),
                                          parent, drvr_loc);
  inserted_buffer_count_++;

  Net *out_net = resizer_->makeUniqueNet();
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);

  // Split the loads with extra slack to an inserted buffer.
  // before
  // drvr_pin -> net -> load_pins
  // after
  // drvr_pin -> net -> load_pins with low slack
  //                 -> buffer_in -> net -> rest of loads
  sta_->connectPin(buffer, input, net);
  resizer_->parasiticsInvalid(net);
  sta_->connectPin(buffer, output, out_net);
  const int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; i++) {
    pair<Vertex*, Slack> fanout_slack = fanout_slacks[i];
    Vertex *load_vertex = fanout_slack.first;
    Pin *load_pin = load_vertex->pin();
    // Leave ports connected to original net so verilog port names are preserved.
    if (!network_->isTopLevelPort(load_pin)) {
      LibertyPort *load_port = network_->libertyPort(load_pin);
      Instance *load = network_->instance(load_pin);

      sta_->disconnectPin(load_pin);
      sta_->connectPin(load, load_port, out_net);
    }
  }
  Pin *buffer_out_pin = network_->findPin(buffer, output);
  resizer_->resizeToTargetSlew(buffer_out_pin);
  resizer_->parasiticsInvalid(net);
  resizer_->parasiticsInvalid(out_net);
}

int
RepairSetup::fanout(Vertex *vertex)
{
  int fanout = 0;
  VertexOutEdgeIterator edge_iter(vertex, graph_);
  while (edge_iter.hasNext()) {
    edge_iter.next();
    fanout++;
  }
  return fanout;
}

void
RepairSetup::getEquivPortList2(sta::FuncExpr *expr,
                               sta::LibertyPortSet &ports,
                               sta::FuncExpr::Operator &status)
{
    using Operator = sta::FuncExpr::Operator ;
    const Operator curr_op = expr->op();

    if (curr_op == Operator::op_not) {
        getEquivPortList2(expr->left(), ports, status);
    }
    else if (status == Operator::op_zero &&
             (curr_op == Operator::op_and ||
              curr_op == Operator::op_or ||
              curr_op == Operator::op_xor)) {
        // Start parsing the equivalent pins (if it is simple or/and/xor)
        status = curr_op;
        getEquivPortList2(expr->left(), ports, status);
        if (status == Operator::op_port) {
          return;
        }
        getEquivPortList2(expr->right(), ports, status);
        if (status == Operator::op_port) {
          return;
        }
        status = Operator::op_one;
    }
    else if (status == curr_op) {
        // handle > 2 input scenarios (up to any arbitrary number)
        getEquivPortList2(expr->left(), ports, status);
        if (status == Operator::op_port) {
            return;
        }
        getEquivPortList2(expr->right(), ports, status);
        if (status == Operator::op_port) {
            return;
        }
    }
    else if (curr_op == Operator::op_port && expr->port() != nullptr) {
        ports.insert(expr->port());
    }
    else {
        status = Operator::op_port; // moved to some other operator.
        ports.clear();
    }
}

void
RepairSetup::getEquivPortList(sta::FuncExpr *expr, sta::LibertyPortSet &ports)
{
    sta::FuncExpr::Operator status = sta::FuncExpr::op_zero;
    ports.clear();
    getEquivPortList2(expr, ports, status);
    if (status == sta::FuncExpr::op_port) {
        ports.clear();
    }
}

// Lets just look at the first list for now.
// We may want to cache this information somwhere (by building it up for the whole
// library).
// Or just generate it when the cell is being created (depending on agreement).
void
RepairSetup::equivCellPins(const LibertyCell *cell, sta::LibertyPortSet &ports)
{
    sta::LibertyCellPortIterator port_iter(cell);
    unsigned outputs = 0;

    // count number of output ports. Skip ports with > 1 output for now.
    while (port_iter.hasNext()) {
        LibertyPort *port = port_iter.next();
        if (port->direction()->isOutput()) {
            ++outputs;
        }
    }

    if (outputs == 1) {
        sta::LibertyCellPortIterator port_iter2(cell);
        while (port_iter2.hasNext()) {
            LibertyPort *port = port_iter2.next();
            sta::FuncExpr *expr = port->function();
            if (expr != nullptr) {
                getEquivPortList(expr, ports);
            }
        }
    }
}

void
RepairSetup::printProgress(const int iteration,
                           const bool force,
                           const bool end) const
{
  const bool start = iteration == 0;

  if (start && !end) {
    logger_->report("Iteration | Resized | Buffers | Cloned Gates | Pin Swaps |   WNS   |   TNS   | Endpoint");
    logger_->report("---------------------------------------------------------------------------------------");
  }

  if (iteration % print_interval_ == 0 || force || end) {
    Slack wns;
    Vertex* worst_vertex;
    sta_->worstSlack(max_, wns, worst_vertex);
    const Slack tns = sta_->totalNegativeSlack(max_);

    std::string itr_field = fmt::format("{}", iteration);
    if (end) {
      itr_field = "final";
    }

    logger_->report("{: >9s} | {: >7d} | {: >7d} | {: >12d} | {: >9d} | {: >7s} | {: >7s} | {}",
                    itr_field,
                    resize_count_,
                    inserted_buffer_count_ + split_load_buffer_count_ + rebuffer_net_count_,
                    cloned_gate_count_,
                    swap_pin_count_,
                    delayAsString(wns, sta_, 3),
                    delayAsString(tns, sta_, 3),
                    worst_vertex != nullptr ? worst_vertex->name(network_) : "");
  }

  if (end) {
    logger_->report("---------------------------------------------------------------------------------------");
  }
}

}  // namespace rsz
