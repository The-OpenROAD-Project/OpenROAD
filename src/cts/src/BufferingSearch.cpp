// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "BufferingSearch.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace cts {
namespace {
using Arrivals = std::vector<std::pair<double, double>>;
struct Decision;
using DecisionPtr = std::shared_ptr<const Decision>;
struct Children;
using ChildrenPtr = std::shared_ptr<Children>;
struct Children
{
  DecisionPtr child;
  ChildrenPtr previous;
  ~Children()
  {
    // A wide net must not recursively destroy one list node per sink.
    while (previous && previous.use_count() == 1) {
      auto next = std::move(previous->previous);
      previous = std::move(next);
    }
  }
};
struct Decision
{
  size_t index;
  BufferingCellChoice cell;
  ChildrenPtr children;
};

struct State
{
  double pin_cap = 0;
  double wire_cap = 0;
  double length = 0;
  size_t fanout = 0;
  Arrivals arrivals;
  size_t depth = 0;
  size_t buffers = 0;
  double area = 0;
  size_t order = 0;
  ChildrenPtr children;
  DecisionPtr tree;
};

double latest(const Arrivals& arrivals)
{
  double result = 0;
  for (const auto& [early, late] : arrivals) {
    result = std::max(result, late);
  }
  return result;
}

double spread(const Arrivals& arrivals)
{
  double result = 0;
  for (const auto& [early, late] : arrivals) {
    result = std::max(result, late - early);
  }
  return result;
}

auto rank(const State& state)
{
  return std::tuple(latest(state.arrivals),
                    spread(state.arrivals),
                    state.depth,
                    state.buffers,
                    state.area,
                    state.pin_cap + state.wire_cap,
                    state.length,
                    state.order);
}

auto rank(const BufferingPlan& plan)
{
  return std::tuple(
      plan.delay(), plan.skew(), plan.depth, plan.buffers, plan.area);
}

double distance(BufferingPoint a, BufferingPoint b)
{
  return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

std::vector<size_t> postorder(const std::vector<BufferingNode>& nodes,
                              size_t root,
                              const BufferingSearchModel& model)
{
  if (nodes.empty() || root >= nodes.size() || model.dimensions == 0
      || model.candidate_limit == 0 || model.candidate_limit > 64
      || !std::isfinite(model.wire_cap) || model.wire_cap < 0
      || !std::isfinite(model.wire_res) || model.wire_res < 0
      || !std::isfinite(model.max_wire_length) || model.max_wire_length <= 0) {
    throw std::invalid_argument("Invalid buffering search model or root");
  }
  std::vector<size_t> order;
  std::vector<bool> visited(nodes.size(), false);
  std::vector<std::pair<size_t, size_t>> pending{{root, 0}};
  while (!pending.empty()) {
    const auto [index, depth] = pending.back();
    pending.pop_back();
    if (index >= nodes.size() || visited[index] || depth >= 64) {
      throw std::invalid_argument("Buffering search requires a bounded tree");
    }
    visited[index] = true;
    const auto& node = nodes[index];
    if (!std::isfinite(node.point.x) || !std::isfinite(node.point.y)
        || !std::isfinite(node.input_cap) || node.input_cap < 0
        || !std::isfinite(node.nominal_input_cap) || node.nominal_input_cap < 0
        || (node.sink != node.children.empty())) {
      throw std::invalid_argument("Invalid buffering search site");
    }
    order.push_back(index);
    for (const auto child : node.children) {
      pending.emplace_back(child, depth + 1);
    }
  }
  if (order.size() != nodes.size()) {
    throw std::invalid_argument("Unconnected buffering search site");
  }
  std::ranges::reverse(order);
  return order;
}

State emptyState(size_t dimensions)
{
  State state;
  state.arrivals.resize(dimensions,
                        {std::numeric_limits<double>::infinity(),
                         -std::numeric_limits<double>::infinity()});
  return state;
}

State leaf(const BufferingNode& node, size_t index, size_t dimensions)
{
  State state;
  state.pin_cap = node.input_cap;
  state.fanout = 1;
  state.arrivals = node.sink_arrivals;
  if (state.arrivals.empty()) {
    state.arrivals.resize(dimensions, {0, 0});
  }
  if (state.arrivals.size() != dimensions) {
    throw std::invalid_argument("Missing sink arrival dimensions");
  }
  for (const auto& [early, late] : state.arrivals) {
    if (!std::isfinite(early) || !std::isfinite(late) || early < 0
        || late < early) {
      throw std::invalid_argument("Invalid sink arrival range");
    }
  }
  state.tree = std::make_shared<Decision>(Decision{index, {}, {}});
  return state;
}

State merge(State merged,
            const State& child,
            double length,
            const BufferingSearchModel& model)
{
  const double wire_cap = length * model.wire_cap;
  // Each edge sees the complete downstream *unbuffered* capacitance. Shared
  // edges are counted once; bypassing a junction never straightens its path.
  const double wire_delay = model.wire_res * length
                            * (child.pin_cap + child.wire_cap + wire_cap / 2);
  merged.pin_cap += child.pin_cap;
  merged.wire_cap += child.wire_cap + wire_cap;
  merged.length = std::max(merged.length, length + child.length);
  merged.fanout += child.fanout;
  merged.depth = std::max(merged.depth, child.depth);
  merged.buffers += child.buffers;
  merged.area += child.area;
  for (size_t dim = 0; dim < model.dimensions; ++dim) {
    merged.arrivals[dim].first = std::min(
        merged.arrivals[dim].first, child.arrivals[dim].first + wire_delay);
    merged.arrivals[dim].second = std::max(
        merged.arrivals[dim].second, child.arrivals[dim].second + wire_delay);
  }
  merged.children
      = std::make_shared<Children>(Children{child.tree, merged.children});
  return merged;
}

void validateChoice(const BufferingCellChoice& cell, size_t dimensions)
{
  if (cell.buffer.master.empty() || !std::isfinite(cell.buffer.input_cap)
      || cell.buffer.input_cap < 0 || !std::isfinite(cell.nominal_input_cap)
      || cell.nominal_input_cap < 0 || !std::isfinite(cell.area)
      || cell.area < 0 || cell.delays.size() != dimensions
      || std::ranges::any_of(
          cell.delays, [](double d) { return !std::isfinite(d) || d < 0; })) {
    throw std::invalid_argument("Invalid buffering timing choice");
  }
}

State buffered(State state, size_t index, const BufferingCellChoice& cell)
{
  validateChoice(cell, state.arrivals.size());
  state.pin_cap = cell.buffer.input_cap;
  state.wire_cap = state.length = 0;
  state.fanout = 1;
  ++state.depth;
  ++state.buffers;
  state.area += cell.area;
  for (size_t dim = 0; dim < state.arrivals.size(); ++dim) {
    state.arrivals[dim].first += cell.delays[dim];
    state.arrivals[dim].second += cell.delays[dim];
  }
  state.tree
      = std::make_shared<Decision>(Decision{index, cell, state.children});
  return state;
}

// Keep delay-, capacitance-, and skew-favouring alternatives. These fixed
// quotas bound the heuristic; the complete legacy reference is never here.
void prune(std::vector<State>& states, size_t limit)
{
  if (states.size() <= limit) {
    return;
  }
  std::vector<size_t> indices(states.size());
  std::iota(indices.begin(), indices.end(), 0);
  std::vector<size_t> kept;
  auto keep = [&](auto key, size_t count) {
    std::ranges::stable_sort(indices, [&](size_t a, size_t b) {
      return key(states[a]) < key(states[b]);
    });
    for (const auto index : indices) {
      if (std::ranges::find(kept, index) == kept.end()) {
        kept.push_back(index);
        if (--count == 0 || kept.size() == limit) {
          break;
        }
      }
    }
  };
  keep([](const State& s) { return rank(s); }, (limit + 1) / 2);
  if (kept.size() < limit) {
    keep(
        [](const State& s) {
          return std::tuple(s.pin_cap + s.wire_cap, rank(s));
        },
        std::max<size_t>(1, limit / 4));
  }
  if (kept.size() < limit) {
    keep([](const State& s) { return std::tuple(spread(s.arrivals), rank(s)); },
         limit - kept.size());
  }
  std::vector<State> result;
  for (const auto index : kept) {
    result.push_back(std::move(states[index]));
  }
  states = std::move(result);
}

std::vector<size_t> frontier(const std::vector<BufferingNode>& nodes,
                             size_t index)
{
  std::vector<size_t> result;
  std::vector<size_t> pending(nodes[index].children.rbegin(),
                              nodes[index].children.rend());
  while (!pending.empty()) {
    const auto child = pending.back();
    pending.pop_back();
    if (nodes[child].sink || !nodes[child].master.empty()) {
      result.push_back(child);
    } else {
      pending.insert(pending.end(),
                     nodes[child].children.rbegin(),
                     nodes[child].children.rend());
    }
  }
  return result;
}

std::vector<BufferingCellChoice> dummyLoads(
    const std::vector<BufferingNode>& nodes,
    const BufferingDummy& select)
{
  std::vector<BufferingCellChoice> loads(nodes.size());
  if (!select) {
    return loads;
  }
  std::vector<float> actual(nodes.size(), 0);
  std::vector<std::vector<size_t>> fanouts(nodes.size());
  for (size_t index = 0; index < nodes.size(); ++index) {
    if (!nodes[index].sink && !nodes[index].master.empty()) {
      fanouts[index] = frontier(nodes, index);
      for (const auto child : fanouts[index]) {
        actual[index] += static_cast<float>(nodes[child].nominal_input_cap);
      }
    }
  }
  for (const auto& children : fanouts) {
    float ideal = 0;
    for (const auto child : children) {
      if (!nodes[child].sink) {
        ideal = std::max(ideal, actual[child]);
      }
    }
    for (const auto child : children) {
      if (!nodes[child].sink) {
        if (auto load = select(actual[child], ideal, fanouts[child].size())) {
          if (load->buffer.master.empty()
              || !std::isfinite(load->buffer.input_cap)
              || load->buffer.input_cap < 0
              || !std::isfinite(load->nominal_input_cap)
              || load->nominal_input_cap < 0 || !std::isfinite(load->area)
              || load->area < 0) {
            throw std::invalid_argument("Invalid buffering dummy load");
          }
          loads[child] = std::move(*load);
        }
      }
    }
  }
  return loads;
}

BufferingPlan materialize(const std::vector<BufferingNode>& sites,
                          const State& state)
{
  BufferingPlan plan{
      sites, state.arrivals, state.depth, state.buffers, state.area};
  std::vector<DecisionPtr> pending{state.tree};
  while (!pending.empty()) {
    const auto decision = pending.back();
    pending.pop_back();
    auto& node = plan.nodes[decision->index];
    if (!node.sink) {
      node.master = decision->cell.buffer.master;
      node.input_cap = decision->cell.buffer.input_cap;
      node.nominal_input_cap = decision->cell.nominal_input_cap;
    }
    for (auto children = decision->children; children;
         children = children->previous) {
      pending.push_back(children->child);
    }
  }
  return plan;
}

bool noWorse(const BufferingPlan& plan, const BufferingPlan& reference)
{
  for (size_t dim = 0; dim < plan.arrivals.size(); ++dim) {
    const auto [early, late] = plan.arrivals[dim];
    const auto [ref_early, ref_late] = reference.arrivals[dim];
    if (late > ref_late || late - early > ref_late - ref_early) {
      return false;
    }
  }
  return true;
}
}  // namespace

double BufferingPlan::delay() const
{
  return latest(arrivals);
}
double BufferingPlan::skew() const
{
  return spread(arrivals);
}

std::optional<BufferingPlan> evaluateBuffering(
    const std::vector<BufferingNode>& nodes,
    size_t root,
    const BufferingSearchModel& model,
    const BufferingChoices& choices,
    const BufferingDummy& dummy)
{
  const auto order = postorder(nodes, root, model);
  if (!choices) {
    throw std::invalid_argument("Missing buffering cell characterization");
  }
  const auto loads = dummyLoads(nodes, dummy);
  std::vector<State> states(nodes.size());
  for (const auto index : order) {
    const auto& node = nodes[index];
    if (node.sink) {
      states[index] = leaf(node, index, model.dimensions);
      continue;
    }
    auto state = emptyState(model.dimensions);
    for (const auto child : node.children) {
      state = merge(std::move(state),
                    states[child],
                    distance(node.point, nodes[child].point),
                    model);
    }
    if (node.master.empty()) {
      if (index == root || !node.fixed_master.empty()) {
        return std::nullopt;
      }
      state.tree
          = std::make_shared<Decision>(Decision{index, {}, state.children});
    } else {
      if (!node.fixed_master.empty() && node.master != node.fixed_master) {
        return std::nullopt;
      }
      const auto& load = loads[index];
      state.pin_cap += load.buffer.input_cap;
      state.fanout += !load.buffer.master.empty();
      state.area += load.area;
      state.buffers += !load.buffer.master.empty();
      if (state.length > model.max_wire_length) {
        return std::nullopt;
      }
      const BufferingLoad driven{
          state.pin_cap, state.wire_cap, state.length, state.fanout};
      const auto cells = choices(driven, node.master);
      const auto cell = std::ranges::find(
          cells, node.master, [](const auto& c) { return c.buffer.master; });
      if (cell == cells.end()) {
        return std::nullopt;
      }
      state = buffered(std::move(state), index, *cell);
    }
    states[index] = std::move(state);
  }
  return materialize(nodes, states[root]);
}

BufferingPlan searchBuffering(const std::vector<BufferingNode>& sites,
                              size_t root,
                              const BufferingSearchModel& model,
                              const BufferingChoices& choices,
                              const BufferingDummy& dummy,
                              const std::optional<BufferingPlan>& reference)
{
  const auto order = postorder(sites, root, model);
  if (reference && reference->arrivals.size() != model.dimensions) {
    throw std::invalid_argument("Missing reference arrival dimensions");
  }
  if (!choices) {
    throw std::invalid_argument("Missing buffering cell characterization");
  }
  std::vector<std::vector<State>> frontiers(sites.size());
  size_t serial = 0;
  size_t peak = 0;
  for (const auto index : order) {
    const auto& node = sites[index];
    if (node.sink) {
      frontiers[index].push_back(leaf(node, index, model.dimensions));
      continue;
    }
    std::vector<State> merged{emptyState(model.dimensions)};
    for (const auto child : node.children) {
      std::vector<State> next;
      for (const auto& partial : merged) {
        for (const auto& alternative : frontiers[child]) {
          auto state = merge(partial,
                             alternative,
                             distance(node.point, sites[child].point),
                             model);
          state.order = serial++;
          next.push_back(std::move(state));
        }
      }
      prune(next, model.candidate_limit);
      merged = std::move(next);
    }
    auto& alternatives = frontiers[index];
    for (auto& state : merged) {
      if (index != root && node.fixed_master.empty()) {
        state.tree
            = std::make_shared<Decision>(Decision{index, {}, state.children});
        alternatives.push_back(state);
      }
      if (state.length <= model.max_wire_length) {
        const BufferingLoad load{
            state.pin_cap, state.wire_cap, state.length, state.fanout};
        for (const auto& cell : choices(load, node.fixed_master)) {
          if (!node.fixed_master.empty()
              && cell.buffer.master != node.fixed_master) {
            continue;
          }
          auto candidate = buffered(state, index, cell);
          candidate.order = serial++;
          alternatives.push_back(std::move(candidate));
        }
      }
    }
    prune(alternatives, model.candidate_limit);
    peak = std::max(peak, alternatives.size());
  }
  auto best = reference;
  if (best) {
    best->reference = true;
  }
  for (const auto& candidate : frontiers[root]) {
    const auto nodes = materialize(sites, candidate).nodes;
    auto plan = evaluateBuffering(nodes, root, model, choices, dummy);
    if (plan && (!reference || noWorse(*plan, *reference))
        && (!best || rank(*plan) < rank(*best))) {
      best = std::move(plan);
    }
  }
  if (!best) {
    throw std::runtime_error(
        "No electrically feasible bottom-up buffering plan");
  }
  best->peak_candidates = peak;
  return *best;
}
}  // namespace cts
