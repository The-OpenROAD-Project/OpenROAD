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

#include <sstream>

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
using std::pair;
using std::string;
using std::vector;
using utl::RSZ;

using sta::Edge;
using sta::fuzzyEqual;
using sta::fuzzyGreater;
using sta::fuzzyLess;
using sta::GraphDelayCalc;
using sta::InstancePinIterator;
using sta::NetConnectedPinIterator;
using sta::PathExpanded;
using sta::VertexOutEdgeIterator;

RepairSetup::RepairSetup(Resizer* resizer) : resizer_(resizer)
{
}

void RepairSetup::init()
{
  logger_ = resizer_->logger_;
  dbStaState::init(resizer_->sta_);
  db_network_ = resizer_->db_network_;
}

void RepairSetup::repairSetup(const float setup_slack_margin,
                              const double repair_tns_end_percent,
                              const int max_passes,
                              const bool verbose,
                              const bool skip_pin_swap,
                              const bool skip_gate_cloning,
                              const bool skip_buffer_removal)
{
  init();
  constexpr int digits = 3;
  inserted_buffer_count_ = 0;
  split_load_buffer_count_ = 0;
  resize_count_ = 0;
  cloned_gate_count_ = 0;
  removed_buffer_count_ = 0;
  resizer_->buffer_moved_into_core_ = false;

  // Sort failing endpoints by slack.
  const VertexSet* endpoints = sta_->endpoints();
  vector<pair<Vertex*, Slack>> violating_ends;
  // logger_->setDebugLevel(RSZ, "repair_setup", 2);
  // Should check here whether we can figure out the clock domain for each
  // vertex. This may be the place where we can do some round robin fun to
  // individually control each clock domain instead of just fixating on fixing
  // one.
  for (Vertex* end : *endpoints) {
    const Slack end_slack = sta_->vertexSlack(end, max_);
    if (end_slack < setup_slack_margin) {
      violating_ends.emplace_back(end, end_slack);
    }
  }
  std::stable_sort(violating_ends.begin(),
                   violating_ends.end(),
                   [](const auto& end_slack1, const auto& end_slack2) {
                     return end_slack1.second < end_slack2.second;
                   });
  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "Violating endpoints {}/{} {}%",
             violating_ends.size(),
             endpoints->size(),
             int(violating_ends.size() / double(endpoints->size()) * 100));

  if (!violating_ends.empty()) {
    logger_->info(RSZ,
                  94,
                  "Found {} endpoints with setup violations.",
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
  swap_pin_inst_set_.clear();  // Make sure we do not swap the same pin twice.

  // Ensure that max cap and max fanout violations don't get worse
  sta_->checkCapacitanceLimitPreamble();
  sta_->checkFanoutLimitPreamble();

  resizer_->incrementalParasiticsBegin();
  int print_iteration = 0;
  if (verbose) {
    printProgress(print_iteration, false, false);
  }
  for (const auto& end_original_slack : violating_ends) {
    Vertex* end = end_original_slack.first;
    Slack end_slack = sta_->vertexSlack(end, max_);
    Slack worst_slack;
    Vertex* worst_vertex;
    sta_->worstSlack(max_, worst_slack, worst_vertex);
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "{} slack = {} worst_slack = {}",
               end->name(network_),
               delayAsString(end_slack, sta_, digits),
               delayAsString(worst_slack, sta_, digits));
    end_index++;
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "Doing {} /{}",
               end_index,
               max_end_count);
    if (end_index > max_end_count) {
      // clang-format off
      debugPrint(logger_, RSZ, "repair_setup", 1, "{} end_index {} is larger than"
                 " max_end_count {}", end->name(network_), end_index,
                 max_end_count);
      // clang-format on
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
        if (pass != 1) {
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     2,
                     "Restoring best slack end slack {} worst slack {}",
                     delayAsString(prev_end_slack, sta_, digits),
                     delayAsString(prev_worst_slack, sta_, digits));
          resizer_->journalRestore(
              resize_count_, inserted_buffer_count_, cloned_gate_count_);
          resizer_->updateParasitics();
          sta_->findRequireds();
        }
        // clang-format off
        debugPrint(logger_, RSZ, "repair_setup", 1, "bailing out {} end_slack {} is larger than"
                   " setup_slack_margin {}", end->name(network_), end_index,
                   max_end_count);
        // clang-format on
        break;
      }
      PathRef end_path = sta_->vertexWorstSlackPath(end, max_);
      const bool changed = repairPath(end_path,
                                      end_slack,
                                      skip_pin_swap,
                                      skip_gate_cloning,
                                      skip_buffer_removal,
                                      setup_slack_margin);
      if (!changed) {
        if (pass != 1) {
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     2,
                     "No change after {} decreasing slack passes.",
                     decreasing_slack_passes);
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     2,
                     "Restoring best slack end slack {} worst slack {}",
                     delayAsString(prev_end_slack, sta_, digits),
                     delayAsString(prev_worst_slack, sta_, digits));
          resizer_->journalRestore(
              resize_count_, inserted_buffer_count_, cloned_gate_count_);
          resizer_->updateParasitics();
          sta_->findRequireds();
        }
        // clang-format off
        debugPrint(logger_, RSZ, "repair_setup", 1, "bailing out {} no changes"
                   " after {} decreasing passes", end->name(network_),
                   decreasing_slack_passes);
        // clang-format on
        break;
      }
      resizer_->updateParasitics();
      sta_->findRequireds();
      end_slack = sta_->vertexSlack(end, max_);
      sta_->worstSlack(max_, worst_slack, worst_vertex);
      const bool better
          = (fuzzyGreater(worst_slack, prev_worst_slack)
             || (end_index != 1 && fuzzyEqual(worst_slack, prev_worst_slack)
                 && fuzzyGreater(end_slack, prev_end_slack)));
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 2,
                 "pass {} slack = {} worst_slack = {} {}",
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
      } else {
        // Allow slack to increase to get out of local minima.
        // Do not update prev_end_slack so it saves the high water mark.
        decreasing_slack_passes++;
        if (decreasing_slack_passes > decreasing_slack_max_passes_) {
          // Undo changes that reduced slack.
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     2,
                     "decreasing slack for {} passes.",
                     decreasing_slack_passes);
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     2,
                     "Restoring best end slack {} worst slack {}",
                     delayAsString(prev_end_slack, sta_, digits),
                     delayAsString(prev_worst_slack, sta_, digits));
          resizer_->journalRestore(
              resize_count_, inserted_buffer_count_, cloned_gate_count_);
          resizer_->updateParasitics();
          sta_->findRequireds();
          // clang-format off
          debugPrint(logger_, RSZ, "repair_setup", 1, "bailing out {} decreasing"
                     " passes {} > decreasig pass limit {}", end->name(network_),
                     decreasing_slack_passes, decreasing_slack_max_passes_);
          // clang-format on
          break;
        }
      }

      if (resizer_->overMaxArea()) {
        // clang-format off
        debugPrint(logger_, RSZ, "repair_setup", 1, "bailing out {} resizer"
                   " over max area", end->name(network_));
        // clang-format on
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

  if (removed_buffer_count_ > 0) {
    logger_->info(RSZ, 59, "Removed {} buffers.", removed_buffer_count_);
  }
  if (inserted_buffer_count_ > 0 && split_load_buffer_count_ == 0) {
    logger_->info(RSZ, 40, "Inserted {} buffers.", inserted_buffer_count_);
  } else if (inserted_buffer_count_ > 0 && split_load_buffer_count_ > 0) {
    logger_->info(RSZ,
                  45,
                  "Inserted {} buffers, {} to split loads.",
                  inserted_buffer_count_,
                  split_load_buffer_count_);
  }
  logger_->metric("design__instance__count__setup_buffer",
                  inserted_buffer_count_);
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
void RepairSetup::repairSetup(const Pin* end_pin)
{
  init();
  inserted_buffer_count_ = 0;
  resize_count_ = 0;
  swap_pin_count_ = 0;
  cloned_gate_count_ = 0;
  removed_buffer_count_ = 0;

  Vertex* vertex = graph_->pinLoadVertex(end_pin);
  const Slack slack = sta_->vertexSlack(vertex, max_);
  PathRef path = sta_->vertexWorstSlackPath(vertex, max_);
  resizer_->incrementalParasiticsBegin();
  repairPath(path, slack, false, false, false, 0.0);
  // Leave the parasitices up to date.
  resizer_->updateParasitics();
  resizer_->incrementalParasiticsEnd();

  if (removed_buffer_count_ > 0) {
    logger_->info(RSZ, 61, "Removed {} buffers.", removed_buffer_count_);
  }
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
 - remove driver (step 1)
 - upsize driver (step 2)
 - rebuffer (step 3)
 - swap pin (step 4)
 - split loads (step 5)
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
bool RepairSetup::repairPath(PathRef& path,
                             const Slack path_slack,
                             const bool skip_pin_swap,
                             const bool skip_gate_cloning,
                             const bool skip_buffer_removal,
                             const float setup_slack_margin)
{
  PathExpanded expanded(&path, sta_);
  bool changed = false;

  if (expanded.size() > 1) {
    const int path_length = expanded.size();
    vector<pair<int, Delay>> load_delays;
    const int start_index = expanded.startIndex();
    const DcalcAnalysisPt* dcalc_ap = path.dcalcAnalysisPt(sta_);
    const int lib_ap = dcalc_ap->libertyIndex();
    // Find load delay for each gate in the path.
    for (int i = start_index; i < path_length; i++) {
      PathRef* path = expanded.path(i);
      Vertex* path_vertex = path->vertex(sta_);
      const Pin* path_pin = path->pin(sta_);
      if (i > 0 && network_->isDriver(path_pin)
          && !network_->isTopLevelPort(path_pin)) {
        TimingArc* prev_arc = expanded.prevArc(i);
        const TimingArc* corner_arc = prev_arc->cornerArc(lib_ap);
        Edge* prev_edge = path->prevEdge(prev_arc, sta_);
        const Delay load_delay
            = graph_->arcDelay(prev_edge, prev_arc, dcalc_ap->index())
              // Remove intrinsic delay to find load dependent delay.
              - corner_arc->intrinsicDelay();
        load_delays.emplace_back(i, load_delay);
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   3,
                   "{} load_delay = {} intrinsic_delay = {}",
                   path_vertex->name(network_),
                   delayAsString(load_delay, sta_, 3),
                   delayAsString(corner_arc->intrinsicDelay(), sta_, 3));
      }
    }

    sort(
        load_delays.begin(),
        load_delays.end(),
        [](pair<int, Delay> pair1, pair<int, Delay> pair2) {
          return pair1.second > pair2.second
                 || (pair1.second == pair2.second && pair1.first > pair2.first);
        });
    // Attack gates with largest load delays first.
    for (const auto& [drvr_index, ignored] : load_delays) {
      PathRef* drvr_path = expanded.path(drvr_index);
      Vertex* drvr_vertex = drvr_path->vertex(sta_);
      const Pin* drvr_pin = drvr_vertex->pin();
      const Net* net = network_->net(drvr_pin);
      LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
      LibertyCell* drvr_cell = drvr_port ? drvr_port->libertyCell() : nullptr;
      const int fanout = this->fanout(drvr_vertex);
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 3,
                 "{} {} fanout = {} drvr_index = {}",
                 network_->pathName(drvr_pin),
                 drvr_cell ? drvr_cell->name() : "none",
                 fanout,
                 drvr_index);

      if (!skip_buffer_removal) {
        if (removeDrvr(drvr_path,
                       drvr_cell,
                       drvr_index,
                       &expanded,
                       setup_slack_margin)) {
          changed = true;
          break;
        }
      }

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
          && fanout < rebuffer_max_fanout_ && !tristate_drvr
          && !resizer_->dontTouch(net) && !db_net->isConnectedByAbutment()) {
        const int rebuffer_count = rebuffer(drvr_pin);
        if (rebuffer_count > 0) {
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     3,
                     "rebuffer {} inserted {}",
                     network_->pathName(drvr_pin),
                     rebuffer_count);
          inserted_buffer_count_ += rebuffer_count;
          changed = true;
          break;
        }
      }

      // Gate cloning
      if (!skip_gate_cloning && fanout > split_load_min_fanout_
          && !tristate_drvr && !resizer_->dontTouch(net)
          && resizer_->inserted_buffer_set_.find(
                 db_network_->instance(drvr_pin))
                 == resizer_->inserted_buffer_set_.end()
          && cloneDriver(drvr_path, drvr_index, path_slack, &expanded)) {
        changed = true;
        break;
      }

      // Don't split loads on low fanout nets.
      if (fanout > split_load_min_fanout_ && !tristate_drvr
          && !resizer_->dontTouch(net) && !db_net->isConnectedByAbutment()) {
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

void RepairSetup::debugCheckMultipleBuffers(PathRef& path,
                                            PathExpanded* expanded)
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

bool RepairSetup::swapPins(PathRef* drvr_path,
                           const int drvr_index,
                           PathExpanded* expanded)
{
  Pin* drvr_pin = drvr_path->pin(this);
  // Skip if there is no liberty model or this is a single-input cell
  LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
  if (drvr_port == nullptr) {
    return false;
  }
  LibertyCell* cell = drvr_port->libertyCell();
  if (cell == nullptr) {
    return false;
  }
  if (cell->isBuffer() || cell->isInverter()) {
    return false;
  }
  Instance* drvr = network_->instance(drvr_pin);
  const DcalcAnalysisPt* dcalc_ap = drvr_path->dcalcAnalysisPt(sta_);
  // int lib_ap = dcalc_ap->libertyIndex(); : check cornerPort
  const float load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap);
  const int in_index = drvr_index - 1;
  PathRef* in_path = expanded->path(in_index);
  Pin* in_pin = in_path->pin(sta_);

  if (!resizer_->dontTouch(drvr)) {
    // We get the driver port and the cell for that port.
    LibertyPort* input_port = network_->libertyPort(in_pin);
    LibertyPort* swap_port = input_port;
    sta::LibertyPortSet ports;

    // Skip output to output paths
    if (input_port->direction()->isOutput()) {
      return false;
    }

    // Check if we have already dealt with this instance
    // and prevent any further swaps.
    if (swap_pin_inst_set_.find(drvr) == swap_pin_inst_set_.end()) {
      swap_pin_inst_set_.insert(drvr);
    } else {
      return false;
    }

    // Pass slews at input pins for more accurate delay/slew estimation
    resizer_->input_slew_map_.clear();
    std::unique_ptr<InstancePinIterator> inst_pin_iter{
        network_->pinIterator(drvr)};
    while (inst_pin_iter->hasNext()) {
      Pin* pin = inst_pin_iter->next();
      if (network_->direction(pin)->isInput()) {
        LibertyPort* port = network_->libertyPort(pin);
        if (port) {
          Vertex* vertex = graph_->pinDrvrVertex(pin);
          InputSlews slews;
          slews[RiseFall::rise()->index()]
              = sta_->vertexSlew(vertex, RiseFall::rise(), dcalc_ap);
          slews[RiseFall::fall()->index()]
              = sta_->vertexSlew(vertex, RiseFall::fall(), dcalc_ap);
          resizer_->input_slew_map_.emplace(port, slews);
        }
      }
    }

    // Find the equivalent pins for a cell (simple implementation for now)
    // stash them. Ports are unique to a cell so we can just cache by port
    // and that should apply to all instances of that cell with this input_port.
    if (equiv_pin_map_.find(input_port) == equiv_pin_map_.end()) {
      equivCellPins(cell, input_port, ports);
      equiv_pin_map_.insert(input_port, ports);
    }
    ports = equiv_pin_map_[input_port];
    if (!ports.empty()) {
      resizer_->findSwapPinCandidate(
          input_port, drvr_port, ports, load_cap, dcalc_ap, &swap_port);
      if (!sta::LibertyPort::equiv(swap_port, input_port)) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   3,
                   "Swap {} ({}) {} {}",
                   network_->name(drvr),
                   cell->name(),
                   input_port->name(),
                   swap_port->name());
        resizer_->swapPins(drvr, input_port, swap_port, true);
        swap_pin_count_++;
        return true;
      }
    }
  }
  return false;
}

