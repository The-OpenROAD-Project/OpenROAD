// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "RepairSetup.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "BaseMove.hh"
#include "BufferMove.hh"
#include "CloneMove.hh"
#include "MoveTracker.hh"
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
#include "ViolatorCollector.hh"
#include "rsz/Resizer.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "sta/TimingArc.hh"
#include "utl/Logger.h"
#include "utl/mem_stats.h"
#include "utl/timer.h"

namespace rsz {

using std::max;
using std::pair;
using std::string;
using std::vector;
using utl::RSZ;

RepairSetup::RepairSetup(Resizer* resizer) : resizer_(resizer)
{
}

RepairSetup::~RepairSetup() = default;

void RepairSetup::init()
{
  logger_ = resizer_->logger_;
  dbStaState::init(resizer_->sta_);
  db_network_ = resizer_->db_network_;
  estimate_parasitics_ = resizer_->estimate_parasitics_;
  initial_design_area_ = resizer_->computeDesignArea();

  violator_collector_ = std::make_unique<ViolatorCollector>(resizer_);

  // Only create MoveTracker if debug level is enabled
  if (logger_->debugCheck(RSZ, "move_tracker", 1)) {
    move_tracker_ = std::make_unique<MoveTracker>(
        logger_, sta_, db_network_, resizer_->block_);
  }
}

void RepairSetup::setupMoveSequence(const std::vector<MoveType>& sequence,
                                    const bool skip_pin_swap,
                                    const bool skip_gate_cloning,
                                    const bool skip_size_down,
                                    const bool skip_buffering,
                                    const bool skip_buffer_removal,
                                    const bool skip_vt_swap)
{
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
}

bool RepairSetup::repairSetup(const float setup_slack_margin,
                              const double repair_tns_end_percent,
                              const int max_passes,
                              const int max_iterations,
                              const int max_repairs_per_pass,
                              const bool verbose,
                              const std::vector<MoveType>& sequence,
                              const char* phases,
                              const bool skip_pin_swap,
                              const bool skip_gate_cloning,
                              const bool skip_size_down,
                              const bool skip_buffering,
                              const bool skip_buffer_removal,
                              const bool skip_last_gasp,
                              const bool skip_vt_swap,
                              const bool skip_crit_vt_swap)
{
  const utl::DebugScopedTimer timer(
      logger_, RSZ, "repair_setup", 1, "Total setup repair time: {}");
  bool repaired = false;
  init();
  resizer_->rebuffer_->init();
  // IMPROVE ME: rebuffering always looks at cmd corner
  resizer_->rebuffer_->initOnCorner(sta_->cmdScene());
  max_repairs_per_pass_ = max_repairs_per_pass;
  resizer_->buffer_moved_into_core_ = false;

  violator_collector_->init(setup_slack_margin);
  violator_collector_->setMaxPassesPerEndpoint(max_passes);
  violator_collector_->clearConsideredPins();

  setupMoveSequence(sequence,
                    skip_pin_swap,
                    skip_gate_cloning,
                    skip_size_down,
                    skip_buffering,
                    skip_buffer_removal,
                    skip_vt_swap);

  // ViolatorCollector has already collected and sorted violating endpoints
  const sta::VertexSet& endpoints = sta_->endpoints();
  int num_viols = violator_collector_->getTotalViolations();

  if (num_viols > 0) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "Violating endpoints {}/{} {}%",
               num_viols,
               endpoints.size(),
               int(num_viols / double(endpoints.size()) * 100));
    logger_->info(
        RSZ, 94, "Found {} endpoints with setup violations.", num_viols);
  } else {
    // Nothing to repair
    logger_->metric("design__instance__count__setup_buffer", 0);
    logger_->info(RSZ, 98, "No setup violations found");
    return false;
  }

  float initial_tns = sta_->totalNegativeSlack(max_);
  float prev_tns = initial_tns;
  max_end_repairs_
      = violator_collector_->getMaxEndpointCount() * repair_tns_end_percent;
  max_end_repairs_ = std::max(max_end_repairs_, 1);
  logger_->info(RSZ,
                99,
                "Repairing {} out of {} ({:0.2f}%) violating endpoints...",
                max_end_repairs_,
                num_viols,
                repair_tns_end_percent * 100.0);

  // Ensure that max cap and max fanout violations don't get worse
  sta_->checkCapacitancesPreamble(sta_->scenes());
  sta_->checkSlewsPreamble();
  sta_->checkFanoutPreamble();

  est::IncrementalParasiticsGuard guard(estimate_parasitics_);
  int opto_iteration = 0;

  // Capture initial slack distribution and original endpoint slack (once
  // before all phases)
  if (move_tracker_) {
    move_tracker_->captureInitialSlackDistribution();
    move_tracker_->captureOriginalEndpointSlack();
  }

  // Set default phase sequence if none specified
  // Default: LEGACY, LAST_GASP
  std::string phases_str;
  if (phases != nullptr && phases[0] != '\0') {
    phases_str = phases;
    logger_->info(RSZ, 221, "Using custom phase sequence: {}", phases);
  } else {
    phases_str = "LEGACY LAST_GASP";
  }

  // Parse and execute phase sequence
  // Example: repair_timing -phases "WNS_PATH TNS WNS_CONE"
  // Helper function to get phase marker from global phase index
  // Marker sequence: *+^&@!-= then a-z then A-Z
  auto getPhaseMarker = [](int phase_index) -> char {
    constexpr char special_markers[] = "*+^&@!-=";
    constexpr int num_special = 8;
    if (phase_index < num_special) {
      return special_markers[phase_index];
    }
    phase_index -= num_special;

    if (phase_index < 26) {
      return 'a' + phase_index;
    }
    phase_index -= 26;

    if (phase_index < 26) {
      return 'A' + phase_index;
    }
    return '?';  // Fallback if we exceed all markers
  };

  std::istringstream iss(phases_str);
  std::string phase_name;

  // Track global phase index for unified marker assignment
  int phase_index = 0;
  char marker = getPhaseMarker(phase_index++);

  while (iss >> phase_name) {
    if (phase_name == "LEGACY") {
      repairSetup_Legacy(setup_slack_margin,
                         max_passes,
                         max_iterations,
                         max_repairs_per_pass,
                         verbose,
                         opto_iteration,
                         initial_tns,
                         prev_tns,
                         marker);  // phase marker

      if (move_tracker_) {
        move_tracker_->printMoveSummary("LEGACY Phase Summary");
        move_tracker_->printEndpointSummary("LEGACY Phase Endpoint Profiler");
        move_tracker_->clear();
      }
    } else if (phase_name == "WNS_PATH" || phase_name == "WNS") {
      repairSetup_Wns(
          setup_slack_margin,
          max_passes,
          max_repairs_per_pass,
          verbose,
          opto_iteration,
          initial_tns,
          prev_tns,
          false,   // use_cone_collection = false (use path-based collection)
          marker,  // phase marker
          ViolatorSortType::SORT_BY_LOAD_DELAY);

      if (move_tracker_) {
        move_tracker_->printMoveSummary("WNS_PATH Phase Summary");
        move_tracker_->printEndpointSummary("WNS_PATH Phase Endpoint Profiler");
        move_tracker_->clear();
      }
    } else if (phase_name == "WNS_CONE") {
      repairSetup_Wns(
          setup_slack_margin,
          max_passes,
          max_repairs_per_pass,
          verbose,
          opto_iteration,
          initial_tns,
          prev_tns,
          true,    // use_cone_collection = true (use cone-based collection)
          marker,  // phase_marker
          ViolatorSortType::SORT_BY_LOAD_DELAY);

      if (move_tracker_) {
        move_tracker_->printMoveSummary("WNS_CONE Phase Summary");
        move_tracker_->printEndpointSummary("WNS_CONE Phase Endpoint Profiler");
        move_tracker_->clear();
      }
    } else if (phase_name == "TNS") {
      repairSetup_Tns(setup_slack_margin,
                      max_passes,
                      max_repairs_per_pass,
                      verbose,
                      opto_iteration,
                      initial_tns,
                      prev_tns,
                      marker,  // phase_marker
                      ViolatorSortType::SORT_BY_LOAD_DELAY);

      if (move_tracker_) {
        move_tracker_->printMoveSummary("TNS Phase Summary");
        move_tracker_->printEndpointSummary("TNS Phase Endpoint Profiler");
        move_tracker_->clear();
      }
    } else if (phase_name == "ENDPOINT_FANIN") {
      repairSetup_EndpointFanin(setup_slack_margin,
                                max_passes,
                                verbose,
                                opto_iteration,
                                marker);  // phase_marker

      if (move_tracker_) {
        move_tracker_->printMoveSummary("ENDPOINT_FANIN Phase Summary");
        move_tracker_->printEndpointSummary(
            "ENDPOINT_FANIN Phase Endpoint Profiler");
        move_tracker_->clear();
      }
    } else if (phase_name == "STARTPOINT_FANOUT") {
      repairSetup_StartpointFanout(setup_slack_margin,
                                   max_passes,
                                   verbose,
                                   opto_iteration,
                                   marker);  // phase_marker

      if (move_tracker_) {
        move_tracker_->printMoveSummary("STARTPOINT_FANOUT Phase Summary");
        move_tracker_->printEndpointSummary(
            "STARTPOINT_FANOUT Phase Startpoint Profiler");
        move_tracker_->clear();
      }
    } else if (phase_name == "LAST_GASP") {
      if (!skip_last_gasp) {
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
        repairSetup_LastGasp(params,
                             num_viols,
                             max_iterations,
                             marker);  // phase_marker

        if (move_tracker_) {
          move_tracker_->printMoveSummary("LAST_GASP Phase Summary");
          move_tracker_->printEndpointSummary(
              "LAST_GASP Phase Endpoint Profiler");
          move_tracker_->clear();
        }
      }
    } else {
      logger_->error(RSZ,
                     217,
                     "Unknown phase name '{}'. Valid phase names are: LEGACY, "
                     "WNS, TNS, ENDPOINT_FANIN, STARTPOINT_FANOUT, LAST_GASP",
                     phase_name);
    }
    // Update marker for next phase
    marker = getPhaseMarker(phase_index++);
  }

  // VT swap phase (runs after all phases if conditions are met)
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
    if (move_tracker_) {
      move_tracker_->printMoveSummary("VT Swap Phase Summary");
      move_tracker_->clear();
    }
  }

  // Print final summary row showing end state of all phases
  // (end=true will also print the footer)
  printProgress(opto_iteration, true, true, ' ');

  // Print comprehensive optimization reports (only if tracking is enabled)
  if (move_tracker_) {
    // Pass all critical pins to MoveTracker for missed opportunities analysis
    const auto& critical_pins = violator_collector_->getViolatingPins();
    move_tracker_->trackCriticalPins(critical_pins);

    logger_->info(RSZ, 211, "");
    logger_->info(RSZ, 212, "=== Optimization Analysis Reports ===");
    move_tracker_->printSlackDistribution("Pin Slack Distribution");
    move_tracker_->printTopBinEndpoints(
        "Most Critical Endpoints After Optimization");
    move_tracker_->printCriticalEndpointPathHistogram(
        "Critical Endpoint Path Distribution");
    move_tracker_->printSuccessReport("Successful Optimizations Report");
    move_tracker_->printFailureReport("Unsuccessful Optimizations Report");
    move_tracker_->printMissedOpportunitiesReport(
        "Missed Opportunities Report");
    logger_->info(RSZ, 213, "");
  }

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
  if (sta::fuzzyLess(worst_slack, setup_slack_margin)) {
    repaired = true;
    logger_->warn(RSZ, 62, "Unable to repair all setup violations.");
  }
  if (resizer_->overMaxArea()) {
    logger_->error(RSZ, 25, "max utilization reached.");
  }

  // Destruct MoveTracker as there's no need to track odb events outside of
  // repair setup
  if (move_tracker_) {
    move_tracker_.reset();
  }

  return repaired;
}

