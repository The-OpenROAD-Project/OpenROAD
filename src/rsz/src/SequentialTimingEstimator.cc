// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SequentialTimingEstimator.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

#include "DelayEstimator.hh"
#include "OptimizerTypes.hh"
#include "RepairSetupProfiler.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/ClkNetwork.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PocvMode.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/Sequential.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingModel.hh"
#include "sta/TimingRole.hh"
#include "sta/VisitPathEnds.hh"

namespace rsz {
namespace {

constexpr double kSideSlackWeightCutoff = 10.0e-12;
constexpr double kSideSlackViolationTolerance = 0.001e-12;

struct PreparedInputTiming
{
  const sta::MinMax* min_max{nullptr};
  std::optional<sta::Pvt> driver_pvt;
  std::optional<DelayStageState> driver;
  std::vector<const sta::GateTimingModel*> driver_gate_models;
  std::vector<float> candidate_connected_input_caps;
  float current_connected_input_cap{0.0f};
  float slew{0.0f};
  float delay_derate{1.0f};
  bool ideal_clock{false};
};

struct PreparedSetupHoldCheck
{
  const sta::MinMax* min_max{nullptr};
  std::optional<sta::Pvt> pvt;
  PreparedInputTiming data;
  PreparedInputTiming clock;
  std::optional<PreparedInputTiming> clock_check_slew_input;
  std::vector<const sta::CheckTimingModel*> candidate_check_models;
  float related_out_cap{0.0f};
  float baseline_check_margin{0.0f};
  float check_derate{1.0f};
  double baseline_slack{0.0};
  bool setup{false};
};

std::optional<sta::Pvt> copyPvt(const sta::Pvt* pvt)
{
  if (pvt == nullptr) {
    return std::nullopt;
  }
  return *pvt;
}

const sta::Pvt* pvtValue(const std::optional<sta::Pvt>& pvt)
{
  return pvt ? &*pvt : nullptr;
}

bool isFlipFlop(const sta::LibertyCell* cell)
{
  return cell != nullptr && !cell->sequentials().empty()
         && std::ranges::all_of(cell->sequentials(),
                                [](const sta::Sequential& sequential) {
                                  return sequential.isRegister();
                                });
}

const sta::TimingArc* matchingArc(const sta::TimingArc* reference,
                                  const sta::LibertyCell* cell)
{
  const sta::LibertyPort* from
      = cell->findLibertyPort(reference->from()->name());
  const sta::LibertyPort* to = cell->findLibertyPort(reference->to()->name());
  if (from == nullptr || to == nullptr) {
    return nullptr;
  }

  for (const sta::TimingArcSet* set : cell->timingArcSets(from, to)) {
    const sta::TimingArc* arc = findMatchingTimingArc(reference, set);
    if (arc != nullptr) {
      const sta::LibertyPort* related = reference->set()->relatedOut();
      const sta::LibertyPort* candidate_related = set->relatedOut();
      if ((related == nullptr) != (candidate_related == nullptr)
          || (related != nullptr
              && related->name() != candidate_related->name())) {
        continue;
      }
      return arc;
    }
  }

  return nullptr;
}

float pinCapacitance(sta::LibertyCell* cell,
                     std::string_view port_name,
                     const sta::Scene* scene,
                     const sta::RiseFall* rf,
                     const sta::MinMax* min_max,
                     const sta::Pvt* pvt)
{
  const float missing = std::numeric_limits<float>::quiet_NaN();
  const sta::LibertyCell* scene_cell = cell->sceneCell(scene, min_max);
  const sta::LibertyPort* port = scene_cell == nullptr
                                     ? nullptr
                                     : scene_cell->findLibertyPort(port_name);
  if (port == nullptr) {
    return missing;
  }

  // The unscaled lookup tells whether the port has a capacitance at all.
  bool exists = false;
  float unscaled_cap = 0.0f;
  port->capacitance(rf, min_max, unscaled_cap, exists);

  // The scaled-port overload can insert into a Liberty map: prepare only.
  const float cap = port->capacitance(
      rf, min_max, scene->sdc()->operatingConditions(min_max), pvt);
  return (exists || cap > 0.0f) && std::isfinite(cap) && cap >= 0.0f ? cap
                                                                     : missing;
}

bool tableDelay(const sta::GateTimingModel* model,
                const sta::Pvt* pvt,
                float slew,
                float cap,
                float& delay,
                float& output_slew)
{
  if (model == nullptr) {
    return false;
  }

  model->gateDelay(pvt, slew, cap, delay, output_slew);
  return std::isfinite(delay) && std::isfinite(output_slew)
         && output_slew >= 0.0f;
}

bool prepareInputCaps(Resizer& resizer,
                      sta::Instance* target_inst,
                      const sta::Path* path,
                      const sta::LibertyCellSeq& candidates,
                      PreparedInputTiming& context)
{
  const sta::StaState* sta = resizer.staState();
  const sta::Scene* scene = path->scene(sta);
  const sta::RiseFall* rf = path->transition(sta);
  const sta::Pvt* pvt
      = resizer.sta()->pvt(target_inst, context.min_max, scene->sdc());

  // Include every replaced input sharing this net, not just the selected D/CK.
  sta::Network* network = resizer.network();
  std::unique_ptr<sta::PinConnectedPinIterator> pins(
      network->connectedPinIterator(path->pin(sta)));

  int drivers = 0;
  bool found_load_port = false;
  while (pins->hasNext()) {
    const sta::Pin* pin = pins->next();
    drivers += network->isDriver(pin);
    if (network->instance(pin) != target_inst || !network->isLoad(pin)) {
      continue;
    }

    const sta::LibertyPort* port = network->libertyPort(pin);
    found_load_port = true;

    context.current_connected_input_cap += pinCapacitance(
        port->libertyCell(), port->name(), scene, rf, context.min_max, pvt);
    for (size_t index = 0; index < candidates.size(); ++index) {
      context.candidate_connected_input_caps[index] += pinCapacitance(
          candidates[index], port->name(), scene, rf, context.min_max, pvt);
    }
  }

  return drivers == 1 && found_load_port
         && std::isfinite(context.current_connected_input_cap);
}

// The gate driving the input on path: its table models at the current load
// and its delay derate, so estimate() can replay a load change.
bool prepareInputDriver(Resizer& resizer,
                        const sta::Path* path,
                        const sta::Path* driver_path,
                        const sta::MinMax* min_max,
                        PreparedInputTiming& context)
{
  const sta::StaState* sta = resizer.staState();
  const sta::Scene* scene = path->scene(sta);
  sta::Pin* driver_pin = driver_path->pin(sta);
  sta::Instance* driver_inst = resizer.network()->instance(driver_pin);
  sta::PathExpanded expanded(driver_path, sta);
  const std::optional<EstimatorContext> prepared
      = DelayEstimator::buildContext(resizer,
                                     driver_inst,
                                     driver_pin,
                                     expanded,
                                     expanded.size() - 1,
                                     scene,
                                     context.min_max,
                                     0);
  if (!prepared) {
    return false;
  }

  context.driver = prepared->target();
  DelayStageState& stage = *context.driver;
  context.driver_pvt = copyPvt(stage.arc.pvt);
  stage.load_cap = sta->graphDelayCalc()->loadCap(
      driver_pin, path->transition(sta), scene, min_max);
  context.driver_gate_models.push_back(
      stage.arc.ref_arc->gateModel(scene, min_max));
  if (!tableDelay(context.driver_gate_models.front(),
                  pvtValue(context.driver_pvt),
                  stage.input_slew,
                  stage.load_cap,
                  stage.current_model_delay,
                  stage.current_model_slew)) {
    return false;
  }

  for (OutputSlewMergeArc& arc : stage.output_slew_merge_arcs) {
    const sta::GateTimingModel* model = arc.ref_arc->gateModel(scene, min_max);
    float delay = 0.0f;
    if (!tableDelay(model,
                    pvtValue(context.driver_pvt),
                    arc.input_slew,
                    stage.load_cap,
                    delay,
                    arc.current_model_slew)) {
      return false;
    }

    context.driver_gate_models.push_back(model);
    if (min_max->compare(arc.current_model_slew, stage.current_model_slew)) {
      stage.current_model_slew = arc.current_model_slew;
    }
  }

  context.delay_derate = scene->sdc()->timingDerateInstance(
      driver_path->prevPath()->pin(sta),
      sta::TimingDerateCellType::cell_delay,
      driver_path->isClock(sta) ? sta::PathClkOrData::clk
                                : sta::PathClkOrData::data,
      stage.arc.inputRiseFall(),
      context.min_max);

  // Leave only immutable Liberty handles and numerical state in the prepared
  // timing.
  stage.arc.scene = nullptr;
  stage.arc.pvt = nullptr;
  stage.driver_pin = nullptr;

  return std::isfinite(stage.load_cap) && std::isfinite(context.delay_derate);
}

bool prepareInput(Resizer& resizer,
                  sta::Instance* target_inst,
                  const sta::Path* path,
                  float check_slew,
                  const sta::MinMax* min_max,
                  const sta::LibertyCellSeq& candidates,
                  PreparedInputTiming& context)
{
  const sta::StaState* sta = resizer.staState();
  const sta::Scene* scene = path->scene(sta);
  context.min_max = min_max;
  context.slew = check_slew;
  context.candidate_connected_input_caps.resize(candidates.size(), 0.0f);
  if (!std::isfinite(check_slew) || check_slew < 0.0f) {
    return false;
  }

  context.ideal_clock
      = path->isClock(sta)
        && scene->mode()->clkNetwork()->isIdealClock(path->pin(sta));
  if (context.ideal_clock) {
    return true;
  }

  if (!prepareInputCaps(resizer, target_inst, path, candidates, context)) {
    return false;
  }

  // A primary input without a gate model is usable only for unchanged load.
  const sta::Path* driver_path = path->prevPath();
  if (driver_path == nullptr || path->prevArc(sta) == nullptr
      || !path->prevArc(sta)->role()->isWire()) {
    return true;
  }

  sta::Instance* driver_inst
      = resizer.network()->instance(driver_path->pin(sta));
  if (driver_inst == target_inst) {
    return false;
  }

  if (driver_path->prevArc(sta) == nullptr) {
    return true;
  }

  return prepareInputDriver(resizer, path, driver_path, min_max, context);
}

bool checkMargin(const sta::CheckTimingModel* model,
                 const PreparedSetupHoldCheck& context,
                 float data_slew,
                 float clock_slew,
                 float& margin)
{
  if (model == nullptr) {
    return false;
  }

  // Liberty check arcs run from related clock to constrained data, in that
  // order.
  margin = sta::delayAsFloat(model->checkDelay(pvtValue(context.pvt),
                                               clock_slew,
                                               data_slew,
                                               context.related_out_cap,
                                               context.min_max,
                                               sta::PocvMode::scalar));
  return std::isfinite(margin);
}

bool prepareSetupHoldCheck(Resizer& resizer,
                           sta::Instance* inst,
                           const sta::PathEnd* end,
                           const sta::LibertyCellSeq& candidates,
                           PreparedSetupHoldCheck& context)
{
  const sta::StaState* sta = resizer.staState();
  const sta::Path* data_path = end->path();
  const sta::Path* clock_path = end->targetClkPath();
  const sta::Scene* scene = data_path->scene(sta);
  context.min_max = data_path->minMax(sta);
  const sta::LibertyCell* cell
      = resizer.network()->libertyCell(inst)->sceneCell(scene, context.min_max);
  if (cell == nullptr || clock_path == nullptr) {
    return false;
  }

  const sta::TimingArc* check_arc = matchingArc(end->checkArc(), cell);
  if (check_arc == nullptr) {
    return false;
  }

  context.pvt
      = copyPvt(resizer.sta()->pvt(inst, context.min_max, scene->sdc()));
  context.setup = check_arc->role() == sta::TimingRole::setup();
  context.baseline_slack = sta::delayAsFloat(end->slack(sta));
  context.check_derate = scene->sdc()->timingDerateInstance(
      clock_path->pin(sta),
      sta::TimingDerateCellType::cell_check,
      sta::PathClkOrData::data,
      data_path->transition(sta),
      context.min_max);
  for (sta::LibertyCell* candidate : candidates) {
    const sta::LibertyCell* candidate_cell
        = candidate->sceneCell(scene, context.min_max);
    const sta::TimingArc* arc = candidate_cell == nullptr
                                    ? nullptr
                                    : matchingArc(check_arc, candidate_cell);
    context.candidate_check_models.push_back(
        arc == nullptr ? nullptr : arc->checkModel(scene, context.min_max));
  }

  const sta::ClkNetwork* clock_network = scene->mode()->clkNetwork();
  const sta::RiseFall* clock_rf = check_arc->fromEdge()->asRiseFall();
  const float clock_slew
      = clock_network->isIdealClock(clock_path->pin(sta))
            ? clock_network->idealClkSlew(
                  clock_path->pin(sta),
                  clock_rf,
                  scene->checkClkSlewMinMax(context.min_max))
            : sta::delayAsFloat(resizer.graph()->slew(
                  clock_path->vertex(sta),
                  clock_rf,
                  scene->checkClkSlewIndex(context.min_max)));
  const float data_slew = sta::delayAsFloat(data_path->slew(sta));
  const sta::MinMax* clock_min_max = clock_path->minMax(sta);
  const sta::MinMax* waveform_min_max
      = scene->checkClkSlewMinMax(context.min_max);
  if (!prepareInput(resizer,
                    inst,
                    data_path,
                    data_slew,
                    context.min_max,
                    candidates,
                    context.data)
      || !prepareInput(resizer,
                       inst,
                       clock_path,
                       clock_slew,
                       clock_min_max,
                       candidates,
                       context.clock)) {
    return false;
  }

  // The check-slew analysis reuses the fixed clock topology, not its arrival
  // sense.  Its delay delta never replaces the capture-arrival delta.
  if (waveform_min_max != clock_min_max && !context.clock.ideal_clock) {
    context.clock_check_slew_input.emplace();
    if (!prepareInput(resizer,
                      inst,
                      clock_path,
                      clock_slew,
                      waveform_min_max,
                      candidates,
                      *context.clock_check_slew_input)) {
      return false;
    }
  }

  const sta::LibertyPort* related_out = check_arc->set()->relatedOut();
  if (related_out != nullptr) {
    const sta::Pin* pin = resizer.network()->findPin(inst, related_out->name());
    if (pin == nullptr) {
      return false;
    }

    context.related_out_cap = sta->graphDelayCalc()->loadCap(
        pin, data_path->transition(sta), scene, context.min_max);
  }

  return std::isfinite(context.baseline_slack)
         && std::isfinite(context.check_derate)
         && std::isfinite(context.related_out_cap)
         && checkMargin(end->checkArc()->checkModel(scene, context.min_max),
                        context,
                        data_slew,
                        clock_slew,
                        context.baseline_check_margin);
}

class SetupHoldCheckCollector : public sta::PathEndVisitor
{
 public:
  SetupHoldCheckCollector(Resizer& resizer,
                          sta::Instance* inst,
                          const sta::LibertyCellSeq& candidates,
                          std::vector<PreparedSetupHoldCheck>& checks,
                          std::string_view& failure_reason)
      : resizer_(resizer),
        inst_(inst),
        candidates_(candidates),
        checks_(checks),
        failure_reason_(failure_reason)
  {
  }