// Remove driver if
// 1) it is a buffer without attributes like dont-touch
// 2) it doesn't create new max fanout violations
// 3) it doesn't create new max cap violations
// 4) it doesn't worsen slack
bool RepairSetup::removeDrvr(PathRef* drvr_path,
                             LibertyCell* drvr_cell,
                             const int drvr_index,
                             PathExpanded* expanded,
                             const float setup_slack_margin)
{
  // TODO:
  // 1. add max slew check
  if (drvr_cell && drvr_cell->isBuffer()) {
    Pin* drvr_pin = drvr_path->pin(this);
    Instance* drvr = network_->instance(drvr_pin);

    // Don't remove buffers from previous sizing, pin swapping, rebuffering, or
    // cloning because such removal may lead to an inifinte loop or long runtime
    std::string reason;
    if (resizer_->all_swapped_pin_inst_set_.count(drvr)) {
      reason = "its pins have been swapped";
    } else if (resizer_->all_cloned_inst_set_.count(drvr)) {
      reason = "it has been cloned";
    } else if (resizer_->all_inserted_buffer_set_.count(drvr)) {
      reason = "it was from rebuffering";
    } else if (resizer_->all_sized_inst_set_.count(drvr)) {
      reason = "it has been resized";
    }
    if (!reason.empty()) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 4,
                 "buffer {} is not removed because {}",
                 db_network_->name(drvr),
                 reason);
      return false;
    }

    // Don't remove buffer if new max fanout violations are created
    Vertex* drvr_vertex = drvr_path->vertex(sta_);
    PathRef* prev_drvr_path = expanded->path(drvr_index - 2);
    Vertex* prev_drvr_vertex = prev_drvr_path->vertex(sta_);
    Pin* prev_drvr_pin = prev_drvr_vertex->pin();
    float curr_fanout, max_fanout, fanout_slack;
    sta_->checkFanout(
        prev_drvr_pin, max_, curr_fanout, max_fanout, fanout_slack);
    float new_fanout = curr_fanout + fanout(drvr_vertex) - 1;
    if (max_fanout > 0.0) {
      // Honor max fanout when the constraint exists
      if (new_fanout > max_fanout) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "buffer {} is not removed because of max fanout limit "
                   "of {} at {}",
                   db_network_->name(drvr),
                   max_fanout,
                   network_->pathName(prev_drvr_pin));
        return false;
      }
    } else {
      // No max fanout exists, but don't exceed default fanout limit
      if (new_fanout > buffer_removal_max_fanout_) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "buffer {} is not removed because of default fanout "
                   "limit of {} at "
                   "{}",
                   db_network_->name(drvr),
                   buffer_removal_max_fanout_,
                   network_->pathName(prev_drvr_pin));
        return false;
      }
    }

    // Watch out for new max cap violations
    float cap, max_cap, cap_slack;
    const Corner* corner;
    const RiseFall* tr;
    sta_->checkCapacitance(prev_drvr_pin,
                           nullptr /* corner */,
                           max_,
                           // return values
                           corner,
                           tr,
                           cap,
                           max_cap,
                           cap_slack);
    if (max_cap > 0.0 && corner) {
      const DcalcAnalysisPt* dcalc_ap = corner->findDcalcAnalysisPt(max_);
      GraphDelayCalc* dcalc = sta_->graphDelayCalc();
      float drvr_cap = dcalc->loadCap(drvr_pin, dcalc_ap);
      LibertyPort *buffer_input_port, *buffer_output_port;
      drvr_cell->bufferPorts(buffer_input_port, buffer_output_port);
      float new_cap = cap + drvr_cap
                      - resizer_->portCapacitance(buffer_input_port, corner);
      if (new_cap > max_cap) {
        debugPrint(
            logger_,
            RSZ,
            "repair_setup",
            2,
            "buffer {} is not removed because of max cap limit of {} at {}",
            db_network_->name(drvr),
            max_cap,
            network_->pathName(prev_drvr_pin));
        return false;
      }
    }

    PathRef* drvr_input_path = expanded->path(drvr_index - 1);
    Vertex* drvr_input_vertex = drvr_input_path->vertex(sta_);
    SlackEstimatorParams params(setup_slack_margin, corner);
    params.driver_pin = drvr_pin;
    params.prev_driver_pin = prev_drvr_pin;
    params.driver_input_pin = drvr_input_vertex->pin();
    params.driver = drvr;
    params.driver_path = drvr_path;
    params.prev_driver_path = prev_drvr_path;
    params.driver_cell = drvr_cell;
    if (!estimatedSlackOK(params)) {
      return false;
    }

    if (resizer_->removeBuffer(drvr, /* honorDontTouch */ true)) {
      removed_buffer_count_++;
      return true;
    }
  }

  return false;
}