// For testing.
void RepairSetup::repairSetup(const sta::Pin* end_pin)
{
  init();
  max_repairs_per_pass_ = 1;

  sta::Vertex* vertex = graph_->pinLoadVertex(end_pin);
  const sta::Slack slack = sta_->slack(vertex, max_);

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
    repairEndpoint(vertex->pin(), slack, 0.0);
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
  sta::VertexOutEdgeIterator edge_iter(vertex, graph_);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    // Disregard output->output timing arcs
    if (edge->isWire()) {
      fanout++;
    }
  }
  return fanout;
}

bool RepairSetup::repairPins(
    const std::vector<const sta::Pin*>& pins,
    const float setup_slack_margin,
    const std::map<const sta::Pin*, std::set<BaseMove*>>* rejected_moves,
    std::vector<std::pair<const sta::Pin*, BaseMove*>>* chosen_moves)
{
  int changed = 0;

  if (pins.empty()) {
    return false;
  }
  int repairs_per_pass
      = violator_collector_->repairsPerPass(max_repairs_per_pass_);
  if (fallback_) {
    repairs_per_pass = 1;
  }
  debugPrint(
      logger_,
      RSZ,
      "repair_setup",
      3,
      "Path slack: {}, repairs: {}, load_delays: {}",
      delayAsString(violator_collector_->getCurrentEndpointSlack(), 3, sta_),
      repairs_per_pass,
      pins.size());
  for (const auto& drvr_pin : pins) {
    if (changed >= repairs_per_pass) {
      break;
    }
    // Track this pin as a violator before attempting moves
    sta::Vertex* drvr_vertex = graph_->pinDrvrVertex(drvr_pin);
    sta::LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
    sta::LibertyCell* drvr_cell
        = drvr_port ? drvr_port->libertyCell() : nullptr;

    // Get detailed information for MoveTracker reporting
    string gate_type = drvr_cell ? drvr_cell->name() : "unknown";
    float load_delay = 0.0;
    float intrinsic_delay = 0.0;

    // Try to get delay data from ViolatorCollector cache first
    if (!violator_collector_->getPinData(
            drvr_pin, load_delay, intrinsic_delay)) {
      // Calculate delays using ViolatorCollector's getEffortDelays method
      auto [load, intrinsic] = violator_collector_->getEffortDelays(drvr_pin);
      load_delay = load;
      intrinsic_delay = intrinsic;
    }

    sta::Slack pin_slack = sta_->slack(drvr_vertex, max_);
    sta::Slack endpoint_slack = violator_collector_->getCurrentEndpointSlack();

    if (move_tracker_) {
      move_tracker_->trackViolatorWithInfo(drvr_pin,
                                           gate_type,
                                           load_delay,
                                           intrinsic_delay,
                                           pin_slack,
                                           endpoint_slack);
    }
    const string drvr_pin_name = network_->pathName(drvr_pin);
    const int fanout = this->fanout(drvr_vertex);
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               4,
               "Running move sequence for driver {} "
               "(cell type {}, fanout = {})",
               drvr_pin_name,
               drvr_cell ? drvr_cell->name() : "none",
               fanout);

    for (BaseMove* move : move_sequence_) {
      // Skip this move if it's been tried and rejected for this pin
      if (rejected_moves) {
        auto it = rejected_moves->find(drvr_pin);
        if (it != rejected_moves->end() && it->second.contains(move)) {
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     6,
                     "Skipping {} for {} (previously rejected)",
                     move->name(),
                     drvr_pin_name);
          continue;
        }
      }

      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 6,
                 "Considering {} for {}",
                 move->name(),
                 drvr_pin_name);

      if (move_tracker_) {
        move_tracker_->trackMove(
            drvr_pin, string(move->name()), rsz::MoveStateType::ATTEMPT);
      }
      if (move->doMove(drvr_pin, setup_slack_margin)) {
        // Record this successful move
        if (chosen_moves) {
          chosen_moves->emplace_back(drvr_pin, move);
        }

        if (move == resizer_->unbuffer_move_.get()) {
          // Only allow one unbuffer move per pass to
          // prevent the use-after-free error of multiple buffer removals.
          changed += repairs_per_pass;
        } else {
          changed++;
        }
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   5,
                   "Move {} succeeded for {}",
                   move->name(),
                   drvr_pin_name);
        // Move on to the next gate
        break;
      }

      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 6,
                 "Move {} failed for {}",
                 move->name(),
                 drvr_pin_name);
    }
  }
  return changed > 0;
}

bool RepairSetup::repairEndpoint(sta::Pin* end_pin,
                                 const sta::Slack path_slack,
                                 const float setup_slack_margin)
{
  sta::Vertex* end_vertex = graph_->pinLoadVertex(end_pin);
  sta::Path* path = sta_->vertexWorstSlackPath(end_vertex, max_);
  sta::PathExpanded expanded(path, sta_);
  int changed = 0;

  if (expanded.size() > 1) {
    const int path_length = expanded.size();
    vector<pair<int, sta::Delay>> load_delays;
    const int start_index = expanded.startIndex();
    const sta::Scene* corner = path->scene(sta_);
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
        const sta::TimingArc* prev_arc = path->prevArc(sta_);
        const sta::TimingArc* corner_arc = prev_arc->sceneArc(lib_ap);
        sta::Edge* prev_edge = path->prevEdge(sta_);
        const sta::Delay load_delay
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
                   delayAsString(load_delay, 3, sta_),
                   delayAsString(corner_arc->intrinsicDelay(), 3, sta_));
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
               delayAsString(path_slack, 3, sta_),
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

      const string drvr_pin_name = network_->pathName(drvr_pin);
      const int fanout = this->fanout(drvr_vertex);
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 4,
                 "Running move sequence for driver {} "
                 "(cell type {}, fanout = {})",
                 drvr_pin_name,
                 drvr_cell ? drvr_cell->name() : "none",
                 fanout);

      for (BaseMove* move : move_sequence_) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   6,
                   "Considering {} for {}",
                   move->name(),
                   drvr_pin_name);

        if (move->doMove(drvr_path, setup_slack_margin)) {
          if (move == resizer_->unbuffer_move_.get()) {
            // Only allow one unbuffer move per pass to
            // prevent the use-after-free error of multiple buffer removals.
            changed += repairs_per_pass;
          } else {
            changed++;
          }
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     5,
                     "Move {} succeeded for {}",
                     move->name(),
                     drvr_pin_name);
          // Move on to the next gate
          break;
        }
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   6,
                   "Move {} failed for {}",
                   move->name(),
                   drvr_pin_name);
      }
    }
  }
  return changed > 0;
}

void RepairSetup::printProgress(const int iteration,
                                const bool force,
                                const bool end,
                                const char phase_marker,
                                const bool use_startpoint_metrics) const
{
  const bool start = iteration == 0;

  if (start && !end) {
    logger_->report(
        "   Iter   | Removed | Resized | Inserted | Cloned |  Pin  |"
        "   Area   |    WNS   |   StTNS    |   EnTNS    |  Viol  |  Worst  ");
    logger_->report(
        "          | Buffers |  Gates  | Buffers  |  Gates | Swaps |"
        "          |          |            |            | Endpts | St/EnPt ");
    logger_->report(
        "---------------------------------------------------------------"
        "---------------------------------------------------------------");
  }

  if (iteration % print_interval_ == 0 || force || end) {
    // Re-collect violating endpoints if all phases ended
    if (end) {
      violator_collector_->collectViolatingEndpoints();
      violator_collector_->collectViolatingStartpoints();
    }
    // Always calculate both startpoint and endpoint metrics for display
    sta::Slack wns = violator_collector_->getWns();  // WNS is same for both
    sta::Slack st_tns = violator_collector_->getTns(true);   // Startpoint TNS
    sta::Slack en_tns = violator_collector_->getTns(false);  // Endpoint TNS
    const sta::Pin* worst_pin
        = violator_collector_->getWorstPin(use_startpoint_metrics);

    std::string itr_field = fmt::format("{}{}", iteration, phase_marker);
    if (end) {
      itr_field = "final";
    }

    const double design_area = resizer_->computeDesignArea();
    const double area_growth = design_area - initial_design_area_;
    double area_growth_percent = std::numeric_limits<double>::infinity();
    if (std::abs(initial_design_area_) > 0.0) {
      area_growth_percent = area_growth / initial_design_area_ * 100.0;
    }

    // This actually prints both committed and pending moves, so the moves
    // could could go down if a pass is rejected and restored by the ECO.
    logger_->report(
        "{: >9s} | {: >7d} | {: >7d} | {: >8d} | {: >6d} | {: >5d} "
        "| {: >+7.1f}% | {: >8s} | {: >10s} | {: >10s} | {: >6d} | {}",
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
        delayAsString(wns, 3, sta_),
        delayAsString(st_tns, 1, sta_),
        delayAsString(en_tns, 1, sta_),
        max(0, violator_collector_->getNumViolatingEndpoints()),
        worst_pin != nullptr ? network_->pathName(worst_pin) : "");

    debugPrint(logger_, RSZ, "memory", 1, "RSS = {}", utl::getCurrentRSS());
  }

  if (end) {
    logger_->report(
        "---------------------------------------------------------------"
        "---------------------------------------------------------------");
  }
}

