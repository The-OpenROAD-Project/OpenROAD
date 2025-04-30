// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "RepairSetup.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

//#include "BufferMove.hh"
#include "CloneMove.hh"
#include "SizeMove.hh"
#include "SplitLoadMove.hh"
#include "SwapPinsMove.hh"

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
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/TimingArc.hh"
#include "sta/Units.hh"
#include "sta/VerilogWriter.hh"
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
using sta::fuzzyGreaterEqual;
using sta::fuzzyLess;
using sta::GraphDelayCalc;
using sta::InstancePinIterator;
using sta::NetConnectedPinIterator;
using sta::PathExpanded;
using sta::Slew;
using sta::VertexOutEdgeIterator;

RepairSetup::RepairSetup(Resizer* resizer) : resizer_(resizer)
{
}

void RepairSetup::init()
{
  logger_ = resizer_->logger_;
  dbStaState::init(resizer_->sta_);
  db_network_ = resizer_->db_network_;

  initial_design_area_ = resizer_->computeDesignArea();
}

bool RepairSetup::repairSetup(const float setup_slack_margin,
                              const double repair_tns_end_percent,
                              const int max_passes,
                              const int max_repairs_per_pass,
                              const bool verbose,
                              const bool skip_pin_swap,
                              const bool skip_gate_cloning,
                              const bool skip_buffering,
                              const bool skip_buffer_removal,
                              const bool skip_last_gasp)
{
  bool repaired = false;
  init();
  constexpr int digits = 3;
  max_repairs_per_pass_ = max_repairs_per_pass;
  inserted_buffer_count_ = 0;
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
    logger_->info(RSZ, 98, "No setup violations found");
    return false;
  }

  int end_index = 0;
  int max_end_count = violating_ends.size() * repair_tns_end_percent;
  float initial_tns = sta_->totalNegativeSlack(max_);
  float prev_tns = initial_tns;
  int num_viols = violating_ends.size();
  // Always repair the worst endpoint, even if tns percent is zero.
  max_end_count = max(max_end_count, 1);
  logger_->info(RSZ,
                99,
                "Repairing {} out of {} ({:0.2f}%) violating endpoints...",
                max_end_count,
                violating_ends.size(),
                repair_tns_end_percent * 100.0);

  // Ensure that max cap and max fanout violations don't get worse
  sta_->checkCapacitanceLimitPreamble();
  sta_->checkFanoutLimitPreamble();

  int opto_iteration = 0;
  bool prev_termination = false;
  bool two_cons_terminations = false;
  printProgress(opto_iteration, false, false, false, num_viols);
  float fix_rate_threshold = inc_fix_rate_threshold_;
  if (!violating_ends.empty()) {
    min_viol_ = -violating_ends.back().second;
    max_viol_ = -violating_ends.front().second;
  }
  for (const auto& end_original_slack : violating_ends) {
    fallback_ = false;
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
      opto_iteration++;
      if (verbose || opto_iteration == 1) {
        printProgress(opto_iteration, false, false, false, num_viols);
      }
      if (terminateProgress(opto_iteration,
                            initial_tns,
                            prev_tns,
                            fix_rate_threshold,
                            end_index,
                            max_end_count)) {
        if (prev_termination) {
          // Abort entire fixing if no progress for 200 iterations
          two_cons_terminations = true;
        } else {
          prev_termination = true;
        }

        // Restore to previous good checkpoint
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "Restoring best slack end slack {} worst slack {}",
                   delayAsString(prev_end_slack, sta_, digits),
                   delayAsString(prev_worst_slack, sta_, digits));
        resizer_->journalRestore();
        break;
      }
      if (opto_iteration % opto_small_interval_ == 0) {
        prev_termination = false;
      }

      if (end_slack > setup_slack_margin) {
        --num_viols;
        if (pass != 1) {
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     2,
                     "Restoring best slack end slack {} worst slack {}",
                     delayAsString(prev_end_slack, sta_, digits),
                     delayAsString(prev_worst_slack, sta_, digits));
          resizer_->journalRestore();
        } else {
          resizer_->journalEnd();
        }
        // clang-format off
        debugPrint(logger_, RSZ, "repair_setup", 1, "bailing out at {}/{} "
                   "end_slack {} is larger than setup_slack_margin {}",
                   end_index, max_end_count, end_slack, setup_slack_margin);
        // clang-format on
        break;
      }
      Path* end_path = sta_->vertexWorstSlackPath(end, max_);

      const bool changed = repairPath(end_path,
                                      end_slack,
                                      skip_pin_swap,
                                      skip_gate_cloning,
                                      skip_buffering,
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
          resizer_->journalRestore();
        } else {
          resizer_->journalEnd();
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
        if (end_slack > setup_slack_margin) {
          --num_viols;
        }
        prev_end_slack = end_slack;
        prev_worst_slack = worst_slack;
        decreasing_slack_passes = 0;
        resizer_->journalEnd();
        // Progress, Save checkpoint so we can back up to here.
        resizer_->journalBegin();
      } else {
        fallback_ = true;
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
          resizer_->journalRestore();
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
        resizer_->journalEnd();
        break;
      }
      if (end_index == 1) {
        end = worst_vertex;
      }
      pass++;
    }  // while pass <= max_passes
    if (verbose || opto_iteration == 1) {
      printProgress(opto_iteration, true, false, false, num_viols);
    }
    if (two_cons_terminations) {
      // clang-format off
      debugPrint(logger_, RSZ, "repair_setup", 1, "bailing out of setup fixing"
                 "due to no TNS progress for two opto cycles");
      // clang-format on
      break;
    }
  }  // for each violating endpoint

  if (!skip_last_gasp) {
    // do some last gasp setup fixing before we give up
    OptoParams params(setup_slack_margin, verbose);
    params.iteration = opto_iteration;
    params.initial_tns = initial_tns;
    repairSetupLastGasp(params, num_viols);
  }

  printProgress(opto_iteration, true, true, false, num_viols);

  int size_moves_ = resizer_->size_move->countMoves();
  int swap_pins_moves_ = resizer_->swap_pins_move->countMoves();
  int clone_moves_ = resizer_->clone_move->countMoves();
  int split_load_moves_ = resizer_->split_load_move->countMoves();

  if (removed_buffer_count_ > 0) {
    repaired = true;
    logger_->info(RSZ, 59, "Removed {} buffers.", removed_buffer_count_);
  }
  if (inserted_buffer_count_ > 0 && split_load_moves_ == 0) {
    repaired = true;
    logger_->info(RSZ, 40, "Inserted {} buffers.", inserted_buffer_count_);
  } else if (inserted_buffer_count_ > 0 && split_load_moves_ > 0) {
    repaired = true;
    logger_->info(RSZ,
                  45,
                  "Inserted {} buffers, {} to split loads.",
                  inserted_buffer_count_,
                  split_load_moves_);
  }
  logger_->metric("design__instance__count__setup_buffer",
                  inserted_buffer_count_);
  if (size_moves_ > 0) {
    repaired = true;
    logger_->info(RSZ, 41, "Resized {} instances.", size_moves_);
  }
  if (swap_pins_moves_ > 0) {
    repaired = true;
    logger_->info(RSZ, 43, "Swapped pins on {} instances.", swap_pins_moves_);
  }
  if (clone_moves_ > 0) {
    repaired = true;
    logger_->info(RSZ, 49, "Cloned {} instances.", clone_moves_);
  }
  const Slack worst_slack = sta_->worstSlack(max_);
  if (fuzzyLess(worst_slack, setup_slack_margin)) {
    repaired = true;
    logger_->warn(RSZ, 62, "Unable to repair all setup violations.");
  }
  if (resizer_->overMaxArea()) {
    logger_->error(RSZ, 25, "max utilization reached.");
  }

  return repaired;
}

