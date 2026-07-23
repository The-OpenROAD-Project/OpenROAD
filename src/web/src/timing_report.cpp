// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "timing_report.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/json/array.hpp"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
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
    pg = sta->cmdMode()->pathGroups()->findPathGroup(path_group, min_max);
  } else if (clk) {
    pg = sta->cmdMode()->pathGroups()->findPathGroup(clk, min_max);
  }

  if (!pg) {
    return;
  }

  sta::VisitPathEnds visit_ends(sta);
  PathGroupSlackVisitor visitor(pg, clk, sta);

  for (sta::Vertex* vertex : sta->endpoints()) {
    visit_ends.visitPathEnds(vertex, &visitor);
    // Count only endpoints that belong to this path group (hasSlack() is false
    // for out-of-group endpoints).  This keeps total_endpoints/unconstrained
    // correct when getSlackHistogram calls this once per group for a stacked
    // histogram — path groups are disjoint, so per-group counts sum to the
    // distinct-endpoint total instead of inflating by the number of groups.
    if (visitor.hasSlack()) {
      total_endpoints++;
      float slack = visitor.worstSlack();
      if (slack >= sta::INF || slack <= -sta::INF) {
        unconstrained_count++;
      } else {
        slacks.push_back(slack / time_scale);
      }
    }
    visitor.reset();
  }
}

// Shared bin geometry so an aggregate histogram and its per-group series use
// identical edges.
struct SlackBinEdges
{
  float bin_min;
  float bin_width;
  int num_bins;
};

static SlackBinEdges computeSlackBinEdges(const std::vector<float>& slacks)
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

  if (min_slack == max_slack) {
    return {min_slack - 0.1f, 0.3f, 1};
  }
  constexpr int kDefaultBuckets = 10;
  const float bin_width
      = snapBinInterval((max_slack - min_slack) / kDefaultBuckets);
  const float bin_min = std::floor(min_slack / bin_width) * bin_width;
  const float bin_max = std::ceil(max_slack / bin_width) * bin_width;
  int num_bins = static_cast<int>(std::round((bin_max - bin_min) / bin_width));
  num_bins = std::max(num_bins, 1);
  return {bin_min, bin_width, num_bins};
}

// Count `values` into the given bin edges (parallel to the edges' bins).
static std::vector<int> binIntoEdges(const std::vector<float>& values,
                                     const SlackBinEdges& edges)
{
  std::vector<int> counts(edges.num_bins, 0);
  for (float s : values) {
    int idx = static_cast<int>((s - edges.bin_min) / edges.bin_width);
    idx = std::clamp(idx, 0, edges.num_bins - 1);
    counts[idx]++;
  }
  return counts;
}

// Collect per-endpoint slacks (user units) for the unfiltered case.
static void collectUnfilteredSlacks(sta::dbSta* sta,
                                    bool is_setup,
                                    std::vector<float>& slacks,
                                    int& total_endpoints,
                                    int& unconstrained_count)
{
  const sta::MinMax* min_max
      = is_setup ? sta::MinMax::max() : sta::MinMax::min();
  sta::SceneSeq scenes = sta->scenes();
  const float time_scale = sta->units()->timeUnit()->scale();
  for (sta::Vertex* vertex : sta->endpoints()) {
    total_endpoints++;
    const sta::Pin* pin = vertex->pin();
    const float slack
        = sta->slack(pin, sta::RiseFallBoth::riseFall(), scenes, min_max);
    if (slack >= sta::INF || slack <= -sta::INF) {
      unconstrained_count++;
      continue;
    }
    slacks.push_back(slack / time_scale);
  }
}