void RepairSetup::printProgressHeader() const
{
  logger_->report(
      "   Iter   | Removed | Resized | Inserted | Cloned |  Pin  |"
      "   Area   |    WNS   |   StTNS    |   EnTNS    |  Viol  |  Worst  ");
  logger_->report(
      "          | Buffers |  Gates  | Buffers  |  Gates | Swaps |"
      "          |          |            |            | Endpts | St/EnPt ");
  logger_->report(
      "---------------------------------------------------------------"
      "---------------------------------------------------------------");
}

void RepairSetup::printProgressFooter() const
{
  logger_->report(
      "---------------------------------------------------------------"
      "---------------------------------------------------------------");
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
                                    const int num_endpts,
                                    const char* phase_name,
                                    const char phase_marker)
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
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "{}{} Phase: Exiting at iteration {} because incr fix rate "
                 "{:0.2f}% is < {:0.2f}% [endpoint {}/{}]",
                 phase_name,
                 phase_marker,
                 iteration,
                 inc_fix_rate * 100,
                 fix_rate_threshold * 100,
                 endpt_index,
                 num_endpts);
      return true;
    }
  }
  return false;
}

// Legacy repair setup function
void RepairSetup::repairSetup_Legacy(const float setup_slack_margin,
                                     const int max_passes,
                                     const int max_iterations,
                                     const int max_repairs_per_pass,
                                     const bool verbose,
                                     int& opto_iteration,
                                     const float initial_tns,
                                     float& prev_tns,
                                     const char phase_marker)
{
  const utl::DebugScopedTimer timer(
      logger_,
      RSZ,
      "repair_setup",
      1,
      fmt::format("LEGACY{} Phase Time: {{}}", phase_marker));
  constexpr int digits = 3;

  // FIXME: We may want to switch to dynamically increasing slack passes
  constexpr int decreasing_slack_max_passes = 50;

  violator_collector_->init(setup_slack_margin);

  int max_endpoint_count = violator_collector_->getMaxEndpointCount();
  if (max_endpoint_count == 0) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "LEGACY{} Phase: No violating endpoints, exiting",
               phase_marker);
    return;
  }
  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "LEGACY{} Phase: {} violating endpoints found",
             phase_marker,
             max_endpoint_count);

  int end_index = 0;
  bool prev_termination = false;
  bool two_cons_terminations = false;
  printProgress(opto_iteration, false, false, phase_marker);
  float fix_rate_threshold = inc_fix_rate_threshold_;

  const auto& violating_ends = violator_collector_->getViolatingEndpoints();
  min_viol_ = -violating_ends.back().second;
  max_viol_ = -violating_ends.front().second;

  // Main Legacy Phase loop - Repair each violating endpoint starting with
  // worst
  while (violator_collector_->hasMoreEndpoints()) {
    fallback_ = false;
    violator_collector_->advanceToNextEndpoint();
    sta::Vertex* end = violator_collector_->getCurrentEndpoint();
    if (!end) {
      continue;
    }
    sta::Slack end_slack = sta_->slack(end, max_);
    sta::Slack worst_slack;
    sta::Vertex* worst_vertex;
    sta_->worstSlack(max_, worst_slack, worst_vertex);
    end_index++;
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "LEGACY{} Phase: Doing endpoint {} ({}/{}) "
               "WNS = {}, endpoint slack = {}, TNS = {}",
               phase_marker,
               end->name(network_),
               end_index,
               max_end_repairs_,
               delayAsString(worst_slack, digits, sta_),
               delayAsString(end_slack, digits, sta_),
               delayAsString(prev_tns, 1, sta_));
    if (end_index > max_end_repairs_) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "LEGACY{} Phase: Hit maximum endpoint repairs of {}",
                 phase_marker,
                 max_end_repairs_);
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
        printProgress(opto_iteration, false, false, phase_marker);
      }
      if (terminateProgress(opto_iteration,
                            initial_tns,
                            prev_tns,
                            fix_rate_threshold,
                            end_index,
                            max_end_repairs_,
                            "LEGACY",
                            phase_marker)) {
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
                   "LEGACY{} Phase: Restoring best slack; "
                   "endpoint slack = {}, WNS = {}",
                   phase_marker,
                   delayAsString(prev_end_slack, digits, sta_),
                   delayAsString(prev_worst_slack, digits, sta_));
        resizer_->journalRestore();
        journal_open = false;
        break;
      }
      if (opto_iteration % opto_small_interval_ == 0) {
        prev_termination = false;
      }

      if (sta::fuzzyGreaterEqual(end_slack, setup_slack_margin)) {
        if (pass != 1) {
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     2,
                     "LEGACY{} Phase: Restoring best slack; "
                     "endpoint slack = {}, WNS = {}",
                     phase_marker,
                     delayAsString(prev_end_slack, digits, sta_),
                     delayAsString(prev_worst_slack, digits, sta_));
          resizer_->journalRestore();
        } else {
          resizer_->journalEnd();
        }
        journal_open = false;
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   1,
                   "LEGACY{} Phase: Endpoint slack {} meets "
                   "slack margin {}, done",
                   phase_marker,
                   delayAsString(worst_slack, digits, sta_),
                   delayAsString(setup_slack_margin, digits, sta_));
        break;
      }

      float prev_tns_local = sta_->totalNegativeSlack(max_);

      const bool changed
          = repairEndpoint(end->pin(), end_slack, setup_slack_margin);

      if (!changed) {
        if (pass != 1) {
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     2,
                     "LEGACY{} Phase: No change after {} "
                     "decreasing slack passes.",
                     phase_marker,
                     decreasing_slack_passes);
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     2,
                     "LEGACY{} Phase: Restoring best slack; "
                     "endpoint slack = {}, WNS = {}",
                     phase_marker,
                     delayAsString(prev_end_slack, digits, sta_),
                     delayAsString(prev_worst_slack, digits, sta_));
          resizer_->journalRestore();
        } else {
          resizer_->journalEnd();
        }
        journal_open = false;
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   1,
                   "LEGACY{} Phase: No change possible for endpoint {} ",
                   phase_marker,
                   end->name(network_));
        break;
      }

      estimate_parasitics_->updateParasitics();
      sta_->findRequireds();
      end_slack = sta_->slack(end, max_);
      sta_->worstSlack(max_, worst_slack, worst_vertex);
      sta::Slack new_tns = sta_->totalNegativeSlack(max_);
      const bool better
          = (sta::fuzzyGreater(worst_slack, prev_worst_slack)
             || (end_index != 1
                 && sta::fuzzyEqual(worst_slack, prev_worst_slack)
                 && sta::fuzzyGreater(end_slack, prev_end_slack)));

      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 3,
                 "LEGACY{} Phase: {} after changes: "
                 "WNS ({} -> {}) "
                 "TNS ({} -> {}) "
                 "Endpoint slack ({} -> {})",
                 phase_marker,
                 better ? "Improved" : "Worsened",
                 delayAsString(prev_worst_slack, digits, sta_),
                 delayAsString(worst_slack, digits, sta_),
                 delayAsString(prev_tns_local, 1, sta_),
                 delayAsString(new_tns, 1, sta_),
                 delayAsString(prev_end_slack, digits, sta_),
                 delayAsString(end_slack, digits, sta_));

      if (better) {
        prev_end_slack = end_slack;
        prev_worst_slack = worst_slack;
        decreasing_slack_passes = 0;
        resizer_->journalEnd();
        if (pass < max_passes) {
          // Progress, Save checkpoint so we can back up to here
          resizer_->journalBegin();
        } else {
          journal_open = false;
        }
      } else {
        fallback_ = true;
        // Allow slack to increase to get out of local minima.
        // Do not update prev_end_slack so it saves the high water mark.
        decreasing_slack_passes++;
        if (decreasing_slack_passes > decreasing_slack_max_passes) {
          // Undo changes that reduced slack
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     2,
                     "LEGACY{} Phase: Endpoint {} stuck after {} "
                     "non-improving passes "
                     "(limit {})",
                     phase_marker,
                     network_->pathName(end->pin()),
                     decreasing_slack_passes,
                     decreasing_slack_max_passes);
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     2,
                     "LEGACY{} Phase: Restoring best slack; "
                     "endpoint slack = {}, WNS = {}",
                     phase_marker,
                     delayAsString(prev_end_slack, digits, sta_),
                     delayAsString(prev_worst_slack, digits, sta_));
          resizer_->journalRestore();
          journal_open = false;
          break;
        }
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   3,
                   "LEGACY{} Phase: Allowing decreasing slack for {}/{} passes",
                   phase_marker,
                   decreasing_slack_passes,
                   decreasing_slack_max_passes);
      }

      if (resizer_->overMaxArea()) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   1,
                   "LEGACY{} Phase: Over max area, exiting",
                   phase_marker);
        resizer_->journalEnd();
        journal_open = false;
        break;
      }
      if (end_index == 1) {
        end = worst_vertex;
        violator_collector_->useWorstEndpoint(end);
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
      printProgress(opto_iteration, true, false, phase_marker);
    }
    if (two_cons_terminations) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "LEGACY{} Phase: Exiting due to no TNS progress "
                 "for two opto cycles",
                 phase_marker);
      break;
    }
  }  // for each violating endpoint

  printProgress(opto_iteration, true, false, phase_marker);

  sta::Slack final_wns;
  sta::Vertex* final_worst;
  sta_->worstSlack(max_, final_wns, final_worst);
  float final_tns = sta_->totalNegativeSlack(max_);

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "LEGACY{} Phase complete. WNS: {}, TNS: {}",
             phase_marker,
             delayAsString(final_wns, digits, sta_),
             delayAsString(final_tns, 1, sta_));
}