// For testing.
void RepairSetup::repairSetup(const Pin* end_pin)
{
  init();
  max_repairs_per_pass_ = 1;

  Vertex* vertex = graph_->pinLoadVertex(end_pin);
  const Slack slack = sta_->vertexSlack(vertex, max_);
  Path* path = sta_->vertexWorstSlackPath(vertex, max_);
  resizer_->incrementalParasiticsBegin();
  repairPath(path, slack, false, false, false, false, 0.0);
  // Leave the parasitices up to date.
  resizer_->updateParasitics();
  resizer_->incrementalParasiticsEnd();

  if (removed_buffer_count_ > 0) {
    logger_->info(RSZ, 61, "Removed {} buffers.", removed_buffer_count_);
  }
  if (inserted_buffer_count_ > 0) {
    logger_->info(RSZ, 30, "Inserted {} buffers.", inserted_buffer_count_);
  }
  if (resizer_->size_move->countMoves() > 0) {
    logger_->info(RSZ, 31, "Resized {} instances.", resizer_->size_move->countMoves());
  }
  if (resizer_->swap_pins_move->countMoves() > 0) {
    logger_->info(RSZ, 44, "Swapped pins on {} instances.", resizer_->swap_pins_move->countMoves());
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
bool RepairSetup::repairPath(Path* path,
                             const Slack path_slack,
                             const bool skip_pin_swap,
                             const bool skip_gate_cloning,
                             const bool skip_buffering,
                             const bool skip_buffer_removal,
                             const float setup_slack_margin)
{
  PathExpanded expanded(path, sta_);
  int changed = 0;

  if (expanded.size() > 1) {
    const int path_length = expanded.size();
    vector<pair<int, Delay>> load_delays;
    const int start_index = expanded.startIndex();
    const DcalcAnalysisPt* dcalc_ap = path->dcalcAnalysisPt(sta_);
    const int lib_ap = dcalc_ap->libertyIndex();
    // Find load delay for each gate in the path.
    for (int i = start_index; i < path_length; i++) {
      const Path* path = expanded.path(i);
      Vertex* path_vertex = path->vertex(sta_);
      const Pin* path_pin = path->pin(sta_);
      if (i > 0 && network_->isDriver(path_pin)
          && !network_->isTopLevelPort(path_pin)) {
        const TimingArc* prev_arc = path->prevArc(sta_);
        const TimingArc* corner_arc = prev_arc->cornerArc(lib_ap);
        Edge* prev_edge = path->prevEdge(sta_);
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
    int repairs_per_pass = 1;
    if (max_viol_ - min_viol_ != 0.0) {
      repairs_per_pass
          += std::round((max_repairs_per_pass_ - 1) * (-path_slack - min_viol_)
                        / (max_viol_ - min_viol_));
    }
    if (fallback_) {
      repairs_per_pass = 1;
    }
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               3,
               "Path slack: {}, repairs: {}, load_delays: {}",
               delayAsString(path_slack, sta_, 3),
               repairs_per_pass,
               load_delays.size());
    for (const auto& [drvr_index, ignored] : load_delays) {
      if (changed >= repairs_per_pass) {
        break;
      }
      const Path* drvr_path = expanded.path(drvr_index);
      Vertex* drvr_vertex = drvr_path->vertex(sta_);
      const Pin* drvr_pin = drvr_vertex->pin();
      const Net* net = db_network_->dbToSta(db_network_->flatNet(drvr_pin));
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
          changed++;
          continue;
        }
      }

      if (resizer_->size_move->doMove(drvr_path, drvr_index, &expanded)) {
        changed++;
        continue;
      }

      // Pin swapping
      if (!skip_pin_swap) {
        if (resizer_->swap_pins_move->doMove(drvr_path, drvr_index, &expanded)) {
          changed++;
          continue;
        }
      }

      // For tristate nets all we can do is resize the driver.
      const bool tristate_drvr = resizer_->isTristateDriver(drvr_pin);
      dbNet* db_net = db_network_->staToDb(net);
      if (!skip_buffering
          && fanout > 1
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
          changed++;
          continue;
        }
      }

      // Gate cloning
      if (!skip_gate_cloning && fanout > split_load_min_fanout_
          && !tristate_drvr && !resizer_->dontTouch(net)
          && resizer_->clone_move->doMove(drvr_path, drvr_index, path_slack, &expanded)) {
        changed++;
        continue;
      }

      if (!skip_buffering) {
        // Don't split loads on low fanout nets.
        if (fanout > split_load_min_fanout_ && !tristate_drvr
            && !resizer_->dontTouch(net) && !db_net->isConnectedByAbutment()) {
          resizer_->split_load_move->doMove(drvr_path, drvr_index, path_slack, &expanded);
          changed++;
          continue;
        }
      }
    }
    for (auto inst : buf_to_remove_) {
      resizer_->removeBuffer(inst, /* recordJournal */ true);
    }
    buf_to_remove_.clear();
  }
  return changed > 0;
}