// Estimate slack impact from driver removal.
// Delay improvement from removed driver should be greater than
// delay degradation from prev driver for driver input pin path.
// Side input paths should absorb delay and slew degradation from prev driver.
// Delay degradation for side input paths comes from two sources:
// 1) delay degradation at prev driver due to increased load cap
// 2) delay degradation at side out pin due to degraded slew from prev driver
//
//  prev_drv_pin ------>  (drvr_input_pin   drvr_pin)  ------>
//               |
//               ------>  (side_input_pin1  side_out_pin1) ----->
//               |
//               ------>  (side_input_pin2  side_out_pin2) ----->
//
bool RepairSetup::estimatedSlackOK(const SlackEstimatorParams& params)
{
  if (params.corner == nullptr) {
    // can't do any estimation without a corner
    return false;
  }

  // Prep for delay calc
  GraphDelayCalc* dcalc = sta_->graphDelayCalc();
  const DcalcAnalysisPt* dcalc_ap = params.corner->findDcalcAnalysisPt(max_);
  LibertyPort* prev_drvr_port = network_->libertyPort(params.prev_driver_pin);
  if (prev_drvr_port == nullptr) {
    return false;
  }
  LibertyPort *buffer_input_port, *buffer_output_port;
  params.driver_cell->bufferPorts(buffer_input_port, buffer_output_port);
  const RiseFall* prev_driver_rf = params.prev_driver_path->transition(sta_);

  // Compute delay degradation at prev driver due to increased load cap
  // TODO: use actual slew at input pins instead of fixed slew
  ArcDelay old_delay[RiseFall::index_count], new_delay[RiseFall::index_count];
  Slew old_slew[RiseFall::index_count], new_slew[RiseFall::index_count];
  float old_cap = dcalc->loadCap(params.prev_driver_pin, dcalc_ap);
  resizer_->gateDelays(prev_drvr_port, old_cap, dcalc_ap, old_delay, old_slew);
  float new_cap = old_cap + dcalc->loadCap(params.driver_pin, dcalc_ap)
                  - resizer_->portCapacitance(buffer_input_port, params.corner);
  resizer_->gateDelays(prev_drvr_port, new_cap, dcalc_ap, new_delay, new_slew);
  float delay_degrad
      = new_delay[prev_driver_rf->index()] - old_delay[prev_driver_rf->index()];
  float delay_imp
      = resizer_->bufferDelay(params.driver_cell,
                              params.driver_path->transition(sta_),
                              dcalc->loadCap(params.driver_pin, dcalc_ap),
                              dcalc_ap);

  Net* prev_net = network_->net(params.prev_driver_pin);
  NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(prev_net);
  while (pin_iter->hasNext()) {
    const Pin* side_input_pin = pin_iter->next();
    if (side_input_pin == params.prev_driver_pin) {
      continue;
    }
    // Check if degraded delay can be absorbed
    if (side_input_pin == params.driver_input_pin) {
      if (delay_imp < delay_degrad) {
        // clang-format off
        debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} is not removed "
                   "because delay degradation of {} at previous driver {} "
                   "is greater than delay improvement of {}",
                   db_network_->name(params.driver), delay_degrad,
                   db_network_->name(params.prev_driver_pin), delay_imp);
        // clang-format on
        return false;
      }
      // clang-format off
      debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} can be removed "
                 "because delay degradation of {} at previous driver {} "
                 "is less than delay improvement of {}",
                 db_network_->name(params.driver), delay_degrad,
                 db_network_->name(params.prev_driver_pin), delay_imp);
      // clang-format on
    } else {
      // side input pin is not driver input pin
      float old_slack = sta_->pinSlack(side_input_pin, max_);
      float new_slack = old_slack - params.setup_slack_margin - delay_degrad;
      if (new_slack < 0) {
        // clang-format off
        debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} is not removed "
                   "because side input pin {} will have a violating slack of {}:"
                   " old slack={}, slack margin={}, delay_degrad={}",
                   db_network_->name(params.driver),
                   db_network_->name(side_input_pin), new_slack, old_slack, 
                   params.setup_slack_margin, delay_degrad);
        // clang-format on
        return false;
      }
      // clang-format off
      debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} can be removed "
                 "because side input pin {} will have a positive slack of {}:"
                 " old slack={}, slack margin={}, delay_degrad={}",
                 db_network_->name(params.driver),
                 db_network_->name(side_input_pin), new_slack, old_slack, 
                 params.setup_slack_margin, delay_degrad);
      // clang-format on

      // Consider secondary degradation at side out pin due to degraded input
      // slew. Include all output pins in case of multi-output gate (MOG).
      Instance* side_inst = network_->instance(side_input_pin);
      InstancePinIterator* side_pin_iter = network_->pinIterator(side_inst);
      while (side_pin_iter->hasNext()) {
        const Pin* side_out_pin = side_pin_iter->next();
        if (!network_->direction(side_out_pin)->isOutput()) {
          continue;
        }
        LibertyPort* side_out_port = network_->libertyPort(side_out_pin);
        if (side_out_port == nullptr) {
          return false;
        }
        float side_load_cap = dcalc->loadCap(side_out_pin, dcalc_ap);
        ArcDelay old_delay2[RiseFall::index_count],
            new_delay2[RiseFall::index_count];
        Slew old_slew2[RiseFall::index_count], new_slew2[RiseFall::index_count];
        resizer_->gateDelays(side_out_port,
                             side_load_cap,
                             old_slew,  // old input slew from prev_driver
                             dcalc_ap,
                             old_delay2,
                             old_slew2);
        resizer_->gateDelays(side_out_port,
                             side_load_cap,
                             new_slew,  // new input slew from prev_driver
                             dcalc_ap,
                             new_delay2,
                             new_slew2);
        float delay_diff = max(new_delay2[RiseFall::riseIndex()]
                                   - old_delay2[RiseFall::riseIndex()],
                               new_delay2[RiseFall::fallIndex()]
                                   - old_delay2[RiseFall::fallIndex()]);
        new_slack -= delay_diff;
        if (new_slack < 0) {
          // clang-format off
          debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} is not removed"
                     "because side output pin {} will have a violating slack of "
                     "{}: old slack={}, slack margin={}, delay_degrad={}",
                     db_network_->name(params.driver),
                     db_network_->name(side_out_pin), new_slack, old_slack, 
                     params.setup_slack_margin, delay_degrad + delay_diff);
          // clang-format on
          return false;
        }
        // clang-format off
        debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} can be removed"
                   "because side output pin {} will have a positive slack of "
                   "{}: old slack={}, slack margin={}, delay_degrad={}",
                   db_network_->name(params.driver),
                   db_network_->name(side_out_pin), new_slack, old_slack, 
                   params.setup_slack_margin, delay_degrad + delay_diff);
        // clang-format on
      }  // for each side_out_pin of side inst
    }
  }  // for each side_input_pin of prev_net

  return true;
}