// WNS Phase: WNS-Focused Repair
// Focus on the worst slack endpoint at any time, switching dynamically when a
// different endpoint becomes worst.
void RepairSetup::repairSetup_Wns(const float setup_slack_margin,
                                  const int max_passes_per_endpoint,
                                  const int max_repairs_per_pass,
                                  const bool verbose,
                                  int& opto_iteration,
                                  const float initial_tns,
                                  float& prev_tns,
                                  const bool use_cone_collection,
                                  const char phase_marker,
                                  const ViolatorSortType sort_type)
{
  const utl::DebugScopedTimer timer(
      logger_,
      RSZ,
      "repair_setup",
      1,
      fmt::format("WNS{} Phase Time: {{}}", phase_marker));
  constexpr int digits = 3;

  // Capture pre-phase slack for tracking per-phase improvements
  if (move_tracker_) {
    move_tracker_->capturePrePhaseSlack();
  }

  // Initialize WNS Phase tracking
  wns_no_progress_count_ = 0;
  rejected_pin_moves_current_endpoint_.clear();

  // WNS Phase: Track passes for endpoints that have been the worst (WNS)
  // endpoint
  std::map<const sta::Pin*, int> wns_endpoint_pass_counts_;

  // Round-robin tracking
  std::set<const sta::Pin*> endpoints_visited_this_round;
  bool any_improvement_this_round = false;
  int round_number = 0;

  // Per-endpoint "stuck" tracking (consecutive passes without improvement)
  sta::Slack target_end_slack = 0;
  sta::Slack target_worst_slack = 0;
  sta::Slack target_tns_local = 0;
  int total_decreasing_slack_passes = 0;
  std::set<const sta::Pin*> decreasing_slack_endpoints;
  std::map<const sta::Pin*, int> decreasing_slack_counts;
  std::map<const sta::Pin*, int> endpoint_pass_limits;
  std::map<const sta::Pin*, sta::Slack> endpoint_prev_slack;

  // Get initial worst endpoint
  sta::Slack worst_slack;
  sta::Vertex* worst_endpoint;
  sta_->worstSlack(max_, worst_slack, worst_endpoint);

  if (!worst_endpoint) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "WNS{} Phase: No worst endpoint found",
               phase_marker);
    return;
  }

  // Print phase header and table
  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "WNS{} Phase: Focusing on worst slack path{}...",
             phase_marker,
             use_cone_collection ? " with fanin cone collection" : "");
  printProgress(opto_iteration, false, false, phase_marker);

  // Initialize the violator collector with the actual worst endpoint
  sta::Vertex* current_endpoint = worst_endpoint;
  violator_collector_->useWorstEndpoint(worst_endpoint);
  violator_collector_->markEndpointVisitedInWns(worst_endpoint->pin());

  // Initialize tracking for the initial endpoint
  const sta::Pin* initial_pin = worst_endpoint->pin();
  endpoint_pass_limits[initial_pin] = initial_decreasing_slack_max_passes_;
  decreasing_slack_counts[initial_pin] = 0;

  // Track initial endpoint in MoveTracker
  if (move_tracker_) {
    move_tracker_->setCurrentEndpoint(current_endpoint->pin());
  }

  overall_no_progress_count_ = 0;
  float fix_rate_threshold = inc_fix_rate_threshold_;
  constexpr int max_no_progress = 4;
  const int phase1_start_iteration = opto_iteration;
  bool journal_open = false;

  // Main WNS Phase loop - Round-robin sticky algorithm
  while (true) {
    // Get current worst endpoint
    sta_->worstSlack(max_, worst_slack, worst_endpoint);

    if (!worst_endpoint) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "WNS{} Phase: No worst endpoint, exiting",
                 phase_marker);
      break;
    }

    const sta::Pin* worst_pin = worst_endpoint->pin();

    // Check if worst endpoint has already been visited this round
    if (endpoints_visited_this_round.contains(worst_pin)) {
      if (!any_improvement_this_round) {
        // No progress in entire round - exit WNS phase
        debugPrint(
            logger_,
            RSZ,
            "repair_setup",
            1,
            "WNS{} Phase: Round {} complete with no improvement, exiting",
            phase_marker,
            round_number);
        break;
      }
      // Made progress this round - start new round
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 2,
                 "WNS{} Phase: Round {} complete with improvement, starting "
                 "round {}",
                 phase_marker,
                 round_number,
                 round_number + 1);
      endpoints_visited_this_round.clear();
      any_improvement_this_round = false;
      round_number++;
      continue;  // Re-evaluate worst endpoint for new round
    }

    // Check if we need to switch to a different endpoint (worst changed)
    // Only switch if current endpoint is either:
    // 1. Not set yet (first iteration)
    // 2. "Stuck" (hit decreasing slack limit)
    // 3. Different from worst and current is already visited this round
    const sta::Pin* current_pin
        = current_endpoint ? current_endpoint->pin() : nullptr;
    bool should_switch = false;

    if (!current_endpoint) {
      // First iteration - use worst endpoint
      should_switch = true;
    } else if (endpoints_visited_this_round.contains(current_pin)) {
      // Current endpoint already visited this round - must switch
      should_switch = true;
    } else if (current_endpoint != worst_endpoint) {
      // Worst changed - only switch if current is stuck
      int decreasing_count = decreasing_slack_counts[current_pin];
      int pass_limit = endpoint_pass_limits[current_pin];
      if (pass_limit == 0) {
        pass_limit = initial_decreasing_slack_max_passes_;
        endpoint_pass_limits[current_pin] = pass_limit;
      }

      if (decreasing_count >= pass_limit) {
        // Current endpoint is stuck - switch to worst
        should_switch = true;
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "WNS{} Phase: Endpoint {} stuck after {} passes (limit {}), "
                   "switching to {}",
                   phase_marker,
                   network_->pathName(current_pin),
                   decreasing_count,
                   pass_limit,
                   network_->pathName(worst_pin));
      }
    }

    if (should_switch && current_endpoint != worst_endpoint) {
      if (current_endpoint) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "WNS{} Phase: Switching from {} to {}",
                   phase_marker,
                   current_endpoint->name(network_),
                   worst_endpoint->name(network_));
      }
      current_endpoint = worst_endpoint;
      violator_collector_->useWorstEndpoint(worst_endpoint);
      violator_collector_->markEndpointVisitedInWns(worst_endpoint->pin());
      rejected_pin_moves_current_endpoint_
          .clear();  // Clear rejected moves for new endpoint

      // Track new endpoint in MoveTracker
      if (move_tracker_) {
        move_tracker_->setCurrentEndpoint(worst_pin);
      }

      // Initialize tracking for this endpoint if needed
      if (!endpoint_pass_limits.contains(worst_pin)) {
        endpoint_pass_limits[worst_pin] = initial_decreasing_slack_max_passes_;
      }
      if (!decreasing_slack_counts.contains(worst_pin)) {
        decreasing_slack_counts[worst_pin] = 0;
      }
    }

    const sta::Pin* endpoint_pin = current_endpoint->pin();

    // Ensure this endpoint is tracked (initialize to 0 if new)
    if (!wns_endpoint_pass_counts_.contains(endpoint_pin)) {
      wns_endpoint_pass_counts_[endpoint_pin] = 0;
    }

    // Check if we've attempted to repair the max endpoint repairs allowed
    // (for unit tests)
    if (wns_endpoint_pass_counts_.size() > max_end_repairs_) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "WNS{} Phase: Hit maximum endpoint repairs of {}",
                 phase_marker,
                 max_end_repairs_);
      break;
    }

    // Check if current WNS endpoint has exceeded its pass limit
    if (wns_endpoint_pass_counts_[endpoint_pin] >= max_passes_per_endpoint) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "WNS{} Phase: WNS endpoint {} exceeded pass limit {}, exiting",
                 phase_marker,
                 network_->pathName(endpoint_pin),
                 max_passes_per_endpoint);
      break;
    }

    // Check if we've exceeded the maximum iterations for WNS Phase
    const int phase1_iterations = opto_iteration - phase1_start_iteration;
    if (phase1_iterations >= max_passes_per_endpoint) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "WNS{} Phase: Reached maximum iterations {}, exiting",
                 phase_marker,
                 max_passes_per_endpoint);
      break;
    }

    // Check WNS termination
    if (sta::fuzzyGreaterEqual(worst_slack, setup_slack_margin)) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "WNS{} Phase: WNS {} meets slack margin {}, done",
                 phase_marker,
                 delayAsString(worst_slack, digits, sta_),
                 delayAsString(setup_slack_margin, digits, sta_));
      printProgress(opto_iteration, true, false, phase_marker);
      break;
    }

    // Check TNS termination
    if (terminateProgress(opto_iteration,
                          initial_tns,
                          prev_tns,
                          fix_rate_threshold,
                          0,
                          1,
                          "WNS",
                          phase_marker)) {
      overall_no_progress_count_++;
      if (overall_no_progress_count_ >= max_no_progress) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   1,
                   "WNS{} Phase: No TNS progress for {} cycles, exiting",
                   phase_marker,
                   overall_no_progress_count_);
        break;
      }
    }

    if (opto_iteration % opto_small_interval_ == 0) {
      overall_no_progress_count_ = 0;
    }

    sta::Slack prev_end_slack;
    sta::Slack prev_worst_slack;
    sta::Slack prev_tns_local;
    // Fresh start if the last repair attempt was successful
    // (i.e. we're not trying to get out of local minima)
    if (total_decreasing_slack_passes == 0) {
      // Collect violators from worst path only
      prev_end_slack = violator_collector_->getCurrentEndpointSlack();
      prev_worst_slack = worst_slack;
      prev_tns_local = sta_->totalNegativeSlack(max_);
      fallback_ = false;
      resizer_->journalBegin();
      journal_open = true;
    } else {
      // Use the target slacks of the local minima pit
      prev_end_slack = target_end_slack;
      prev_worst_slack = target_worst_slack;
      prev_tns_local = target_tns_local;
    }

    opto_iteration++;

    if (verbose || opto_iteration % print_interval_ == 0) {
      printProgress(opto_iteration, false, false, phase_marker);
    }

    // Collect violators based on collection mode
    vector<const sta::Pin*> viol_pins;
    if (use_cone_collection) {
      // Cone-based collection with adaptive threshold
      viol_pins = violator_collector_->collectViolatorsByConeTraversal(
          current_endpoint, sort_type);

      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 2,
                 "WNS{} Phase: Collected {} pins from cone for endpoint {}",
                 phase_marker,
                 viol_pins.size(),
                 network_->pathName(endpoint_pin));

      // Debug: List collected pins for comparison with path-based
      if (logger_->debugCheck(RSZ, "repair_setup", 3)) {
        debugPrint(logger_, RSZ, "repair_setup", 3, "Cone-collected pins:");
        for (size_t i = 0; i < std::min(viol_pins.size(), size_t(20)); i++) {
          sta::Vertex* pin_vertex = graph_->pinDrvrVertex(viol_pins[i]);
          sta::Slack pin_slack
              = pin_vertex ? sta_->slack(pin_vertex, max_) : sta::Slack(0.0);
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     3,
                     "  [{}] {} slack={}",
                     i,
                     network_->pathName(viol_pins[i]),
                     delayAsString(pin_slack, digits, sta_));
        }
        if (viol_pins.size() > 20) {
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     3,
                     "  ... ({} total)",
                     viol_pins.size());
        }
      }
    } else {
      // Path-based collection: Always use 1 path per endpoint
      viol_pins = violator_collector_->collectViolators(1, -1, sort_type);

      // Debug: List collected pins for comparison with cone-based
      if (logger_->debugCheck(RSZ, "repair_setup", 3)) {
        debugPrint(logger_, RSZ, "repair_setup", 3, "Path-collected pins:");
        for (size_t i = 0; i < std::min(viol_pins.size(), size_t(20)); i++) {
          sta::Vertex* pin_vertex = graph_->pinDrvrVertex(viol_pins[i]);
          sta::Slack pin_slack
              = pin_vertex ? sta_->slack(pin_vertex, max_) : sta::Slack(0.0);
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     3,
                     "  [{}] {} slack={}",
                     i,
                     network_->pathName(viol_pins[i]),
                     delayAsString(pin_slack, digits, sta_));
        }
        if (viol_pins.size() > 20) {
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     3,
                     "  ... ({} total)",
                     viol_pins.size());
        }
      }
    }

    // For cone-based: process ALL pins before re-collecting
    int repairs_this_pass = max_repairs_per_pass_;
    if (use_cone_collection) {
      repairs_this_pass = viol_pins.size();  // Process entire cone

      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 2,
                 "WNS_CONE: Processing all {} pins before re-collecting cone",
                 repairs_this_pass);
    }

    // Track which (pin, move) combinations succeeded
    vector<std::pair<const sta::Pin*, BaseMove*>> chosen_moves;

    // Temporarily override max_repairs_per_pass_
    int saved_max_repairs = max_repairs_per_pass_;
    max_repairs_per_pass_ = repairs_this_pass;

    const bool changed = repairPins(viol_pins,
                                    setup_slack_margin,
                                    &rejected_pin_moves_current_endpoint_,
                                    &chosen_moves);

    // Restore original
    max_repairs_per_pass_ = saved_max_repairs;

    if (!changed) {
      if (total_decreasing_slack_passes > 0) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "WNS{} Phase: No change after {} "
                   "total decreasing slack passes.",
                   phase_marker,
                   total_decreasing_slack_passes);
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "WNS{} Phase: Restoring best slack, "
                   "end slack {} worst slack {}",
                   phase_marker,
                   delayAsString(prev_end_slack, digits, sta_),
                   delayAsString(prev_worst_slack, digits, sta_));
        total_decreasing_slack_passes = 0;
        for (auto endpoint : decreasing_slack_endpoints) {
          decreasing_slack_counts[endpoint] = 0;
        }
        decreasing_slack_endpoints.clear();
        resizer_->journalRestore();
      } else {
        resizer_->journalEnd();
      }
      journal_open = false;
      wns_endpoint_pass_counts_[endpoint_pin]++;

      // No changes possible - mark endpoint as visited and continue to next
      endpoints_visited_this_round.insert(endpoint_pin);
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 2,
                 "WNS{} Phase: No changes possible for endpoint {}, marking as "
                 "visited",
                 phase_marker,
                 network_->pathName(endpoint_pin));
      continue;  // Move to next worst endpoint
    }

    // Update parasitics and timing
    estimate_parasitics_->updateParasitics();
    sta_->findRequireds();

    sta::Slack end_slack = violator_collector_->getCurrentEndpointSlack();
    sta::Slack new_wns;
    sta::Vertex* new_worst_vertex;
    sta_->worstSlack(max_, new_wns, new_worst_vertex);
    float new_tns = sta_->totalNegativeSlack(max_);

    // Accept if WNS improved OR (WNS same AND (endpoint improved OR TNS
    // improved))
    const bool wns_improved = sta::fuzzyGreater(new_wns, prev_worst_slack);
    const bool wns_same = sta::fuzzyEqual(new_wns, prev_worst_slack);
    const bool endpoint_improved = sta::fuzzyGreater(end_slack, prev_end_slack);
    const bool tns_improved = sta::fuzzyGreater(new_tns, prev_tns_local);
    const bool better
        = wns_improved || (wns_same && (endpoint_improved || tns_improved));

    wns_endpoint_pass_counts_[endpoint_pin]++;

    if (better) {
      // Improvement! Mark progress and reset decreasing slack counter
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 3,
                 "WNS{} Phase: Improved after changes: "
                 "WNS ({} -> {}) "
                 "TNS ({} -> {}) "
                 "Endpoint slack ({} -> {})",
                 phase_marker,
                 delayAsString(prev_worst_slack, digits, sta_),
                 delayAsString(new_wns, digits, sta_),
                 delayAsString(prev_tns_local, 1, sta_),
                 delayAsString(new_tns, 1, sta_),
                 delayAsString(prev_end_slack, digits, sta_),
                 delayAsString(end_slack, digits, sta_));

      any_improvement_this_round = true;
      total_decreasing_slack_passes = 0;
      for (auto endpoint : decreasing_slack_endpoints) {
        decreasing_slack_counts[endpoint] = 0;
      }
      decreasing_slack_endpoints.clear();
      endpoint_prev_slack[endpoint_pin] = end_slack;

      // Check if we should increase pass limit (>50% success rate after
      // current_limit passes) Cap at 25 to prevent runaway iteration counts
      constexpr int max_adaptive_limit = 25;
      int successful_passes = wns_endpoint_pass_counts_[endpoint_pin]
                              - decreasing_slack_counts[endpoint_pin];
      int total_passes = wns_endpoint_pass_counts_[endpoint_pin];
      int current_limit = endpoint_pass_limits[endpoint_pin];

      if (total_passes >= current_limit && successful_passes > current_limit / 2
          && current_limit < max_adaptive_limit) {
        endpoint_pass_limits[endpoint_pin] += pass_limit_increment_;
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "WNS{} Phase: Endpoint {} has {} successes in {} passes, "
                   "increasing limit to {}",
                   phase_marker,
                   network_->pathName(endpoint_pin),
                   successful_passes,
                   total_passes,
                   endpoint_pass_limits[endpoint_pin]);
      }

      if (move_tracker_) {
        move_tracker_->commitMoves();
      }
      resizer_->journalEnd();
      journal_open = false;
    } else {
      // No improvement - increment decreasing slack counter
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 3,
                 "WNS{} Phase: Worsened after changes: "
                 "WNS ({} -> {}) "
                 "TNS ({} -> {}) "
                 "Endpoint slack ({} -> {})",
                 phase_marker,
                 delayAsString(prev_worst_slack, digits, sta_),
                 delayAsString(new_wns, digits, sta_),
                 delayAsString(prev_tns_local, 1, sta_),
                 delayAsString(new_tns, 1, sta_),
                 delayAsString(prev_end_slack, digits, sta_),
                 delayAsString(end_slack, digits, sta_));

      fallback_ = true;
      if (total_decreasing_slack_passes == 0) {
        target_end_slack = prev_end_slack;
        target_worst_slack = prev_worst_slack;
        target_tns_local = prev_tns_local;
      }
      total_decreasing_slack_passes++;
      decreasing_slack_endpoints.insert(endpoint_pin);
      decreasing_slack_counts[endpoint_pin]++;

      // Check if this endpoint is now stuck
      int current_limit = endpoint_pass_limits[endpoint_pin];
      if (decreasing_slack_counts[endpoint_pin] > current_limit) {
        // Endpoint is stuck - mark as visited for this round
        debugPrint(
            logger_,
            RSZ,
            "repair_setup",
            2,
            "WNS{} Phase: Endpoint {} stuck after {} non-improving passes "
            "(limit {}), marking as visited",
            phase_marker,
            network_->pathName(endpoint_pin),
            decreasing_slack_counts[endpoint_pin],
            current_limit);
        total_decreasing_slack_passes = 0;
        for (auto endpoint : decreasing_slack_endpoints) {
          decreasing_slack_counts[endpoint] = 0;
        }
        decreasing_slack_endpoints.clear();
        endpoints_visited_this_round.insert(endpoint_pin);
        if (move_tracker_) {
          move_tracker_->rejectMoves();
        }
        resizer_->journalRestore();
        journal_open = false;
        // Mark the chosen (pin, move) combinations as rejected since overall
        // change was rejected
        for (const auto& [pin, move] : chosen_moves) {
          rejected_pin_moves_current_endpoint_[pin].insert(move);
        }
      } else {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   3,
                   "WNS{} Phase: Allowing decreasing slack for {}/{} passes",
                   phase_marker,
                   decreasing_slack_counts[endpoint_pin],
                   current_limit);
      }
    }

    if (resizer_->overMaxArea()) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "WNS{} Phase: Over max area, exiting",
                 phase_marker);
      break;
    }
  }
  if (journal_open) {
    resizer_->journalEnd();
  }

  // Print phase completion
  printProgress(opto_iteration, true, false, phase_marker);

  sta::Slack final_wns;
  sta::Vertex* final_worst;
  sta_->worstSlack(max_, final_wns, final_worst);
  float final_tns = sta_->totalNegativeSlack(max_);

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "WNS{} Phase complete. WNS: {}, TNS: {}",
             phase_marker,
             delayAsString(final_wns, digits, sta_),
             delayAsString(final_tns, 1, sta_));
}

