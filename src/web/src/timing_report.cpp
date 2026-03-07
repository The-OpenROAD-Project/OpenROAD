// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "timing_report.h"

#include <set>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/ExceptionPath.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Path.hh"
#include "sta/PathAnalysisPt.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/Units.hh"

namespace web {

TimingReport::TimingReport(sta::dbSta* sta) : sta_(sta)
{
}

void TimingReport::expandPath(sta::Path* path,
                              sta::DcalcAnalysisPt* dcalc_ap,
                              float offset,
                              bool clock_expanded,
                              std::vector<TimingNode>& nodes,
                              int& logic_depth,
                              int& total_fanout) const
{
  if (!path) {
    return;
  }

  float arrival_prev = 0.0f;
  float arrival_cur = 0.0f;

  const float time_scale = sta_->units()->timeUnit()->scale();
  const float cap_scale = sta_->units()->capacitanceUnit()->scale();

  sta::PathExpanded expand(path, sta_);
  auto* graph = sta_->graph();
  auto* network = sta_->network();
  auto* sdc = sta_->sdc();

  // Track logic instances for depth computation
  std::set<sta::Instance*> logic_insts;

  for (size_t i = 0; i < expand.size(); i++) {
    const auto* ref = expand.path(i);
    sta::Vertex* vertex = ref->vertex(sta_);
    const auto pin = vertex->pin();
    const bool pin_is_clock = sta_->isClock(pin);
    const bool is_driver = network->isDriver(pin);
    const bool is_rising = ref->transition(sta_) == sta::RiseFall::rise();

    // Compute fanout (wire edges from this vertex)
    int node_fanout = 0;
    sta::VertexOutEdgeIterator iter(vertex, graph);
    while (iter.hasNext()) {
      sta::Edge* edge = iter.next();
      if (edge->isWire()) {
        const sta::Pin* to_pin = edge->to(graph)->pin();
        if (network->isTopLevelPort(to_pin)) {
          sta::Port* port = network->port(to_pin);
          node_fanout += sdc->portExtFanout(
                             port, dcalc_ap->corner(), sta::MinMax::max())
                         + 1;
        } else {
          node_fanout++;
        }
      }
    }

    // Load capacitance
    float cap = 0.0f;
    if (is_driver && !(!clock_expanded && (network->isCheckClk(pin) || !i))) {
      sta::GraphDelayCalc* gdc = sta_->graphDelayCalc();
      cap = gdc->loadCap(pin, ref->transition(sta_), dcalc_ap);
    }

    // Pin name via dbNetwork
    odb::dbITerm* term;
    odb::dbBTerm* port;
    odb::dbModITerm* moditerm;
    sta_->getDbNetwork()->staToDb(pin, term, port, moditerm);

    std::string pin_name;
    if (term) {
      pin_name = term->getName();
    } else if (port) {
      pin_name = port->getName();
    }

    // Arrival and slew
    float slew = 0.0f;
    if (!sta_->isIdealClock(pin)) {
      arrival_cur = ref->arrival();
      slew = ref->slew(sta_);
    }
    const float pin_delay = arrival_cur - arrival_prev;

    // Track logic depth for data path nodes
    if (!pin_is_clock) {
      sta::Instance* inst = network->instance(pin);
      if (inst) {
        sta::LibertyCell* lib_cell = network->libertyCell(inst);
        if (lib_cell && !lib_cell->isBuffer() && !lib_cell->isInverter()) {
          logic_insts.insert(inst);
        }
      }
      total_fanout += node_fanout;
    }

    nodes.push_back(TimingNode{pin_name,
                               node_fanout,
                               is_rising,
                               pin_is_clock,
                               (arrival_cur + offset) / time_scale,
                               pin_delay / time_scale,
                               slew / time_scale,
                               cap / cap_scale});

    arrival_prev = arrival_cur;
  }

  logic_depth = static_cast<int>(logic_insts.size());
}

std::vector<TimingPathSummary> TimingReport::getReport(bool is_setup,
                                                       int max_paths) const
{
  std::vector<TimingPathSummary> result;
  if (!sta_) {
    return result;
  }

  sta_->ensureGraph();
  sta_->searchPreamble();

  sta::Search* search = sta_->search();
  sta::PathEndSeq path_ends = search->findPathEnds(
      /*from*/ nullptr,
      /*thrus*/ nullptr,
      /*to*/ nullptr,
      /*unconstrained*/ false,
      /*corner*/ nullptr,
      is_setup ? sta::MinMaxAll::max() : sta::MinMaxAll::min(),
      /*group_count*/ max_paths,
      /*endpoint_count*/ 1,
      /*unique_pins*/ true,
      /*unique_edges*/ true,
      -sta::INF,
      sta::INF,
      /*sort_by_slack*/ true,
      /*group_names*/ nullptr,
      /*setup*/ is_setup,
      /*hold*/ !is_setup,
      /*recovery*/ false,
      /*removal*/ false,
      /*clk_gating_setup*/ false,
      /*clk_gating_hold*/ false);

  for (auto& path_end : path_ends) {
    TimingPathSummary summary;

    sta::Path* path = path_end->path();
    sta::DcalcAnalysisPt* dcalc_ap
        = path->pathAnalysisPt(sta_)->dcalcAnalysisPt();

    // Clocks
    auto* src_edge = path_end->sourceClkEdge(sta_);
    summary.start_clk = src_edge ? src_edge->clock()->name() : "<No clock>";
    auto* end_clk = path_end->targetClk(sta_);
    summary.end_clk = end_clk ? end_clk->name() : "<No clock>";

    // Path-level metrics (convert from internal units to user units)
    const float time_scale = sta_->units()->timeUnit()->scale();
    summary.slack = path_end->slack(sta_) / time_scale;
    summary.arrival = path_end->dataArrivalTime(sta_) / time_scale;
    summary.required = path_end->requiredTime(sta_) / time_scale;
    summary.skew = path_end->clkSkew(sta_) / time_scale;
    auto* pd = path_end->pathDelay();
    summary.path_delay = pd ? pd->delay() / time_scale : 0.0f;

    bool clock_propagated = src_edge && src_edge->clock()->isPropagated();

    // Expand data path
    expandPath(path,
               dcalc_ap,
               0.0f,
               clock_propagated,
               summary.data_nodes,
               summary.logic_depth,
               summary.fanout);

    // Expand capture path
    int capture_depth = 0;
    int capture_fanout = 0;
    expandPath(path_end->targetClkPath(),
               dcalc_ap,
               path_end->targetClkOffset(sta_),
               clock_propagated,
               summary.capture_nodes,
               capture_depth,
               capture_fanout);

    // Start/end pin names
    // Find first non-clock node in data path
    int clk_end = -1;
    for (int i = 0; i < static_cast<int>(summary.data_nodes.size()); i++) {
      if (!summary.data_nodes[i].is_clock) {
        clk_end = i - 1;
        break;
      }
    }
    if (clk_end < 0) {
      clk_end = static_cast<int>(summary.data_nodes.size()) - 1;
    }

    int start_idx = clk_end + 1;
    if (start_idx >= static_cast<int>(summary.data_nodes.size())) {
      start_idx = 0;
    }
    summary.start_pin = summary.data_nodes[start_idx].pin_name;
    if (!summary.data_nodes.empty()) {
      summary.end_pin = summary.data_nodes.back().pin_name;
    }

    result.push_back(std::move(summary));
  }

  return result;
}

}  // namespace web
