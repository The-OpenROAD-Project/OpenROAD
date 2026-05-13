// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SizeDownGenerator.hh"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "SizeDownCandidate.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/ContainerHelpers.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"

namespace rsz {

namespace {

using utl::RSZ;

constexpr int kSizeDownMaxFanout = 10;

struct SizeDownContext
{
  Resizer& resizer;
  MoveCommitter& committer;
  const Target& target;
  sta::Pin* drvr_pin{nullptr};
  sta::LibertyPort* drvr_port{nullptr};
  sta::Vertex* drvr_vertex{nullptr};
  const sta::Scene* scene{nullptr};
  const sta::MinMax* min_max{nullptr};
};

struct SizeDownLoadContext
{
  sta::Pin* load_pin{nullptr};
  sta::LibertyPort* load_port{nullptr};
  sta::LibertyCell* load_cell{nullptr};
  sta::Instance* load_inst{nullptr};
  sta::Slack load_slack{0.0};
  sta::Slack delay_budget{0.0};
  int lib_ap{0};
  float input_cap{0.0f};
};

struct SizeDownOutputProfile
{
  std::vector<const sta::Pin*> output_pins;
  std::vector<float> output_caps;
  std::vector<float> output_slew_factors;
  std::vector<float> output_delays;
  std::vector<std::string> output_port_names;
};

bool resolveDriverContext(SizeDownContext& ctx)
{
  ctx.drvr_pin = ctx.target.resolvedPin(ctx.resizer);
  ctx.drvr_vertex = ctx.target.vertex(ctx.resizer);
  if (ctx.drvr_pin == nullptr || ctx.drvr_vertex == nullptr) {
    return false;
  }
  if (ctx.target.fanout >= kSizeDownMaxFanout) {
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "size_down_move",
               2,
               "REJECT SizeDownMove {}: Fanout {} >= {} max fanout",
               ctx.resizer.network()->pathName(ctx.drvr_pin),
               ctx.target.fanout,
               kSizeDownMaxFanout);
    return false;
  }

  ctx.drvr_port = ctx.resizer.network()->libertyPort(ctx.drvr_pin);
  ctx.scene = ctx.target.endpoint_path->scene(ctx.resizer.sta());
  ctx.min_max = ctx.target.endpoint_path->minMax(ctx.resizer.sta());
  return ctx.drvr_port != nullptr && ctx.scene != nullptr
         && ctx.min_max != nullptr;
}

std::vector<std::pair<sta::Vertex*, sta::Slack>> sortedFanoutSlacks(
    const SizeDownContext& ctx)
{
  std::vector<std::pair<sta::Vertex*, sta::Slack>> fanout_slacks;
  sta::VertexOutEdgeIterator edge_iter(ctx.drvr_vertex, ctx.resizer.graph());
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    if (!edge->isWire()) {
      continue;
    }

    sta::Vertex* fanout_vertex = edge->to(ctx.resizer.graph());
    sta::Pin* fanout_pin = fanout_vertex->pin();
    sta::Instance* fanout_inst = ctx.resizer.network()->instance(fanout_pin);
    if (fanout_inst != nullptr
        && ctx.committer.hasMoves(MoveType::kSizeDown, fanout_inst)) {
      continue;
    }

    const sta::Slack fanout_slack = ctx.resizer.sta()->slack(
        fanout_vertex, ctx.resizer.maxAnalysisMode());
    fanout_slacks.emplace_back(fanout_vertex, fanout_slack);
  }

  std::ranges::sort(fanout_slacks, [&ctx](const auto& lhs, const auto& rhs) {
    return lhs.second > rhs.second
           || (lhs.second == rhs.second
               && ctx.resizer.network()->pathNameLess(lhs.first->pin(),
                                                      rhs.first->pin()));
  });

  for (const auto& [fanout_vertex, fanout_slack] : fanout_slacks) {
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "size_down_move",
               2,
               " fanout {} slack: {} drvr slack: {}",
               ctx.resizer.network()->pathName(fanout_vertex->pin()),
               delayAsString(fanout_slack, 3, ctx.resizer.staState()),
               delayAsString(ctx.target.slack, 3, ctx.resizer.staState()));
  }

  return fanout_slacks;
}