// TNS Phase: TNS-Focused Repair
// Work on all violating endpoints sequentially, trying to improve each one.
void RepairSetup::repairSetup_Tns(const float setup_slack_margin,
                                  const int max_passes_per_endpoint,
                                  const int max_repairs_per_pass,
                                  const bool verbose,
                                  int& opto_iteration,
                                  const float initial_tns,
                                  float& prev_tns,
                                  const char phase_marker,
                                  const ViolatorSortType sort_type)
{
  const utl::DebugScopedTimer timer(
      logger_,
      RSZ,
      "repair_setup",
      1,
      fmt::format("TNS{} Phase Time: {{}}", phase_marker));
  constexpr int digits = 3;

  // Capture pre-phase slack for tracking per-phase improvements
  if (move_tracker_) {
    move_tracker_->capturePrePhaseSlack();
  }

  // Initialize TNS Phase tracking
  overall_no_progress_count_ = 0;

  // Reset endpoint tracking from WNS Phase
  violator_collector_->resetEndpointPasses();

  // Print phase header (no table header - continue in same table from WNS
  // Phase)
  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "TNS{} Phase: Focusing on all violating endpoints...",
             phase_marker);

  // Mark the start of TNS Phase in the table
  printProgress(opto_iteration, false, false, phase_marker);

  // Track global WNS at start of TNS phase - must not degrade below this
  sta::Slack initial_tns_phase_wns;
  sta::Vertex* initial_tns_phase_worst;
  sta_->worstSlack(max_, initial_tns_phase_wns, initial_tns_phase_worst);
  sta::Slack prev_global_wns = initial_tns_phase_wns;

  // Track TNS at the start of TNS phase
  sta::Slack prev_tns_local = sta_->totalNegativeSlack(max_);

  // Collect all violating endpoints once at the start
  violator_collector_->collectViolatingEndpoints();
  int max_endpoint_count = violator_collector_->getMaxEndpointCount();

  if (max_endpoint_count == 0) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "TNS{} Phase: No violating endpoints, exiting",
               phase_marker);
    return;
  }

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "TNS{} Phase: Processing {} violating endpoints (skipping worst "
             "endpoint)",
             phase_marker,
             max_endpoint_count);

  // Track adaptive pass limits per endpoint
  std::map<const sta::Pin*, int> endpoint_pass_limits;

  // Process each endpoint sequentially, starting from index 1 to skip worst
  // endpoint
  for (int endpoint_index = 1; endpoint_index < max_endpoint_count;
       endpoint_index++) {
    // Check if we've attempted to repair the max endpoint repairs allowed
    // (for unit tests)
    if (endpoint_index >= max_end_repairs_) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "TNS{} Phase: Hit maximum endpoint repairs of {}",
                 phase_marker,
                 max_end_repairs_);
      break;
    }

    violator_collector_->setToEndpoint(endpoint_index);
    sta::Vertex* endpoint = violator_collector_->getCurrentEndpoint();

    if (!endpoint) {
      continue;
    }

    const sta::Pin* endpoint_pin = endpoint->pin();
    sta::Slack endpoint_slack = sta_->slack(endpoint, max_);

    if (sta::fuzzyGreaterEqual(endpoint_slack, setup_slack_margin)) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 2,
                 "TNS{} Phase: Endpoint {} (index {}) already meets timing, "
                 "skipping",
                 phase_marker,
                 network_->pathName(endpoint_pin),
                 endpoint_index);
      continue;
    }

    // Skip endpoints already optimized in WNS phase
    if (violator_collector_->wasEndpointVisitedInWns(endpoint_pin)) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 2,
                 "TNS{} Phase: Skipping endpoint {} (index {}) - already "
                 "optimized in WNS phase",
                 phase_marker,
                 network_->pathName(endpoint_pin),
                 endpoint_index);
      continue;
    }

    // Track this endpoint in MoveTracker
    if (move_tracker_) {
      move_tracker_->setCurrentEndpoint(endpoint_pin);
    }

    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "TNS{} Phase: Working on endpoint {} (index {}), slack = {}",
               phase_marker,
               network_->pathName(endpoint_pin),
               endpoint_index,
               delayAsString(endpoint_slack, digits, sta_));

    // Initialize adaptive pass limit for this endpoint
    if (!endpoint_pass_limits.contains(endpoint_pin)) {
      endpoint_pass_limits[endpoint_pin] = initial_decreasing_slack_max_passes_;
    }
    int current_limit = endpoint_pass_limits[endpoint_pin];

    // Try multiple passes on this endpoint
    int pass = 1;
    int decreasing_slack_passes = 0;
    int successful_passes = 0;  // Track number of accepted moves
    sta::Slack prev_endpoint_slack = endpoint_slack;

    resizer_->journalBegin();
    bool journal_open = true;
    while (pass <= max_passes_per_endpoint) {
      // Collect violating pins from this endpoint's critical path
      vector<const sta::Pin*> viol_pins
          = violator_collector_->collectViolators(1, -1, sort_type);

      if (viol_pins.empty()) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "TNS{} Phase: No violating pins for endpoint {}, pass {}",
                   phase_marker,
                   network_->pathName(endpoint_pin),
                   pass);
        resizer_->journalEnd();
        journal_open = false;
        break;
      }

      const bool changed = repairPins(viol_pins, setup_slack_margin);

      if (!changed) {
        if (decreasing_slack_passes > current_limit) {
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     2,
                     "TNS{} Phase: No change after {} "
                     "decreasing slack passes.",
                     phase_marker,
                     decreasing_slack_passes);
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     2,
                     "TNS{} Phase: Restoring best end slack {}",
                     phase_marker,
                     delayAsString(prev_endpoint_slack, digits, sta_));
          resizer_->journalRestore();
        } else {
          resizer_->journalEnd();
        }
        journal_open = false;
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "TNS{} Phase: No changes for endpoint {}, pass {}",
                   phase_marker,
                   network_->pathName(endpoint_pin),
                   pass);
        break;
      }

      // Update parasitics and timing
      estimate_parasitics_->updateParasitics();
      sta_->findRequireds();

      endpoint_slack = sta_->slack(endpoint, max_);
      sta::Slack global_wns;
      sta::Vertex* global_wns_vertex;
      sta_->worstSlack(max_, global_wns, global_wns_vertex);
      sta::Slack new_tns = sta_->totalNegativeSlack(max_);

      // Accept if WNS improves OR (WNS same AND endpoint improves)
      // Always compare against initial TNS phase WNS to prevent drift
      const bool wns_improved = sta::fuzzyGreater(global_wns, prev_global_wns);
      const bool wns_same = sta::fuzzyEqual(global_wns, prev_global_wns);
      const bool endpoint_improved
          = sta::fuzzyGreater(endpoint_slack, prev_endpoint_slack);
      const bool better = wns_improved || (wns_same && endpoint_improved);

      if (better) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   3,
                   "TNS{} Phase: Improved after changes: "
                   "WNS ({} -> {}) "
                   "TNS ({} -> {}) "
                   "Endpoint slack ({} -> {})",
                   phase_marker,
                   delayAsString(prev_global_wns, digits, sta_),
                   delayAsString(global_wns, digits, sta_),
                   delayAsString(prev_tns_local, 1, sta_),
                   delayAsString(new_tns, 1, sta_),
                   delayAsString(prev_endpoint_slack, digits, sta_),
                   delayAsString(endpoint_slack, digits, sta_));
        prev_global_wns = global_wns;
        prev_tns_local = new_tns;
        if (sta::fuzzyGreaterEqual(endpoint_slack, setup_slack_margin)) {
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     1,
                     "TNS{} Phase: Endpoint {} meets timing, done",
                     phase_marker,
                     network_->pathName(endpoint_pin));
          resizer_->journalEnd();
          journal_open = false;
          break;
        }
        prev_endpoint_slack = endpoint_slack;
        decreasing_slack_passes = 0;
        successful_passes++;

        // Only increase pass limit if we've done current_limit passes
        // and more than 50% were successful
        // Cap at 25 to prevent runaway iteration counts
        constexpr int max_adaptive_limit = 25;
        if (pass >= current_limit && successful_passes > current_limit / 2
            && current_limit < max_adaptive_limit) {
          endpoint_pass_limits[endpoint_pin] += pass_limit_increment_;
          current_limit = endpoint_pass_limits[endpoint_pin];
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     2,
                     "TNS{} Phase: Endpoint {} has {} successes in {} passes, "
                     "increasing limit to {}",
                     phase_marker,
                     network_->pathName(endpoint_pin),
                     successful_passes,
                     pass,
                     current_limit);
        }
        resizer_->journalEnd();
        // Progress, Save checkpoint so we can back up to here
        resizer_->journalBegin();
      } else {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   3,
                   "TNS{} Phase: Worsened after changes: "
                   "WNS ({} -> {}) "
                   "TNS ({} -> {}) "
                   "Endpoint slack ({} -> {})",
                   phase_marker,
                   delayAsString(prev_global_wns, digits, sta_),
                   delayAsString(global_wns, digits, sta_),
                   delayAsString(prev_tns_local, 1, sta_),
                   delayAsString(new_tns, 1, sta_),
                   delayAsString(prev_endpoint_slack, digits, sta_),
                   delayAsString(endpoint_slack, digits, sta_));

        // Rejected move - restore and try again
        fallback_ = true;
        decreasing_slack_passes++;
        if (decreasing_slack_passes > current_limit) {
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     2,
                     "TNS{} Phase: Decreasing slack for {} after {} passes "
                     "(limit {}), stopping",
                     phase_marker,
                     network_->pathName(endpoint_pin),
                     decreasing_slack_passes,
                     current_limit);
          resizer_->journalRestore();
          journal_open = false;
          break;
        }
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   3,
                   "TNS{} Phase: Allowing decreasing slack for {}/{} passes",
                   phase_marker,
                   decreasing_slack_passes,
                   current_limit);
      }

      if (resizer_->overMaxArea()) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   1,
                   "TNS{} Phase: Over max area, exiting",
                   phase_marker);
        resizer_->journalEnd();
        printProgress(opto_iteration, true, true, phase_marker);
        printProgressFooter();
        return;
      }

      pass++;
    }
    if (journal_open) {
      resizer_->journalEnd();
    }

    // Increment iteration and print progress after each endpoint
    opto_iteration++;
    if (verbose || endpoint_index == 1
        || opto_iteration % print_interval_ == 0) {
      printProgress(opto_iteration, false, false, phase_marker);
    }
  }

  // Print phase completion
  printProgress(opto_iteration, true, false, phase_marker);

  sta::Slack final_wns;
  sta::Vertex* final_worst;
  sta_->worstSlack(max_, final_wns, final_worst);
  float final_tns = sta_->totalNegativeSlack(max_);

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "TNS{} Phase complete. WNS: {}, TNS: {}",
             phase_marker,
             delayAsString(final_wns, digits, sta_),
             delayAsString(final_tns, 1, sta_));
}