bool RepairSetup::upsizeDrvr(PathRef* drvr_path,
                             const int drvr_index,
                             PathExpanded* expanded)
{
  Pin* drvr_pin = drvr_path->pin(this);
  Instance* drvr = network_->instance(drvr_pin);
  const DcalcAnalysisPt* dcalc_ap = drvr_path->dcalcAnalysisPt(sta_);
  const float load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap);
  const int in_index = drvr_index - 1;
  PathRef* in_path = expanded->path(in_index);
  Pin* in_pin = in_path->pin(sta_);
  LibertyPort* in_port = network_->libertyPort(in_pin);
  if (!resizer_->dontTouch(drvr)
      || resizer_->cloned_inst_set_.find(drvr)
             != resizer_->cloned_inst_set_.end()) {
    float prev_drive;
    if (drvr_index >= 2) {
      const int prev_drvr_index = drvr_index - 2;
      PathRef* prev_drvr_path = expanded->path(prev_drvr_index);
      Pin* prev_drvr_pin = prev_drvr_path->pin(sta_);
      prev_drive = 0.0;
      LibertyPort* prev_drvr_port = network_->libertyPort(prev_drvr_pin);
      if (prev_drvr_port) {
        prev_drive = prev_drvr_port->driveResistance();
      }
    } else {
      prev_drive = 0.0;
    }
    LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
    LibertyCell* upsize
        = upsizeCell(in_port, drvr_port, load_cap, prev_drive, dcalc_ap);
    if (upsize) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 3,
                 "resize {} {} -> {}",
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

LibertyCell* RepairSetup::upsizeCell(LibertyPort* in_port,
                                     LibertyPort* drvr_port,
                                     const float load_cap,
                                     const float prev_drive,
                                     const DcalcAnalysisPt* dcalc_ap)
{
  const int lib_ap = dcalc_ap->libertyIndex();
  LibertyCell* cell = drvr_port->libertyCell();
  LibertyCellSeq* equiv_cells = sta_->equivCells(cell);
  if (equiv_cells) {
    const char* in_port_name = in_port->name();
    const char* drvr_port_name = drvr_port->name();
    sort(equiv_cells, [=](const LibertyCell* cell1, const LibertyCell* cell2) {
      LibertyPort* port1
          = cell1->findLibertyPort(drvr_port_name)->cornerPort(lib_ap);
      LibertyPort* port2
          = cell2->findLibertyPort(drvr_port_name)->cornerPort(lib_ap);
      const float drive1 = port1->driveResistance();
      const float drive2 = port2->driveResistance();
      const ArcDelay intrinsic1 = port1->intrinsicDelay(this);
      const ArcDelay intrinsic2 = port2->intrinsicDelay(this);
      return drive1 > drive2
             || ((drive1 == drive2 && intrinsic1 < intrinsic2)
                 || (intrinsic1 == intrinsic2
                     && port1->capacitance() < port2->capacitance()));
    });
    const float drive = drvr_port->cornerPort(lib_ap)->driveResistance();
    const float delay
        = resizer_->gateDelay(drvr_port, load_cap, resizer_->tgt_slew_dcalc_ap_)
          + prev_drive * in_port->cornerPort(lib_ap)->capacitance();

    for (LibertyCell* equiv : *equiv_cells) {
      LibertyCell* equiv_corner = equiv->cornerCell(lib_ap);
      LibertyPort* equiv_drvr = equiv_corner->findLibertyPort(drvr_port_name);
      LibertyPort* equiv_input = equiv_corner->findLibertyPort(in_port_name);
      const float equiv_drive = equiv_drvr->driveResistance();
      // Include delay of previous driver into equiv gate.
      const float equiv_delay
          = resizer_->gateDelay(equiv_drvr, load_cap, dcalc_ap)
            + prev_drive * equiv_input->capacitance();
      if (!resizer_->dontUse(equiv) && equiv_drive < drive
          && equiv_delay < delay) {
        return equiv;
      }
    }
  }
  return nullptr;
}

Point RepairSetup::computeCloneGateLocation(
    const Pin* drvr_pin,
    const vector<pair<Vertex*, Slack>>& fanout_slacks)
{
  int count(1);  // driver_pin counts as one

  int centroid_x = db_network_->location(drvr_pin).getX();
  int centroid_y = db_network_->location(drvr_pin).getY();

  const int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; i++) {
    const pair<Vertex*, Slack>& fanout_slack = fanout_slacks[i];
    const Vertex* load_vertex = fanout_slack.first;
    const Pin* load_pin = load_vertex->pin();
    centroid_x += db_network_->location(load_pin).getX();
    centroid_y += db_network_->location(load_pin).getY();
    ++count;
  }
  return {centroid_x / count, centroid_y / count};
}