std::vector<const sta::Pin*> getOutputPins(const SizeDownContext& ctx,
                                           const sta::Instance* inst)
{
  std::vector<const sta::Pin*> outputs;
  auto pin_iter = std::unique_ptr<sta::InstancePinIterator>(
      ctx.resizer.network()->pinIterator(inst));
  while (pin_iter->hasNext()) {
    const sta::Pin* pin = pin_iter->next();
    if (ctx.resizer.network()->direction(pin)->isOutput()) {
      outputs.push_back(pin);
    }
  }
  return outputs;
}

bool resolveLoadContext(const SizeDownContext& ctx,
                        sta::Vertex* load_vertex,
                        const sta::Slack load_slack,
                        SizeDownLoadContext& load_ctx)
{
  load_ctx.load_pin = load_vertex->pin();
  load_ctx.load_port = ctx.resizer.network()->libertyPort(load_ctx.load_pin);
  if (load_ctx.load_port == nullptr) {
    return false;
  }

  load_ctx.load_cell = load_ctx.load_port->libertyCell();
  load_ctx.load_inst = ctx.resizer.network()->instance(load_ctx.load_pin);
  if (load_ctx.load_inst == nullptr || ctx.resizer.dontTouch(load_ctx.load_inst)
      || !ctx.resizer.isLogicStdCell(load_ctx.load_inst)) {
    return false;
  }

  load_ctx.load_slack = load_slack;
  load_ctx.lib_ap = ctx.scene->libertyIndex(ctx.min_max);
  load_ctx.input_cap = static_cast<const sta::LibertyPort*>(load_ctx.load_port)
                           ->scenePort(load_ctx.lib_ap)
                           ->capacitance();
  return true;
}

sta::ArcDelay getWorstIntrinsicDelay(const sta::LibertyPort* input_port)
{
  sta::ArcDelay max_intrinsic = -sta::INF;
  sta::TimingArcSetSeq arc_sets
      = input_port->libertyCell()->timingArcSets(input_port, nullptr);
  for (const sta::TimingArcSet* arc_set : arc_sets) {
    for (const sta::TimingArc* arc : arc_set->arcs()) {
      const sta::ArcDelay intrinsic = arc->intrinsicDelay();
      max_intrinsic = std::max(max_intrinsic, intrinsic);
    }
  }
  return sta::delayAsFloat(max_intrinsic) == -sta::INF ? sta::ArcDelay(0.0f)
                                                       : max_intrinsic;
}

sta::Slack getWorstInputSlack(const SizeDownContext& ctx, sta::Instance* inst)
{
  sta::Slack worst_slack = sta::INF;
  auto pin_iter = std::unique_ptr<sta::InstancePinIterator>(
      ctx.resizer.network()->pinIterator(inst));
  while (pin_iter->hasNext()) {
    sta::Pin* pin = pin_iter->next();
    if (ctx.resizer.network()->direction(pin)->isInput()) {
      sta::Vertex* vertex = ctx.resizer.graph()->pinDrvrVertex(pin);
      if (vertex != nullptr) {
        worst_slack = std::min(
            worst_slack,
            ctx.resizer.sta()->slack(vertex, ctx.resizer.maxAnalysisMode()));
      }
    }
  }
  return worst_slack;
}

sta::Slack getWorstOutputSlack(const SizeDownContext& ctx, sta::Instance* inst)
{
  sta::Slack worst_slack = sta::INF;
  auto pin_iter = std::unique_ptr<sta::InstancePinIterator>(
      ctx.resizer.network()->pinIterator(inst));
  while (pin_iter->hasNext()) {
    sta::Pin* pin = pin_iter->next();
    if (ctx.resizer.network()->direction(pin)->isOutput()) {
      sta::Vertex* vertex = ctx.resizer.graph()->pinLoadVertex(pin);
      if (vertex != nullptr) {
        worst_slack = std::min(
            worst_slack,
            ctx.resizer.sta()->slack(vertex, ctx.resizer.maxAnalysisMode()));
      }
    }
  }
  return worst_slack;
}

bool checkMaxCapViolation(const SizeDownContext& ctx,
                          const sta::Pin* output_pin,
                          sta::LibertyPort* output_port,
                          const float output_cap)
{
  float max_cap = 0.0f;
  bool cap_limit_exists = false;
  output_port->capacitanceLimit(
      ctx.resizer.maxAnalysisMode(), max_cap, cap_limit_exists);
  if (cap_limit_exists && max_cap > 0.0f && output_cap > max_cap) {
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "opt_moves",
               2,
               "  skip based on max cap {} gate={} cap={} max_cap={}",
               ctx.resizer.network()->pathName(output_pin),
               output_port->libertyCell()->name(),
               output_cap,
               max_cap);
    return true;
  }
  return false;
}