  sta::PathEndVisitor* copy() const override
  {
    return new SetupHoldCheckCollector(*this);
  }

  void visit(sta::PathEnd* end) override
  {
    if (!failure_reason_.empty() || !end->isCheck()) {
      return;
    }

    const sta::TimingRole* role = end->checkArc()->role();
    if (role != sta::TimingRole::setup() && role != sta::TimingRole::hold()) {
      return;
    }

    PreparedSetupHoldCheck check;
    if (!prepareSetupHoldCheck(resizer_, inst_, end, candidates_, check)) {
      failure_reason_ = "sequential_missing_check_context";
      return;
    }

    checks_.push_back(std::move(check));
  }

 private:
  Resizer& resizer_;
  sta::Instance* inst_;
  const sta::LibertyCellSeq& candidates_;
  std::vector<PreparedSetupHoldCheck>& checks_;
  std::string_view& failure_reason_;
};

bool estimateInputTimingForCap(const PreparedInputTiming& context,
                               float candidate_connected_input_cap,
                               double& arrival_delta,
                               float& slew)
{
  arrival_delta = 0.0;
  slew = context.slew;
  if (context.ideal_clock) {
    return true;
  }

  if (!std::isfinite(candidate_connected_input_cap)) {
    return false;
  }

  const float delta_cap
      = candidate_connected_input_cap - context.current_connected_input_cap;
  if (delta_cap == 0.0f) {
    return true;
  }

  if (!context.driver) {
    return false;
  }

  const DelayStageState& driver = *context.driver;
  const float load_cap = driver.load_cap + delta_cap;
  float delay = 0.0f;
  float model_slew = 0.0f;
  if (!std::isfinite(load_cap) || load_cap < 0.0f
      || !tableDelay(context.driver_gate_models.front(),
                     pvtValue(context.driver_pvt),
                     driver.input_slew,
                     load_cap,
                     delay,
                     model_slew)) {
    return false;
  }

  for (size_t index = 0; index < driver.output_slew_merge_arcs.size();
       ++index) {
    const OutputSlewMergeArc& arc = driver.output_slew_merge_arcs[index];
    float merge_delay = 0.0f;
    float merge_slew = 0.0f;
    if (!tableDelay(context.driver_gate_models[index + 1],
                    pvtValue(context.driver_pvt),
                    arc.input_slew,
                    load_cap,
                    merge_delay,
                    merge_slew)) {
      return false;
    }

    if (context.min_max->compare(merge_slew, model_slew)) {
      model_slew = merge_slew;
    }
  }

  // Preserve the measured wire contribution; only the driver table delta moves.
  arrival_delta = (static_cast<double>(delay) - driver.current_model_delay)
                  * context.delay_derate;
  slew = std::max(0.0f, context.slew + model_slew - driver.current_model_slew);
  return std::isfinite(arrival_delta) && std::isfinite(slew);
}

}  // namespace

struct SequentialTimingEstimator::PreparedFlipFlopTiming
{
  sta::LibertyCellSeq candidates;
  std::vector<PreparedSetupHoldCheck> checks;
  std::string_view prepare_failure_reason;
};

SequentialTimingEstimator::SequentialTimingEstimator(
    std::shared_ptr<const PreparedFlipFlopTiming> prepared_timing)
    : prepared_timing_(std::move(prepared_timing))
{
}

std::shared_ptr<const SequentialTimingEstimator>
SequentialTimingEstimator::prepare(Resizer& resizer, const Target& target)
{
  sta::Instance* inst = target.inst(resizer);
  sta::LibertyCell* cell
      = inst != nullptr ? resizer.network()->libertyCell(inst) : nullptr;
  if (!isFlipFlop(cell)) {
    return nullptr;
  }

  const ProfileScope profile("prepare_sequential");
  std::shared_ptr<PreparedFlipFlopTiming> prepared_timing
      = std::make_shared<PreparedFlipFlopTiming>();
  const sta::LibertyPort* output
      = resizer.network()->libertyPort(target.resolvedPin(resizer));

  bool has_clock_to_q = false;
  for (const sta::TimingArcSet* set : cell->timingArcSetsTo(output)) {
    has_clock_to_q |= set->role() == sta::TimingRole::regClkToQ();
  }

  if (!has_clock_to_q || resizer.sta()->pocvMode() != sta::PocvMode::scalar) {
    prepared_timing->prepare_failure_reason
        = "sequential_unsupported_target_context";
  } else {
    std::set<sta::LibertyCell*> candidates{cell};
    for (sta::LibertyCell* candidate : resizer.getSwappableCells(cell)) {
      if (isFlipFlop(candidate)) {
        candidates.insert(candidate);
      }
    }

    for (sta::LibertyCell* candidate : resizer.getVTEquivCells(cell)) {
      if (isFlipFlop(candidate)) {
        candidates.insert(candidate);
      }
    }

    prepared_timing->candidates.assign(candidates.begin(), candidates.end());

    // Liberty roles identify constrained inputs, including scan-data inputs.
    std::set<const sta::LibertyPort*> data_ports;
    for (const sta::TimingArcSet* set : cell->timingArcSets()) {
      if (set->role() == sta::TimingRole::setup()
          || set->role() == sta::TimingRole::hold()) {
        data_ports.insert(set->to());
      }
    }

    SetupHoldCheckCollector visitor(resizer,
                                    inst,
                                    prepared_timing->candidates,
                                    prepared_timing->checks,
                                    prepared_timing->prepare_failure_reason);

    sta::VisitPathEnds visit_ends(resizer.staState());
    for (const sta::LibertyPort* port : data_ports) {
      const sta::Pin* pin = resizer.network()->findPin(inst, port->name());
      sta::Vertex* vertex
          = pin == nullptr ? nullptr : resizer.graph()->pinLoadVertex(pin);
      if (vertex == nullptr) {
        prepared_timing->prepare_failure_reason = "sequential_missing_data_pin";
        break;
      }

      visit_ends.visitPathEnds(vertex, &visitor);
      if (!prepared_timing->prepare_failure_reason.empty()) {
        break;
      }
    }

    if (data_ports.empty() && prepared_timing->prepare_failure_reason.empty()) {
      prepared_timing->prepare_failure_reason
          = "sequential_missing_check_context";
    }
  }

  RepairSetupProfiler::event("sequential_active_checks",
                             prepared_timing->checks.size());
  RepairSetupProfiler::event("sequential_candidate_cells_prepared",
                             prepared_timing->candidates.size());
  return std::shared_ptr<const SequentialTimingEstimator>(
      new SequentialTimingEstimator(std::move(prepared_timing)));
}

SequentialTimingEstimator::Result SequentialTimingEstimator::estimate(
    const sta::LibertyCell* candidate_cell) const
{
  if (!prepared_timing_->prepare_failure_reason.empty()) {
    return {.reason = prepared_timing_->prepare_failure_reason};
  }

  const sta::LibertyCellSeq::const_iterator candidate
      = std::ranges::find(prepared_timing_->candidates, candidate_cell);
  if (candidate == prepared_timing_->candidates.end()) {
    return {.reason = "sequential_missing_candidate_cell"};
  }

  const size_t index = candidate - prepared_timing_->candidates.begin();
  Result result{.legal = true, .reason = "sequential_estimate_legal"};
  for (const PreparedSetupHoldCheck& check : prepared_timing_->checks) {
    double data_delta = 0.0;
    double clock_delta = 0.0;
    float data_slew = 0.0f;
    float clock_slew = 0.0f;
    float candidate_check_margin = 0.0f;
    if (!estimateInputTimingForCap(
            check.data,
            check.data.candidate_connected_input_caps[index],
            data_delta,
            data_slew)
        || !estimateInputTimingForCap(
            check.clock,
            check.clock.candidate_connected_input_caps[index],
            clock_delta,
            clock_slew)) {
      return {.reason = "sequential_missing_candidate_model"};
    }

    if (check.clock_check_slew_input) {
      double unused_arrival_delta = 0.0;
      const PreparedInputTiming& clock_check_slew_input
          = *check.clock_check_slew_input;
      if (!estimateInputTimingForCap(
              clock_check_slew_input,
              clock_check_slew_input.candidate_connected_input_caps[index],
              unused_arrival_delta,
              clock_slew)) {
        return {.reason = "sequential_missing_candidate_model"};
      }
    }

    if (!checkMargin(check.candidate_check_models[index],
                     check,
                     data_slew,
                     clock_slew,
                     candidate_check_margin)) {
      return {.reason = "sequential_missing_candidate_model"};
    }

    const double margin_delta = (static_cast<double>(candidate_check_margin)
                                 - check.baseline_check_margin)
                                * check.check_derate;
    // Setup = clock - data - setup; hold = data - clock - hold.
    // Signed Liberty constraints (including negative hold) keep the same signs.
    const double slack_delta
        = (check.setup ? clock_delta - data_delta : data_delta - clock_delta)
          - margin_delta;
    const double after = check.baseline_slack + slack_delta;
    const double loss = std::max(0.0, -slack_delta);
    const double weight = std::clamp(
        1.0 - std::min(check.baseline_slack, after) / kSideSlackWeightCutoff,
        0.0,
        1.0);
    result.weighted_side_slack_loss = std::max(
        result.weighted_side_slack_loss, static_cast<float>(weight * loss));
    result.max_side_slack_loss
        = std::max(result.max_side_slack_loss, static_cast<float>(loss));
    if (after
        < std::min(0.0, check.baseline_slack) - kSideSlackViolationTolerance) {
      result.legal = false;
      result.reason = check.setup ? "sequential_setup_violation"
                                  : "sequential_hold_violation";
    }
  }

  return result;
}

}  // namespace rsz
