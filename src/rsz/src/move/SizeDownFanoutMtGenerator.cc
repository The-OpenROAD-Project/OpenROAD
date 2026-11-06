// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SizeDownFanoutMtGenerator.hh"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "SizeDownFanoutMtCandidate.hh"
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
#include "sta/PortDirection.hh"
#include "utl/Logger.h"

namespace rsz {

namespace {

using utl::RSZ;

// The target driver and the scene the load checks of prepareInputs() read.
struct SizeDownFanoutContext
{
  Resizer& resizer;
  MoveCommitter& committer;
  const Target& target;
  sta::Pin* drvr_pin{nullptr};
  sta::LibertyPort* drvr_port{nullptr};
  sta::Vertex* drvr_vertex{nullptr};
  const sta::Scene* scene{nullptr};
  const sta::MinMax* min_max{nullptr};
  int lib_ap{0};
};

// The current timing of one output of a fanout load.
struct SizeDownOutput
{
  const sta::Pin* pin{nullptr};
  std::string port_name;
  float cap{0.0f};
  float slew_factor{0.0f};
  float delay{0.0f};
};

bool resolveDriverContext(SizeDownFanoutContext& ctx)
{
  ctx.drvr_pin = ctx.target.resolvedPin(ctx.resizer);
  ctx.drvr_vertex = ctx.target.vertex(ctx.resizer);
  if (ctx.drvr_pin == nullptr || ctx.drvr_vertex == nullptr) {
    return false;
  }

  ctx.drvr_port = ctx.resizer.network()->libertyPort(ctx.drvr_pin);
  // Use the Target accessors: endpoint_path can be stale after earlier moves,
  // and reading it would crash.
  ctx.scene = ctx.target.activeScene(ctx.resizer);
  ctx.min_max = ctx.target.minMax(ctx.resizer);
  if (ctx.drvr_port == nullptr || ctx.scene == nullptr
      || ctx.min_max == nullptr) {
    return false;
  }
  ctx.lib_ap = ctx.scene->libertyIndex(ctx.min_max);
  return true;
}

std::vector<std::pair<sta::Vertex*, sta::Slack>> sortedFanoutSlacks(
    const SizeDownFanoutContext& ctx)
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
        && ctx.committer.hasMoves(MoveType::kSizeDownFanout, fanout_inst)) {
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
               "size_down_fanout_move",
               2,
               " fanout {} slack: {} drvr slack: {}",
               ctx.resizer.network()->pathName(fanout_vertex->pin()),
               delayAsString(fanout_slack, 3, ctx.resizer.staState()),
               delayAsString(ctx.target.slack, 3, ctx.resizer.staState()));
  }

  return fanout_slacks;
}

