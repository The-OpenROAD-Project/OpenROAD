// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "RepairHold.hh"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "BufferMove.hh"
#include "RepairDesign.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InputDrive.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Parasitics.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchPred.hh"
#include "sta/TimingArc.hh"
#include "sta/Units.hh"
#include "sta/Vector.hh"
#include "utl/Logger.h"
#include "utl/mem_stats.h"

namespace rsz {

using std::max;
using std::min;
using std::string;
using std::vector;

using utl::RSZ;

using sta::Edge;
using sta::fuzzyLess;
using sta::INF;
using sta::PathExpanded;
using sta::Port;
using sta::VertexOutEdgeIterator;

RepairHold::RepairHold(Resizer* resizer) : resizer_(resizer)
{
}

void RepairHold::init()
{
  logger_ = resizer_->logger_;
  dbStaState::init(resizer_->sta_);
  db_network_ = resizer_->db_network_;
  estimate_parasitics_ = resizer_->estimate_parasitics_;
  initial_design_area_ = resizer_->computeDesignArea();
}

bool RepairHold::repairHold(
    const double setup_margin,
    const double hold_margin,
    const bool allow_setup_violations,
    // Max buffer count as percent of design instance count.
    const float max_buffer_percent,
    const int max_passes,
    const bool verbose)
{
  bool repaired = false;
  init();
  sta_->checkSlewLimitPreamble();
  sta_->checkCapacitanceLimitPreamble();
  LibertyCell* buffer_cell = findHoldBuffer();

  sta_->findRequireds();
  VertexSet* ends = sta_->search()->endpoints();
  VertexSeq ends1;
  for (Vertex* end : *ends) {
    ends1.push_back(end);
  }
  sort(ends1, sta::VertexIdLess(graph_));

  int max_buffer_count = max_buffer_percent * network_->instanceCount();
  // Prevent it from being too small on trivial designs
  max_buffer_count = std::max(max_buffer_count, 100);

  {
    est::IncrementalParasiticsGuard guard(estimate_parasitics_);
    repaired = repairHold(ends1,
                          buffer_cell,
                          setup_margin,
                          hold_margin,
                          allow_setup_violations,
                          max_buffer_count,
                          max_passes,
                          verbose);
  }

  return repaired;
}

// For testing/debug.
void RepairHold::repairHold(const Pin* end_pin,
                            const double setup_margin,
                            const double hold_margin,
                            const bool allow_setup_violations,
                            const float max_buffer_percent,
                            const int max_passes)
{
  init();
  sta_->checkSlewLimitPreamble();
  sta_->checkCapacitanceLimitPreamble();
  LibertyCell* buffer_cell = findHoldBuffer();

  Vertex* end = graph_->pinLoadVertex(end_pin);
  VertexSeq ends;
  ends.push_back(end);

  sta_->findRequireds();
  const int max_buffer_count = max_buffer_percent * network_->instanceCount();

  {
    est::IncrementalParasiticsGuard guard(estimate_parasitics_);
    repairHold(ends,
               buffer_cell,
               setup_margin,
               hold_margin,
               allow_setup_violations,
               max_buffer_count,
               max_passes,
               false);
  }
}

LibertyCell* RepairHold::reportHoldBuffer()
{
  init();
  return findHoldBuffer();
}

// Find a good hold buffer using delay/area as the metric.
LibertyCell* RepairHold::findHoldBuffer()
{
  // Build a vector of buffers sorted by the metric
  struct MetricBuffer
  {
    float metric;
    LibertyCell* cell;
  };
  std::vector<MetricBuffer> buffers;
  LibertyCellSeq* hold_buffers = nullptr;
  LibertyCellSeq buffer_list;
  if (resizer_->disable_buffer_pruning_) {
    hold_buffers = &resizer_->buffer_cells_;
  } else {
    filterHoldBuffers(buffer_list);
    hold_buffers = &buffer_list;
  }

  for (LibertyCell* buffer : *hold_buffers) {
    const float buffer_area = buffer->area();
    if (buffer_area != 0.0) {
      const float buffer_cost = bufferHoldDelay(buffer) / buffer_area;
      buffers.push_back({buffer_cost, buffer});
    }
  }

  std::sort(buffers.begin(),
            buffers.end(),
            [](const MetricBuffer& lhs, const MetricBuffer& rhs) {
              return lhs.metric < rhs.metric;
            });

  if (buffers.empty()) {
    return nullptr;
  }

  // Select the highest metric
  MetricBuffer& best_buffer = *buffers.rbegin();
  const MetricBuffer& highest_metric = best_buffer;

  // See if there is a smaller choice with nearly as good a metric.
  const float margin = 0.95;
  for (auto itr = buffers.rbegin() + 1; itr != buffers.rend(); itr++) {
    if (itr->metric >= margin * highest_metric.metric) {
      // buffer within margin, so check if area is smaller
      const float best_buffer_area = best_buffer.cell->area();
      const float buffer_area = itr->cell->area();

      if (buffer_area < best_buffer_area) {
        best_buffer = *itr;
      }
    }
  }

  return best_buffer.cell;
}

// Only DEL, DLY, and dlygate are supported in current PDKs
// leading  3 nm:  DEL
// common   7 nm:  DLY
//         12 nm:  DLY
// skywater130hs:  dlygate
bool isDelayCell(const std::string& cell_name)
{
  return (!cell_name.empty()
          && (cell_name.find("DEL") != std::string::npos
              || cell_name.find("DLY") != std::string::npos
              || cell_name.find("dlygate") != std::string::npos));
}

void RepairHold::filterHoldBuffers(LibertyCellSeq& hold_buffers)
{
  LibertyCellSeq buffer_list;
  resizer_->getBufferList(buffer_list);

  // Pick the least leaky VT for multiple VTs
  int best_vt_index = -1;
  int num_vt = resizer_->lib_data_->sorted_vt_categories.size();
  if (num_vt > 0) {
    best_vt_index = resizer_->lib_data_->sorted_vt_categories[0].first.vt_index;
  }

  // Pick the shortest cell site because this offers the most flexibility for
  // hold fixing
  int best_height = std::numeric_limits<int>::max();
  odb::dbSite* best_site = nullptr;
  for (const auto& site_data : resizer_->lib_data_->cells_by_site) {
    int height = site_data.first->getHeight();
    if (height < best_height) {
      best_height = height;
      best_site = site_data.first;
    }
  }

  // Use DELAY cell footprint if available
  bool lib_has_footprints = false;
  for (const auto& [ft_str, count] : resizer_->lib_data_->cells_by_footprint) {
    if (isDelayCell(ft_str)) {
      lib_has_footprints = true;
      break;
    }
  }

  bool match = false;
  // Match site, footprint and vt
  if (addMatchingBuffers(buffer_list,
                         hold_buffers,
                         best_vt_index,
                         best_site,
                         lib_has_footprints,
                         true /* match_site */,
                         true /* match_vt */,
                         true /* match_footprint */)) {
    match = true;
    // Match footprint and vt only
  } else if (addMatchingBuffers(buffer_list,
                                hold_buffers,
                                best_vt_index,
                                best_site,
                                lib_has_footprints,
                                false /* match_site */,
                                true /* match_vt */,
                                true /* match_footprint */)) {
    match = true;
    // Match footprint only
  } else if (addMatchingBuffers(buffer_list,
                                hold_buffers,
                                best_vt_index,
                                best_site,
                                lib_has_footprints,
                                false /* match_site */,
                                false /* match_vt */,
                                true /* match_footprint */)) {
    match = true;
    // Relax all
  } else if (addMatchingBuffers(buffer_list,

                                hold_buffers,
                                best_vt_index,
                                best_site,
                                lib_has_footprints,
                                false /* match_site */,
                                false /* match_vt */,
                                false /* match_footprint */)) {
    match = true;
  }

  if (!match) {
    logger_->error(RSZ, 167, "No suitable hold buffers have been found");
  }
}

bool RepairHold::addMatchingBuffers(const LibertyCellSeq& buffer_list,
                                    LibertyCellSeq& hold_buffers,
                                    int best_vt_index,
                                    odb::dbSite* best_site,
                                    bool lib_has_footprints,
                                    bool match_site,
                                    bool match_vt,
                                    bool match_footprint)
{
  bool hold_buffer_found = false;
  for (LibertyCell* buffer : buffer_list) {
    odb::dbMaster* master = db_network_->staToDb(buffer);

    bool site_matches = true;
    if (match_site) {
      site_matches = master->getSite() == best_site;
    }

    bool vt_matches = true;
    if (match_vt) {
      auto vt_type = resizer_->cellVTType(master);
      vt_matches = best_vt_index == -1 || vt_type.vt_index == best_vt_index;
    }

    bool footprint_matches = true;
    if (match_footprint) {
      const char* footprint_cstr = buffer->footprint();
      std::string footprint = footprint_cstr ? footprint_cstr : "";
      footprint_matches = !lib_has_footprints || isDelayCell(footprint);
    }

    if (site_matches && vt_matches && footprint_matches) {
      hold_buffers.emplace_back(buffer);
      debugPrint(logger_,
                 RSZ,
                 "resizer",
                 1,
                 "{} added to hold buffer",
                 buffer->name());
      hold_buffer_found = true;
    }
  }

  return hold_buffer_found;
}

float RepairHold::bufferHoldDelay(LibertyCell* buffer)
{
  Delay delays[RiseFall::index_count];
  bufferHoldDelays(buffer, delays);
  return min(delays[RiseFall::riseIndex()], delays[RiseFall::fallIndex()]);
}

// Min self delay across corners; buffer -> buffer
void RepairHold::bufferHoldDelays(LibertyCell* buffer,
                                  // Return values.
                                  Delay delays[RiseFall::index_count])
{
  LibertyPort *input, *output;
  buffer->bufferPorts(input, output);

  for (int rf_index : RiseFall::rangeIndex()) {
    delays[rf_index] = MinMax::min()->initValue();
  }
  for (Corner* corner : *sta_->corners()) {
    LibertyPort* corner_port = input->cornerPort(corner->libertyIndex(max_));
    const DcalcAnalysisPt* dcalc_ap = corner->findDcalcAnalysisPt(max_);
    const float load_cap = corner_port->capacitance();
    ArcDelay gate_delays[RiseFall::index_count];
    Slew slews[RiseFall::index_count];
    resizer_->gateDelays(output, load_cap, dcalc_ap, gate_delays, slews);
    for (int rf_index : RiseFall::rangeIndex()) {
      delays[rf_index] = min(delays[rf_index], gate_delays[rf_index]);
    }
  }
}

bool RepairHold::repairHold(VertexSeq& ends,
                            LibertyCell* buffer_cell,
                            const double setup_margin,
                            const double hold_margin,
                            const bool allow_setup_violations,
                            const int max_buffer_count,
                            const int max_passes,
                            const bool verbose)
{
  bool repaired = false;
  // Find endpoints with hold violations.
  VertexSeq hold_failures;
  Slack worst_slack;
  findHoldViolations(ends, hold_margin, worst_slack, hold_failures);
  inserted_buffer_count_ = 0;
  if (!hold_failures.empty()) {
    logger_->info(RSZ,
                  46,
                  "Found {} endpoints with hold violations.",
                  hold_failures.size());
    bool progress = true;
    printProgress(0, true, false);
    int pass = 1;
    while (worst_slack < hold_margin && progress && !resizer_->overMaxArea()
           && inserted_buffer_count_ <= max_buffer_count
           && pass <= max_passes) {
      if (verbose || pass == 1) {
        printProgress(pass, false, false);
      }
      debugPrint(logger_,
                 RSZ,
                 "repair_hold",
                 1,
                 "pass {} hold slack {} setup slack {}",
                 pass,
                 delayAsString(worst_slack, sta_, 3),
                 delayAsString(sta_->worstSlack(max_), sta_, 3));
      int hold_buffer_count_before = inserted_buffer_count_;
      repairHoldPass(hold_failures,
                     buffer_cell,
                     setup_margin,
                     hold_margin,
                     allow_setup_violations,
                     max_buffer_count,
                     verbose,
                     pass);
      debugPrint(logger_,
                 RSZ,
                 "repair_hold",
                 1,
                 "inserted {}",
                 inserted_buffer_count_ - hold_buffer_count_before);
      sta_->findRequireds();
      findHoldViolations(ends, hold_margin, worst_slack, hold_failures);
      progress = inserted_buffer_count_ > hold_buffer_count_before;
    }
    printProgress(pass, true, true);
    if (hold_margin == 0.0 && fuzzyLess(worst_slack, 0.0)) {
      logger_->warn(RSZ, 66, "Unable to repair all hold violations.");
    } else if (fuzzyLess(worst_slack, hold_margin)) {
      logger_->warn(RSZ, 64, "Unable to repair all hold checks within margin.");
    }

    if (inserted_buffer_count_ > 0) {
      repaired = true;
      logger_->info(
          RSZ, 32, "Inserted {} hold buffers.", inserted_buffer_count_);
      resizer_->level_drvr_vertices_valid_ = false;
    }
    if (inserted_buffer_count_ > max_buffer_count) {
      logger_->error(RSZ, 60, "Max buffer count reached.");
    }
    if (resizer_->overMaxArea()) {
      logger_->error(RSZ, 50, "Max utilization reached.");
    }
  } else {
    repaired = false;
    logger_->info(RSZ, 33, "No hold violations found.");
  }
  logger_->metric("design__instance__count__hold_buffer",
                  inserted_buffer_count_);

  return repaired;
}

void RepairHold::findHoldViolations(VertexSeq& ends,
                                    const double hold_margin,
                                    // Return values.
                                    Slack& worst_slack,
                                    VertexSeq& hold_violations)
{
  worst_slack = INF;
  hold_violations.clear();
  debugPrint(logger_, RSZ, "repair_hold", 3, "Hold violations");
  for (Vertex* end : ends) {
    const Slack slack = sta_->vertexSlack(end, min_);
    if (!sta_->isClock(end->pin()) && slack < hold_margin) {
      debugPrint(logger_,
                 RSZ,
                 "repair_hold",
                 3,
                 " {} hold_slack={} setup_slack={}",
                 end->name(sdc_network_),
                 delayAsString(slack, sta_),
                 delayAsString(sta_->vertexSlack(end, max_), sta_));
      if (slack < worst_slack) {
        worst_slack = slack;
      }
      hold_violations.push_back(end);
    }
  }
}

void RepairHold::repairHoldPass(VertexSeq& hold_failures,
                                LibertyCell* buffer_cell,
                                const double setup_margin,
                                const double hold_margin,
                                const bool allow_setup_violations,
                                const int max_buffer_count,
                                bool verbose,
                                int& pass)
{
  estimate_parasitics_->updateParasitics();
  sort(hold_failures, [=](Vertex* end1, Vertex* end2) {
    return sta_->vertexSlack(end1, min_) < sta_->vertexSlack(end2, min_);
  });
  for (Vertex* end_vertex : hold_failures) {
    if (verbose) {
      printProgress(pass, false, false);
    }

    estimate_parasitics_->updateParasitics();
    repairEndHold(end_vertex,
                  buffer_cell,
                  setup_margin,
                  hold_margin,
                  allow_setup_violations);
    pass++;
    if (inserted_buffer_count_ > max_buffer_count) {
      break;
    }
  }
}

void RepairHold::repairEndHold(Vertex* end_vertex,
                               LibertyCell* buffer_cell,
                               const double setup_margin,
                               const double hold_margin,
                               const bool allow_setup_violations)
{
  Path* end_path = sta_->vertexWorstSlackPath(end_vertex, min_);
  if (end_path) {
    debugPrint(logger_,
               RSZ,
               "repair_hold",
               3,
               "repair end {} hold_slack={} setup_slack={}",
               end_vertex->name(network_),
               delayAsString(end_path->slack(sta_), sta_),
               delayAsString(sta_->vertexSlack(end_vertex, max_), sta_));
    PathExpanded expanded(end_path, sta_);
    sta::SearchPredNonLatch2 pred(sta_);
    const int path_length = expanded.size();
    if (path_length > 1) {
      sta::VertexSeq path_vertices;
      // Inserting bufferes invalidates the paths so copy out the vertices
      // in the path.
      for (int i = expanded.startIndex(); i < path_length; i++) {
        path_vertices.push_back(expanded.path(i)->vertex(sta_));
      }
      // Stop one short of the end so we can get the load.
      for (int i = 0; i < path_vertices.size() - 1; i++) {
        Vertex* path_vertex = path_vertices[i];
        Pin* path_pin = path_vertex->pin();

        if (path_vertex->isDriver(network_)
            && resizer_->okToBufferNet(path_pin)) {
          PinSeq load_pins;
          Slacks slacks;
          mergeInit(slacks);
          float excluded_cap = 0.0;
          bool loads_have_out_port = false;
          VertexOutEdgeIterator edge_iter(path_vertex, graph_);
          while (edge_iter.hasNext()) {
            Edge* edge = edge_iter.next();
            Vertex* fanout = edge->to(graph_);
            if (pred.searchTo(fanout) && pred.searchThru(edge)) {
              Slack fanout_hold_slack = sta_->vertexSlack(fanout, min_);
              Pin* load_pin = fanout->pin();
              if (fanout_hold_slack < hold_margin) {
                load_pins.push_back(load_pin);
                Slacks fanout_slacks;
                sta_->vertexSlacks(fanout, fanout_slacks);
                mergeInto(fanout_slacks, slacks);
                if (network_->direction(load_pin)->isAnyOutput()
                    && network_->isTopLevelPort(load_pin)) {
                  loads_have_out_port = true;
                }
              } else {
                LibertyPort* load_port = network_->libertyPort(load_pin);
                if (load_port) {
                  excluded_cap += load_port->capacitance();
                }
              }
            }
          }
          if (!load_pins.empty()) {
            debugPrint(logger_,
                       RSZ,
                       "repair_hold",
                       3,
                       " {} hold_slack={}/{} setup_slack={}/{} fanouts={}",
                       path_vertex->name(network_),
                       delayAsString(slacks[rise_index_][min_index_], sta_),
                       delayAsString(slacks[fall_index_][min_index_], sta_),
                       delayAsString(slacks[rise_index_][max_index_], sta_),
                       delayAsString(slacks[fall_index_][max_index_], sta_),
                       load_pins.size());
            const DcalcAnalysisPt* dcalc_ap
                = sta_->cmdCorner()->findDcalcAnalysisPt(max_);
            float load_cap
                = graph_delay_calc_->loadCap(end_vertex->pin(), dcalc_ap)
                  - excluded_cap;
            ArcDelay buffer_delays[RiseFall::index_count];
            Slew buffer_slews[RiseFall::index_count];
            resizer_->bufferDelays(
                buffer_cell, load_cap, dcalc_ap, buffer_delays, buffer_slews);
            // setup_slack > -hold_slack
            if (allow_setup_violations
                || (slacks[rise_index_][max_index_] - setup_margin
                        > -(slacks[rise_index_][min_index_] - hold_margin)
                    && slacks[fall_index_][max_index_] - setup_margin
                           > -(slacks[fall_index_][min_index_] - hold_margin)
                    // enough slack to insert the buffer
                    // setup_slack > buffer_delay
                    && (slacks[rise_index_][max_index_] - setup_margin)
                           > buffer_delays[rise_index_]
                    && (slacks[fall_index_][max_index_] - setup_margin)
                           > buffer_delays[fall_index_])) {
              Vertex* path_load = path_vertices[i + 1];
              Point path_load_loc = db_network_->location(path_load->pin());
              Point drvr_loc = db_network_->location(path_vertex->pin());
              Point buffer_loc((drvr_loc.x() + path_load_loc.x()) / 2,
                               (drvr_loc.y() + path_load_loc.y()) / 2);
              // Despite checking for setup slack to insert the bufffer,
              // increased slews downstream can increase delays and
              // reduce setup slack in ways that are too expensive to
              // predict. Use the journal to back out the change if
              // the hold buffer blows through the setup margin.
              resizer_->journalBegin();
              Slack setup_slack_before = sta_->worstSlack(max_);
              Slew slew_before = sta_->vertexSlew(path_vertex, max_);
              makeHoldDelay(path_vertex,
                            load_pins,
                            loads_have_out_port,
                            buffer_cell,
                            buffer_loc);
              Slew slew_after = sta_->vertexSlew(path_vertex, max_);
              Slack setup_slack_after = sta_->worstSlack(max_);
              float slew_factor
                  = (slew_before > 0) ? slew_after / slew_before : 1.0;

              if (slew_factor > 1.20
                  || (!allow_setup_violations
                      && fuzzyLess(setup_slack_after, setup_slack_before)
                      && setup_slack_after < setup_margin)) {
                resizer_->journalRestore();
                inserted_buffer_count_ = 0;
              } else {
                resizer_->journalEnd();
              }
            }
          }
        }
      }
    }
  }
}

void RepairHold::mergeInit(Slacks& slacks)
{
  slacks[rise_index_][min_index_] = INF;
  slacks[fall_index_][min_index_] = INF;
  slacks[rise_index_][max_index_] = -INF;
  slacks[fall_index_][max_index_] = -INF;
}

void RepairHold::mergeInto(Slacks& from, Slacks& result)
{
  result[rise_index_][min_index_]
      = min(result[rise_index_][min_index_], from[rise_index_][min_index_]);
  result[fall_index_][min_index_]
      = min(result[fall_index_][min_index_], from[fall_index_][min_index_]);
  result[rise_index_][max_index_]
      = max(result[rise_index_][max_index_], from[rise_index_][max_index_]);
  result[fall_index_][max_index_]
      = max(result[fall_index_][max_index_], from[fall_index_][max_index_]);
}

void RepairHold::makeHoldDelay(Vertex* drvr,
                               PinSeq& load_pins,
                               bool loads_have_out_port,  // top level port
                               LibertyCell* buffer_cell,
                               const Point& loc)
{
  Pin* drvr_pin = drvr->pin();
  dbNet* db_drvr_net = db_network_->findFlatDbNet(drvr_pin);
  odb::dbModNet* mod_drvr_net = db_network_->hierNet(drvr_pin);
  Instance* parent = db_network_->getOwningInstanceParent(drvr_pin);

  // If output port is in load pins, do "Driver pin buffering".
  // - Verilog uses nets as ports, so the net connected to an output port has
  // - to be preserved.
  // - Move the driver pin over to gensym'd net.
  // Otherwise, do "Load pin buffering".
  bool driver_pin_buffering = loads_have_out_port;
  Net* buf_input_net = nullptr;
  Net* buf_output_net = nullptr;
  if (driver_pin_buffering) {
    // Do "Driver pin buffering": Buffer drives the existing net.
    //
    //     driver --- (new_net) --- new_buffer ---- (existing_net) ---- loads
    //
    buf_input_net = db_network_->makeNet(parent);  // New net
    Port* drvr_port = network_->port(drvr_pin);
    Instance* drvr_inst = network_->instance(drvr_pin);
    sta_->disconnectPin(drvr_pin);
    sta_->connectPin(drvr_inst, drvr_port, buf_input_net);
    buf_output_net = db_network_->dbToSta(db_drvr_net);
  } else {
    // Do "Load pin buffering": The existing net drives the new buffer.
    //
    //     driver --- (existing_net) --- new_buffer ---- (new_net) ---- loads
    //
    buf_input_net = db_network_->dbToSta(db_drvr_net);
    buf_output_net = db_network_->makeNet(parent);  // New net
  }

  // Make the buffer in the driver pin's parent hierarchy
  Instance* buffer = resizer_->makeBuffer(buffer_cell, "hold", parent, loc);
  inserted_buffer_count_++;
  debugPrint(
      logger_, RSZ, "repair_hold", 3, " insert {}", network_->name(buffer));

  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  Pin* buffer_in_pin = network_->findPin(buffer, input);
  Pin* buffer_out_pin = network_->findPin(buffer, output);

  // Connect input and output of the new buffer
  sta_->connectPin(buffer, input, buf_input_net);
  sta_->connectPin(buffer, output, buf_output_net);

  // TODO: Revisit this. Looks not good.
  if (mod_drvr_net) {
    //  The input of the buffer is a new load on the original hierarchical net.
    db_network_->connectPin(buffer_in_pin, (Net*) mod_drvr_net);
  }

  // Buffering target load pins.
  // - No need in driver pin buffering case
  if (driver_pin_buffering == false) {
    // Do load pins buffering
    for (const Pin* load_pin : load_pins) {
      if (resizer_->dontTouch(load_pin)) {
        continue;
      }

      // Disconnect the load pin
      db_network_->disconnectPin(const_cast<Pin*>(load_pin));

      // Connect with the buffer output
      db_network_->hierarchicalConnect(db_network_->flatPin(buffer_out_pin),
                                       db_network_->flatPin(load_pin));
    }
  }

  // Update RC and delay. Resize if necessary
  Vertex* buffer_out_vertex = graph_->pinDrvrVertex(buffer_out_pin);
  estimate_parasitics_->updateParasitics();
  // Sta::checkMaxSlewCap does not force dcalc update so do it explicitly.
  sta_->findDelays(buffer_out_vertex);
  if (!checkMaxSlewCap(buffer_out_pin)
      && resizer_->resizeToTargetSlew(buffer_out_pin)) {
    estimate_parasitics_->updateParasitics();
    resize_count_++;
  }
}

bool RepairHold::checkMaxSlewCap(const Pin* drvr_pin)
{
  float cap, limit, slack;
  const Corner* corner;
  const RiseFall* tr;
  sta_->checkCapacitance(
      drvr_pin, nullptr, max_, corner, tr, cap, limit, slack);
  float slack_limit_ratio = slack / limit;
  if (slack_limit_ratio < hold_slack_limit_ratio_max_) {
    return false;
  }

  Slew slew;
  sta_->checkSlew(
      drvr_pin, nullptr, max_, false, corner, tr, slew, limit, slack);
  slack_limit_ratio = slack / limit;
  if (slack_limit_ratio < hold_slack_limit_ratio_max_) {
    return false;
  }

  resizer_->checkLoadSlews(drvr_pin, 0.0, slew, limit, slack, corner);
  slack_limit_ratio = slack / limit;
  if (slack_limit_ratio < hold_slack_limit_ratio_max_) {
    return false;
  }

  return true;
}

void RepairHold::printProgress(int iteration, bool force, bool end) const
{
  const bool start = iteration == 0;

  if (start) {
    logger_->report(
        "Iteration | Resized | Buffers | Cloned Gates |   Area   |   WNS   "
        "|   TNS   | Endpoint");
    logger_->report("{:->86}", "");
  }

  if (iteration % print_interval_ == 0 || force || end) {
    Slack wns;
    Vertex* worst_vertex;
    sta_->worstSlack(min_, wns, worst_vertex);
    const Slack tns = sta_->totalNegativeSlack(min_);

    std::string itr_field = fmt::format("{}", iteration);
    if (end) {
      itr_field = "final";
    }

    const double design_area = resizer_->computeDesignArea();
    const double area_growth = design_area - initial_design_area_;

    logger_->report(
        "{: >9s} | {: >7d} | {: >7d} | {: >12d} | {: >+7.1f}% | {: >7s} | {: "
        ">7s} | {}",
        itr_field,
        resize_count_,
        inserted_buffer_count_,
        cloned_gate_count_,
        area_growth / initial_design_area_ * 1e2,
        delayAsString(wns, sta_, 3),
        delayAsString(tns, sta_, 3),
        worst_vertex->name(network_));

    debugPrint(logger_, RSZ, "memory", 1, "RSS = {}", utl::getCurrentRSS());
  }

  if (end) {
    logger_->report("{:->86}", "");
  }
}

}  // namespace rsz
