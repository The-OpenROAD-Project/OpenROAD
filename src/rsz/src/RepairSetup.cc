// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "RepairSetup.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "BaseMove.hh"
#include "BufferMove.hh"
#include "CloneMove.hh"
#include "Rebuffer.hh"
#include "SizeDownMove.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "sta/Delay.hh"
#include "sta/GraphClass.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/SearchClass.hh"
// This includes SizeUpMatchMove
#include "SizeUpMove.hh"
#include "SplitLoadMove.hh"
#include "SwapPinsMove.hh"
#include "UnbufferMove.hh"
#include "VTSwapMove.hh"
#include "rsz/Resizer.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InputDrive.hh"
#include "sta/Liberty.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "sta/TimingArc.hh"
#include "sta/VerilogWriter.hh"
#include "utl/Logger.h"
#include "utl/mem_stats.h"

namespace rsz {

using namespace sta;  // NOLINT

using std::max;
using std::pair;
using std::string;
using std::vector;
using utl::RSZ;

RepairSetup::RepairSetup(Resizer* resizer) : resizer_(resizer)
{
}

void RepairSetup::init()
{
  logger_ = resizer_->logger_;
  dbStaState::init(resizer_->sta_);
  db_network_ = resizer_->db_network_;
  estimate_parasitics_ = resizer_->estimate_parasitics_;
  initial_design_area_ = resizer_->computeDesignArea();
}

bool RepairSetup::repairSetup(const float setup_slack_margin,
                              const double repair_tns_end_percent,
                              const int max_passes,
                              const int max_iterations,
                              const int max_repairs_per_pass,
                              const bool verbose,
                              const std::vector<MoveType>& sequence,
                              const bool skip_pin_swap,
                              const bool skip_gate_cloning,
                              const bool skip_size_down,
                              const bool skip_buffering,
                              const bool skip_buffer_removal,
                              const bool skip_last_gasp,
                              const bool skip_vt_swap,
                              const bool skip_crit_vt_swap)
{
  bool repaired = false;
  init();
  resizer_->rebuffer_->init();
  // IMPROVE ME: rebuffering always looks at cmd corner
  resizer_->rebuffer_->initOnCorner(sta_->cmdScene());
  constexpr int digits = 3;
  max_repairs_per_pass_ = max_repairs_per_pass;
  resizer_->buffer_moved_into_core_ = false;

  if (!sequence.empty()) {
    move_sequence_.clear();
    for (MoveType move : sequence) {
      switch (move) {
        case MoveType::BUFFER:
          if (!skip_buffering) {
            move_sequence_.push_back(resizer_->buffer_move_.get());
          }
          break;
        case MoveType::UNBUFFER:
          if (!skip_buffer_removal) {
            move_sequence_.push_back(resizer_->unbuffer_move_.get());
          }
          break;
        case MoveType::SWAP:
          if (!skip_pin_swap) {
            move_sequence_.push_back(resizer_->swap_pins_move_.get());
          }
          break;
        case MoveType::SIZE:
          move_sequence_.push_back(resizer_->size_up_move_.get());
          if (!skip_size_down) {
            move_sequence_.push_back(resizer_->size_down_move_.get());
          }
          break;
        case MoveType::SIZEUP:
          move_sequence_.push_back(resizer_->size_up_move_.get());
          break;
        case MoveType::SIZEDOWN:
          if (!skip_size_down) {
            move_sequence_.push_back(resizer_->size_down_move_.get());
          }
          break;
        case MoveType::CLONE:
          if (!skip_gate_cloning) {
            move_sequence_.push_back(resizer_->clone_move_.get());
          }
          break;
        case MoveType::SPLIT:
          if (!skip_buffering) {
            move_sequence_.push_back(resizer_->split_load_move_.get());
          }
          break;
        case MoveType::VTSWAP_SPEED:
          if (!skip_vt_swap
              && resizer_->lib_data_->sorted_vt_categories.size() > 1) {
            move_sequence_.push_back(resizer_->vt_swap_speed_move_.get());
          }
          break;
        case MoveType::SIZEUP_MATCH:
          move_sequence_.push_back(resizer_->size_up_match_move_.get());
          break;
      }
    }

  } else {
    move_sequence_.clear();
    if (!skip_buffer_removal) {
      move_sequence_.push_back(resizer_->unbuffer_move_.get());
    }
    if (!skip_vt_swap && resizer_->lib_data_->sorted_vt_categories.size() > 1) {
      move_sequence_.push_back(resizer_->vt_swap_speed_move_.get());
    }
    // Always  have sizing
    move_sequence_.push_back(resizer_->size_up_move_.get());
    // Disabled by default for now
    if (!skip_size_down) {
      // move_sequence_.push_back(resizer_->size_down_move_.get());
    }
    if (!skip_pin_swap) {
      move_sequence_.push_back(resizer_->swap_pins_move_.get());
    }
    if (!skip_buffering) {
      move_sequence_.push_back(resizer_->buffer_move_.get());
    }
    if (!skip_gate_cloning) {
      move_sequence_.push_back(resizer_->clone_move_.get());
    }
    if (!skip_buffering) {
      move_sequence_.push_back(resizer_->split_load_move_.get());
    }
  }

  string repair_moves = "Repair move sequence: ";
  for (auto move : move_sequence_) {
    move->init();
    repair_moves += move->name() + string(" ");
  }
  logger_->info(RSZ, 100, repair_moves);

  // Sort failing endpoints by slack.
  const VertexSet& endpoints = sta_->endpoints();
  vector<pair<Vertex*, Slack>> violating_ends;
  // logger_->setDebugLevel(RSZ, "repair_setup", 2);
  // Should check here whether we can figure out the clock domain for each
  // vertex. This may be the place where we can do some round robin fun to
  // individually control each clock domain instead of just fixating on fixing
  // one.
  for (Vertex* end : endpoints) {
    const Slack end_slack = sta_->slack(end, max_);
    if (end_slack < setup_slack_margin) {
      violating_ends.emplace_back(end, end_slack);
    }
  }
  std::ranges::stable_sort(violating_ends,
                           [](const auto& end_slack1, const auto& end_slack2) {
                             return end_slack1.second < end_slack2.second;
                           });
  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "Violating endpoints {}/{} {}%",
             violating_ends.size(),
             endpoints.size(),
             int(violating_ends.size() / double(endpoints.size()) * 100));

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
  sta_->checkCapacitancesPreamble(sta_->scenes());
  sta_->checkSlewsPreamble();
  sta_->checkFanoutPreamble();