bool RepairSetup::cloneDriver(PathRef* drvr_path,
                              const int drvr_index,
                              const Slack drvr_slack,
                              PathExpanded* expanded)
{
  Pin* drvr_pin = drvr_path->pin(this);
  PathRef* load_path = expanded->path(drvr_index + 1);
  Vertex* load_vertex = load_path->vertex(sta_);
  Pin* load_pin = load_vertex->pin();
  // Divide and conquer.
  debugPrint(logger_,
             RSZ,
             "repair_setup",
             3,
             "clone driver {} -> {}",
             network_->pathName(drvr_pin),
             network_->pathName(load_pin));

  Vertex* drvr_vertex = drvr_path->vertex(sta_);
  const RiseFall* rf = drvr_path->transition(sta_);
  // Sort fanouts of the drvr on the critical path by slack margin
  // wrt the critical path slack.
  vector<pair<Vertex*, Slack>> fanout_slacks;
  VertexOutEdgeIterator edge_iter(drvr_vertex, graph_);
  while (edge_iter.hasNext()) {
    Edge* edge = edge_iter.next();
    Vertex* fanout_vertex = edge->to(graph_);
    const Slack fanout_slack = sta_->vertexSlack(fanout_vertex, rf, max_);
    const Slack slack_margin = fanout_slack - drvr_slack;
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               4,
               " fanin {} slack_margin = {}",
               network_->pathName(fanout_vertex->pin()),
               delayAsString(slack_margin, sta_, 3));
    fanout_slacks.emplace_back(fanout_vertex, slack_margin);
  }

  sort(fanout_slacks.begin(),
       fanout_slacks.end(),
       [=](const pair<Vertex*, Slack>& pair1,
           const pair<Vertex*, Slack>& pair2) {
         return (pair1.second > pair2.second
                 || (pair1.second == pair2.second
                     && network_->pathNameLess(pair1.first->pin(),
                                               pair2.first->pin())));
       });

  Instance* drvr_inst = db_network_->instance(drvr_pin);

  if (!resizer_->isSingleOutputCombinational(drvr_inst)) {
    return false;
  }

  const string buffer_name = resizer_->makeUniqueInstName("clone");
  Instance* parent = db_network_->topInstance();

  // This is the meat of the gate cloning code.
  // We need to downsize the current driver AND we need to insert another drive
  // that splits the load
  // For now we will defer the downsize to a later juncture.

  LibertyCell* original_cell = network_->libertyCell(drvr_inst);
  LibertyCell* clone_cell = resizer_->halfDrivingPowerCell(original_cell);

  if (clone_cell == nullptr) {
    clone_cell = original_cell;  // no clone available use original
  }

  Point drvr_loc = computeCloneGateLocation(drvr_pin, fanout_slacks);
  Instance* clone_inst
      = resizer_->journalCloneInstance(clone_cell,
                                       buffer_name.c_str(),
                                       network_->instance(drvr_pin),
                                       parent,
                                       drvr_loc);

  cloned_gate_count_++;

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             3,
             "clone {} ({}) -> {} ({})",
             network_->pathName(drvr_pin),
             original_cell->name(),
             network_->pathName(clone_inst),
             clone_cell->name());

  Net* out_net = resizer_->makeUniqueNet();
  std::unique_ptr<InstancePinIterator> inst_pin_iter{
      network_->pinIterator(drvr_inst)};
  while (inst_pin_iter->hasNext()) {
    Pin* pin = inst_pin_iter->next();
    if (network_->direction(pin)->isInput()) {
      // Connect to all the inputs of the original cell.
      auto libPort = network_->libertyPort(
          pin);  // get the liberty port of the original inst/pin
      auto net = network_->net(pin);
      sta_->connectPin(
          clone_inst,
          libPort,
          net);  // connect the same liberty port of the new instance
      resizer_->parasiticsInvalid(net);
    }
  }

  // Get the output pin
  Pin* clone_output_pin = nullptr;
  std::unique_ptr<InstancePinIterator> clone_pin_iter{
      network_->pinIterator(clone_inst)};
  while (clone_pin_iter->hasNext()) {
    Pin* pin = clone_pin_iter->next();
    // If output pin then cache for later use.
    if (network_->direction(pin)->isOutput()) {
      clone_output_pin = pin;
      break;
    }
  }
  // Connect to the new output net we just created
  auto* clone_output_port = network_->port(clone_output_pin);
  sta_->connectPin(clone_inst, clone_output_port, out_net);

  // Divide the list of pins in half and connect them to the new net we
  // created as part of gate cloning. Skip ports connected to the original net
  int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; i++) {
    pair<Vertex*, Slack> fanout_slack = fanout_slacks[i];
    Vertex* load_vertex = fanout_slack.first;
    Pin* load_pin = load_vertex->pin();
    // Leave ports connected to original net so verilog port names are
    // preserved.
    if (!network_->isTopLevelPort(load_pin)) {
      auto* load_port = network_->port(load_pin);
      Instance* load = network_->instance(load_pin);
      sta_->disconnectPin(load_pin);
      sta_->connectPin(load, load_port, out_net);
    }
  }
  resizer_->parasiticsInvalid(out_net);
  return true;
}