float computeElmoreSlewFactor(const SizeDownContext& ctx,
                              const sta::Pin* output_pin,
                              sta::LibertyPort* output_port,
                              const float output_load_cap)
{
  const sta::Slew slew
      = ctx.resizer.sta()->slew(ctx.resizer.graph()->pinLoadVertex(output_pin),
                                sta::RiseFallBoth::riseFall(),
                                ctx.resizer.sta()->scenes(),
                                ctx.resizer.maxAnalysisMode());
  const float output_res = output_port->driveResistance();
  return output_res > 0.0f && output_load_cap > 0.0f
             ? slew / (output_res * output_load_cap)
             : 0.0f;
}

bool checkMaxSlewViolation(const SizeDownContext& ctx,
                           const sta::Pin* output_pin,
                           sta::LibertyPort* output_port,
                           const float output_slew_factor,
                           const float output_cap)
{
  const float new_slew
      = output_slew_factor * output_port->driveResistance() * output_cap;
  float max_slew = 0.0f;
  bool slew_limit_exists = false;
  ctx.resizer.sta()->findSlewLimit(output_port,
                                   ctx.scene,
                                   ctx.resizer.maxAnalysisMode(),
                                   max_slew,
                                   slew_limit_exists);
  if (slew_limit_exists && new_slew > max_slew) {
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "opt_moves",
               2,
               "  skip based on max slew {} gate={} slew={} max_slew={}",
               ctx.resizer.network()->pathName(output_pin),
               output_port->libertyCell()->name(),
               new_slew,
               max_slew);
    return true;
  }
  return false;
}

sta::LibertyCellSeq rankSwappableCells(const SizeDownContext& ctx,
                                       const SizeDownLoadContext& load_ctx)
{
  const std::string& load_port_name = load_ctx.load_port->name();
  sta::LibertyCellSeq swappable_cells
      = load_ctx.load_cell->isBuffer()
            ? ctx.resizer.getFastBufferSizes(load_ctx.load_cell)
            : ctx.resizer.getSwappableCells(load_ctx.load_cell);

  if (swappable_cells.size() > 1) {
    sort(&swappable_cells,
         [&load_ctx, load_port_name](const sta::LibertyCell* cell1,
                                     const sta::LibertyCell* cell2) {
           const sta::LibertyPort* port1
               = static_cast<const sta::LibertyPort*>(
                     cell1->findLibertyPort(load_port_name))
                     ->scenePort(load_ctx.lib_ap);
           const sta::LibertyPort* port2
               = static_cast<const sta::LibertyPort*>(
                     cell2->findLibertyPort(load_port_name))
                     ->scenePort(load_ctx.lib_ap);
           const float cap1 = port1->capacitance();
           const float cap2 = port2->capacitance();
           const sta::ArcDelay intrinsic1 = getWorstIntrinsicDelay(port1);
           const sta::ArcDelay intrinsic2 = getWorstIntrinsicDelay(port2);
           return std::tie(cap1, intrinsic2) < std::tie(cap2, intrinsic1);
         });
  }

  return swappable_cells;
}

void logSwappableCells(const SizeDownContext& ctx,
                       const SizeDownLoadContext& load_ctx,
                       const sta::LibertyCellSeq& swappable_cells)
{
  if (ctx.resizer.logger()->debugCheck(RSZ, "size_down_move", 3)) {
    std::string swappable_names;
    for (sta::LibertyCell* swappable : swappable_cells) {
      if (!swappable_names.empty()) {
        swappable_names += " ";
      }
      if (swappable == load_ctx.load_cell) {
        swappable_names += "*";
      }
      swappable_names += swappable->name();
      if (swappable == load_ctx.load_cell) {
        swappable_names += "*";
      }
    }
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "size_down_move",
               3,
               "size_down fanout {} swaps={}",
               ctx.resizer.network()->pathName(load_ctx.load_pin),
               swappable_names.c_str());
  }
}