  est::IncrementalParasiticsGuard guard(estimate_parasitics_);
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
    Slack end_slack = sta_->slack(end, max_);
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
    sta::Slack prev_end_slack = end_slack;
    sta::Slack prev_worst_slack = worst_slack;
    int pass = 1;
    int decreasing_slack_passes = 0;
    resizer_->journalBegin();
    bool journal_open = true;
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
        journal_open = false;
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
        journal_open = false;
        // clang-format off
        debugPrint(logger_, RSZ, "repair_setup", 1, "bailing out at {}/{} "
                   "end_slack {} is larger than setup_slack_margin {}",
                   end_index, max_end_count, end_slack, setup_slack_margin);
        // clang-format on
        break;
      }
      sta::Path* end_path = sta_->vertexWorstSlackPath(end, max_);

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
        journal_open = false;
        // clang-format off
        debugPrint(logger_, RSZ, "repair_setup", 1, "bailing out {} no changes"
                   " after {} decreasing passes", end->name(network_),
                   decreasing_slack_passes);
        // clang-format on
        break;
      }
      estimate_parasitics_->updateParasitics();
      sta_->findRequireds();
      end_slack = sta_->slack(end, max_);
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
        if (pass < max_passes) {
          // Progress, Save checkpoint so we can back up to here.
          resizer_->journalBegin();
        } else {
          journal_open = false;
        }
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
          journal_open = false;
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
        journal_open = false;
        break;
      }
      if (end_index == 1) {
        end = worst_vertex;
      }
      pass++;
      if (max_iterations > 0 && opto_iteration >= max_iterations) {
        resizer_->journalEnd();
        journal_open = false;
        break;
      }
    }  // while pass <= max_passes
    if (journal_open) {
      resizer_->journalEnd();
    }
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
    if (max_iterations > 0 && opto_iteration >= max_iterations) {
      break;
    }
  }  // for each violating endpoint

  if (!skip_last_gasp) {
    // do some last gasp setup fixing before we give up
    OptoParams params(setup_slack_margin,
                      verbose,
                      skip_pin_swap,
                      skip_gate_cloning,
                      skip_size_down,
                      skip_buffering,
                      skip_buffer_removal,
                      skip_vt_swap);
    params.iteration = opto_iteration;
    params.initial_tns = initial_tns;
    repairSetupLastGasp(params, num_viols, max_iterations);
  }

  if (!skip_crit_vt_swap && !skip_vt_swap
      && resizer_->lib_data_->sorted_vt_categories.size() > 1) {
    // Swap most critical cells to fastest VT

    OptoParams params(setup_slack_margin,
                      verbose,
                      skip_pin_swap,
                      skip_gate_cloning,
                      skip_size_down,
                      skip_buffering,
                      skip_buffer_removal,
                      skip_vt_swap);
    if (swapVTCritCells(params, num_viols)) {
      estimate_parasitics_->updateParasitics();
      sta_->findRequireds();
    }
  }

  printProgress(opto_iteration, true, true, false, num_viols);

  int buffer_moves_ = resizer_->buffer_move_->numCommittedMoves();
  int size_up_moves_ = resizer_->size_up_move_->numCommittedMoves();
  int size_down_moves_ = resizer_->size_down_move_->numCommittedMoves();
  int swap_pins_moves_ = resizer_->swap_pins_move_->numCommittedMoves();
  int clone_moves_ = resizer_->clone_move_->numCommittedMoves();
  int split_load_moves_ = resizer_->split_load_move_->numCommittedMoves();
  int unbuffer_moves_ = resizer_->unbuffer_move_->numCommittedMoves();
  int vt_swap_moves_ = resizer_->vt_swap_speed_move_->numCommittedMoves();
  int size_up_match_moves_ = resizer_->size_up_match_move_->numCommittedMoves();

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
  if (size_up_moves_ + size_down_moves_ + size_up_match_moves_ + vt_swap_moves_
      > 0) {
    repaired = true;
    logger_->info(RSZ,
                  51,
                  "Resized {} instances: {} up, {} up match, {} down, {} VT",
                  size_up_moves_ + size_up_match_moves_ + size_down_moves_
                      + vt_swap_moves_,
                  size_up_moves_,
                  size_up_match_moves_,
                  size_down_moves_,
                  vt_swap_moves_);
  }
  if (swap_pins_moves_ > 0) {
    repaired = true;
    logger_->info(RSZ, 43, "Swapped pins on {} instances.", swap_pins_moves_);
  }
  if (clone_moves_ > 0) {
    repaired = true;
    logger_->info(RSZ, 49, "Cloned {} instances.", clone_moves_);
  }
  const sta::Slack worst_slack = sta_->worstSlack(max_);
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
void RepairSetup::repairSetup(const sta::Pin* end_pin)
{
  init();
  max_repairs_per_pass_ = 1;

  Vertex* vertex = graph_->pinLoadVertex(end_pin);
  const Slack slack = sta_->slack(vertex, max_);
  Path* path = sta_->vertexWorstSlackPath(vertex, max_);

  move_sequence_.clear();
  move_sequence_ = {resizer_->unbuffer_move_.get(),
                    resizer_->vt_swap_speed_move_.get(),
                    resizer_->size_down_move_.get(),
                    resizer_->size_up_move_.get(),
                    resizer_->swap_pins_move_.get(),
                    resizer_->buffer_move_.get(),
                    resizer_->clone_move_.get(),
                    resizer_->split_load_move_.get()};

  {
    est::IncrementalParasiticsGuard guard(estimate_parasitics_);
    repairPath(path, slack, 0.0);
  }

  int unbuffer_moves_ = resizer_->unbuffer_move_->numCommittedMoves();
  if (unbuffer_moves_ > 0) {
    logger_->info(RSZ, 61, "Removed {} buffers.", unbuffer_moves_);
  }
  int buffer_moves_ = resizer_->buffer_move_->numCommittedMoves();
  int split_load_moves_ = resizer_->split_load_move_->numMoves();
  if (buffer_moves_ + split_load_moves_ > 0) {
    logger_->info(
        RSZ, 30, "Inserted {} buffers.", buffer_moves_ + split_load_moves_);
  }
  int size_up_moves_ = resizer_->size_up_move_->numMoves();
  int size_down_moves_ = resizer_->size_down_move_->numMoves();
  if (size_up_moves_ + size_down_moves_ > 0) {
    logger_->info(RSZ,
                  38,
                  "Resized {} instances, {} sized up, {} sized down.",
                  size_up_moves_ + size_down_moves_,
                  size_up_moves_,
                  size_down_moves_);
  }
  int swap_pins_moves_ = resizer_->swap_pins_move_->numMoves();
  if (swap_pins_moves_ > 0) {
    logger_->info(RSZ, 44, "Swapped pins on {} instances.", swap_pins_moves_);
  }
}

