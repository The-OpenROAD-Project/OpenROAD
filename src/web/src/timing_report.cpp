// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "timing_report.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Clock.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathGroup.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/Sta.hh"
#include "sta/StringUtil.hh"
#include "sta/Units.hh"
#include "sta/VisitPathEnds.hh"

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
          node_fanout += sdc->portExtFanout(port, sta::MinMax::max()) + 1;
        } else {
          node_fanout++;
        }
      }
    }

    // Load capacitance
    float cap = 0.0f;
    if (is_driver && (clock_expanded || (!network->isCheckClk(pin) && i))) {
      sta::GraphDelayCalc* gdc = sta_->graphDelayCalc();
      cap = gdc->loadCap(
          pin, ref->transition(sta_), ref->scene(sta_), ref->minMax(sta_));
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

    nodes.push_back(TimingNode{.pin_name = std::move(pin_name),
                               .fanout = node_fanout,
                               .is_rising = is_rising,
                               .is_clock = pin_is_clock,
                               .time = (arrival_cur + offset) / time_scale,
                               .delay = pin_delay / time_scale,
                               .slew = slew / time_scale,
                               .load = cap / cap_scale});

    arrival_prev = arrival_cur;
  }

  logic_depth = static_cast<int>(logic_insts.size());
}

std::vector<TimingPathSummary> TimingReport::getReport(bool is_setup,
                                                       int max_paths,
                                                       float slack_min,
                                                       float slack_max) const
{
  std::vector<TimingPathSummary> result;
  if (!sta_) {
    return result;
  }

  sta_->ensureGraph();
  sta_->searchPreamble();

  // Convert user-unit slack bounds to internal units.
  const float time_scale = sta_->units()->timeUnit()->scale();
  const float sta_slack_min
      = (slack_min <= -1e30f) ? -sta::INF : slack_min * time_scale;
  const float sta_slack_max
      = (slack_max >= 1e30f) ? sta::INF : slack_max * time_scale;

  sta::Search* search = sta_->search();
  sta::SceneSeq scenes = sta_->scenes();
  sta::StringSeq group_names;
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
      sta_slack_min,
      sta_slack_max,
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

    if (!summary.data_nodes.empty()) {
      int start_idx = clk_end + 1;
      if (start_idx >= static_cast<int>(summary.data_nodes.size())) {
        start_idx = 0;
      }
      summary.start_pin = summary.data_nodes[start_idx].pin_name;
      if (!summary.data_nodes.empty()) {
        summary.end_pin = summary.data_nodes.back().pin_name;
      }
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

// Visitor that finds worst slack per endpoint within a specific path group,
// optionally filtered by clock.  Mirrors PathGroupSlackEndVisitor from
// staGuiInterface.cpp.
class PathGroupSlackVisitor : public sta::PathEndVisitor
{
 public:
  PathGroupSlackVisitor(const sta::PathGroup* group,
                        const sta::Clock* clk,
                        sta::Sta* sta)
      : group_(group), sta_(sta), clk_(clk)
  {
  }

  sta::PathEndVisitor* copy() const override
  {
    return new PathGroupSlackVisitor(*this);
  }

  void visit(sta::PathEnd* path_end) override
  {
    sta::PathGroupSeq groups = sta_->cmdScene()->mode()->pathGroups(path_end);
    if (std::ranges::find(groups, group_) == groups.end()) {
      return;
    }
    if (clk_) {
      const sta::Clock* end_clk = path_end->targetClk(sta_);
      if (end_clk != clk_) {
        return;
      }
    }
    float slack = path_end->slack(sta_);
    if (slack < worst_slack_) {
      worst_slack_ = slack;
      has_slack_ = true;
    }
  }

  float worstSlack() const { return worst_slack_; }
  bool hasSlack() const { return has_slack_; }
  void reset()
  {
    worst_slack_ = std::numeric_limits<float>::max();
    has_slack_ = false;
  }

 private:
  const sta::PathGroup* group_;
  sta::Sta* sta_;
  const sta::Clock* clk_;
  bool has_slack_ = false;
  float worst_slack_ = std::numeric_limits<float>::max();
};

// Ensure path groups exist so findPathGroup works.
static void ensurePathGroups(sta::dbSta* sta, bool is_setup)
{
  sta::StringSeq empty_names;
  for (sta::Mode* mode : sta->modes()) {
    mode->makePathGroups(1,
                         1,
                         false,
                         false,
                         -sta::INF,
                         sta::INF,
                         empty_names,
                         is_setup,
                         !is_setup,
                         false,
                         false,
                         false,
                         false,
                         false);
  }
}

// Collect per-endpoint slacks using the path group visitor pattern.
// Returns slacks in user units.
static void collectFilteredSlacks(sta::dbSta* sta,
                                  bool is_setup,
                                  const std::string& path_group,
                                  const std::string& clock_name,
                                  std::vector<float>& slacks,
                                  int& total_endpoints,
                                  int& unconstrained_count)
{
  ensurePathGroups(sta, is_setup);

  const sta::MinMax* min_max
      = is_setup ? sta::MinMax::max() : sta::MinMax::min();
  const float time_scale = sta->units()->timeUnit()->scale();

  // Find the clock pointer if a clock name was given.
  const sta::Clock* clk = nullptr;
  if (!clock_name.empty()) {
    for (sta::Clock* c : sta->cmdScene()->sdc()->clocks()) {
      if (clock_name == c->name()) {
        clk = c;
        break;
      }
    }
  }

  // Find the path group.
  sta::PathGroup* pg = nullptr;
  if (!path_group.empty()) {
    pg = sta->cmdMode()->pathGroups()->findPathGroup(path_group.c_str(),
                                                     min_max);
  } else if (clk) {
    pg = sta->cmdMode()->pathGroups()->findPathGroup(clk, min_max);
  }

  if (!pg) {
    return;
  }

  sta::VisitPathEnds visit_ends(sta);
  PathGroupSlackVisitor visitor(pg, clk, sta);

  for (sta::Vertex* vertex : sta->endpoints()) {
    total_endpoints++;
    visit_ends.visitPathEnds(vertex, &visitor);
    if (visitor.hasSlack()) {
      float slack = visitor.worstSlack();
      if (slack >= sta::INF || slack <= -sta::INF) {
        unconstrained_count++;
      } else {
        slacks.push_back(slack / time_scale);
      }
    } else {
      unconstrained_count++;
    }
    visitor.reset();
  }
}

// Helper: given a vector of slack values, bin them and populate result.
static void binSlacks(const std::vector<float>& slacks,
                      SlackHistogramResult& result)
{
  float min_slack = std::numeric_limits<float>::max();
  float max_slack = std::numeric_limits<float>::lowest();
  for (float s : slacks) {
    min_slack = std::min(min_slack, s);
    max_slack = std::max(max_slack, s);
  }

  // Extend range to include zero so negative/positive split is meaningful.
  min_slack = std::min(0.0f, min_slack);
  max_slack = std::max(0.0f, max_slack);

  int num_bins;
  float bin_min;
  float bin_width;

  if (min_slack == max_slack) {
    num_bins = 1;
    bin_min = min_slack - 0.1f;
    bin_width = 0.3f;
  } else {
    constexpr int kDefaultBuckets = 10;
    bin_width = snapBinInterval((max_slack - min_slack) / kDefaultBuckets);
    bin_min = std::floor(min_slack / bin_width) * bin_width;
    const float bin_max = std::ceil(max_slack / bin_width) * bin_width;
    num_bins = static_cast<int>(std::round((bin_max - bin_min) / bin_width));
    num_bins = std::max(num_bins, 1);
  }

  std::vector<int> counts(num_bins, 0);
  for (float s : slacks) {
    int idx = static_cast<int>((s - bin_min) / bin_width);
    idx = std::clamp(idx, 0, num_bins - 1);
    counts[idx]++;
  }

  result.bins.reserve(num_bins);
  for (int i = 0; i < num_bins; i++) {
    float lower = bin_min + i * bin_width;
    float upper = lower + bin_width;
    float center = (lower + upper) / 2.0f;
    result.bins.push_back({lower, upper, counts[i], center < 0});
  }
}

SlackHistogramResult TimingReport::getSlackHistogram(
    bool is_setup,
    const std::string& path_group,
    const std::string& clock_name) const
{
  SlackHistogramResult result;
  if (!sta_) {
    return result;
  }

  sta_->ensureGraph();
  sta_->searchPreamble();

  result.time_unit = sta_->units()->timeUnit()->scaleAbbrevSuffix();

  std::vector<float> slacks;

  if (!path_group.empty() || !clock_name.empty()) {
    // Filtered mode: use path group visitor pattern.
    collectFilteredSlacks(sta_,
                          is_setup,
                          path_group,
                          clock_name,
                          slacks,
                          result.total_endpoints,
                          result.unconstrained_count);
  } else {
    // Unfiltered mode: simple slack query per endpoint.
    const sta::MinMax* min_max
        = is_setup ? sta::MinMax::max() : sta::MinMax::min();
    sta::SceneSeq scenes = sta_->scenes();
    const float time_scale = sta_->units()->timeUnit()->scale();

    for (sta::Vertex* vertex : sta_->endpoints()) {
      result.total_endpoints++;
      const sta::Pin* pin = vertex->pin();
      float slack
          = sta_->slack(pin, sta::RiseFallBoth::riseFall(), scenes, min_max);

      if (slack >= sta::INF || slack <= -sta::INF) {
        result.unconstrained_count++;
        continue;
      }

      slacks.push_back(slack / time_scale);
    }
  }

  if (!slacks.empty()) {
    binSlacks(slacks, result);
  }

  return result;
}

ChartFilters TimingReport::getChartFilters() const
{
  ChartFilters filters;
  if (!sta_) {
    return filters;
  }

  // Path groups.
  sta::Sdc* sdc = sta_->cmdScene()->sdc();
  for (const auto& [name, group_paths] : sdc->groupPaths()) {
    filters.path_groups.emplace_back(name);
  }

  // Clocks.
  for (sta::Clock* clk : sdc->clocks()) {
    filters.clocks.emplace_back(clk->name());
  }

  return filters;
}

}  // namespace web