SizeDownOutputProfile buildOutputProfile(const SizeDownContext& ctx,
                                         const SizeDownLoadContext& load_ctx)
{
  SizeDownOutputProfile profile;
  profile.output_pins = getOutputPins(ctx, load_ctx.load_inst);
  profile.output_caps.reserve(profile.output_pins.size());
  profile.output_slew_factors.reserve(profile.output_pins.size());
  profile.output_delays.reserve(profile.output_pins.size());
  profile.output_port_names.reserve(profile.output_pins.size());

  for (const sta::Pin* output_pin : profile.output_pins) {
    sta::LibertyPort* output_port
        = ctx.resizer.network()->libertyPort(output_pin);
    profile.output_port_names.push_back(output_port->name());

    const float output_load_cap = ctx.resizer.sta()->graphDelayCalc()->loadCap(
        output_pin, ctx.scene, ctx.min_max);
    profile.output_caps.push_back(output_load_cap);

    const float output_slew = ctx.resizer.sta()->slew(
        ctx.resizer.graph()->pinLoadVertex(output_pin),
        sta::RiseFallBoth::riseFall(),
        ctx.resizer.sta()->scenes(),
        ctx.resizer.maxAnalysisMode());
    profile.output_slew_factors.push_back(
        computeElmoreSlewFactor(ctx, output_pin, output_port, output_load_cap));

    const float load_delay = ctx.resizer.gateDelay(
        output_port, output_load_cap, ctx.scene, ctx.min_max);
    profile.output_delays.push_back(load_delay);

    debugPrint(ctx.resizer.logger(),
               RSZ,
               "size_down_move",
               4,
               " current {}->{} gate={} delay={} cap={} slew={} slack={}",
               ctx.resizer.network()->pathName(load_ctx.load_pin),
               ctx.resizer.network()->pathName(output_pin),
               load_ctx.load_cell->name(),
               delayAsString(load_delay, 3, ctx.resizer.staState()),
               output_load_cap,
               delayAsString(output_slew, 3, ctx.resizer.staState()),
               delayAsString(load_ctx.load_slack, 3, ctx.resizer.staState()));
  }

  return profile;
}

sta::Slack computeDelayBudget(const SizeDownContext& ctx,
                              const SizeDownLoadContext& load_ctx)
{
  if (load_ctx.load_cell->hasSequentials()) {
    const sta::Slack worst_output_slack
        = getWorstOutputSlack(ctx, load_ctx.load_inst);
    debugPrint(
        ctx.resizer.logger(),
        RSZ,
        "size_down_move",
        4,
        " Sequential element: using worst output slack: {} (pin slack: {})",
        delayAsString(worst_output_slack, 3, ctx.resizer.staState()),
        delayAsString(load_ctx.load_slack, 3, ctx.resizer.staState()));
    return worst_output_slack;
  }

  const sta::Slack worst_input_slack
      = getWorstInputSlack(ctx, load_ctx.load_inst);
  const sta::Slack delay_budget
      = std::min(load_ctx.load_slack, worst_input_slack);
  debugPrint(ctx.resizer.logger(),
             RSZ,
             "size_down_move",
             4,
             " Combinational gate: using worst input slack: {} (pin slack: {}, "
             "worst input: {})",
             delayAsString(delay_budget, 3, ctx.resizer.staState()),
             delayAsString(load_ctx.load_slack, 3, ctx.resizer.staState()),
             delayAsString(worst_input_slack, 3, ctx.resizer.staState()));
  return delay_budget;
}

float candidateInputCap(const SizeDownLoadContext& load_ctx,
                        sta::LibertyCell* cell)
{
  return static_cast<const sta::LibertyPort*>(
             cell->findLibertyPort(load_ctx.load_port->name()))
      ->scenePort(load_ctx.lib_ap)
      ->capacitance();
}

bool isWorseCapOrArea(const SizeDownLoadContext& load_ctx,
                      sta::LibertyCell* best_cell,
                      sta::LibertyCell* swappable)
{
  return candidateInputCap(load_ctx, swappable)
             > candidateInputCap(load_ctx, best_cell)
         || swappable->area() > best_cell->area();
}

bool violatesOutputLimits(const SizeDownContext& ctx,
                          const SizeDownOutputProfile& profile,
                          sta::LibertyCell* swappable)
{
  for (size_t i = 0; i < profile.output_pins.size(); ++i) {
    sta::LibertyPort* output_port
        = swappable->findLibertyPort(profile.output_port_names[i]);
    if (checkMaxCapViolation(
            ctx, profile.output_pins[i], output_port, profile.output_caps[i])
        || checkMaxSlewViolation(ctx,
                                 profile.output_pins[i],
                                 output_port,
                                 profile.output_slew_factors[i],
                                 profile.output_caps[i])) {
      return true;
    }
  }
  return false;
}