std::vector<const sta::Pin*> getOutputPins(const SizeDownFanoutContext& ctx,
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

// The load of load_vertex if its cell may change: its pin, port, cell,
// instance and input capacitance.
bool resolveLoad(const SizeDownFanoutContext& ctx,
                 sta::Vertex* load_vertex,
                 const sta::Slack load_slack,
                 SizeDownFanoutMtLoad& load)
{
  load.load_pin = load_vertex->pin();
  load.load_port = ctx.resizer.network()->libertyPort(load.load_pin);
  if (load.load_port == nullptr) {
    return false;
  }

  load.current_cell = load.load_port->libertyCell();
  load.load_inst = ctx.resizer.network()->instance(load.load_pin);
  if (load.load_inst == nullptr || ctx.resizer.dontTouch(load.load_inst)
      || ctx.resizer.isFixed(load.load_inst)
      || !ctx.resizer.isLogicStdCell(load.load_inst)) {
    return false;
  }

  load.load_slack = load_slack;
  // scenePort() can be null, for example in multi-VT libraries.
  const sta::LibertyPort* load_scene_port
      = static_cast<const sta::LibertyPort*>(load.load_port)
            ->scenePort(ctx.lib_ap);
  if (load_scene_port == nullptr) {
    return false;
  }
  load.input_cap = load_scene_port->capacitance();
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

sta::Slack getWorstInputSlack(const SizeDownFanoutContext& ctx,
                              sta::Instance* inst)
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

sta::Slack getWorstOutputSlack(const SizeDownFanoutContext& ctx,
                               sta::Instance* inst)
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

bool checkMaxCapViolation(const SizeDownFanoutContext& ctx,
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

float computeElmoreSlewFactor(const SizeDownFanoutContext& ctx,
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
  if (output_res <= 0.0f || output_load_cap <= 0.0f) {
    return 0.0f;
  }
  const float elmore_denominator = output_res * output_load_cap;
  return slew / elmore_denominator;
}

bool checkMaxSlewViolation(const SizeDownFanoutContext& ctx,
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

sta::LibertyCellSeq rankSwappableCells(const SizeDownFanoutContext& ctx,
                                       const SizeDownFanoutMtLoad& load)
{
  const std::string& load_port_name = load.load_port->name();
  sta::LibertyCellSeq swappable_cells
      = load.current_cell->isBuffer()
            ? ctx.resizer.getFastBufferSizes(load.current_cell)
            : ctx.resizer.getSwappableCells(load.current_cell);

  if (swappable_cells.size() > 1) {
    sort(&swappable_cells,
         [&ctx, load_port_name](const sta::LibertyCell* cell1,
                                const sta::LibertyCell* cell2) {
           // A cell without the load port or its scene port ranks last and is
           // never chosen.
           const sta::LibertyPort* raw1 = static_cast<const sta::LibertyPort*>(
               cell1->findLibertyPort(load_port_name));
           const sta::LibertyPort* raw2 = static_cast<const sta::LibertyPort*>(
               cell2->findLibertyPort(load_port_name));
           const sta::LibertyPort* port1
               = raw1 != nullptr ? raw1->scenePort(ctx.lib_ap) : nullptr;
           const sta::LibertyPort* port2
               = raw2 != nullptr ? raw2->scenePort(ctx.lib_ap) : nullptr;
           if (port1 == nullptr || port2 == nullptr) {
             return port1 != nullptr;
           }
           const float cap1 = port1->capacitance();
           const float cap2 = port2->capacitance();
           const sta::ArcDelay intrinsic1 = getWorstIntrinsicDelay(port1);
           const sta::ArcDelay intrinsic2 = getWorstIntrinsicDelay(port2);
           return std::tie(cap1, intrinsic2) < std::tie(cap2, intrinsic1);
         });
  }

  return swappable_cells;
}

void logSwappableCells(const SizeDownFanoutContext& ctx,
                       const SizeDownFanoutMtLoad& load,
                       const sta::LibertyCellSeq& swappable_cells)
{
  if (ctx.resizer.logger()->debugCheck(RSZ, "size_down_fanout_move", 3)) {
    std::string swappable_names;
    for (sta::LibertyCell* swappable : swappable_cells) {
      if (!swappable_names.empty()) {
        swappable_names += " ";
      }
      if (swappable == load.current_cell) {
        swappable_names += "*";
      }
      swappable_names += swappable->name();
      if (swappable == load.current_cell) {
        swappable_names += "*";
      }
    }
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "size_down_fanout_move",
               3,
               "size_down_fanout fanout {} swaps={}",
               ctx.resizer.network()->pathName(load.load_pin),
               swappable_names.c_str());
  }
}

std::vector<SizeDownOutput> measureOutputs(const SizeDownFanoutContext& ctx,
                                           const SizeDownFanoutMtLoad& load)
{
  std::vector<SizeDownOutput> outputs;
  for (const sta::Pin* output_pin : getOutputPins(ctx, load.load_inst)) {
    sta::LibertyPort* output_port
        = ctx.resizer.network()->libertyPort(output_pin);
    SizeDownOutput& output = outputs.emplace_back();
    output.pin = output_pin;
    output.port_name = output_port->name();
    output.cap = ctx.resizer.sta()->graphDelayCalc()->loadCap(
        output_pin, ctx.scene, ctx.min_max);

    const float output_slew = ctx.resizer.sta()->slew(
        ctx.resizer.graph()->pinLoadVertex(output_pin),
        sta::RiseFallBoth::riseFall(),
        ctx.resizer.sta()->scenes(),
        ctx.resizer.maxAnalysisMode());
    output.slew_factor
        = computeElmoreSlewFactor(ctx, output_pin, output_port, output.cap);
    output.delay = ctx.resizer.gateDelay(
        output_port, output.cap, ctx.scene, ctx.min_max);

    debugPrint(ctx.resizer.logger(),
               RSZ,
               "size_down_fanout_move",
               4,
               " current {}->{} gate={} delay={} cap={} slew={} slack={}",
               ctx.resizer.network()->pathName(load.load_pin),
               ctx.resizer.network()->pathName(output_pin),
               load.current_cell->name(),
               delayAsString(output.delay, 3, ctx.resizer.staState()),
               output.cap,
               delayAsString(output_slew, 3, ctx.resizer.staState()),
               delayAsString(load.load_slack, 3, ctx.resizer.staState()));
  }

  return outputs;
}

sta::Slack computeDelayBudget(const SizeDownFanoutContext& ctx,
                              const SizeDownFanoutMtLoad& load)
{
  if (load.current_cell->isSequential()) {
    const sta::Slack worst_output_slack
        = getWorstOutputSlack(ctx, load.load_inst);
    debugPrint(
        ctx.resizer.logger(),
        RSZ,
        "size_down_fanout_move",
        4,
        " Sequential element: using worst output slack: {} (pin slack: {})",
        delayAsString(worst_output_slack, 3, ctx.resizer.staState()),
        delayAsString(load.load_slack, 3, ctx.resizer.staState()));
    return worst_output_slack;
  }

  const sta::Slack worst_input_slack = getWorstInputSlack(ctx, load.load_inst);
  const sta::Slack delay_budget = std::min(load.load_slack, worst_input_slack);
  debugPrint(ctx.resizer.logger(),
             RSZ,
             "size_down_fanout_move",
             4,
             " Combinational gate: using worst input slack: {} (pin slack: {}, "
             "worst input: {})",
             delayAsString(delay_budget, 3, ctx.resizer.staState()),
             delayAsString(load.load_slack, 3, ctx.resizer.staState()),
             delayAsString(worst_input_slack, 3, ctx.resizer.staState()));
  return delay_budget;
}

float candidateInputCap(const SizeDownFanoutContext& ctx,
                        const SizeDownFanoutMtLoad& load,
                        sta::LibertyCell* cell)
{
  // A cell without the load port or its scene port costs the most and is never
  // preferred.
  const sta::LibertyPort* port = static_cast<const sta::LibertyPort*>(
      cell->findLibertyPort(load.load_port->name()));
  if (port == nullptr) {
    return sta::INF;
  }
  const sta::LibertyPort* scene_port = port->scenePort(ctx.lib_ap);
  if (scene_port == nullptr) {
    return sta::INF;
  }
  return scene_port->capacitance();
}

bool violatesOutputLimits(const SizeDownFanoutContext& ctx,
                          const std::vector<SizeDownOutput>& outputs,
                          sta::LibertyCell* swappable)
{
  for (const SizeDownOutput& output : outputs) {
    sta::LibertyPort* output_port
        = swappable->findLibertyPort(output.port_name);
    // A cell without this output port cannot be checked; reject it.
    if (output_port == nullptr) {
      return true;
    }
    if (checkMaxCapViolation(ctx, output.pin, output_port, output.cap)
        || checkMaxSlewViolation(
            ctx, output.pin, output_port, output.slew_factor, output.cap)) {
      return true;
    }
  }
  return false;
}

float computeDriverDelayDelta(const SizeDownFanoutContext& ctx,
                              const SizeDownFanoutMtLoad& load,
                              sta::LibertyCell* swappable)
{
  return -ctx.drvr_port->driveResistance()
         * (load.input_cap - candidateInputCap(ctx, load, swappable));
}

float computeWorstDelayChange(const SizeDownFanoutContext& ctx,
                              const SizeDownFanoutMtLoad& load,
                              const std::vector<SizeDownOutput>& outputs,
                              sta::LibertyCell* swappable)
{
  const float drvr_delta_delay = computeDriverDelayDelta(ctx, load, swappable);
  float worst_delay_change = -sta::INF;
  for (const SizeDownOutput& output : outputs) {
    sta::LibertyPort* output_port
        = swappable->findLibertyPort(output.port_name);
    const float new_load_delay = ctx.resizer.gateDelay(
        output_port, output.cap, ctx.scene, ctx.min_max);
    const float delay_change
        = load.current_cell->isSequential()
              ? new_load_delay - output.delay
              : new_load_delay + drvr_delta_delay - output.delay;
    worst_delay_change = std::max(worst_delay_change, delay_change);
  }
  return worst_delay_change;
}

// The measurements of one smaller cell: its output limits and the worst
// delay change of its outputs, for generate() to judge against the budget.
SizeDownFanoutMtCellOption measureCellOption(
    const SizeDownFanoutContext& ctx,
    const SizeDownFanoutMtLoad& load,
    const std::vector<SizeDownOutput>& outputs,
    const sta::LibertyPort* current_scene_port,
    const SelectedArc& arc,
    sta::LibertyCell* cell)
{
  SizeDownFanoutMtCellOption option{
      .cell = cell,
      .input_cap = candidateInputCap(ctx, load, cell),
      .violates_output_limits = violatesOutputLimits(ctx, outputs, cell)};
  // A cell past a limit is out whatever its delay.
  if (!option.violates_output_limits) {
    option.worst_delay_change
        = computeWorstDelayChange(ctx, load, outputs, cell);
    debugPrint(
        ctx.resizer.logger(),
        RSZ,
        "size_down_fanout_move",
        4,
        " new delay {} gate={} drvr_delta {} worst change {} budget {}",
        ctx.resizer.network()->pathName(load.load_pin),
        cell->name(),
        delayAsString(computeDriverDelayDelta(ctx, load, cell),
                      3,
                      ctx.resizer.staState()),
        delayAsString(option.worst_delay_change, 3, ctx.resizer.staState()),
        delayAsString(load.delay_budget, 3, ctx.resizer.staState()));
  }

  sta::LibertyCell* scene_cell = cell->sceneCell(arc.scene, arc.min_max);
  const sta::LibertyPort* scene_port
      = scene_cell != nullptr
            ? scene_cell->findLibertyPort(load.load_port->name())
            : nullptr;
  if (current_scene_port != nullptr && scene_port != nullptr) {
    option.target_load_delta
        = scene_port->capacitance() - current_scene_port->capacitance();
  }
  return option;
}

// The measurements of one load and of the cell it steps down to; null when
// the load cannot change or no cell is smaller at no more area.
std::shared_ptr<const SizeDownFanoutMtLoad> measureLoad(
    const SizeDownFanoutContext& ctx,
    const SelectedArc& arc,
    sta::Vertex* load_vertex,
    const sta::Slack load_slack)
{
  SizeDownFanoutMtLoad load;
  if (!resolveLoad(ctx, load_vertex, load_slack, load)) {
    return nullptr;
  }

  load.delay_budget = computeDelayBudget(ctx, load);
  const sta::LibertyCellSeq swappable_cells = rankSwappableCells(ctx, load);
  logSwappableCells(ctx, load, swappable_cells);

  // Step down one drive strength: of the cells with a smaller input pin at no
  // more area, the largest pin, as size-up takes the next stronger cell.  One
  // step keeps the load change small, so it fits the delay budget more often.
  // >= picks the last of equal-capacitance cells, the one with the lower
  // intrinsic delay.
  sta::LibertyCell* step_down = nullptr;
  float step_down_cap = 0.0f;
  for (sta::LibertyCell* swappable : swappable_cells) {
    const float input_cap = candidateInputCap(ctx, load, swappable);
    if (swappable == load.current_cell || input_cap >= load.input_cap
        || swappable->area() > load.current_cell->area()) {
      continue;
    }
    if (step_down == nullptr || input_cap >= step_down_cap) {
      step_down = swappable;
      step_down_cap = input_cap;
    }
  }
  if (step_down == nullptr) {
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "size_down_fanout_move",
               3,
               "REJECT SizeDownFanoutMove {} -> {}: ({} -> {}) slack={}",
               ctx.resizer.network()->pathName(ctx.drvr_pin),
               ctx.resizer.network()->pathName(load.load_pin),
               load.current_cell->name(),
               "none",
               delayAsString(load.load_slack, 3, ctx.resizer.staState()));
    return nullptr;
  }

  const std::vector<SizeDownOutput> outputs = measureOutputs(ctx, load);
  sta::LibertyCell* current
      = load.current_cell->sceneCell(arc.scene, arc.min_max);
  const sta::LibertyPort* current_scene_port
      = current != nullptr ? current->findLibertyPort(load.load_port->name())
                           : nullptr;

  load.step_down = measureCellOption(
      ctx, load, outputs, current_scene_port, arc, step_down);
  return std::make_shared<const SizeDownFanoutMtLoad>(std::move(load));
}

std::vector<std::shared_ptr<const SizeDownFanoutMtLoad>> measureLoads(
    const SizeDownFanoutContext& ctx,
    const SelectedArc& arc)
{
  debugPrint(ctx.resizer.logger(),
             RSZ,
             "size_down_fanout_move",
             2,
             "sizing down for crit fanout {}",
             ctx.resizer.network()->pathName(ctx.drvr_pin));

  const std::vector<std::pair<sta::Vertex*, sta::Slack>> fanout_slacks
      = sortedFanoutSlacks(ctx);

  std::vector<std::shared_ptr<const SizeDownFanoutMtLoad>> loads;
  for (const auto& [load_vertex, load_slack] : fanout_slacks) {
    std::shared_ptr<const SizeDownFanoutMtLoad> load
        = measureLoad(ctx, arc, load_vertex, load_slack);
    if (load != nullptr) {
      loads.push_back(std::move(load));
    }
  }
  return loads;
}

// A positive budget bounds the delay change; zero or a negative one allows no
// growth.
bool fitsDelayBudget(const sta::Slack delay_budget,
                     const float worst_delay_change)
{
  return delay_budget > 0 ? worst_delay_change <= delay_budget
                          : worst_delay_change <= 0;
}

}  // namespace

SizeDownFanoutMtGenerator::SizeDownFanoutMtGenerator(
    const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool SizeDownFanoutMtGenerator::isApplicable(const Target& target) const
{
  return MoveGenerator::isApplicable(target)
         && target.estimator_context.has_value();
}

void SizeDownFanoutMtGenerator::prepareInputs(Target& target) const
{
  SizeDownFanoutContext ctx{
      .resizer = resizer_, .committer = committer_, .target = target};
  if (!resolveDriverContext(ctx)) {
    return;
  }

  target.size_down_fanout_loads
      = measureLoads(ctx, target.estimator_context->target().arc);
}

std::vector<std::unique_ptr<MoveCandidate>> SizeDownFanoutMtGenerator::generate(
    const Target& target)
{
  // One drive step down per load, when it keeps the output limits and fits
  // the load's delay budget.
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  for (const std::shared_ptr<const SizeDownFanoutMtLoad>& load :
       target.size_down_fanout_loads) {
    const SizeDownFanoutMtCellOption& step_down = load->step_down;
    if (step_down.violates_output_limits
        || !fitsDelayBudget(load->delay_budget, step_down.worst_delay_change)
        || !step_down.target_load_delta.has_value()) {
      continue;
    }

    candidates.push_back(std::make_unique<SizeDownFanoutMtCandidate>(
        resizer_,
        target,
        load,
        policy_config_.size_down_fanout_mt_score_scale));
  }
  return candidates;
}

}  // namespace rsz
