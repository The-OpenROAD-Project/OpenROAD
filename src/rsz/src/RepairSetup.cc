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

#include "BaseMove.hh"
#include "BufferMove.hh"
#include "CloneMove.hh"
#include "SizeMove.hh"
#include "SplitLoadMove.hh"
#include "SwapPinsMove.hh"
#include "UnbufferMove.hh"
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
                              const std::vector<MoveType>& sequence,
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
  resizer_->buffer_moved_into_core_ = false;

  if (!sequence.empty()) {
    move_sequence.clear();
    for (MoveType move : sequence) {
      switch (move) {
        case MoveType::BUFFER:
          move_sequence.push_back(resizer_->buffer_move);
          break;
        case MoveType::UNBUFFER:
          move_sequence.push_back(resizer_->unbuffer_move);
          break;
        case MoveType::SWAP:
          move_sequence.push_back(resizer_->swap_pins_move);
          break;
        case MoveType::SIZE:
          move_sequence.push_back(resizer_->size_move);
          break;
        case MoveType::CLONE:
          move_sequence.push_back(resizer_->clone_move);
          break;
        case MoveType::SPLIT:
          move_sequence.push_back(resizer_->split_load_move);
          break;
      }
    }

  } else {
    move_sequence.clear();
    if (!skip_buffer_removal) {
      move_sequence.push_back(resizer_->unbuffer_move);
    }
    // Always  have sizing
    move_sequence.push_back(resizer_->size_move);
    if (!skip_pin_swap) {
      move_sequence.push_back(resizer_->swap_pins_move);
    }
    if (!skip_buffering) {
      move_sequence.push_back(resizer_->buffer_move);
    }
    if (!skip_gate_cloning) {
      move_sequence.push_back(resizer_->clone_move);
    }
    if (!skip_buffering) {
      move_sequence.push_back(resizer_->split_load_move);
    }
  }

  string repair_moves = "Repair move sequence: ";
  for (auto move : move_sequence) {
    repair_moves += move->name() + string(" ");
  }
  logger_->info(RSZ, 100, repair_moves);

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

      const bool changed = repairPath(end_path, end_slack, setup_slack_margin);
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

  int buffer_moves_ = resizer_->buffer_move->numCommittedMoves();
  int size_moves_ = resizer_->size_move->numCommittedMoves();
  int swap_pins_moves_ = resizer_->swap_pins_move->numCommittedMoves();
  int clone_moves_ = resizer_->clone_move->numCommittedMoves();
  int split_load_moves_ = resizer_->split_load_move->numCommittedMoves();
  int unbuffer_moves_ = resizer_->unbuffer_move->numCommittedMoves();

  if (unbuffer_moves_ > 0) {
    repaired = true;
    logger_->info(RSZ, 59, "Removed {} buffers.", unbuffer_moves_);
  }
  if (buffer_moves_ > 0 || split_load_moves_ > 0) {
    repaired = true;
    if (split_load_moves_ == 0) {
      logger_->info(RSZ, 40, "Inserted {} buffers.", buffer_moves_);
    } else {
      logger_->info(RSZ,
                    45,
                    "Inserted {} buffers, {} to split loads.",
                    buffer_moves_ + split_load_moves_,
                    split_load_moves_);
    }
  }
  logger_->metric("design__instance__count__setup_buffer",
                  buffer_moves_ + split_load_moves_);
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
  move_sequence.clear();
  move_sequence = {resizer_->unbuffer_move,
                   resizer_->size_move,
                   resizer_->swap_pins_move,
                   resizer_->buffer_move,
                   resizer_->clone_move,
                   resizer_->split_load_move};
  repairPath(path, slack, 0.0);
  // Leave the parasitices up to date.
  resizer_->updateParasitics();
  resizer_->incrementalParasiticsEnd();

  int unbuffer_moves_ = resizer_->unbuffer_move->numCommittedMoves();
  if (unbuffer_moves_ > 0) {
    logger_->info(RSZ, 61, "Removed {} buffers.", unbuffer_moves_);
  }
  int buffer_moves_ = resizer_->buffer_move->numCommittedMoves();
  int split_load_moves_ = resizer_->split_load_move->numMoves();
  if (buffer_moves_ + split_load_moves_ > 0) {
    logger_->info(
        RSZ, 30, "Inserted {} buffers.", buffer_moves_ + split_load_moves_);
  }
  int size_moves_ = resizer_->size_move->numMoves();
  if (size_moves_ > 0) {
    logger_->info(RSZ, 31, "Resized {} instances.", size_moves_);
  }
  int swap_pins_moves_ = resizer_->swap_pins_move->numMoves();
  if (swap_pins_moves_ > 0) {
    logger_->info(RSZ, 44, "Swapped pins on {} instances.", swap_pins_moves_);
  }
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

      for (BaseMove* move : move_sequence) {
        debugPrint(logger_,
                   RSZ,
                   "moves",
                   2,
                   "Considering {} for {}",
                   move->name(),
                   network_->pathName(drvr_pin));

        if (move->doMove(drvr_path,
                         drvr_index,
                         path_slack,
                         &expanded,
                         setup_slack_margin)) {
          if (move == resizer_->unbuffer_move) {
            // Only allow one unbuffer move per pass to
            // prevent the use-after-free error of multiple buffer removals.
            changed += repairs_per_pass;
          } else {
            changed++;
          }
          // Move on to the next gate
          break;
        }
        debugPrint(logger_,
                   RSZ,
                   "moves",
                   2,
                   "Move {} failed for {}",
                   move->name(),
                   network_->pathName(drvr_pin));
      }
    }
  }
  return changed > 0;
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

    // This actually prints both committed and pending moves, so the moves could
    // could go down if a pass is restrored by the journal.
    logger_->report(
        "{: >9s} | {: >7d} | {: >7d} | {: >8d} | {: >6d} | {: >5d} "
        "| {: >+7.1f}% | {: >8s} | {: >10s} | {: >6d} | {}",
        itr_field,
        resizer_->unbuffer_move->numMoves(),
        resizer_->size_move->numMoves(),
        resizer_->buffer_move->numMoves()
            + resizer_->split_load_move->numMoves(),
        resizer_->clone_move->numMoves(),
        resizer_->swap_pins_move->numMoves(),
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

      const bool changed
          = repairPath(end_path, end_slack, params.setup_slack_margin);

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
