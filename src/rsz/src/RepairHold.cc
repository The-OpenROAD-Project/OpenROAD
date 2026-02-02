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
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InputDrive.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Parasitics.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchPred.hh"
#include "sta/TimingArc.hh"
#include "sta/Transition.hh"
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
    const int max_iterations,
    const bool verbose)
{
  bool repaired = false;
  init();
  sta_->checkSlewLimitPreamble();
  sta_->checkCapacitanceLimitPreamble();
  sta::LibertyCell* buffer_cell = findHoldBuffer();

  sta_->findRequireds();
  sta::VertexSet* ends = sta_->search()->endpoints();
  sta::VertexSeq ends1;
  for (sta::Vertex* end : *ends) {
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
                          max_iterations,
                          verbose);
  }

  return repaired;
}

// For testing/debug.
void RepairHold::repairHold(const sta::Pin* end_pin,
                            const double setup_margin,
                            const double hold_margin,
                            const bool allow_setup_violations,
                            const float max_buffer_percent,
                            const int max_passes)
{
  init();
  sta_->checkSlewLimitPreamble();
  sta_->checkCapacitanceLimitPreamble();
  sta::LibertyCell* buffer_cell = findHoldBuffer();

  sta::Vertex* end = graph_->pinLoadVertex(end_pin);
  sta::VertexSeq ends;
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
               // set max_iterations to max_passes for testing
               max_passes,
               false);
  }
}

sta::LibertyCell* RepairHold::reportHoldBuffer()
{
  init();
  return findHoldBuffer();
}