void RepairSetup::debugCheckMultipleBuffers(Path* path, PathExpanded* expanded)
{
  if (expanded->size() > 1) {
    const int path_length = expanded->size();
    const int start_index = expanded->startIndex();
    for (int i = start_index; i < path_length; i++) {
      const Path* path = expanded->path(i);
      const Pin* path_pin = path->pin(sta_);
      if (i > 0 && network_->isDriver(path_pin)
          && !network_->isTopLevelPort(path_pin)) {
        const TimingArc* prev_arc = path->prevArc(sta_);
        printf("repair_setup %s: %s ---> %s \n",
               prev_arc->from()->libertyCell()->name(),
               prev_arc->from()->name(),
               prev_arc->to()->name());
      }
    }
  }
  printf("done\n");
}


// Remove driver if
// 1) it is a buffer without attributes like dont-touch
// 2) it doesn't create new max fanout violations
// 3) it doesn't create new max cap violations
// 4) it doesn't worsen slack
bool RepairSetup::removeDrvr(const Path* drvr_path,
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
    if (resizer_->swap_pins_move->countMoves(drvr)) {
      reason = "its pins have been swapped";
    } else if (resizer_->clone_move->countMoves(drvr)) {
      reason = "it has been cloned";
    } else if (resizer_->split_load_move->countMoves(drvr)) {
      reason = "it was from split load buffering";
    } else if (resizer_->all_inserted_buffer_set_.count(drvr)) {
      reason = "it was from rebuffering";
    } else if (resizer_->size_move->countMoves(drvr)) {
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
    const Path* prev_drvr_path = expanded->path(drvr_index - 2);
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

    const Path* drvr_input_path = expanded->path(drvr_index - 1);
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

    if (resizer_->canRemoveBuffer(drvr, /* honorDontTouch */ true)) {
      buf_to_remove_.push_back(drvr);
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
// Acceptance criteria are as follows:
// For direct fanout paths (fanout paths of drvr_pin), accept buffer removal
// if slack improves (may still be violating)
// For side fanout paths (fanout paths of side_out_pin*), accept buffer removal
// if slack doesn't become violating (no new violations)
//
//               input_net                             output_net
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
  resizer_->annotateInputSlews(network_->instance(params.prev_driver_pin),
                               dcalc_ap);
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
  resizer_->resetInputSlews();

  // Check if degraded delay & slew can be absorbed by driver pin fanouts
  Net* output_net = network_->net(params.driver_pin);
  NetConnectedPinIterator* pin_iter
      = network_->connectedPinIterator(output_net);
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (pin == params.driver_pin) {
      continue;
    }
    float old_slack = sta_->pinSlack(pin, max_);
    float new_slack = old_slack - delay_degrad + delay_imp;
    if (fuzzyGreater(old_slack, new_slack)) {
      // clang-format off
      debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} is not removed "
                 "because new output pin slack {} is worse than old slack {}",
                 db_network_->name(params.driver), db_network_->name(pin),
                 new_slack, old_slack);
      // clang-format on
      return false;
    }

    // Check if output pin of direct fanout instance can absorb delay and slew
    // degradation
    if (!estimateInputSlewImpact(network_->instance(pin),
                                 dcalc_ap,
                                 old_slew,
                                 new_slew,
                                 delay_degrad - delay_imp,
                                 params,
                                 /* accept if slack improves */ true)) {
      return false;
    }
  }

  // Check side fanout paths.  Side fanout paths get no delay benefit from
  // buffer removal.
  Net* input_net = network_->net(params.prev_driver_pin);
  pin_iter = network_->connectedPinIterator(input_net);
  while (pin_iter->hasNext()) {
    const Pin* side_input_pin = pin_iter->next();
    if (side_input_pin == params.prev_driver_pin
        || side_input_pin == params.driver_input_pin) {
      continue;
    }
    float old_slack = sta_->pinSlack(side_input_pin, max_);
    float new_slack = old_slack - delay_degrad - params.setup_slack_margin;
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

    // Consider secondary degradation at side out pin from degraded input
    // slew.
    if (!estimateInputSlewImpact(network_->instance(side_input_pin),
                                 dcalc_ap,
                                 old_slew,
                                 new_slew,
                                 delay_degrad,
                                 params,
                                 /* accept only if no new viol */ false)) {
      return false;
    }
  }  // for each pin of input_net

  // clang-format off
  debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} can be removed because"
             " direct fanouts and side fanouts can absorb delay/slew degradation",
             db_network_->name(params.driver));
  // clang-format on
  return true;
}  // namespace rsz