int RepairSetup::fanout(sta::Vertex* vertex)
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
bool RepairSetup::repairPath(sta::Path* path,
                             const sta::Slack path_slack,
                             const float setup_slack_margin)
{
  PathExpanded expanded(path, sta_);
  int changed = 0;

  if (expanded.size() > 1) {
    const int path_length = expanded.size();
    vector<pair<int, sta::Delay>> load_delays;
    const int start_index = expanded.startIndex();
    const Scene* corner = path->scene(sta_);
    if (path->minMax(sta_) != resizer_->max_) {
      logger_->error(RSZ, 500, "repairSetup expects max delay path");
      return false;
    }
    const int lib_ap = corner->libertyIndex(resizer_->max_);
    // Find load delay for each gate in the path.
    for (int i = start_index; i < path_length; i++) {
      const sta::Path* path = expanded.path(i);
      sta::Vertex* path_vertex = path->vertex(sta_);
      const sta::Pin* path_pin = path->pin(sta_);
      if (i > 0 && path_vertex->isDriver(network_)
          && !network_->isTopLevelPort(path_pin)) {
        const TimingArc* prev_arc = path->prevArc(sta_);
        const TimingArc* corner_arc = prev_arc->sceneArc(lib_ap);
        Edge* prev_edge = path->prevEdge(sta_);
        const Delay load_delay
            = graph_->arcDelay(
                  prev_edge, prev_arc, corner->dcalcAnalysisPtIndex(max_))
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

    std::ranges::sort(
        load_delays,

        [](pair<int, sta::Delay> pair1, pair<int, sta::Delay> pair2) {
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
      const sta::Path* drvr_path = expanded.path(drvr_index);
      sta::Vertex* drvr_vertex = drvr_path->vertex(sta_);
      const sta::Pin* drvr_pin = drvr_vertex->pin();
      sta::LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
      sta::LibertyCell* drvr_cell
          = drvr_port ? drvr_port->libertyCell() : nullptr;
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

      for (BaseMove* move : move_sequence_) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   1,
                   "Considering {} for {}",
                   move->name(),
                   network_->pathName(drvr_pin));

        if (move->doMove(drvr_path,
                         drvr_index,
                         path_slack,
                         &expanded,
                         setup_slack_margin)) {
          if (move == resizer_->unbuffer_move_.get()) {
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
                   "repair_setup",
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
    sta::Slack wns;
    sta::Vertex* worst_vertex;
    sta_->worstSlack(max_, wns, worst_vertex);
    const sta::Slack tns = sta_->totalNegativeSlack(max_);

    std::string itr_field
        = fmt::format("{}{}", iteration, (last_gasp ? "*" : ""));
    if (end) {
      itr_field = "final";
    }

    const double design_area = resizer_->computeDesignArea();
    const double area_growth = design_area - initial_design_area_;
    double area_growth_percent = std::numeric_limits<double>::infinity();
    if (std::abs(initial_design_area_) > 0.0) {
      area_growth_percent = area_growth / initial_design_area_ * 100.0;
    }

    // This actually prints both committed and pending moves, so the moves could
    // could go down if a pass is rejected and restored by the ECO.
    logger_->report(
        "{: >9s} | {: >7d} | {: >7d} | {: >8d} | {: >6d} | {: >5d} "
        "| {: >+7.1f}% | {: >8s} | {: >10s} | {: >6d} | {}",
        itr_field,
        resizer_->unbuffer_move_->numMoves(),
        resizer_->size_up_move_->numMoves()
            + resizer_->size_down_move_->numMoves()
            + resizer_->size_up_match_move_->numMoves()
            + resizer_->vt_swap_speed_move_->numMoves(),
        resizer_->buffer_move_->numMoves()
            + resizer_->split_load_move_->numMoves(),
        resizer_->clone_move_->numMoves(),
        resizer_->swap_pins_move_->numMoves(),
        area_growth_percent,
        delayAsString(wns, sta_, 3),
        delayAsString(tns, sta_, 1),
        max(0, num_viols),
        worst_vertex != nullptr ? worst_vertex->name(network_) : "");

    debugPrint(logger_, RSZ, "memory", 1, "RSS = {}", utl::getCurrentRSS());
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
void RepairSetup::repairSetupLastGasp(const OptoParams& params,
                                      int& num_viols,
                                      const int max_iterations)
{
  move_sequence_.clear();
  if (!params.skip_vt_swap) {
    move_sequence_.push_back(resizer_->vt_swap_speed_move_.get());
  }
  move_sequence_.push_back(resizer_->size_up_match_move_.get());
  move_sequence_.push_back(resizer_->size_up_move_.get());
  if (!params.skip_pin_swap) {
    move_sequence_.push_back(resizer_->swap_pins_move_.get());
  }

  // Sort remaining failing endpoints
  const VertexSet& endpoints = sta_->endpoints();
  vector<pair<Vertex*, Slack>> violating_ends;
  for (Vertex* end : endpoints) {
    const Slack end_slack = sta_->slack(end, max_);
    if (end_slack < params.setup_slack_margin) {
      violating_ends.emplace_back(end, end_slack);
    }
  }
  std::ranges::stable_sort(violating_ends,
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
  sta::Slack curr_worst_slack = violating_ends[0].second;
  sta::Slack prev_worst_slack = curr_worst_slack;
  bool prev_termination = false;
  bool two_cons_terminations = false;
  float fix_rate_threshold = inc_fix_rate_threshold_;

  for (const auto& end_original_slack : violating_ends) {
    if (max_iterations > 0 && opto_iteration >= max_iterations) {
      break;
    }

    fallback_ = false;
    Vertex* end = end_original_slack.first;
    Slack end_slack = sta_->slack(end, max_);
    Slack worst_slack;
    Vertex* worst_vertex;
    sta_->worstSlack(max_, worst_slack, worst_vertex);
    end_index++;
    if (end_index > max_end_count) {
      break;
    }
    int pass = 1;
    resizer_->journalBegin();
    bool journal_open = true;
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
        journal_open = false;
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
        journal_open = false;
        break;
      }
      sta::Path* end_path = sta_->vertexWorstSlackPath(end, max_);

      const bool changed
          = repairPath(end_path, end_slack, params.setup_slack_margin);

      if (!changed) {
        if (pass != 1) {
          resizer_->journalRestore();
        } else {
          resizer_->journalEnd();
        }
        journal_open = false;
        break;
      }
      estimate_parasitics_->updateParasitics();
      sta_->findRequireds();
      end_slack = sta_->slack(end, max_);
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
        if (pass < max_last_gasp_passes_) {
          resizer_->journalBegin();
        } else {
          journal_open = false;
        }
      } else {
        fallback_ = true;
        resizer_->journalRestore();
        journal_open = false;
        break;
      }

      if (resizer_->overMaxArea()) {
        resizer_->journalEnd();
        journal_open = false;
        break;
      }
      if (end_index == 1) {
        end = worst_vertex;
      }
      pass++;
      if (max_iterations > 0 && opto_iteration >= max_iterations) {
        resizer_->journalEnd();
        journal_open = false;
        break;
      }
    }  // while pass <= max_last_gasp_passes_
    if (journal_open) {
      resizer_->journalEnd();
    }
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

// Perform VT swap on remaining critical cells as a last resort
bool RepairSetup::swapVTCritCells(const OptoParams& params, int& num_viols)
{
  bool changed = false;

  // Start with sorted violating endpoints
  const VertexSet& endpoints = sta_->endpoints();
  vector<pair<Vertex*, Slack>> violating_ends;
  for (Vertex* end : endpoints) {
    const Slack end_slack = sta_->slack(end, max_);
    if (end_slack < params.setup_slack_margin) {
      violating_ends.emplace_back(end, end_slack);
    }
  }
  std::ranges::stable_sort(violating_ends,
                           [](const auto& end_slack1, const auto& end_slack2) {
                             return end_slack1.second < end_slack2.second;
                           });

  // Collect 50 critical instances from worst 100 violating endpoints
  // 50 x 100 = 5000 instances
  const size_t max_endpoints = 100;
  if (violating_ends.size() > max_endpoints) {
    violating_ends.resize(max_endpoints);
  }
  std::unordered_map<sta::Instance*, float> crit_insts;
  std::unordered_set<sta::Vertex*> visited;
  std::unordered_set<sta::Instance*> notSwappable;
  for (const auto& [endpoint, slack] : violating_ends) {
    traverseFaninCone(endpoint, crit_insts, visited, notSwappable, params);
  }
  debugPrint(logger_,
             RSZ,
             "swap_crit_vt",
             1,
             "identified {} critical instances",
             crit_insts.size());

  // Do VT swap on critical instances for now
  // Other transforms can follow later
  VTSwapSpeedMove* move = resizer_->vt_swap_speed_move_.get();
  for (auto crit_inst_slack : crit_insts) {
    if (move->doMove(crit_inst_slack.first, notSwappable)) {
      changed = true;
      debugPrint(logger_,
                 RSZ,
                 "swap_crit_vt",
                 1,
                 "inst {} did crit VT swap",
                 network_->pathName(crit_inst_slack.first));
    }
  }
  if (changed) {
    move->commitMoves();
    estimate_parasitics_->updateParasitics();
    sta_->findRequireds();
    violating_ends.clear();
    for (Vertex* end : endpoints) {
      const Slack end_slack = sta_->slack(end, max_);
      if (end_slack < params.setup_slack_margin) {
        violating_ends.emplace_back(end, end_slack);
      }
    }
    num_viols = violating_ends.size();
  }

  return changed;
}

// Traverse fanin code starting from this violaitng endpoint.
// Visit fanin instances only if they have violating slack.
// This avoids exponential path enumeration in findPathEnds.
void RepairSetup::traverseFaninCone(
    sta::Vertex* endpoint,
    std::unordered_map<sta::Instance*, float>& crit_insts,
    std::unordered_set<sta::Vertex*>& visited,
    std::unordered_set<sta::Instance*>& notSwappable,
    const OptoParams& params)

{
  if (visited.find(endpoint) != visited.end()) {
    return;
  }

  visited.insert(endpoint);
  // Limit number of critical instances per endpoint
  const int max_instances = 50;
  std::queue<sta::Vertex*> queue;
  queue.push(endpoint);
  int endpoint_insts = 0;
  sta::LibertyCell* best_lib_cell;

  while (!queue.empty() && endpoint_insts < max_instances) {
    sta::Vertex* current = queue.front();
    queue.pop();

    // Get the instance associated with this vertex
    sta::Instance* inst = nullptr;
    sta::Pin* pin = current->pin();
    if (pin) {
      inst = network_->instance(pin);
    }

    if (inst) {
      // Check if VT swap is possible
      if (resizer_->checkAndMarkVTSwappable(
              inst, notSwappable, best_lib_cell)) {
        // Check if this instance has negative slack
        const sta::Slack inst_slack = getInstanceSlack(inst);
        if (inst_slack < params.setup_slack_margin) {
          // Update worst slack for this instance
          auto it = crit_insts.find(inst);
          if (it == crit_insts.end()) {
            crit_insts[inst] = inst_slack;
            endpoint_insts++;
            debugPrint(logger_,
                       RSZ,
                       "swap_crit_vt",
                       1,
                       "swapVTCritCells: found crit inst {}: slack {}",
                       network_->name(inst),
                       inst_slack);
          }
        }
      }
    }

    // Traverse fanin edges
    VertexInEdgeIterator edge_iter(current, graph_);
    while (edge_iter.hasNext()) {
      Edge* edge = edge_iter.next();
      sta::Vertex* fanin_vertex = edge->from(graph_);
      if (fanin_vertex->isRegClk()) {
        continue;
      }

      // Only traverse if we haven't visited and the fanin has negative slack
      if (visited.find(fanin_vertex) == visited.end()) {
        const Slack fanin_slack = sta_->slack(fanin_vertex, max_);
        if (fanin_slack < params.setup_slack_margin) {
          queue.push(fanin_vertex);
          visited.insert(fanin_vertex);
        }
      }
    }
  }

  debugPrint(logger_,
             RSZ,
             "swap_crit_vt",
             1,
             "traverseFaninCone: endpoint {} has {} critical instances:",
             endpoint->name(network_),
             endpoint_insts);
  if (logger_->debugCheck(RSZ, "swap_crit_vt", 1)) {
    for (auto crit_inst_slack : crit_insts) {
      logger_->report(" {}", network_->pathName(crit_inst_slack.first));
    }
  }
}

sta::Slack RepairSetup::getInstanceSlack(sta::Instance* inst)
{
  sta::Slack worst_slack = std::numeric_limits<float>::max();

  // Check all output pins of the instance
  InstancePinIterator* pin_iter = network_->pinIterator(inst);
  while (pin_iter->hasNext()) {
    sta::Pin* pin = pin_iter->next();
    if (network_->direction(pin)->isAnyOutput()) {
      sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
      if (vertex) {
        const Slack pin_slack = sta_->slack(vertex, max_);
        worst_slack = std::min(worst_slack, pin_slack);
      }
    }
  }
  delete pin_iter;

  return worst_slack;
}

}  // namespace rsz