// Find a good hold buffer using delay/area as the metric.
sta::LibertyCell* RepairHold::findHoldBuffer()
{
  // Build a vector of buffers sorted by the metric
  struct MetricBuffer
  {
    float metric;
    sta::LibertyCell* cell;
  };
  std::vector<MetricBuffer> buffers;
  sta::LibertyCellSeq* hold_buffers = nullptr;
  sta::LibertyCellSeq buffer_list;
  if (resizer_->disable_buffer_pruning_) {
    hold_buffers = &resizer_->buffer_cells_;
  } else {
    filterHoldBuffers(buffer_list);
    hold_buffers = &buffer_list;
  }

  for (sta::LibertyCell* buffer : *hold_buffers) {
    const float buffer_area = buffer->area();
    if (buffer_area != 0.0) {
      const float buffer_cost = bufferHoldDelay(buffer) / buffer_area;
      buffers.push_back({buffer_cost, buffer});
    }
  }

  std::ranges::sort(buffers,
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
static bool isDelayCell(const std::string& cell_name)
{
  return (!cell_name.empty()
          && (cell_name.find("DEL") != std::string::npos
              || cell_name.find("DLY") != std::string::npos
              || cell_name.find("dlygate") != std::string::npos));
}

void RepairHold::filterHoldBuffers(sta::LibertyCellSeq& hold_buffers)
{
  sta::LibertyCellSeq buffer_list;
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

bool RepairHold::addMatchingBuffers(const sta::LibertyCellSeq& buffer_list,
                                    sta::LibertyCellSeq& hold_buffers,
                                    int best_vt_index,
                                    odb::dbSite* best_site,
                                    bool lib_has_footprints,
                                    bool match_site,
                                    bool match_vt,
                                    bool match_footprint)
{
  bool hold_buffer_found = false;
  for (sta::LibertyCell* buffer : buffer_list) {
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

float RepairHold::bufferHoldDelay(sta::LibertyCell* buffer)
{
  sta::Delay delays[sta::RiseFall::index_count];
  bufferHoldDelays(buffer, delays);
  return min(delays[sta::RiseFall::riseIndex()],
             delays[sta::RiseFall::fallIndex()]);
}

// Min self delay across corners; buffer -> buffer
void RepairHold::bufferHoldDelays(sta::LibertyCell* buffer,
                                  // Return values.
                                  sta::Delay delays[sta::RiseFall::index_count])
{
  sta::LibertyPort *input, *output;
  buffer->bufferPorts(input, output);

  for (int rf_index : sta::RiseFall::rangeIndex()) {
    delays[rf_index] = sta::MinMax::min()->initValue();
  }
  for (sta::Corner* corner : *sta_->corners()) {
    sta::LibertyPort* corner_port
        = input->cornerPort(corner->libertyIndex(max_));
    const sta::DcalcAnalysisPt* dcalc_ap = corner->findDcalcAnalysisPt(max_);
    const float load_cap = corner_port->capacitance();
    sta::ArcDelay gate_delays[sta::RiseFall::index_count];
    sta::Slew slews[sta::RiseFall::index_count];
    resizer_->gateDelays(output, load_cap, dcalc_ap, gate_delays, slews);
    for (int rf_index : sta::RiseFall::rangeIndex()) {
      delays[rf_index] = min(delays[rf_index], gate_delays[rf_index]);
    }
  }
}

bool RepairHold::repairHold(sta::VertexSeq& ends,
                            sta::LibertyCell* buffer_cell,
                            const double setup_margin,
                            const double hold_margin,
                            const bool allow_setup_violations,
                            const int max_buffer_count,
                            const int max_passes,
                            const int max_iterations,
                            const bool verbose)
{
  bool repaired = false;
  // Find endpoints with hold violations.
  sta::VertexSeq hold_failures;
  sta::Slack worst_slack;
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
           && inserted_buffer_count_ <= max_buffer_count && pass <= max_passes
           && (max_iterations < 0 || pass <= max_iterations)) {
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

void RepairHold::findHoldViolations(sta::VertexSeq& ends,
                                    const double hold_margin,
                                    // Return values.
                                    sta::Slack& worst_slack,
                                    sta::VertexSeq& hold_violations)
{
  worst_slack = INF;
  hold_violations.clear();
  debugPrint(logger_, RSZ, "repair_hold", 3, "Hold violations");
  for (sta::Vertex* end : ends) {
    const sta::Slack slack = sta_->vertexSlack(end, min_);
    if (!sta_->isClock(end->pin()) && slack < hold_margin) {
      debugPrint(logger_,
                 RSZ,
                 "repair_hold",
                 3,
                 " {} hold_slack={} setup_slack={}",
                 end->name(sdc_network_),
                 delayAsString(slack, sta_),
                 delayAsString(sta_->vertexSlack(end, max_), sta_));
      worst_slack = std::min(slack, worst_slack);
      hold_violations.push_back(end);
    }
  }
}

void RepairHold::repairHoldPass(sta::VertexSeq& hold_failures,
                                sta::LibertyCell* buffer_cell,
                                const double setup_margin,
                                const double hold_margin,
                                const bool allow_setup_violations,
                                const int max_buffer_count,
                                bool verbose,
                                int& pass)
{
  estimate_parasitics_->updateParasitics();
  sort(hold_failures, [=, this](sta::Vertex* end1, sta::Vertex* end2) {
    return sta_->vertexSlack(end1, min_) < sta_->vertexSlack(end2, min_);
  });
  for (sta::Vertex* end_vertex : hold_failures) {
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

void RepairHold::repairEndHold(sta::Vertex* end_vertex,
                               sta::LibertyCell* buffer_cell,
                               const double setup_margin,
                               const double hold_margin,
                               const bool allow_setup_violations)
{
  sta::Path* end_path = sta_->vertexWorstSlackPath(end_vertex, min_);
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
        sta::Vertex* path_vertex = path_vertices[i];
        sta::Pin* path_pin = path_vertex->pin();

        if (path_vertex->isDriver(network_)
            && resizer_->okToBufferNet(path_pin)) {
          sta::PinSeq load_pins;
          Slacks slacks;
          mergeInit(slacks);
          float excluded_cap = 0.0;
          bool loads_have_out_port = false;
          VertexOutEdgeIterator edge_iter(path_vertex, graph_);
          while (edge_iter.hasNext()) {
            Edge* edge = edge_iter.next();
            sta::Vertex* fanout = edge->to(graph_);
            if (pred.searchTo(fanout) && pred.searchThru(edge)) {
              sta::Slack fanout_hold_slack = sta_->vertexSlack(fanout, min_);
              sta::Pin* load_pin = fanout->pin();
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
                sta::LibertyPort* load_port = network_->libertyPort(load_pin);
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
            const sta::DcalcAnalysisPt* dcalc_ap
                = sta_->cmdCorner()->findDcalcAnalysisPt(max_);
            float load_cap
                = graph_delay_calc_->loadCap(end_vertex->pin(), dcalc_ap)
                  - excluded_cap;
            sta::ArcDelay buffer_delays[sta::RiseFall::index_count];
            sta::Slew buffer_slews[sta::RiseFall::index_count];
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
              sta::Vertex* path_load = path_vertices[i + 1];
              odb::Point path_load_loc
                  = db_network_->location(path_load->pin());
              odb::Point drvr_loc = db_network_->location(path_vertex->pin());
              odb::Point buffer_loc((drvr_loc.x() + path_load_loc.x()) / 2,
                                    (drvr_loc.y() + path_load_loc.y()) / 2);
              // Despite checking for setup slack to insert the bufffer,
              // increased slews downstream can increase delays and
              // reduce setup slack in ways that are too expensive to
              // predict. Use the journal to back out the change if
              // the hold buffer blows through the setup margin.
              resizer_->journalBegin();
              sta::Slack setup_slack_before = sta_->worstSlack(max_);
              sta::Slew slew_before = sta_->vertexSlew(path_vertex, max_);
              makeHoldDelay(path_vertex,
                            load_pins,
                            loads_have_out_port,
                            buffer_cell,
                            buffer_loc);
              sta::Slew slew_after = sta_->vertexSlew(path_vertex, max_);
              sta::Slack setup_slack_after = sta_->worstSlack(max_);
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

void RepairHold::makeHoldDelay(sta::Vertex* drvr,
                               sta::PinSeq& load_pins,
                               bool loads_have_out_port,  // top level port
                               sta::LibertyCell* buffer_cell,
                               const odb::Point& loc)
{
  sta::Instance* buffer = nullptr;
  sta::Pin* buffer_out_pin = nullptr;

  // New insert buffer behavior
  sta::Pin* drvr_pin = drvr->pin();
  odb::dbObject* drvr_db_pin = db_network_->staToDb(drvr_pin);
  odb::dbNet* drvr_dbnet = nullptr;
  if (drvr_db_pin->getObjectType() == odb::dbObjectType::dbBTermObj) {
    drvr_dbnet = static_cast<odb::dbBTerm*>(drvr_db_pin)->getNet();
  } else {
    drvr_dbnet = static_cast<odb::dbITerm*>(drvr_db_pin)->getNet();
  }

  sta::Net* drvr_net = db_network_->dbToSta(drvr_dbnet);

  // PinSeq -> PinSet
  sta::PinSet load_pins_set(network_);
  for (const sta::Pin* load_pin : load_pins) {
    if (load_pin != nullptr) {
      if (resizer_->dontTouch(load_pin)) {
        continue;
      }
      load_pins_set.insert(const_cast<sta::Pin*>(load_pin));
    }
  }

  buffer = resizer_->insertBufferBeforeLoads(
      drvr_net, &load_pins_set, buffer_cell, &loc, "hold");
  if (buffer == nullptr) {
    const char* drvr_pin_name = db_network_->pathName(drvr_pin);
    logger_->error(RSZ,
                   3009,
                   "insert_buffer failed on drvr_pin '{}'.",
                   drvr_pin_name ? drvr_pin_name : "<unknown>");
    return;
  }

  odb::dbInst* new_buffer = db_network_->staToDb(buffer);
  debugPrint(
      logger_, RSZ, "repair_hold", 3, " insert {}", new_buffer->getName());

  buffer_out_pin = db_network_->dbToSta(new_buffer->getFirstOutput());

  inserted_buffer_count_++;

  // Update RC and delay. Resize if necessary
  sta::Vertex* buffer_out_vertex = graph_->pinDrvrVertex(buffer_out_pin);
  estimate_parasitics_->updateParasitics();
  // Sta::checkMaxSlewCap does not force dcalc update so do it explicitly.
  sta_->findDelays(buffer_out_vertex);
  if (!checkMaxSlewCap(buffer_out_pin)
      && resizer_->resizeToTargetSlew(buffer_out_pin)) {
    estimate_parasitics_->updateParasitics();
    resize_count_++;
  }
}

bool RepairHold::checkMaxSlewCap(const sta::Pin* drvr_pin)
{
  float cap, limit, slack;
  const sta::Corner* corner;
  const sta::RiseFall* tr;
  sta_->checkCapacitance(
      drvr_pin, nullptr, max_, corner, tr, cap, limit, slack);
  float slack_limit_ratio = slack / limit;
  if (slack_limit_ratio < hold_slack_limit_ratio_max_) {
    return false;
  }

  sta::Slew slew;
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
    sta::Slack wns;
    sta::Vertex* worst_vertex;
    sta_->worstSlack(min_, wns, worst_vertex);
    const sta::Slack tns = sta_->totalNegativeSlack(min_);

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