// Estimate impact from degraded input slew for this instance.
// Include all output pins for multi-outut gate (MOG) cells.
bool RepairSetup::estimateInputSlewImpact(
    Instance* instance,
    const DcalcAnalysisPt* dcalc_ap,
    Slew old_in_slew[RiseFall::index_count],
    Slew new_in_slew[RiseFall::index_count],
    // delay adjustment from prev stage
    float delay_adjust,
    SlackEstimatorParams params,
    bool accept_if_slack_improves)
{
  GraphDelayCalc* dcalc = sta_->graphDelayCalc();
  InstancePinIterator* pin_iter = network_->pinIterator(instance);
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (!network_->direction(pin)->isOutput()) {
      continue;
    }
    LibertyPort* port = network_->libertyPort(pin);
    if (port == nullptr) {
      // reject the transform if we can't estimate
      // clang-format off
      debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} is not removed"
                 "because pin {} has no liberty port",
                 db_network_->name(params.driver), db_network_->name(pin));
      // clang-format on
      return false;
    }
    float load_cap = dcalc->loadCap(pin, dcalc_ap);
    ArcDelay old_delay[RiseFall::index_count], new_delay[RiseFall::index_count];
    Slew old_slew[RiseFall::index_count], new_slew[RiseFall::index_count];
    resizer_->gateDelays(
        port, load_cap, old_in_slew, dcalc_ap, old_delay, old_slew);
    resizer_->gateDelays(
        port, load_cap, new_in_slew, dcalc_ap, new_delay, new_slew);
    float delay_diff = max(
        new_delay[RiseFall::riseIndex()] - old_delay[RiseFall::riseIndex()],
        new_delay[RiseFall::fallIndex()] - old_delay[RiseFall::fallIndex()]);

    float old_slack = sta_->pinSlack(pin, max_) - params.setup_slack_margin;
    float new_slack
        = old_slack - delay_diff - delay_adjust - params.setup_slack_margin;
    if ((accept_if_slack_improves && fuzzyGreater(old_slack, new_slack))
        || (!accept_if_slack_improves && new_slack < 0)) {
      // clang-format off
      debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} is not removed"
                 "because pin {} will have a violating or worse slack of {}",
                 db_network_->name(params.driver), db_network_->name(pin),
                 new_slack);
      // clang-format on
      return false;
    }
  }

  return true;
}