// ENDPOINT_FANIN Phase: Iteratively refine threshold from 0 to endpoint slack
// For each endpoint, try progressively tighter thresholds until diminishing
// returns
void RepairSetup::repairSetup_EndpointFanin(const float setup_slack_margin,
                                            const int max_passes_per_endpoint,
                                            const bool verbose,
                                            int& opto_iteration,
                                            const char phase_marker)
{
  // Wrapper: call unified directional function with use_startpoints=false
  repairSetup_Directional(false,  // use_startpoints
                          setup_slack_margin,
                          max_passes_per_endpoint,
                          verbose,
                          opto_iteration,
                          phase_marker);
}

void RepairSetup::repairSetup_StartpointFanout(
    const float setup_slack_margin,
    const int max_passes_per_startpoint,
    const bool verbose,
    int& opto_iteration,
    const char phase_marker)
{
  // Wrapper: call unified directional function with use_startpoints=true
  repairSetup_Directional(true,  // use_startpoints
                          setup_slack_margin,
                          max_passes_per_startpoint,
                          verbose,
                          opto_iteration,
                          phase_marker);
}

// Unified directional repair function that handles both ENDPOINT_FANIN and
// STARTPOINT_FANOUT modes
void RepairSetup::repairSetup_Directional(const bool use_startpoints,
                                          const float setup_slack_margin,
                                          const int max_passes_per_point,
                                          const bool verbose,
                                          int& opto_iteration,
                                          const char phase_marker)
{
  const char* phase_name
      = use_startpoints ? "STARTPOINT_FANOUT" : "ENDPOINT_FANIN";
  const char* point_type = use_startpoints ? "startpoint" : "endpoint";
  const char* point_type_cap = use_startpoints ? "Startpoint" : "Endpoint";
  const utl::DebugScopedTimer timer(
      logger_,
      RSZ,
      "repair_setup",
      1,
      fmt::format("{}{} Phase Time: {{}}", phase_name, phase_marker));
  constexpr int digits = 3;

  // Capture pre-phase slack for tracking per-phase improvements
  if (move_tracker_) {
    move_tracker_->capturePrePhaseSlack();
  }

  // Threshold margin percentages to try for each point.
  // These percentages are relative to the point's slack, creating
  // progressively tighter thresholds: 20%, 50%, and 100% of the point's
  // slack. Reduced from 5 to 3 thresholds for faster runtime.
  const float margin_percentages[] = {0.20, 0.50, 1.00};
  const int num_thresholds = 3;

  // Diminishing returns threshold: stop if improvement < 10ps
  // Increased from 1ps to 10ps to reduce iterations with minimal improvement
  constexpr float diminishing_returns_threshold = 1e-11;  // 10 picoseconds

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "{}{} Phase: Iterative threshold refinement...",
             phase_name,
             phase_marker);

  // Collect all violating points once at the start
  violator_collector_->collectViolatingPoints(use_startpoints);
  int max_point_count = violator_collector_->getMaxPointCount(use_startpoints);

  // Print progress
  printProgress(opto_iteration, false, false, phase_marker, use_startpoints);

  if (max_point_count == 0) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "{}{} Phase: No violating {}s, exiting",
               phase_name,
               phase_marker,
               point_type);
    return;
  }

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "{}{} Phase: Processing {} violating {}s",
             phase_name,
             phase_marker,
             max_point_count,
             point_type);

  int points_processed = 0;

  // Main loop: process each violating point in order from most critical
  for (int point_index = 0; point_index < max_point_count; point_index++) {
    // Check if we've attempted to repair the max endpoint repairs allowed
    // (for unit tests)
    if (point_index >= max_end_repairs_) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "{}{} Phase: Hit maximum point repairs of {}",
                 phase_name,
                 phase_marker,
                 max_end_repairs_);
      break;
    }

    violator_collector_->setToPoint(point_index, use_startpoints);
    sta::Vertex* point = violator_collector_->getCurrentPoint(use_startpoints);

    if (!point) {
      continue;
    }

    const sta::Pin* point_pin
        = violator_collector_->getCurrentPointPin(use_startpoints);
    sta::Slack point_slack = sta_->slack(point, max_);

    // Check if point already meets timing
    if (sta::fuzzyGreaterEqual(point_slack, setup_slack_margin)) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 2,
                 "{}{} Phase: {} {} (index {}) already meets timing, skipping",
                 phase_name,
                 phase_marker,
                 point_type_cap,
                 network_->pathName(point_pin),
                 point_index);
      continue;
    }

    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "{}{} Phase: Processing {} {} (index {}), slack = {}",
               phase_name,
               phase_marker,
               point_type,
               network_->pathName(point_pin),
               point_index,
               delayAsString(point_slack, digits, sta_));

    // Clear rejected moves for this point
    rejected_pin_moves_current_endpoint_.clear();

    int point_pass_count = 0;
    sta::Slack prev_point_slack = point_slack;

    // Try progressively tighter thresholds for this point
    for (int threshold_idx = 0; threshold_idx < num_thresholds;
         threshold_idx++) {
      // Check if we've exceeded pass limit for this point
      if (point_pass_count >= max_passes_per_point) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   1,
                   "{}{} Phase: {} {} reached pass limit {}, moving to next {}",
                   phase_name,
                   phase_marker,
                   point_type_cap,
                   network_->pathName(point_pin),
                   max_passes_per_point,
                   point_type);
        break;
      }

      // Re-check point slack (may have improved from previous thresholds)
      point_slack = sta_->slack(point, max_);

      // Check if point now meets timing
      if (sta::fuzzyGreaterEqual(point_slack, setup_slack_margin)) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   1,
                   "{}{} Phase: {} {} now meets timing, moving to next {}",
                   phase_name,
                   phase_marker,
                   point_type_cap,
                   network_->pathName(point_pin),
                   point_type);
        break;
      }

      float margin_pct = margin_percentages[threshold_idx];
      sta::Slack margin = std::abs(point_slack) * margin_pct;
      sta::Slack threshold = point_slack + margin;

      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 2,
                 "{}{} Phase: Trying threshold {}/{}: margin={:.0f}%, "
                 "threshold={}",
                 phase_name,
                 phase_marker,
                 threshold_idx + 1,
                 num_thresholds,
                 margin_pct * 100.0,
                 delayAsString(threshold, digits, sta_));

      // Save state before repair attempt
      sta::Slack prev_wns = violator_collector_->getWns();
      sta::Slack prev_endpoint_tns
          = violator_collector_->getTns(false);  // Endpoint TNS
      sta::Slack prev_startpoint_tns
          = violator_collector_->getTns(true);  // Startpoint TNS

      // Collect violators with explicit threshold
      resizer_->journalBegin();
      point_pass_count++;
      opto_iteration++;

      if (verbose || opto_iteration % print_interval_ == 0) {
        printProgress(
            opto_iteration, false, false, phase_marker, use_startpoints);
      }

      vector<const sta::Pin*> viol_pins
          = violator_collector_->collectViolatorsByDirectionalTraversal(
              use_startpoints,
              point_index,
              threshold,
              ViolatorSortType::SORT_BY_LOAD_DELAY);

      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 2,
                 "{}{} Phase: Collected {} pins with threshold {}",
                 phase_name,
                 phase_marker,
                 viol_pins.size(),
                 delayAsString(threshold, digits, sta_));

      if (viol_pins.empty()) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "{}{} Phase: No pins collected, moving to next threshold",
                   phase_name,
                   phase_marker);
        resizer_->journalEnd();
        continue;
      }

      // Repair all pins in this threshold iteration
      // Save/restore max_repairs_per_pass_ member to avoid affecting other
      // phases
      int saved_max_repairs_member = max_repairs_per_pass_;
      max_repairs_per_pass_ = viol_pins.size();  // Process entire cone

      vector<std::pair<const sta::Pin*, BaseMove*>> chosen_moves;
      const bool changed = repairPins(viol_pins,
                                      setup_slack_margin,
                                      &rejected_pin_moves_current_endpoint_,
                                      &chosen_moves);

      max_repairs_per_pass_ = saved_max_repairs_member;  // Restore

      if (!changed) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "{}{} Phase: No repairs made, moving to next threshold",
                   phase_name,
                   phase_marker);
        resizer_->journalEnd();
        continue;
      }

      // Update parasitics and timing to check improvement
      estimate_parasitics_->updateParasitics();
      sta_->findRequireds();

      // Check improvement
      sta::Slack new_point_slack = sta_->slack(point, max_);
      sta::Slack new_wns = violator_collector_->getWns();
      sta::Slack new_endpoint_tns
          = violator_collector_->getTns(false);  // Endpoint TNS
      sta::Slack new_startpoint_tns
          = violator_collector_->getTns(true);  // Startpoint TNS

      const bool wns_improved = sta::fuzzyGreater(new_wns, prev_wns);
      const bool wns_same = sta::fuzzyEqual(new_wns, prev_wns);
      const bool point_improved
          = sta::fuzzyGreater(new_point_slack, prev_point_slack);
      const bool endpoint_tns_improved
          = sta::fuzzyGreater(new_endpoint_tns, prev_endpoint_tns);
      const bool startpoint_tns_improved
          = sta::fuzzyGreater(new_startpoint_tns, prev_startpoint_tns);
      const bool endpoint_tns_same
          = sta::fuzzyEqual(new_endpoint_tns, prev_endpoint_tns);
      const bool startpoint_tns_same
          = sta::fuzzyEqual(new_startpoint_tns, prev_startpoint_tns);

      // Accept if:
      // 1. WNS improved (always accept), OR
      // 2. WNS same AND (point improved OR focused TNS improved) AND both TNS
      // don't degrade
      const bool both_tns_ok
          = (endpoint_tns_improved || endpoint_tns_same)
            && (startpoint_tns_improved || startpoint_tns_same);
      const bool focused_tns_improved
          = use_startpoints ? startpoint_tns_improved : endpoint_tns_improved;
      const bool better
          = wns_improved
            || (wns_same && (point_improved || focused_tns_improved)
                && both_tns_ok);

      sta::Slack improvement = new_point_slack - prev_point_slack;

      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 2,
                 "{}{} Phase: Bools: wns_imp={} wns_same={} pt_imp={} "
                 "EnTNS_imp={} EnTNS_same={} StTNS_imp={} StTNS_same={} "
                 "both_ok={} better={}",
                 phase_name,
                 phase_marker,
                 wns_improved,
                 wns_same,
                 point_improved,
                 endpoint_tns_improved,
                 endpoint_tns_same,
                 startpoint_tns_improved,
                 startpoint_tns_same,
                 both_tns_ok,
                 better);

      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "{}{} Phase: Threshold {}/{}: {} slack {} -> {} (imp: {}), "
                 "WNS {} -> {}, EnTNS {} -> {}, StTNS {} -> {}{}",
                 phase_name,
                 phase_marker,
                 threshold_idx + 1,
                 num_thresholds,
                 point_type,
                 delayAsString(prev_point_slack, digits, sta_),
                 delayAsString(new_point_slack, digits, sta_),
                 delayAsString(improvement, digits, sta_),
                 delayAsString(prev_wns, digits, sta_),
                 delayAsString(new_wns, digits, sta_),
                 delayAsString(prev_endpoint_tns, 1, sta_),
                 delayAsString(new_endpoint_tns, 1, sta_),
                 delayAsString(prev_startpoint_tns, 1, sta_),
                 delayAsString(new_startpoint_tns, 1, sta_),
                 better ? " [ACCEPT]" : " [REJECT]");

      if (better) {
        // Accept the changes
        if (move_tracker_) {
          move_tracker_->commitMoves();
        }
        resizer_->journalEnd();
        prev_point_slack = new_point_slack;

        // Check for diminishing returns
        if (improvement < diminishing_returns_threshold) {
          debugPrint(logger_,
                     RSZ,
                     "repair_setup",
                     1,
                     "{}{} Phase: Improvement {} < {} (diminishing returns), "
                     "moving to next {}",
                     phase_name,
                     phase_marker,
                     delayAsString(improvement, digits, sta_),
                     delayAsString(diminishing_returns_threshold, digits, sta_),
                     point_type);
          break;  // Move to next point
        }
      } else {
        // Reject the changes
        if (move_tracker_) {
          move_tracker_->rejectMoves();
        }
        resizer_->journalRestore();

        // Mark the chosen (pin, move) combinations as rejected
        for (const auto& [pin, move] : chosen_moves) {
          rejected_pin_moves_current_endpoint_[pin].insert(move);
        }

        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "{}{} Phase: Changes rejected, moving to next threshold",
                   phase_name,
                   phase_marker);
      }
    }

    points_processed++;
  }

  // Print phase completion
  printProgress(opto_iteration, true, false, phase_marker, use_startpoints);

  // Get final metrics using proxy methods
  sta::Slack final_wns = violator_collector_->getWns();
  sta::Slack final_tns = violator_collector_->getTns(use_startpoints);

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "{}{} Phase complete. {}s processed: {}, WNS: {}, TNS: {}",
             phase_name,
             phase_marker,
             point_type_cap,
             points_processed,
             delayAsString(final_wns, digits, sta_),
             delayAsString(final_tns, 1, sta_));
}