SlackHistogramResult TimingReport::getSlackHistogram(
    bool is_setup,
    const std::string& path_group,
    const std::string& clock_name,
    const std::vector<std::string>& path_groups) const
{
  SlackHistogramResult result;
  if (!sta_) {
    return result;
  }

  sta_->ensureGraph();
  sta_->searchPreamble();
  result.time_unit = sta_->units()->timeUnit()->scaleAbbrevSuffix();

  // Build (group_name, slacks) entries.  Multiple entries → stacked series.
  std::vector<std::pair<std::string, std::vector<float>>> group_slacks;
  if (!path_groups.empty()) {
    for (const std::string& group : path_groups) {
      std::vector<float> slacks;
      collectFilteredSlacks(sta_,
                            is_setup,
                            group,
                            clock_name,
                            slacks,
                            result.total_endpoints,
                            result.unconstrained_count);
      group_slacks.emplace_back(group, std::move(slacks));
    }
  } else if (!path_group.empty() || !clock_name.empty()) {
    std::vector<float> slacks;
    collectFilteredSlacks(sta_,
                          is_setup,
                          path_group,
                          clock_name,
                          slacks,
                          result.total_endpoints,
                          result.unconstrained_count);
    group_slacks.emplace_back(path_group, std::move(slacks));
  } else {
    std::vector<float> slacks;
    collectUnfilteredSlacks(sta_,
                            is_setup,
                            slacks,
                            result.total_endpoints,
                            result.unconstrained_count);
    group_slacks.emplace_back("", std::move(slacks));
  }

  // Union of all slacks defines the shared bin edges.
  std::vector<float> all_slacks;
  for (const auto& [name, slacks] : group_slacks) {
    all_slacks.insert(all_slacks.end(), slacks.begin(), slacks.end());
  }
  if (all_slacks.empty()) {
    return result;
  }

  const SlackBinEdges edges = computeSlackBinEdges(all_slacks);
  const std::vector<int> counts = binIntoEdges(all_slacks, edges);
  result.bins.reserve(edges.num_bins);
  for (int i = 0; i < edges.num_bins; i++) {
    const float lower = edges.bin_min + i * edges.bin_width;
    const float upper = lower + edges.bin_width;
    const float center = (lower + upper) / 2.0f;
    result.bins.push_back({lower, upper, counts[i], center < 0});
  }

  // Per-group stacked series (only meaningful for >1 group).
  if (group_slacks.size() > 1) {
    result.series.reserve(group_slacks.size());
    for (const auto& [name, slacks] : group_slacks) {
      result.series.push_back({name, binIntoEdges(slacks, edges)});
    }
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

// ── Timing cone ──

namespace {

// Walk the STA timing graph from `source_pin`, one logic level at a time, over
// the pins reachable in the given direction (`seed_pins` = findFanin/FanoutPins
// result).  Returns depth (>0) → pins.  Register clock pins are dropped, as in
// the Qt GUI.  `max_depth` (>0) caps the number of levels; 0 = unlimited.
std::map<int, std::set<const sta::Pin*>> walkConeLevels(
    sta::dbSta* sta,
    const sta::Pin* source_pin,
    sta::PinSet seed_pins,
    bool is_fanin,
    int max_depth)
{
  auto* network = sta->getDbNetwork();
  auto* graph = sta->graph();

  seed_pins.erase(source_pin);

  std::map<int, std::set<const sta::Pin*>> levels;
  levels[0].insert(source_pin);

  int level = 0;
  int remaining = -1;
  while (!seed_pins.empty()
         && remaining != static_cast<int>(seed_pins.size())) {
    if (max_depth > 0 && level >= max_depth) {
      break;
    }
    remaining = static_cast<int>(seed_pins.size());
    const int next_level = level + 1;

    auto& current = levels[level];
    auto& next = levels[next_level];

    for (const auto* pin : current) {
      // Input pins only have a load vertex; output pins only a driver vertex.
      // Fall back to pinLoadVertex so the walk expands through input pins
      // instead of stopping one level in.
      auto* pin_vertex = graph->pinDrvrVertex(pin);
      if (pin_vertex == nullptr) {
        pin_vertex = graph->pinLoadVertex(pin);
      }
      if (pin_vertex == nullptr) {
        continue;
      }

      std::unique_ptr<sta::VertexEdgeIterator> itr;
      if (is_fanin) {
        itr = std::make_unique<sta::VertexInEdgeIterator>(pin_vertex, graph);
      } else {
        itr = std::make_unique<sta::VertexOutEdgeIterator>(pin_vertex, graph);
      }

      while (itr->hasNext()) {
        auto* edge = itr->next();
        sta::Vertex* next_vertex = edge->to(graph);
        if (next_vertex == pin_vertex) {
          next_vertex = edge->from(graph);
        }
        const auto* next_pin = next_vertex->pin();
        auto found = seed_pins.find(next_pin);
        if (found != seed_pins.end()) {
          if (!network->isRegClkPin(next_pin)) {
            next.insert(next_pin);
          }
          seed_pins.erase(found);
        }
      }
    }

    level = next_level;
  }

  levels.erase(0);  // source is added separately by the caller
  return levels;
}

}  // namespace

TimingConeResult TimingReport::computeTimingCone(const std::string& pin_name,
                                                 bool fanin,
                                                 bool fanout,
                                                 int fanin_depth,
                                                 int fanout_depth) const
{
  TimingConeResult result;
  if (!sta_) {
    result.error = "no STA";
    return result;
  }
  if (!fanin && !fanout) {
    result.ok = true;  // nothing requested — an empty cone is valid (clear)
    return result;
  }

  auto* network = sta_->getDbNetwork();
  odb::dbBlock* block = network->block();
  if (block == nullptr) {
    result.error = "no design";
    return result;
  }

  // Resolve the pin (ITerm "inst/pin" or top-level BTerm).
  odb::dbITerm* src_iterm = block->findITerm(pin_name.c_str());
  odb::dbBTerm* src_bterm
      = src_iterm ? nullptr : block->findBTerm(pin_name.c_str());
  if (src_iterm == nullptr && src_bterm == nullptr) {
    result.error = "pin not found: " + pin_name;
    return result;
  }
  if ((src_iterm && src_iterm->getSigType().isSupply())
      || (src_bterm && src_bterm->getSigType().isSupply())) {
    result.error = "cannot build a timing cone from a supply pin";
    return result;
  }

  const sta::Pin* source_pin
      = src_iterm ? network->dbToSta(src_iterm) : network->dbToSta(src_bterm);
  if (source_pin == nullptr) {
    result.error = "pin has no timing graph vertex";
    return result;
  }

  sta_->ensureGraph();
  sta_->searchPreamble();
  auto* graph = sta_->graph();
  const sta::Mode* mode = sta_->cmdMode();

  // depth (signed) → pins.  Fanin levels are negative, fanout positive.
  std::map<int, std::set<const sta::Pin*>> depth_map;
  depth_map[0].insert(source_pin);

  if (fanin) {
    sta::PinSeq seed;
    seed.push_back(source_pin);
    sta::PinSet pins = sta_->findFaninPins(&seed,
                                           true,   // flat
                                           false,  // startpoints_only
                                           0,
                                           0,
                                           true,  // thru_disabled
                                           true,  // thru_constants
                                           mode);
    for (auto& [level, pin_list] :
         walkConeLevels(sta_, source_pin, std::move(pins), true, fanin_depth)) {
      depth_map[-level].insert(pin_list.begin(), pin_list.end());
    }
  }
  if (fanout) {
    sta::PinSeq seed;
    seed.push_back(source_pin);
    sta::PinSet pins = sta_->findFanoutPins(&seed,
                                            true,   // flat
                                            false,  // endpoints_only
                                            0,
                                            0,
                                            true,  // thru_disabled
                                            true,  // thru_constants
                                            mode);
    for (auto& [level, pin_list] : walkConeLevels(
             sta_, source_pin, std::move(pins), false, fanout_depth)) {
      depth_map[level].insert(pin_list.begin(), pin_list.end());
    }
  }

  // Materialize nodes and remember, per sta::Pin, its node index so we can wire
  // up flight-line source→sink pairs afterwards.
  std::map<const sta::Pin*, int> pin_to_index;
  const float time_scale = sta_->units()->timeUnit()->scale();
  float min_slack = std::numeric_limits<float>::max();
  float max_slack = std::numeric_limits<float>::lowest();

  for (const auto& [depth, pins] : depth_map) {
    for (const auto* pin : pins) {
      odb::dbITerm* iterm = nullptr;
      odb::dbBTerm* bterm = nullptr;
      odb::dbModITerm* moditerm = nullptr;
      network->staToDb(pin, iterm, bterm, moditerm);
      if (iterm == nullptr && bterm == nullptr) {
        continue;  // modITerm / hierarchical pin — not drawable on the layout
      }

      TimingConeNode node;
      node.iterm = iterm;
      node.bterm = bterm;
      node.inst = iterm ? iterm->getInst() : nullptr;
      node.depth = depth;

      // Input pins have only a load vertex — fall back to it so slack is
      // annotated for the whole cone, not just driver pins.
      sta::Vertex* vertex = graph->pinDrvrVertex(pin);
      if (vertex == nullptr) {
        vertex = graph->pinLoadVertex(pin);
      }
      if (vertex != nullptr) {
        const sta::Slack slack = sta_->slack(vertex, sta::MinMax::max());
        if (!sta::delayInf(slack, sta_)) {
          node.slack = sta::delayAsFloat(slack) / time_scale;
          node.has_slack = true;
          min_slack = std::min(min_slack, node.slack);
          max_slack = std::max(max_slack, node.slack);
        }
      }

      pin_to_index[pin] = static_cast<int>(result.nodes.size());
      result.nodes.push_back(std::move(node));
    }
  }

  // Pair each node at level L with nodes at level L+1 it drives (matches the Qt
  // GUI's buildConeConnectivity): the flight line runs source(L) → sink(L+1).
  // Iterate the depth_map in order so L / L+1 are adjacent.
  auto level_it = depth_map.begin();
  for (; level_it != depth_map.end(); ++level_it) {
    const int level = level_it->first;
    auto next_level_it = depth_map.find(level + 1);
    if (next_level_it == depth_map.end()) {
      continue;
    }

    // Map both driver and load vertices of the next level's pins to node index.
    std::map<sta::Vertex*, int> next_vertices;
    for (const auto* next_pin : next_level_it->second) {
      auto idx_it = pin_to_index.find(next_pin);
      if (idx_it == pin_to_index.end()) {
        continue;
      }
      next_vertices[graph->pinDrvrVertex(next_pin)] = idx_it->second;
      next_vertices[graph->pinLoadVertex(next_pin)] = idx_it->second;
    }

    for (const auto* src_pin : level_it->second) {
      auto src_idx_it = pin_to_index.find(src_pin);
      if (src_idx_it == pin_to_index.end()) {
        continue;
      }
      sta::Vertex* drvr = graph->pinDrvrVertex(src_pin);
      if (drvr == nullptr) {
        continue;
      }
      sta::VertexOutEdgeIterator fanout_iter(drvr, graph);
      while (fanout_iter.hasNext()) {
        sta::Edge* edge = fanout_iter.next();
        auto found = next_vertices.find(edge->to(graph));
        if (found != next_vertices.end()) {
          result.nodes[found->second].source_indices.push_back(
              src_idx_it->second);
        }
      }
    }
  }

  result.constrained = max_slack >= min_slack;
  if (result.constrained) {
    result.min_slack = min_slack;
    result.max_slack = max_slack;
  }
  auto* time_unit = sta_->units()->timeUnit();
  result.time_unit = std::string(time_unit->scaleAbbreviation())
                     + std::string(time_unit->suffix());
  result.ok = true;
  return result;
}

// ── JSON serialization helpers ──

boost::json::object serializeTimingNode(const TimingNode& n)
{
  boost::json::object o;
  o["pin"] = n.pin_name;
  o["fanout"] = n.fanout;
  o["rise"] = n.is_rising;
  o["clk"] = n.is_clock;
  o["time"] = n.time;
  o["delay"] = n.delay;
  o["slew"] = n.slew;
  o["load"] = n.load;
  return o;
}

boost::json::object serializeTimingPath(const TimingPathSummary& p)
{
  boost::json::object o;
  o["start_clk"] = p.start_clk;
  o["end_clk"] = p.end_clk;
  o["required"] = p.required;
  o["arrival"] = p.arrival;
  o["slack"] = p.slack;
  o["skew"] = p.skew;
  o["path_delay"] = p.path_delay;
  o["logic_depth"] = p.logic_depth;
  o["fanout"] = p.fanout;
  o["start_pin"] = p.start_pin;
  o["end_pin"] = p.end_pin;
  boost::json::array data;
  data.reserve(p.data_nodes.size());
  for (const auto& n : p.data_nodes) {
    data.emplace_back(serializeTimingNode(n));
  }
  o["data_nodes"] = std::move(data);
  boost::json::array capture;
  capture.reserve(p.capture_nodes.size());
  for (const auto& n : p.capture_nodes) {
    capture.emplace_back(serializeTimingNode(n));
  }
  o["capture_nodes"] = std::move(capture);
  return o;
}

boost::json::object serializeTimingPaths(
    const std::vector<TimingPathSummary>& paths)
{
  boost::json::object o;
  boost::json::array arr;
  arr.reserve(paths.size());
  for (const auto& p : paths) {
    arr.emplace_back(serializeTimingPath(p));
  }
  o["paths"] = std::move(arr);
  return o;
}

boost::json::object serializeSlackHistogram(const SlackHistogramResult& h)
{
  boost::json::object o;
  boost::json::array bins;
  bins.reserve(h.bins.size());
  for (const auto& bin : h.bins) {
    boost::json::object b;
    b["lower"] = bin.lower;
    b["upper"] = bin.upper;
    b["count"] = bin.count;
    b["negative"] = bin.is_negative;
    bins.emplace_back(std::move(b));
  }
  o["bins"] = std::move(bins);
  o["unconstrained_count"] = h.unconstrained_count;
  o["total_endpoints"] = h.total_endpoints;
  o["time_unit"] = h.time_unit;
  if (!h.series.empty()) {
    boost::json::array series;
    series.reserve(h.series.size());
    for (const auto& s : h.series) {
      boost::json::object so;
      so["name"] = s.name;
      boost::json::array counts;
      counts.reserve(s.counts.size());
      for (int c : s.counts) {
        counts.emplace_back(c);
      }
      so["counts"] = std::move(counts);
      series.emplace_back(std::move(so));
    }
    o["series"] = std::move(series);
  }
  return o;
}

boost::json::object serializeChartFilters(const ChartFilters& f)
{
  boost::json::object o;
  boost::json::array groups;
  groups.reserve(f.path_groups.size());
  for (const auto& name : f.path_groups) {
    groups.emplace_back(name);
  }
  o["path_groups"] = std::move(groups);
  boost::json::array clocks;
  clocks.reserve(f.clocks.size());
  for (const auto& name : f.clocks) {
    clocks.emplace_back(name);
  }
  o["clocks"] = std::move(clocks);
  return o;
}

// ── Net fanout histogram ──

FanoutHistogramResult computeFanoutHistogram(odb::dbBlock* block)
{
  FanoutHistogramResult result;
  if (!block) {
    return result;
  }

  std::vector<int> fanouts;
  int max_fanout = 0;
  for (odb::dbNet* net : block->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }
    const int term_count = static_cast<int>(net->getITermCount())
                           + static_cast<int>(net->getBTermCount());
    const int fanout = std::max(0, term_count - 1);
    fanouts.push_back(fanout);
    max_fanout = std::max(max_fanout, fanout);
  }
  result.total_nets = static_cast<int>(fanouts.size());
  if (fanouts.empty()) {
    return result;
  }

  int bin_width;
  if (max_fanout <= 20) {
    bin_width = 1;
  } else {
    constexpr int kDefaultBuckets = 10;
    const float exact
        = static_cast<float>(max_fanout) / static_cast<float>(kDefaultBuckets);
    bin_width = std::max(1, static_cast<int>(snapBinInterval(exact)));
  }
  const int num_bins = std::max(1, max_fanout / bin_width + 1);

  std::vector<int> counts(num_bins, 0);
  for (int f : fanouts) {
    int idx = f / bin_width;
    idx = std::clamp(idx, 0, num_bins - 1);
    counts[idx]++;
  }
  result.bins.reserve(num_bins);
  for (int i = 0; i < num_bins; i++) {
    const int lo = i * bin_width;
    const int hi = lo + bin_width;
    result.bins.push_back({lo, hi, counts[i]});
  }
  return result;
}

boost::json::object serializeFanoutHistogram(const FanoutHistogramResult& h)
{
  boost::json::object o;
  boost::json::array bins;
  bins.reserve(h.bins.size());
  for (const auto& bin : h.bins) {
    boost::json::object b;
    b["lower"] = bin.lower;
    b["upper"] = bin.upper;
    b["count"] = bin.count;
    bins.emplace_back(std::move(b));
  }
  o["bins"] = std::move(bins);
  o["total_nets"] = h.total_nets;
  return o;
}

// ── Net-length (HPWL) histogram ──

int netHpwlDbu(odb::dbNet* net)
{
  if (!net) {
    return 0;
  }
  const odb::Rect bbox = net->getTermBBox();
  if (bbox.dx() < 0 || bbox.dy() < 0) {
    return 0;  // no terminals with valid locations
  }
  return bbox.dx() + bbox.dy();
}

NetLengthHistogramResult computeNetLengthHistogram(odb::dbBlock* block,
                                                   bool use_dbu)
{
  NetLengthHistogramResult result;
  if (!block) {
    return result;
  }
  const double dbu_per_micron = std::max(1, block->getDbUnitsPerMicron());
  const double scale = use_dbu ? 1.0 : (1.0 / dbu_per_micron);
  result.length_unit = use_dbu ? "DBU" : "µm";  // µm

  std::vector<float> lengths;
  float max_len = 0.0f;
  for (odb::dbNet* net : block->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }
    const float len = static_cast<float>(netHpwlDbu(net) * scale);
    lengths.push_back(len);
    max_len = std::max(max_len, len);
  }
  result.total_nets = static_cast<int>(lengths.size());
  if (lengths.empty() || max_len <= 0.0f) {
    return result;
  }

  constexpr int kDefaultBuckets = 10;
  const float bin_width = snapBinInterval(max_len / kDefaultBuckets);
  const float bin_max = std::ceil(max_len / bin_width) * bin_width;
  const int num_bins
      = std::max(1, static_cast<int>(std::round(bin_max / bin_width)));

  std::vector<int> counts(num_bins, 0);
  for (float len : lengths) {
    int idx = static_cast<int>(len / bin_width);
    idx = std::clamp(idx, 0, num_bins - 1);
    counts[idx]++;
  }
  result.bins.reserve(num_bins);
  for (int i = 0; i < num_bins; i++) {
    const float lower = i * bin_width;
    const float upper = lower + bin_width;
    result.bins.push_back({lower, upper, counts[i]});
  }
  return result;
}

boost::json::object serializeNetLengthHistogram(
    const NetLengthHistogramResult& h)
{
  boost::json::object o;
  boost::json::array bins;
  bins.reserve(h.bins.size());
  for (const auto& bin : h.bins) {
    boost::json::object b;
    b["lower"] = bin.lower;
    b["upper"] = bin.upper;
    b["count"] = bin.count;
    bins.emplace_back(std::move(b));
  }
  o["bins"] = std::move(bins);
  o["total_nets"] = h.total_nets;
  o["length_unit"] = h.length_unit;
  return o;
}

}  // namespace web
