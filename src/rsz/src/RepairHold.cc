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

#include "RepairHold.hh"

#include "RepairDesign.hh"
#include "db_sta/dbNetwork.hh"
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
#include "sta/Search.hh"
#include "sta/TimingArc.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"
#include "utl/timer.h"

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
}

void RepairHold::repairHold(
    const double setup_margin,
    const double hold_margin,
    const bool allow_setup_violations,
    // Max buffer count as percent of design instance count.
    const float max_buffer_percent,
    const int max_passes,
    const bool verbose)
{
  init();
  utl::ScopedStatistics stat(logger_, "repair_timing");
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
  resizer_->incrementalParasiticsBegin();
  repairHold(ends1,
             buffer_cell,
             setup_margin,
             hold_margin,
             allow_setup_violations,
             max_buffer_count,
             max_passes,
             verbose);

  // Leave the parasitices up to date.
  resizer_->updateParasitics();
  resizer_->incrementalParasiticsEnd();
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
  utl::ScopedStatistics stat(logger_, "repair_timing");
  sta_->checkSlewLimitPreamble();
  sta_->checkCapacitanceLimitPreamble();
  LibertyCell* buffer_cell = findHoldBuffer();

  Vertex* end = graph_->pinLoadVertex(end_pin);
  VertexSeq ends;
  ends.push_back(end);

  sta_->findRequireds();
  const int max_buffer_count = max_buffer_percent * network_->instanceCount();
  resizer_->incrementalParasiticsBegin();
  repairHold(ends,
             buffer_cell,
             setup_margin,
             hold_margin,
             allow_setup_violations,
             max_buffer_count,
             max_passes,
             false);
  // Leave the parasitices up to date.
  resizer_->updateParasitics();
  resizer_->incrementalParasiticsEnd();
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
  for (LibertyCell* buffer : resizer_->buffer_cells_) {
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

void RepairHold::repairHold(VertexSeq& ends,
                            LibertyCell* buffer_cell,
                            const double setup_margin,
                            const double hold_margin,
                            const bool allow_setup_violations,
                            const int max_buffer_count,
                            const int max_passes,
                            const bool verbose)
{
  // Find endpoints with hold violations.
  VertexSeq hold_failures;
  Slack worst_slack;
  findHoldViolations(ends, hold_margin, worst_slack, hold_failures);
  if (!hold_failures.empty()) {
    logger_->info(RSZ,
                  46,
                  "Found {} endpoints with hold violations.",
                  hold_failures.size());
    inserted_buffer_count_ = 0;
    bool progress = true;
    if (verbose) {
      printProgress(0, true, false);
    }
    int pass = 1;
    while (worst_slack < hold_margin && progress && !resizer_->overMaxArea()
           && inserted_buffer_count_ <= max_buffer_count
           && pass <= max_passes) {
      if (verbose) {
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
                     max_buffer_count);
      debugPrint(logger_,
                 RSZ,
                 "repair_hold",
                 1,
                 "inserted {}",
                 inserted_buffer_count_ - hold_buffer_count_before);
      sta_->findRequireds();
      findHoldViolations(ends, hold_margin, worst_slack, hold_failures);
      pass++;
      progress = inserted_buffer_count_ > hold_buffer_count_before;
    }
    if (verbose) {
      printProgress(pass, true, true);
    }
    if (hold_margin == 0.0 && fuzzyLess(worst_slack, 0.0)) {
      logger_->warn(RSZ, 66, "Unable to repair all hold violations.");
    } else if (fuzzyLess(worst_slack, hold_margin)) {
      logger_->warn(RSZ, 64, "Unable to repair all hold checks within margin.");
    }

    if (inserted_buffer_count_ > 0) {
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
    logger_->info(RSZ, 33, "No hold violations found.");
  }
  logger_->metric("design__instance__count__hold_buffer",
                  inserted_buffer_count_);
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
                                const int max_buffer_count)
{
  resizer_->updateParasitics();
  sort(hold_failures, [=](Vertex* end1, Vertex* end2) {
    return sta_->vertexSlack(end1, min_) < sta_->vertexSlack(end2, min_);
  });
  for (Vertex* end_vertex : hold_failures) {
    resizer_->updateParasitics();
    repairEndHold(end_vertex,
                  buffer_cell,
                  setup_margin,
                  hold_margin,
                  allow_setup_violations);
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
  PathRef end_path = sta_->vertexWorstSlackPath(end_vertex, min_);
  if (!end_path.isNull()) {
    debugPrint(logger_,
               RSZ,
               "repair_hold",
               3,
               "repair end {} hold_slack={} setup_slack={}",
               end_vertex->name(network_),
               delayAsString(end_path.slack(sta_), sta_),
               delayAsString(sta_->vertexSlack(end_vertex, max_), sta_));
    PathExpanded expanded(&end_path, sta_);
    sta::SearchPredNonLatch2 pred(sta_);
    const int path_length = expanded.size();
    if (path_length > 1) {
      for (int i = expanded.startIndex(); i < path_length; i++) {
        PathRef* path = expanded.path(i);
        Vertex* path_vertex = path->vertex(sta_);
        Pin* path_pin = path_vertex->pin();
        Net* path_net = network_->isTopLevelPort(path_pin)
                            ? network_->net(network_->term(path_pin))
                            : network_->net(path_pin);
        dbNet* db_path_net = db_network_->staToDb(path_net);
        if (path_vertex->isDriver(network_) && !resizer_->dontTouch(path_net)
            && !db_path_net->isConnectedByAbutment()) {
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
              Vertex* path_load = expanded.path(i + 1)->vertex(sta_);
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
                resizer_->journalRestore(
                    resize_count_, inserted_buffer_count_, cloned_gate_count_);
              }
              resizer_->journalEnd();
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
                               bool loads_have_out_port,
                               LibertyCell* buffer_cell,
                               const Point& loc)
{
  Pin* drvr_pin = drvr->pin();
  Instance* parent = db_network_->topInstance();
  Net* drvr_net = network_->isTopLevelPort(drvr_pin)
                      ? db_network_->net(db_network_->term(drvr_pin))
                      : db_network_->net(drvr_pin);
  Net *in_net, *out_net;
  if (loads_have_out_port) {
    // Verilog uses nets as ports, so the net connected to an output port has
    // to be preserved.
    // Move the driver pin over to gensym'd net.
    in_net = resizer_->makeUniqueNet();
    Port* drvr_port = network_->port(drvr_pin);
    Instance* drvr_inst = network_->instance(drvr_pin);
    sta_->disconnectPin(drvr_pin);
    sta_->connectPin(drvr_inst, drvr_port, in_net);
    out_net = drvr_net;
  } else {
    in_net = drvr_net;
    out_net = resizer_->makeUniqueNet();
  }

  resizer_->parasiticsInvalid(in_net);

  Net* buf_in_net = in_net;
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  // drvr_pin->drvr_net->hold_buffer->net2->load_pins
  string buffer_name = resizer_->makeUniqueInstName("hold");
  Instance* buffer
      = resizer_->makeBuffer(buffer_cell, buffer_name.c_str(), parent, loc);
  inserted_buffer_count_++;
  debugPrint(
      logger_, RSZ, "repair_hold", 3, " insert {}", network_->name(buffer));

  sta_->connectPin(buffer, input, buf_in_net);
  sta_->connectPin(buffer, output, out_net);
  resizer_->parasiticsInvalid(out_net);

  for (const Pin* load_pin : load_pins) {
    Net* load_net = network_->isTopLevelPort(load_pin)
                        ? network_->net(network_->term(load_pin))
                        : network_->net(load_pin);
    if (load_net != out_net) {
      Instance* load = db_network_->instance(load_pin);
      Port* load_port = db_network_->port(load_pin);
      sta_->disconnectPin(const_cast<Pin*>(load_pin));
      sta_->connectPin(load, load_port, out_net);
    }
  }

  Pin* buffer_out_pin = network_->findPin(buffer, output);
  Vertex* buffer_out_vertex = graph_->pinDrvrVertex(buffer_out_pin);
  resizer_->updateParasitics();
  // Sta::checkMaxSlewCap does not force dcalc update so do it explicitly.
  sta_->findDelays(buffer_out_vertex);
  if (!checkMaxSlewCap(buffer_out_pin)
      && resizer_->resizeToTargetSlew(buffer_out_pin)) {
    resizer_->updateParasitics();
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
        "Iteration | Resized | Buffers | Cloned Gates |   WNS   |   TNS   | "
        "Endpoint");
    logger_->report(
        "----------------------------------------------------------------------"
        "-----");
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

    logger_->report(
        "{: >9s} | {: >7d} | {: >7d} | {: >12d} | {: >7s} | {: >7s} | {}",
        itr_field,
        resize_count_,
        inserted_buffer_count_,
        cloned_gate_count_,
        delayAsString(wns, sta_, 3),
        delayAsString(tns, sta_, 3),
        worst_vertex->name(network_));
  }

  if (end) {
    logger_->report(
        "----------------------------------------------------------------------"
        "-----");
  }
}

}  // namespace rsz