int RepairSetup::fanout(Vertex* vertex)
{
  int fanout = 0;
  VertexOutEdgeIterator edge_iter(vertex, graph_);
  while (edge_iter.hasNext()) {
    Edge* edge = edge_iter.next();
    // Disregard output->output timing arcs
    if (edge->isWire()) {
      fanout++;
    }
  }
  return fanout;
}


void RepairSetup::printProgress(const int iteration,
                                const bool force,
                                const bool end,
                                const bool last_gasp,
                                const int num_viols) const
{
  const bool start = iteration == 0;

  if (start && !end) {
    logger_->report(
        "   Iter   | Removed | Resized | Inserted | Cloned |  Pin  |"
        "   Area   |    WNS   |   TNS      |  Viol  | Worst");
    logger_->report(
        "          | Buffers |  Gates  | Buffers  |  Gates | Swaps |"
        "          |          |            | Endpts | Endpt");
    logger_->report(
        "-----------------------------------------------------------"
        "---------------------------------------------------");
  }

  if (iteration % print_interval_ == 0 || force || end) {
    Slack wns;
    Vertex* worst_vertex;
    sta_->worstSlack(max_, wns, worst_vertex);
    const Slack tns = sta_->totalNegativeSlack(max_);

    std::string itr_field
        = fmt::format("{}{}", iteration, (last_gasp ? "*" : ""));
    if (end) {
      itr_field = "final";
    }

    const double design_area = resizer_->computeDesignArea();
    const double area_growth = design_area - initial_design_area_;

    logger_->report(
        "{: >9s} | {: >7d} | {: >7d} | {: >8d} | {: >6d} | {: >5d} "
        "| {: >+7.1f}% | {: >8s} | {: >10s} | {: >6d} | {}",
        itr_field,
        removed_buffer_count_,
        resizer_->size_move->countMoves(),
        inserted_buffer_count_ + resizer_->split_load_move->countMoves() + rebuffer_net_count_,
        resizer_->clone_move->countMoves(),
        resizer_->swap_pins_move->countMoves(),
        area_growth / initial_design_area_ * 1e2,
        delayAsString(wns, sta_, 3),
        delayAsString(tns, sta_, 1),
        max(0, num_viols),
        worst_vertex != nullptr ? worst_vertex->name(network_) : "");
  }

  if (end) {
    logger_->report(
        "-----------------------------------------------------------"
        "---------------------------------------------------");
  }
}