void RepairSetup::splitLoads(PathRef* drvr_path,
                             const int drvr_index,
                             const Slack drvr_slack,
                             PathExpanded* expanded)
{
  Pin* drvr_pin = drvr_path->pin(this);
  PathRef* load_path = expanded->path(drvr_index + 1);
  Vertex* load_vertex = load_path->vertex(sta_);
  Pin* load_pin = load_vertex->pin();
  // Divide and conquer.
  debugPrint(logger_,
             RSZ,
             "repair_setup",
             3,
             "split loads {} -> {}",
             network_->pathName(drvr_pin),
             network_->pathName(load_pin));

  Vertex* drvr_vertex = drvr_path->vertex(sta_);
  const RiseFall* rf = drvr_path->transition(sta_);
  // Sort fanouts of the drvr on the critical path by slack margin
  // wrt the critical path slack.
  vector<pair<Vertex*, Slack>> fanout_slacks;
  VertexOutEdgeIterator edge_iter(drvr_vertex, graph_);
  while (edge_iter.hasNext()) {
    Edge* edge = edge_iter.next();
    // Watch out for problematic asap7 output->output timing arcs.
    if (edge->isWire()) {
      Vertex* fanout_vertex = edge->to(graph_);
      const Slack fanout_slack = sta_->vertexSlack(fanout_vertex, rf, max_);
      const Slack slack_margin = fanout_slack - drvr_slack;
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 4,
                 " fanin {} slack_margin = {}",
                 network_->pathName(fanout_vertex->pin()),
                 delayAsString(slack_margin, sta_, 3));
      fanout_slacks.emplace_back(fanout_vertex, slack_margin);
    }
  }

  sort(fanout_slacks.begin(),
       fanout_slacks.end(),
       [=](const pair<Vertex*, Slack>& pair1,
           const pair<Vertex*, Slack>& pair2) {
         return (pair1.second > pair2.second
                 || (pair1.second == pair2.second
                     && network_->pathNameLess(pair1.first->pin(),
                                               pair2.first->pin())));
       });

  Net* net = network_->net(drvr_pin);
  const string buffer_name = resizer_->makeUniqueInstName("split");
  Instance* parent = db_network_->topInstance();
  LibertyCell* buffer_cell = resizer_->buffer_lowest_drive_;
  const Point drvr_loc = db_network_->location(drvr_pin);
  Instance* buffer = resizer_->makeBuffer(
      buffer_cell, buffer_name.c_str(), parent, drvr_loc);
  inserted_buffer_count_++;

  Net* out_net = resizer_->makeUniqueNet();
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
    Vertex* load_vertex = fanout_slack.first;
    Pin* load_pin = load_vertex->pin();
    // Leave ports connected to original net so verilog port names are
    // preserved.
    if (!network_->isTopLevelPort(load_pin)) {
      LibertyPort* load_port = network_->libertyPort(load_pin);
      Instance* load = network_->instance(load_pin);

      sta_->disconnectPin(load_pin);
      sta_->connectPin(load, load_port, out_net);
    }
  }
  Pin* buffer_out_pin = network_->findPin(buffer, output);
  resizer_->resizeToTargetSlew(buffer_out_pin);
  resizer_->parasiticsInvalid(net);
  resizer_->parasiticsInvalid(out_net);
}