float computeDriverDelayDelta(const SizeDownContext& ctx,
                              const SizeDownLoadContext& load_ctx,
                              sta::LibertyCell* swappable)
{
  return -ctx.drvr_port->driveResistance()
         * (load_ctx.input_cap - candidateInputCap(load_ctx, swappable));
}

float computeWorstDelayChange(const SizeDownContext& ctx,
                              const SizeDownLoadContext& load_ctx,
                              const SizeDownOutputProfile& profile,
                              sta::LibertyCell* swappable)
{
  const float drvr_delta_delay
      = computeDriverDelayDelta(ctx, load_ctx, swappable);
  float worst_delay_change = -sta::INF;
  for (size_t output_index = 0; output_index < profile.output_pins.size();
       ++output_index) {
    sta::LibertyPort* output_port
        = swappable->findLibertyPort(profile.output_port_names[output_index]);
    const float new_load_delay = ctx.resizer.gateDelay(
        output_port, profile.output_caps[output_index], ctx.scene, ctx.min_max);
    const float delay_change
        = load_ctx.load_cell->hasSequentials()
              ? new_load_delay - profile.output_delays[output_index]
              : new_load_delay + drvr_delta_delay
                    - profile.output_delays[output_index];
    worst_delay_change = std::max(worst_delay_change, delay_change);
  }
  return worst_delay_change;
}

bool fitsDelayBudget(const SizeDownContext& ctx,
                     const SizeDownLoadContext& load_ctx,
                     const SizeDownOutputProfile& profile,
                     sta::LibertyCell* swappable)
{
  const float drvr_delta_delay
      = computeDriverDelayDelta(ctx, load_ctx, swappable);
  const float worst_delay_change
      = computeWorstDelayChange(ctx, load_ctx, profile, swappable);
  sta::LibertyPort* first_output_port
      = swappable->findLibertyPort(profile.output_port_names[0]);
  const float first_new_load_delay = ctx.resizer.gateDelay(
      first_output_port, profile.output_caps[0], ctx.scene, ctx.min_max);

  debugPrint(
      ctx.resizer.logger(),
      RSZ,
      "size_down_move",
      4,
      " new delay {}->{} gate={} drvr_delta {} + new_delay {} - old_delay "
      "{} < slack {} ({} < {})",
      ctx.resizer.network()->pathName(load_ctx.load_pin),
      ctx.resizer.network()->pathName(profile.output_pins[0]),
      swappable->name(),
      delayAsString(drvr_delta_delay, 3, ctx.resizer.staState()),
      delayAsString(first_new_load_delay, 3, ctx.resizer.staState()),
      delayAsString(profile.output_delays[0], 3, ctx.resizer.staState()),
      delayAsString(load_ctx.delay_budget, 3, ctx.resizer.staState()),
      delayAsString(worst_delay_change, 3, ctx.resizer.staState()),
      delayAsString(load_ctx.delay_budget, 3, ctx.resizer.staState()));

  const bool violates_budget
      = (load_ctx.delay_budget > 0
         && worst_delay_change > load_ctx.delay_budget)
        || (load_ctx.delay_budget < 0 && worst_delay_change > 0);
  if (violates_budget) {
    debugPrint(
        ctx.resizer.logger(),
        RSZ,
        "size_down_move",
        4,
        " skip based on delay {}->{} gate={} drvr_delta {} + new_delay {} "
        "- old_delay {} < slack {} ({} < {})",
        ctx.resizer.network()->pathName(load_ctx.load_pin),
        ctx.resizer.network()->pathName(profile.output_pins[0]),
        swappable->name(),
        delayAsString(drvr_delta_delay, 3, ctx.resizer.staState()),
        delayAsString(first_new_load_delay, 3, ctx.resizer.staState()),
        delayAsString(profile.output_delays[0], 3, ctx.resizer.staState()),
        delayAsString(load_ctx.delay_budget, 3, ctx.resizer.staState()),
        delayAsString(worst_delay_change, 3, ctx.resizer.staState()),
        delayAsString(load_ctx.delay_budget, 3, ctx.resizer.staState()));
  }
  return !violates_budget;
}