// Terminate progress if incremental fix rate within an opto interval falls
// below the threshold.   Bump up the threshold after each large opto
// interval.
bool RepairSetup::terminateProgress(const int iteration,
                                    const float initial_tns,
                                    float& prev_tns,
                                    float& fix_rate_threshold,
                                    // for debug only
                                    const int endpt_index,
                                    const int num_endpts)
{
  if (iteration % opto_large_interval_ == 0) {
    fix_rate_threshold *= 2.0;
  }
  if (iteration % opto_small_interval_ == 0) {
    float curr_tns = sta_->totalNegativeSlack(max_);
    float inc_fix_rate = (prev_tns - curr_tns) / initial_tns;
    prev_tns = curr_tns;
    if (iteration > 1000  // allow for some initial fixing for 1000 iterations
        && inc_fix_rate < fix_rate_threshold) {
      // clang-format off
      debugPrint(logger_, RSZ, "repair_setup", 1, "bailing out at iter {}"
                 " because incr fix rate {:0.2f}% is < {:0.2f}% [endpt {}/{}]",
                 iteration, inc_fix_rate*100, fix_rate_threshold*100,
                 endpt_index, num_endpts);
      // clang-format on
      return true;
    }
  }
  return false;
}

// Perform some last fixing based on sizing only.
// This is a greedy opto that does not degrade WNS or TNS.
// TODO: add VT swap
void RepairSetup::repairSetupLastGasp(const OptoParams& params, int& num_viols)
{
  // Sort remaining failing endpoints
  const VertexSet* endpoints = sta_->endpoints();
  vector<pair<Vertex*, Slack>> violating_ends;
  for (Vertex* end : *endpoints) {
    const Slack end_slack = sta_->vertexSlack(end, max_);
    if (end_slack < params.setup_slack_margin) {
      violating_ends.emplace_back(end, end_slack);
    }
  }
  std::stable_sort(violating_ends.begin(),
                   violating_ends.end(),
                   [](const auto& end_slack1, const auto& end_slack2) {
                     return end_slack1.second < end_slack2.second;
                   });
  num_viols = violating_ends.size();

  float curr_tns = sta_->totalNegativeSlack(max_);
  if (fuzzyGreaterEqual(curr_tns, 0)) {
    // clang-format off
    debugPrint(logger_, RSZ, "repair_setup", 1, "last gasp is bailing out "
               "because TNS is {:0.2f}", curr_tns);
    // clang-format on
    return;
  }

  // Don't do anything unless there was some progress from previous fixing
  if ((params.initial_tns - curr_tns) / params.initial_tns < 0.05) {
    // clang-format off
    debugPrint(logger_, RSZ, "repair_setup", 1, "last gasp is bailing out "
               "because TNS was reduced by < 5% from previous fixing");
    // clang-format on
    return;
  }

  int end_index = 0;
  int max_end_count = violating_ends.size();
  if (max_end_count == 0) {
    // clang-format off
    debugPrint(logger_, RSZ, "repair_setup", 1, "last gasp is bailing out "
               "because there are no violating endpoints");
    // clang-format on
    return;
  }
  // clang-format off
  debugPrint(logger_, RSZ, "repair_setup", 1, "{} violating endpoints remain",
             max_end_count);
  // clang-format on
  int opto_iteration = params.iteration;
  printProgress(opto_iteration, false, false, true, num_viols);

  float prev_tns = curr_tns;
  Slack curr_worst_slack = violating_ends[0].second;
  Slack prev_worst_slack = curr_worst_slack;
  bool prev_termination = false;
  bool two_cons_terminations = false;
  float fix_rate_threshold = inc_fix_rate_threshold_;

  for (const auto& end_original_slack : violating_ends) {
    fallback_ = false;
    Vertex* end = end_original_slack.first;
    Slack end_slack = sta_->vertexSlack(end, max_);
    Slack worst_slack;
    Vertex* worst_vertex;
    sta_->worstSlack(max_, worst_slack, worst_vertex);
    end_index++;
    if (end_index > max_end_count) {
      break;
    }
    int pass = 1;
    resizer_->journalBegin();
    while (pass <= max_last_gasp_passes_) {
      opto_iteration++;
      if (terminateProgress(opto_iteration,
                            params.initial_tns,
                            prev_tns,
                            fix_rate_threshold,
                            end_index,
                            max_end_count)) {
        if (prev_termination) {
          // Abort entire fixing if no progress for 200 iterations
          two_cons_terminations = true;
        } else {
          prev_termination = true;
        }
        resizer_->journalEnd();
        break;
      }
      if (opto_iteration % opto_small_interval_ == 0) {
        prev_termination = false;
      }
      if (params.verbose || opto_iteration == 1) {
        printProgress(opto_iteration, false, false, true, num_viols);
      }
      if (end_slack > params.setup_slack_margin) {
        --num_viols;
        resizer_->journalEnd();
        break;
      }
      Path* end_path = sta_->vertexWorstSlackPath(end, max_);

      const bool changed = repairPath(end_path,
                                      end_slack,
                                      true /* skip_pin_swap */,
                                      true /* skip_gate_cloning */,
                                      true /* skip_buffering */,
                                      true /* skip_buffer_removal */,
                                      params.setup_slack_margin);

      if (!changed) {
        if (pass != 1) {
          resizer_->journalRestore();
        } else {
          resizer_->journalEnd();
        }
        break;
      }
      resizer_->updateParasitics();
      sta_->findRequireds();
      end_slack = sta_->vertexSlack(end, max_);
      sta_->worstSlack(max_, curr_worst_slack, worst_vertex);
      curr_tns = sta_->totalNegativeSlack(max_);

      // Accept only moves that improve both WNS and TNS
      if (fuzzyGreaterEqual(curr_worst_slack, prev_worst_slack)
          && fuzzyGreaterEqual(curr_tns, prev_tns)) {
        // clang-format off
        debugPrint(logger_, RSZ, "repair_setup", 1, "sizing move accepted for "
                   "endpoint {} pass {} because WNS improved to {:0.3f} and "
                   "TNS improved to {:0.3f}",
                   end_index, pass, curr_worst_slack, curr_tns);
        // clang-format on
        prev_worst_slack = curr_worst_slack;
        prev_tns = curr_tns;
        if (end_slack > params.setup_slack_margin) {
          --num_viols;
        }
        resizer_->journalEnd();
        resizer_->journalBegin();
      } else {
        fallback_ = true;
        resizer_->journalRestore();
        break;
      }

      if (resizer_->overMaxArea()) {
        resizer_->journalEnd();
        break;
      }
      if (end_index == 1) {
        end = worst_vertex;
      }
      pass++;
    }  // while pass <= max_last_gasp_passes_
    if (params.verbose || opto_iteration == 1) {
      printProgress(opto_iteration, true, false, true, num_viols);
    }
    if (two_cons_terminations) {
      // clang-format off
      debugPrint(logger_, RSZ, "repair_setup", 1, "bailing out of last gasp fixing"
                 "due to no TNS progress for two opto cycles");
      // clang-format on
      break;
    }
  }  // for each violating endpoint
}

}  // namespace rsz