int RepairSetup::fanout(Vertex* vertex)
{
  int fanout = 0;
  VertexOutEdgeIterator edge_iter(vertex, graph_);
  while (edge_iter.hasNext()) {
    edge_iter.next();
    fanout++;
  }
  return fanout;
}

bool RepairSetup::simulateExpr(
    sta::FuncExpr* expr,
    sta::UnorderedMap<const LibertyPort*, std::vector<bool>>& port_stimulus,
    size_t table_index)
{
  using Operator = sta::FuncExpr::Operator;
  const Operator curr_op = expr->op();

  switch (curr_op) {
    case Operator::op_not:
      return !simulateExpr(expr->left(), port_stimulus, table_index);
    case Operator::op_and:
      return simulateExpr(expr->left(), port_stimulus, table_index)
             && simulateExpr(expr->right(), port_stimulus, table_index);
    case Operator::op_or:
      return simulateExpr(expr->left(), port_stimulus, table_index)
             || simulateExpr(expr->right(), port_stimulus, table_index);
    case Operator::op_xor:
      return simulateExpr(expr->left(), port_stimulus, table_index)
             ^ simulateExpr(expr->right(), port_stimulus, table_index);
    case Operator::op_one:
      return true;
    case Operator::op_zero:
      return false;
    case Operator::op_port:
      return port_stimulus[expr->port()][table_index];
  }

  logger_->error(RSZ, 91, "unrecognized expr op from OpenSTA");
}

std::vector<bool> RepairSetup::simulateExpr(
    sta::FuncExpr* expr,
    sta::UnorderedMap<const LibertyPort*, std::vector<bool>>& port_stimulus)
{
  size_t table_length = 0x1 << port_stimulus.size();
  std::vector<bool> result;
  result.resize(table_length);
  for (size_t i = 0; i < table_length; i++) {
    result[i] = simulateExpr(expr, port_stimulus, i);
  }

  return result;
}