// Perform some last fixing based on sizing only.
// This is a greedy opto that does not degrade WNS or TNS.
void RepairSetup::repairSetup_LastGasp(const OptoParams& params,
                                       int& num_viols,
                                       const int max_iterations,
                                       const char phase_marker)
{
  const utl::DebugScopedTimer timer(
      logger_,
      RSZ,
      "repair_setup",
      1,
      fmt::format("LAST_GASP{} Phase Time: {{}}", phase_marker));
  constexpr int digits = 3;

  move_sequence_.clear();
  if (!params.skip_vt_swap) {
    move_sequence_.push_back(resizer_->vt_swap_speed_move_.get());
  }
  move_sequence_.push_back(resizer_->size_up_match_move_.get());
  move_sequence_.push_back(resizer_->size_up_move_.get());
  if (!params.skip_pin_swap) {
    move_sequence_.push_back(resizer_->swap_pins_move_.get());
  }

  violator_collector_->init(params.setup_slack_margin);

  float curr_tns = sta_->totalNegativeSlack(max_);
  if (sta::fuzzyGreaterEqual(curr_tns, 0)) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "LAST_GASP{} Phase: TNS is {}, exiting",
               phase_marker,
               delayAsString(curr_tns, 1, sta_));
    return;
  }

  int end_index = 0;
  int max_end_count = violator_collector_->getMaxEndpointCount();
  num_viols = max_end_count;
  if (max_end_count == 0) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "LAST_GASP{} Phase: No violating endpoints found",
               phase_marker);
    return;
  }
  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "LAST_GASP{} Phase: {} violating endpoints remain",
             phase_marker,
             max_end_count);
  int opto_iteration = params.iteration;
  printProgress(opto_iteration, false, false, phase_marker);

  float prev_tns = curr_tns;
  sta::Slack curr_worst_slack = violator_collector_->getWns();
  sta::Slack prev_worst_slack = curr_worst_slack;
  bool prev_termination = false;
  bool two_cons_terminations = false;
  float fix_rate_threshold = inc_fix_rate_threshold_;

  while (violator_collector_->hasMoreEndpoints()) {
    if (max_iterations > 0 && opto_iteration >= max_iterations) {
      break;
    }

    fallback_ = false;
    violator_collector_->advanceToNextEndpoint();
    sta::Vertex* end = violator_collector_->getCurrentEndpoint();
    sta::Slack end_slack = sta_->slack(end, max_);
    sta::Slack worst_slack;
    sta::Vertex* worst_vertex;
    sta_->worstSlack(max_, worst_slack, worst_vertex);
    end_index++;
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "LAST_GASP{} Phase: Doing endpoint {} ({}/{}) "
               "endpoint slack = {}, WNS = {}",
               phase_marker,
               end->name(network_),
               end_index,
               max_end_count,
               delayAsString(end_slack, digits, sta_),
               delayAsString(worst_slack, digits, sta_));
    if (end_index > max_end_count) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "LAST_GASP{} Phase: Hit maximum endpoint repairs of {}",
                 phase_marker,
                 max_end_count);
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
                            max_end_count,
                            "LAST_GASP",
                            phase_marker)) {
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
        printProgress(opto_iteration, false, false, phase_marker);
      }
      if (sta::fuzzyGreaterEqual(end_slack, params.setup_slack_margin)) {
        --num_viols;
        resizer_->journalEnd();
        journal_open = false;
        break;
      }

      const bool changed
          = repairEndpoint(end->pin(), end_slack, params.setup_slack_margin);

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

      const bool wns_improved
          = sta::fuzzyGreaterEqual(curr_worst_slack, prev_worst_slack);
      const bool tns_improved = sta::fuzzyGreaterEqual(curr_tns, prev_tns);

      // Accept only moves that improve both WNS and TNS
      if (wns_improved && tns_improved) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "LAST_GASP{} Phase: Move accepted for endpoint {} pass {} "
                   "because WNS improved {} -> {} and TNS improved {} -> {}",
                   phase_marker,
                   end_index,
                   pass,
                   delayAsString(prev_worst_slack, digits, sta_),
                   delayAsString(curr_worst_slack, digits, sta_),
                   delayAsString(prev_tns, 1, sta_),
                   delayAsString(curr_tns, 1, sta_));
        prev_worst_slack = curr_worst_slack;
        prev_tns = curr_tns;
        if (sta::fuzzyGreaterEqual(end_slack, params.setup_slack_margin)) {
          --num_viols;
        }
        resizer_->journalEnd();
        if (pass < max_last_gasp_passes_) {
          resizer_->journalBegin();
        } else {
          journal_open = false;
        }
      } else {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "LAST_GASP{} Phase: Move rejected for endpoint {} pass {} "
                   "because WNS {} {} -> {} and TNS {} {} -> {}",
                   phase_marker,
                   end_index,
                   pass,
                   wns_improved ? "improved" : "worsened",
                   delayAsString(prev_worst_slack, digits, sta_),
                   delayAsString(curr_worst_slack, digits, sta_),
                   tns_improved ? "improved" : "worsened",
                   delayAsString(prev_tns, 1, sta_),
                   delayAsString(curr_tns, 1, sta_));
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
      printProgress(opto_iteration, true, false, phase_marker);
    }
    if (two_cons_terminations) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "LAST_GASP{} Phase: No TNS progress for two opto cycles, "
                 "exiting",
                 phase_marker);
      break;
    }
  }  // for each violating endpoint

  sta::Slack final_wns;
  sta::Vertex* final_worst;
  sta_->worstSlack(max_, final_wns, final_worst);
  float final_tns = sta_->totalNegativeSlack(max_);

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "LAST_GASP{} Phase complete. WNS: {}, TNS: {}, Time: {:.2f}s",
             phase_marker,
             delayAsString(final_wns, digits, sta_),
             delayAsString(final_tns, 1, sta_),
             timer.elapsed());
}