sta::LibertyCell* selectReplacementCell(
    const SizeDownContext& ctx,
    const SizeDownLoadContext& load_ctx,
    const sta::LibertyCellSeq& swappable_cells,
    const SizeDownOutputProfile& profile)
{
  sta::LibertyCell* best_cell = load_ctx.load_cell;
  for (sta::LibertyCell* swappable : swappable_cells) {
    if (swappable == load_ctx.load_cell) {
      continue;
    }

    debugPrint(ctx.resizer.logger(),
               RSZ,
               "size_down_move",
               4,
               " considering swap {} {} -> {}",
               ctx.resizer.network()->pathName(load_ctx.load_pin),
               load_ctx.load_cell->name(),
               swappable->name());

    if (isWorseCapOrArea(load_ctx, best_cell, swappable)) {
      debugPrint(ctx.resizer.logger(),
                 RSZ,
                 "size_down_move",
                 4,
                 "  skip based on cap/area {} gate={} cap={}>{} area={}>{}",
                 ctx.resizer.network()->pathName(load_ctx.load_pin),
                 swappable->name(),
                 candidateInputCap(load_ctx, swappable),
                 candidateInputCap(load_ctx, best_cell),
                 swappable->area(),
                 best_cell->area());
      continue;
    }

    if (violatesOutputLimits(ctx, profile, swappable)
        || !fitsDelayBudget(ctx, load_ctx, profile, swappable)) {
      continue;
    }

    best_cell = swappable;
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "size_down_move",
               3,
               " new best size down {} -> {} ({} -> {})",
               ctx.resizer.network()->pathName(load_ctx.load_pin),
               ctx.resizer.network()->pathName(profile.output_pins[0]),
               load_ctx.load_cell->name(),
               swappable->name());
  }

  return best_cell != load_ctx.load_cell ? best_cell : nullptr;
}

std::unique_ptr<MoveCandidate> buildCandidate(const SizeDownContext& ctx,
                                              sta::Vertex* load_vertex,
                                              const sta::Slack load_slack)
{
  SizeDownLoadContext load_ctx;
  if (!resolveLoadContext(ctx, load_vertex, load_slack, load_ctx)) {
    return nullptr;
  }

  load_ctx.delay_budget = computeDelayBudget(ctx, load_ctx);
  const sta::LibertyCellSeq swappable_cells = rankSwappableCells(ctx, load_ctx);
  logSwappableCells(ctx, load_ctx, swappable_cells);
  const SizeDownOutputProfile profile = buildOutputProfile(ctx, load_ctx);
  sta::LibertyCell* replacement
      = selectReplacementCell(ctx, load_ctx, swappable_cells, profile);
  if (replacement == nullptr) {
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "size_down_move",
               3,
               "REJECT SizeDownMove {} -> {}: ({} -> {}) slack={}",
               ctx.resizer.network()->pathName(ctx.drvr_pin),
               ctx.resizer.network()->pathName(load_ctx.load_pin),
               load_ctx.load_cell->name(),
               "none",
               delayAsString(load_ctx.load_slack, 3, ctx.resizer.staState()));
    return nullptr;
  }

  return std::make_unique<SizeDownCandidate>(ctx.resizer,
                                             ctx.target,
                                             ctx.drvr_pin,
                                             load_ctx.load_inst,
                                             load_ctx.load_pin,
                                             load_ctx.load_cell,
                                             replacement,
                                             load_ctx.load_slack);
}

std::vector<std::unique_ptr<MoveCandidate>> buildCandidates(
    const SizeDownContext& ctx)
{
  debugPrint(ctx.resizer.logger(),
             RSZ,
             "size_down_move",
             2,
             "sizing down for crit fanout {}",
             ctx.resizer.network()->pathName(ctx.drvr_pin));

  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  for (const auto& [load_vertex, load_slack] : sortedFanoutSlacks(ctx)) {
    auto candidate = buildCandidate(ctx, load_vertex, load_slack);
    if (!candidate) {
      continue;
    }

    candidates.push_back(std::move(candidate));
  }
  if (candidates.empty()) {
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "size_down_move",
               2,
               "REJECT SizeDownMove {}: Couldn't size down any gates",
               ctx.resizer.network()->pathName(ctx.drvr_pin));
  }
  return candidates;
}

}  // namespace

SizeDownGenerator::SizeDownGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

std::vector<std::unique_ptr<MoveCandidate>> SizeDownGenerator::generate(
    const Target& target)
{
  SizeDownContext ctx{
      .resizer = resizer_, .committer = committer_, .target = target};
  if (!resolveDriverContext(ctx)) {
    return {};
  }

  return buildCandidates(ctx);
}

}  // namespace rsz
