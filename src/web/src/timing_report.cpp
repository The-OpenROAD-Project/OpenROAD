// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "timing_report.h"

#include <algorithm>
#include <cmath>
#include <limits>
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
#include "sta/Mode.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/Units.hh"

namespace web {

TimingReport::TimingReport(sta::dbSta* sta) : sta_(sta)
{
}

void TimingReport::expandPath(sta::Path* path,
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
  auto* mode = sta_->cmdMode();
  auto* sdc = mode->sdc();

  // Track logic instances for depth computation
  std::set<sta::Instance*> logic_insts;

  for (size_t i = 0; i < expand.size(); i++) {
    const auto* ref = expand.path(i);
    sta::Vertex* vertex = ref->vertex(sta_);
    const auto pin = vertex->pin();
    const bool pin_is_clock = sta_->isClock(pin, mode);
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
                             port, sta::MinMax::max())
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
      cap = gdc->loadCap(pin, ref->transition(sta_), ref->scene(sta_), ref->minMax(sta_));
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
    if (!sta_->isIdealClock(pin, mode)) {
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
  sta::SceneSeq scenes = sta_->scenes();
  sta::StdStringSeq group_names;
  sta::PathEndSeq path_ends = search->findPathEnds(
      /*from*/ nullptr,
      /*thrus*/ nullptr,
      /*to*/ nullptr,
      /*unconstrained*/ false,
      scenes,
      is_setup ? sta::MinMaxAll::max() : sta::MinMaxAll::min(),
      /*group_count*/ max_paths,
      /*endpoint_count*/ 1,
      /*unique_pins*/ true,
      /*unique_edges*/ true,
      -sta::INF,
      sta::INF,
      /*sort_by_slack*/ true,
      group_names,
      /*setup*/ is_setup,
      /*hold*/ !is_setup,
      /*recovery*/ false,
      /*removal*/ false,
      /*clk_gating_setup*/ false,
      /*clk_gating_hold*/ false);

  for (auto& path_end : path_ends) {
    TimingPathSummary summary;

    sta::Path* path = path_end->path();

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
               0.0f,
               clock_propagated,
               summary.data_nodes,
               summary.logic_depth,
               summary.fanout);

    // Expand capture path
    int capture_depth = 0;
    int capture_fanout = 0;
    expandPath(path_end->targetClkPath(),
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

// Snap an exact bin interval to a "nice" value (1, 2, 5, or 10 × 10^n).
// Matches the Qt GUI algorithm in chartsWidget.cpp:computeSnapBucketInterval().
static float snapBinInterval(float exact_interval)
{
  const int exp = static_cast<int>(std::floor(std::log10(exact_interval)));
  const double mag = std::pow(10.0, exp);
  const double residual = exact_interval / mag;

  double nice_coeff;
  if (residual < 1.5) {
    nice_coeff = 1.0;
  } else if (residual < 3.0) {
    nice_coeff = 2.0;
  } else if (residual < 7.0) {
    nice_coeff = 5.0;
  } else {
    nice_coeff = 10.0;
  }

  return static_cast<float>(nice_coeff * mag);
}

SlackHistogramResult TimingReport::getSlackHistogram(bool is_setup) const
{
  SlackHistogramResult result;
  if (!sta_) {
    return result;
  }

  sta_->ensureGraph();
  sta_->searchPreamble();

  const sta::MinMax* min_max
      = is_setup ? sta::MinMax::max() : sta::MinMax::min();
  sta::SceneSeq scenes = sta_->scenes();
  const float time_scale = sta_->units()->timeUnit()->scale();

  result.time_unit = sta_->units()->timeUnit()->scaleAbbrevSuffix();

  // Collect slack values for all constrained endpoints.
  std::vector<float> slacks;
  float min_slack = std::numeric_limits<float>::max();
  float max_slack = std::numeric_limits<float>::lowest();

  for (sta::Vertex* vertex : sta_->endpoints()) {
    result.total_endpoints++;
    const sta::Pin* pin = vertex->pin();
    float slack = sta_->slack(
        pin, sta::RiseFallBoth::riseFall(), scenes, min_max);

    // Skip unconstrained endpoints (infinite slack).
    if (slack >= sta::INF || slack <= -sta::INF) {
      result.unconstrained_count++;
      continue;
    }

    // Convert to user units.
    slack /= time_scale;
    slacks.push_back(slack);
    min_slack = std::min(min_slack, slack);
    max_slack = std::max(max_slack, slack);
  }

  if (slacks.empty()) {
    return result;
  }

  // Extend range to include zero so negative/positive split is meaningful.
  min_slack = std::min(0.0f, min_slack);
  max_slack = std::max(0.0f, max_slack);

  // Compute nice bin interval and boundaries.
  int num_bins;
  float bin_min;
  float bin_width;

  if (min_slack == max_slack) {
    // Degenerate case: all slacks identical.
    num_bins = 1;
    bin_min = min_slack - 0.1f;
    bin_width = 0.3f;
  } else {
    constexpr int kDefaultBuckets = 10;
    bin_width = snapBinInterval((max_slack - min_slack) / kDefaultBuckets);
    bin_min = std::floor(min_slack / bin_width) * bin_width;
    const float bin_max = std::ceil(max_slack / bin_width) * bin_width;
    num_bins = static_cast<int>(std::round((bin_max - bin_min) / bin_width));
    if (num_bins < 1) {
      num_bins = 1;
    }
  }

  // Count endpoints per bin.
  std::vector<int> counts(num_bins, 0);
  for (float s : slacks) {
    int idx = static_cast<int>((s - bin_min) / bin_width);
    if (idx < 0) {
      idx = 0;
    }
    if (idx >= num_bins) {
      idx = num_bins - 1;
    }
    counts[idx]++;
  }

  // Build result bins.
  result.bins.reserve(num_bins);
  for (int i = 0; i < num_bins; i++) {
    float lower = bin_min + i * bin_width;
    float upper = lower + bin_width;
    float center = (lower + upper) / 2.0f;
    result.bins.push_back({lower, upper, counts[i], center < 0});
  }

  return result;
}

}  // namespace web