// Perform VT swap on remaining critical cells as a last resort
bool RepairSetup::swapVTCritCells(const OptoParams& params, int& num_viols)
{
  bool changed = false;

  // Start with sorted violating endpoints
  const sta::VertexSet& endpoints = sta_->endpoints();
  vector<pair<sta::Vertex*, sta::Slack>> violating_ends;
  for (sta::Vertex* end : endpoints) {
    const sta::Slack end_slack = sta_->slack(end, max_);
    if (sta::fuzzyLess(end_slack, params.setup_slack_margin)) {
      violating_ends.emplace_back(end, end_slack);
    }
  }
  std::ranges::stable_sort(violating_ends.begin(),
                           violating_ends.end(),
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
    for (sta::Vertex* end : endpoints) {
      const sta::Slack end_slack = sta_->slack(end, max_);
      if (sta::fuzzyLess(end_slack, params.setup_slack_margin)) {
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
  if (visited.contains(endpoint)) {
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
        if (sta::fuzzyLess(inst_slack, params.setup_slack_margin)) {
          // Update worst slack for this instance
          if (!crit_insts.contains(inst)) {
            crit_insts[inst] = inst_slack;
            endpoint_insts++;
            debugPrint(logger_,
                       RSZ,
                       "swap_crit_vt",
                       1,
                       "swapVTCritCells: found crit inst {}: slack {}",
                       network_->name(inst),
                       float(inst_slack));
          }
        }
      }
    }

    // Traverse fanin edges
    sta::VertexInEdgeIterator edge_iter(current, graph_);
    while (edge_iter.hasNext()) {
      sta::Edge* edge = edge_iter.next();
      sta::Vertex* fanin_vertex = edge->from(graph_);
      if (fanin_vertex->isRegClk()) {
        continue;
      }

      // Only traverse if we haven't visited and the fanin has negative slack
      if (!visited.contains(fanin_vertex)) {
        const sta::Slack fanin_slack = sta_->slack(fanin_vertex, max_);
        if (sta::fuzzyLess(fanin_slack, params.setup_slack_margin)) {
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
  sta::InstancePinIterator* pin_iter = network_->pinIterator(inst);
  while (pin_iter->hasNext()) {
    sta::Pin* pin = pin_iter->next();
    if (network_->direction(pin)->isAnyOutput()) {
      sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
      if (vertex) {
        const sta::Slack pin_slack = sta_->slack(vertex, max_);
        worst_slack = std::min(worst_slack, pin_slack);
      }
    }
  }
  delete pin_iter;

  return worst_slack;
}

}  // namespace rsz