bool RepairSetup::isPortEqiv(sta::FuncExpr* expr,
                             const LibertyCell* cell,
                             const LibertyPort* port_a,
                             const LibertyPort* port_b)
{
  if (port_a->libertyCell() != cell || port_b->libertyCell() != cell) {
    return false;
  }

  sta::LibertyCellPortIterator port_iter(cell);
  sta::UnorderedMap<const LibertyPort*, std::vector<bool>> port_stimulus;
  size_t input_port_count = 0;
  while (port_iter.hasNext()) {
    LibertyPort* port = port_iter.next();
    if (port->direction()->isInput()) {
      ++input_port_count;
      port_stimulus[port] = {};
    }
  }

  if (input_port_count > 16) {
    // Not worth manually simulating all these values.
    // Probably need to do SAT solving or something else instead.
    return false;
  }

  // Generate stimulus for the ports
  size_t var_index = 0;
  for (auto& it : port_stimulus) {
    size_t truth_table_length = 0x1 << input_port_count;
    std::vector<bool>& variable_stimulus = it.second;
    variable_stimulus.resize(truth_table_length, false);
    for (int i = 0; i < truth_table_length; i++) {
      variable_stimulus[i] = static_cast<bool>((i >> var_index) & 0x1);
    }
    var_index++;
  }

  std::vector<bool> result_no_swap = simulateExpr(expr, port_stimulus);

  // Swap pins
  std::swap(port_stimulus.at(port_a), port_stimulus.at(port_b));

  std::vector<bool> result_with_swap = simulateExpr(expr, port_stimulus);

  // Check if truth tables are equivalent post swap. If they are then pins
  // are equivalent.
  for (size_t i = 0; i < result_no_swap.size(); i++) {
    if (result_no_swap[i] != result_with_swap[i]) {
      return false;
    }
  }

  return true;
}

// Lets just look at the first list for now.
// We may want to cache this information somwhere (by building it up for the
// whole library). Or just generate it when the cell is being created (depending
// on agreement).
void RepairSetup::equivCellPins(const LibertyCell* cell,
                                LibertyPort* input_port,
                                sta::LibertyPortSet& ports)
{
  if (cell->hasSequentials() || cell->isIsolationCell()) {
    ports.clear();
    return;
  }
  sta::LibertyCellPortIterator port_iter(cell);
  int outputs = 0;
  int inputs = 0;

  // count number of output ports.
  while (port_iter.hasNext()) {
    LibertyPort* port = port_iter.next();
    if (port->direction()->isOutput()) {
      ++outputs;
    } else {
      ++inputs;
    }
  }

  if (outputs >= 1 && inputs >= 2) {
    sta::LibertyCellPortIterator port_iter2(cell);
    while (port_iter2.hasNext()) {
      LibertyPort* candidate_port = port_iter2.next();
      if (!candidate_port->direction()->isInput()) {
        continue;
      }

      sta::LibertyCellPortIterator output_port_iter(cell);
      std::optional<bool> is_equivalent;
      // Loop through all the output ports and make sure they are equivalent
      // under swaps of candidate_port and input_port. For multi-ouput gates
      // like full adders.
      while (output_port_iter.hasNext()) {
        LibertyPort* output_candidate_port = output_port_iter.next();
        sta::FuncExpr* output_expr = output_candidate_port->function();
        if (!output_candidate_port->direction()->isOutput()) {
          continue;
        }

        if (output_expr == nullptr) {
          continue;
        }

        if (input_port == candidate_port) {
          continue;
        }

        bool is_equivalent_result
            = isPortEqiv(output_expr, cell, input_port, candidate_port);

        if (!is_equivalent.has_value()) {
          is_equivalent = is_equivalent_result;
          continue;
        }

        is_equivalent = is_equivalent.value() && is_equivalent_result;
      }

      // candidate_port is equivalent to input_port under all output ports
      // of this cell.
      if (is_equivalent.has_value() && is_equivalent.value()) {
        ports.insert(candidate_port);
      }
    }
  }
}

void RepairSetup::reportSwappablePins()
{
  init();
  std::unique_ptr<sta::LibertyLibraryIterator> iter(
      db_network_->libertyLibraryIterator());
  while (iter->hasNext()) {
    sta::LibertyLibrary* library = iter->next();
    sta::LibertyCellIterator cell_iter(library);
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      sta::LibertyCellPortIterator port_iter(cell);
      while (port_iter.hasNext()) {
        LibertyPort* port = port_iter.next();
        if (!port->direction()->isInput()) {
          continue;
        }
        sta::LibertyPortSet ports;
        equivCellPins(cell, port, ports);
        std::ostringstream ostr;
        for (auto port : ports) {
          ostr << ' ' << port->name();
        }
        logger_->report("{}/{} ->{}", cell->name(), port->name(), ostr.str());
      }
    }
  }
}

void RepairSetup::printProgress(const int iteration,
                                const bool force,
                                const bool end) const
{
  const bool start = iteration == 0;

  if (start && !end) {
    logger_->report(
        "Iteration | Removed | Resized | Inserted | Cloned Gates | Pin Swaps |"
        "   WNS   |   TNS   | Endpoint");
    logger_->report(
        "          | Buffers |         | Buffers  |              |           |"
        "         |         |");
    logger_->report(
        "----------------------------------------------------------------------"
        "---------------------------");
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

    logger_->report(
        "{: >9s} | {: >7d} | {: >7d} | {: >8d} | {: >12d} | {: >9d} | {: >7s} "
        "| {: >7s} "
        "| {}",
        itr_field,
        removed_buffer_count_,
        resize_count_,
        inserted_buffer_count_ + split_load_buffer_count_ + rebuffer_net_count_,
        cloned_gate_count_,
        swap_pin_count_,
        delayAsString(wns, sta_, 3),
        delayAsString(tns, sta_, 3),
        worst_vertex != nullptr ? worst_vertex->name(network_) : "");
  }

  if (end) {
    logger_->report(
        "----------------------------------------------------------------------"
        "---------------------------");
  }
}

}  // namespace rsz
