// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "DelayEstimatorReporter.hh"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "DelayEstimator.hh"
#include "OptimizerTypes.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/Scene.hh"
#include "sta/SearchClass.hh"
#include "sta/Sta.hh"
#include "sta/TimingArc.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"
#include "utl/env.h"

namespace rsz {

using utl::RSZ;

namespace {

// === Diagnostic-mode constants ============================================
constexpr float kSecondsToPicoseconds = 1.0e12f;
constexpr float kFaradsToFemtofarads = 1.0e15f;
// Reporter accepts at most 2 fanin/fanout levels.  Beyond that the table
// cascade approximation accumulates error faster than the diagnostic value.
constexpr int kMaxDelayLevels = 2;
// Returns NaN when the value should display as missing.  Reporter uses this
// uniformly so summaries and per-stage tables agree on what counts as "no
// data".
constexpr float kMissingValue = std::numeric_limits<float>::quiet_NaN();

// Shared "missing" placeholder for cell name columns; lets call sites bind
// to a const std::string& without per-iteration "-" allocation.
const std::string kDashCell = "-";

float finiteOrZero(float value)
{
  return std::isfinite(value) ? value : 0.0f;
}

std::string formatValue(float value, float scale)
{
  if (!std::isfinite(value)) {
    return "-";
  }
  return fmt::format("{:.3f}", value * scale);
}

std::string formatDelta(float estimated, float golden, float scale)
{
  if (!std::isfinite(estimated) || !std::isfinite(golden)) {
    return "-";
  }
  return fmt::format("{:+.3f}", (estimated - golden) * scale);
}

bool dmpSlewBiasEnabled()
{
  return utl::readEnvarInt("RSZ_MT_DMP_SLEW_BIAS", 0) > 0;
}

}  // namespace

// === Public name parsing (single source of truth for Tcl + C++) ===========

const std::vector<std::string_view>&
DelayEstimatorReporter::knownEstimatorNames()
{
  static const std::vector<std::string_view> kNames
      = {"legacy", "delay_estimator", "legacy_mt", "mt"};
  return kNames;
}

std::optional<DelayEstimatorReporter::EstimatorKind>
DelayEstimatorReporter::parseEstimatorKind(const std::string& estimator)
{
  if (estimator == "legacy") {
    return EstimatorKind::kLegacy;
  }
  if (estimator == "delay_estimator" || estimator == "legacy_mt"
      || estimator == "mt") {
    return EstimatorKind::kDelayEstimator;
  }
  return std::nullopt;
}

// === Stage-row scalar helpers =============================================

float DelayEstimatorReporter::estimatedStageScoreDelay(
    const StageProfileRow& row)
{
  return finiteOrZero(row.cell_delay) + finiteOrZero(row.extra_delay);
}

float DelayEstimatorReporter::goldenStagePathDelay(const StageProfileRow& row)
{
  return finiteOrZero(row.cell_delay) + finiteOrZero(row.wire_delay);
}

float DelayEstimatorReporter::sumEstimatedStageScoreDelays(
    const std::vector<StageProfileRow>& rows)
{
  float total = 0.0f;
  for (const StageProfileRow& row : rows) {
    total += estimatedStageScoreDelay(row);
  }
  return total;
}

float DelayEstimatorReporter::sumGoldenStagePathDelays(
    const std::vector<StageProfileRow>& rows)
{
  float total = 0.0f;
  for (const StageProfileRow& row : rows) {
    total += goldenStagePathDelay(row);
  }
  return total;
}

// === Construction =========================================================

DelayEstimatorReporter::DelayEstimatorReporter(Resizer& resizer)
    : resizer_(resizer),
      sta_(resizer.sta()),
      network_(resizer.network()),
      graph_(resizer.graph()),
      logger_(resizer.logger()),
      max_min_max_(resizer.maxAnalysisMode()),
      block_(resizer.block_)
{
}

// === Top-level orchestration ==============================================

void DelayEstimatorReporter::reportAccuracyForSizing(
    sta::Instance* inst,
    sta::LibertyCell* replacement,
    const std::string& estimator,
    const int delay_levels)
{
  // Tcl is the system boundary for argument validation.  Treat unrecognized
  // values here as a programming error.
  const std::optional<EstimatorKind> kind_opt = parseEstimatorKind(estimator);
  if (!kind_opt.has_value()) {
    logger_->error(RSZ,
                   3200,
                   "Unknown estimator '{}'.  Tcl wrapper should have rejected "
                   "this earlier.",
                   estimator);
  }
  const EstimatorKind kind = *kind_opt;

  resizer_.init();
  // Standalone reporter runs outside repair_timing, so initialize equivalence
  // data before DelayEstimator prepares candidate load ranges.
  resizer_.resizePreamble();
  // init() populates the active OpenDB block; refresh the cached pointer before
  // opening the ECO journal.
  block_ = resizer_.block_;
  sta_->findRequireds();

  const int normalized_levels = std::clamp(delay_levels, 0, kMaxDelayLevels);
  const int report_levels
      = kind == EstimatorKind::kDelayEstimator ? normalized_levels : 0;

  // Phase 1: locate the worst path stage driven by `inst`.
  const std::optional<TargetMatch> before_match
      = findWorstTargetForInstance(inst, report_levels);
  if (!before_match.has_value()) {
    logger_->error(RSZ,
                   3201,
                   "Could not find a setup timing path stage driven by "
                   "instance {}.",
                   network_->pathName(inst));
  }
  const Target& before_target = before_match->target;

  // Validate that the replacement cell exposes the original driver port.
  // replaceCell() is permissive about master swaps; without this check a
  // mismatched cell would crash later in measureGoldenAfterSwap when
  // findPin() returns null.
  sta::LibertyPort* driver_port
      = network_->libertyPort(before_target.driver_pin);
  if (replacement->findLibertyPort(driver_port->name()) == nullptr) {
    logger_->error(RSZ,
                   3209,
                   "Replacement cell {} does not expose driver port {} "
                   "required to swap instance {}.",
                   replacement->name(),
                   driver_port->name(),
                   network_->pathName(inst));
  }

  // Phase 2: estimator-side prediction (no DB mutation).
  const EstimatorProfile estimator_profile
      = kind == EstimatorKind::kLegacy
            ? buildLegacyProfile(before_target, replacement)
            : buildDelayEstimatorProfile(
                  before_target, replacement, normalized_levels);

  // Phase 3: golden-before snapshot (still no DB mutation).
  const sta::Scene* scene = before_target.activeScene(resizer_);
  const float before_arrival = arrivalAtDriver(before_target.driver_pin, scene);
  GoldenProfile golden_profile;
  golden_profile.before_arrival = before_arrival;
  golden_profile.after_arrival = before_arrival;
  const std::vector<FixedStage> fixed_stages
      = captureFixedStages(before_target, report_levels);
  golden_profile.before_stages = buildGoldenStageRows(fixed_stages);

  // Phase 4: apply the candidate inside an ECO journal so golden STA can be
  // measured, then unconditionally undo.  Resolved instance/pin handles must
  // be re-found post-swap because replaceCell may invalidate the old ones.
  const std::string inst_name = network_->pathName(inst);
  sta::LibertyCell* current_cell = network_->libertyCell(inst);
  bool replaced = false;
  {
    // Local class so its dtor inherits the enclosing function's friend access
    // to Resizer's protected init helpers.  Begin/undo bracket guarantees the
    // database is restored even if the diagnostic body throws.
    struct EcoJournalScope
    {
      DelayEstimatorReporter& reporter;
      explicit EcoJournalScope(DelayEstimatorReporter& r) : reporter(r)
      {
        odb::dbDatabase::beginEco(reporter.block_);
      }
      ~EcoJournalScope()
      {
        reporter.resizer_.initForJournalRestore();
        odb::dbDatabase::undoEco(reporter.block_);
        reporter.resizer_.updateParasiticsAndTiming();
      }
    };

    EcoJournalScope eco(*this);
    if (resizer_.replaceCell(inst, replacement)) {
      replaced = true;
      resizer_.updateParasiticsAndTiming();
      measureGoldenAfterSwap(*before_match, fixed_stages, golden_profile);
    }
  }

  if (!replaced) {
    logger_->error(RSZ,
                   3202,
                   "Could not replace instance {} from {} to {}.",
                   inst_name,
                   current_cell->name(),
                   replacement->name());
  }

  // Phase 5: print everything.
  printSummary(inst,
               replacement,
               *before_match,
               estimator,
               report_levels,
               estimator_profile,
               golden_profile);
}

// === Phase implementations ================================================

void DelayEstimatorReporter::measureGoldenAfterSwap(
    const TargetMatch& before_match,
    const std::vector<FixedStage>& fixed_stages,
    GoldenProfile& golden_profile) const
{
  const std::string before_inst_name
      = network_->pathName(before_match.target.inst(resizer_));
  sta::LibertyPort* driver_port
      = network_->libertyPort(before_match.target.driver_pin);
  const std::string driver_port_name = driver_port->name();
  const sta::Scene* scene = before_match.target.activeScene(resizer_);

  sta::Instance* live_inst = network_->findInstance(before_inst_name);
  if (live_inst == nullptr) {
    logger_->error(RSZ,
                   3211,
                   "Could not find instance {} after sizing ECO.",
                   before_inst_name);
  }

  sta::Pin* live_driver_pin = network_->findPin(live_inst, driver_port_name);
  if (live_driver_pin == nullptr) {
    logger_->error(RSZ,
                   3212,
                   "Could not find driver pin {} on instance {} after sizing "
                   "ECO.",
                   driver_port_name,
                   before_inst_name);
  }
  const float after_arrival = arrivalAtDriver(live_driver_pin, scene);

  // Use the exact stage identities captured before the ECO.  Re-selecting a
  // worst path here would mix estimator data from one path with golden data
  // from another path when the swap changes endpoint path ordering.
  golden_profile.after_stages = buildGoldenStageRows(fixed_stages);
  golden_profile.after_arrival = after_arrival;
  golden_profile.arrival_impr = golden_profile.before_arrival - after_arrival;
}

void DelayEstimatorReporter::printSummary(
    sta::Instance* inst,
    sta::LibertyCell* replacement,
    const TargetMatch& before_match,
    const std::string& estimator_label,
    const int report_levels,
    const EstimatorProfile& estimator_profile,
    const GoldenProfile& golden_profile) const
{
  const std::string inst_name = network_->pathName(inst);
  const std::string driver_pin_name
      = network_->pathName(before_match.target.driver_pin);
  sta::LibertyCell* current_cell = network_->libertyCell(inst);

  const float estimated_current_stage_delay
      = sumEstimatedStageScoreDelays(estimator_profile.current_stages);
  const float estimated_candidate_stage_delay
      = sumEstimatedStageScoreDelays(estimator_profile.candidate_stages);
  const float estimated_stage_impr
      = estimated_current_stage_delay - estimated_candidate_stage_delay;
  const float golden_before_stage_delay
      = sumGoldenStagePathDelays(golden_profile.before_stages);
  const float golden_after_stage_delay
      = sumGoldenStagePathDelays(golden_profile.after_stages);
  const float golden_stage_impr
      = golden_before_stage_delay - golden_after_stage_delay;

  logger_->report("Delay estimator sizing accuracy");
  logger_->report("  instance:          {}", inst_name);
  logger_->report("  driver pin:        {}", driver_pin_name);
  logger_->report("  endpoint path:     {}", before_match.endpoint_name);
  logger_->report("  current cell:      {}", current_cell->name());
  logger_->report("  replacement cell:  {}", replacement->name());
  logger_->report("  estimator:         {}", estimator_label);
  logger_->report("  delay levels:      {}", report_levels);
  logger_->report("  estimator legal:   {} ({})",
                  estimator_profile.legal ? "true" : "false",
                  failReasonName(estimator_profile.fail_reason));
  logger_->report("");
  logger_->report("Stage delay delta summary");
  logger_->report("  estimated current stage delay:   {:>10.3f} ps",
                  estimated_current_stage_delay * kSecondsToPicoseconds);
  if (estimator_profile.candidate_stages_incomplete) {
    // Estimator failed mid-walk; partial sums would mislead.  Show explicit
    // marker; per-stage tables below still display the prefix that succeeded.
    logger_->report(
        "  estimated candidate stage delay: (incomplete: {} of {} stages)",
        estimator_profile.candidate_stages.size(),
        estimator_profile.current_stages.size());
    logger_->report("  estimated stage delay delta:     (incomplete)");
  } else {
    logger_->report("  estimated candidate stage delay: {:>10.3f} ps",
                    estimated_candidate_stage_delay * kSecondsToPicoseconds);
    logger_->report("  estimated stage delay delta:     {:>10.3f} ps",
                    estimated_stage_impr * kSecondsToPicoseconds);
  }
  logger_->report("  golden before stage delay:       {:>10.3f} ps",
                  golden_before_stage_delay * kSecondsToPicoseconds);
  logger_->report("  golden after stage delay:        {:>10.3f} ps",
                  golden_after_stage_delay * kSecondsToPicoseconds);
  logger_->report("  golden stage delay delta:        {:>10.3f} ps",
                  golden_stage_impr * kSecondsToPicoseconds);
  if (estimator_profile.candidate_stages_incomplete) {
    logger_->report("  stage delay delta error:         (incomplete)");
  } else {
    logger_->report(
        "  stage delay delta error:          {:>+10.3f} ps",
        (estimated_stage_impr - golden_stage_impr) * kSecondsToPicoseconds);
  }
  logger_->report("");
  logger_->report("Driver arrival reference");
  logger_->report("  golden before arrival:           {:>10.3f} ps",
                  golden_profile.before_arrival * kSecondsToPicoseconds);
  logger_->report("  golden after arrival:            {:>10.3f} ps",
                  golden_profile.after_arrival * kSecondsToPicoseconds);
  logger_->report("  golden driver arrival delta:     {:>10.3f} ps",
                  golden_profile.arrival_impr * kSecondsToPicoseconds);

  printStageScoreBreakdown(estimator_profile.current_stages,
                           estimator_profile.candidate_stages,
                           golden_profile.before_stages,
                           golden_profile.after_stages);
  printStageComparison("Current stage absolute comparison",
                       estimator_profile.current_stages,
                       golden_profile.before_stages);
  printStageComparison("Candidate stage absolute comparison",
                       estimator_profile.candidate_stages,
                       golden_profile.after_stages);
}

// === Role helpers =========================================================

const char* DelayEstimatorReporter::roleName(const Role role)
{
  switch (role) {
    case Role::kFanin:
      return "fanin";
    case Role::kTarget:
      return "target";
    case Role::kFanout:
      return "fanout";
  }
  return "?";
}

DelayEstimatorReporter::Role DelayEstimatorReporter::roleFor(
    const int stage_index,
    const int target_index)
{
  if (stage_index < target_index) {
    return Role::kFanin;
  }
  if (stage_index == target_index) {
    return Role::kTarget;
  }
  return Role::kFanout;
}

// === Target selection =====================================================

std::vector<sta::Pin*> DelayEstimatorReporter::outputPins(
    sta::Instance* inst) const
{
  std::vector<sta::Pin*> pins;
  std::unique_ptr<sta::InstancePinIterator> pin_iter(
      network_->pinIterator(inst));
  while (pin_iter->hasNext()) {
    sta::Pin* pin = pin_iter->next();
    sta::PortDirection* direction = network_->direction(pin);
    if (direction != nullptr && direction->isAnyOutput()) {
      pins.push_back(pin);
    }
  }
  return pins;
}

std::optional<DelayEstimatorReporter::TargetMatch>
DelayEstimatorReporter::findWorstTargetForInstance(sta::Instance* inst,
                                                   const int delay_levels) const
{
  // For each output pin, ask STA directly for the worst-slack setup path
  // that traverses that pin's vertex.  vertexWorstSlackPath walks all path
  // tags reaching the vertex and picks the worst slack, so it works on
  // internal driver vertices, not just endpoints.
  std::optional<TargetMatch> best_match;
  for (sta::Pin* driver_pin : outputPins(inst)) {
    sta::Vertex* driver_vertex = graph_->pinDrvrVertex(driver_pin);
    if (driver_vertex == nullptr) {
      continue;
    }
    sta::Path* path = sta_->vertexWorstSlackPath(driver_vertex, max_min_max_);
    if (path == nullptr) {
      continue;
    }

    sta::PathExpanded expanded(path, resizer_.staState());
    // Locate driver_pin's stage in the expanded path.  vertexWorstSlackPath
    // guarantees driver_vertex is on the path; we additionally require a
    // valid prevArc/prevEdge so the stage can drive a sizing target.
    int driver_path_index = -1;
    for (int i = expanded.startIndex(); i < expanded.size(); ++i) {
      const sta::Path* stage_path = expanded.path(i);
      if (stage_path->pin(resizer_.staState()) == driver_pin
          && stage_path->prevArc(resizer_.staState()) != nullptr
          && stage_path->prevEdge(resizer_.staState()) != nullptr) {
        driver_path_index = i;
        break;
      }
    }
    if (driver_path_index < 0) {
      continue;
    }

    const sta::Slack slack = path->slack(resizer_.staState());
    const sta::Pin* endpoint_pin
        = expanded.path(expanded.size() - 1)->pin(resizer_.staState());
    TargetMatch match;
    match.target = makePathDriverTarget(
        path, expanded, driver_path_index, slack, resizer_);
    prepareTargetContext(match.target,
                         inst,
                         driver_pin,
                         expanded,
                         driver_path_index,
                         path,
                         delay_levels);
    match.endpoint_name
        = endpoint_pin != nullptr ? network_->pathName(endpoint_pin) : "<none>";
    match.slack = slack;

    if (!best_match.has_value() || match.slack < best_match->slack) {
      best_match = match;
    }
  }
  return best_match;
}

// === Profile construction =================================================

DelayEstimatorReporter::EstimatorProfile
DelayEstimatorReporter::buildLegacyProfile(const Target& target,
                                           sta::LibertyCell* replacement) const
{
  EstimatorProfile profile;

  const sta::Scene* scene = target.activeScene(resizer_);
  const sta::MinMax* min_max = target.minMax(resizer_);
  if (min_max == nullptr) {
    min_max = max_min_max_;
  }

  sta::Pin* driver_pin = target.driver_pin;
  const float load_cap
      = sta_->graphDelayCalc()->loadCap(driver_pin, scene, min_max);

  const sta::Path* input_path = target.inputPath(resizer_);
  sta::Pin* input_pin
      = input_path != nullptr ? input_path->pin(resizer_.staState()) : nullptr;
  sta::LibertyPort* input_port
      = input_pin != nullptr ? network_->libertyPort(input_pin) : nullptr;
  sta::LibertyPort* driver_port = network_->libertyPort(driver_pin);
  if (input_port == nullptr || driver_port == nullptr) {
    profile.fail_reason = FailReason::kMissingCurrentPortMap;
    return profile;
  }

  const int lib_ap = scene->libertyIndex(min_max);
  const sta::LibertyPort* scene_input_port
      = static_cast<const sta::LibertyPort*>(input_port)->scenePort(lib_ap);
  sta::LibertyCell* replacement_corner = replacement->sceneCell(lib_ap);
  sta::LibertyPort* replacement_driver_port
      = replacement_corner != nullptr
            ? replacement_corner->findLibertyPort(driver_port->name())
            : nullptr;
  sta::LibertyPort* replacement_input_port
      = replacement_corner != nullptr
            ? replacement_corner->findLibertyPort(input_port->name())
            : nullptr;
  if (scene_input_port == nullptr || replacement_driver_port == nullptr
      || replacement_input_port == nullptr) {
    profile.fail_reason = FailReason::kMissingCandidatePort;
    return profile;
  }

  // Resizer::driveResistance handles top-level input ports correctly via SDC.
  const sta::Path* prev_driver_path = target.prevDriverPath(resizer_);
  sta::Pin* prev_driver_pin = prev_driver_path != nullptr
                                  ? prev_driver_path->pin(resizer_.staState())
                                  : nullptr;
  const float prev_drive = prev_driver_pin != nullptr
                               ? resizer_.driveResistance(prev_driver_pin)
                               : 0.0f;
  const float current_cell_delay
      = resizer_.gateDelay(driver_port, load_cap, scene, min_max);
  const float current_fanin_penalty
      = prev_drive * scene_input_port->capacitance();
  const float candidate_cell_delay
      = resizer_.gateDelay(replacement_driver_port, load_cap, scene, min_max);
  const float candidate_fanin_penalty
      = prev_drive * replacement_input_port->capacitance();

  profile.current_delay = current_cell_delay + current_fanin_penalty;
  profile.candidate_delay = candidate_cell_delay + candidate_fanin_penalty;
  profile.arrival_impr = profile.current_delay - profile.candidate_delay;
  profile.legal = profile.arrival_impr > 0.0f;
  profile.fail_reason = profile.legal ? FailReason::kEstimateLegal
                                      : FailReason::kEstimateNonImproving;

  // Per-stage rows are best-effort.  If the context for input_slew capture
  // is unavailable, return the legal/fail_reason already computed above
  // and skip the optional stage tables instead of overwriting the result.
  const std::optional<ArcDelayState> context
      = contextForTarget(target, 0, nullptr);
  if (!context.has_value()) {
    return profile;
  }

  sta::PathExpanded expanded(target.endpoint_path, resizer_.staState());
  StageProfileRow current_row{
      .path_index = target.path_index,
      .role = Role::kTarget,
      .pin_name = stagePinName(expanded, target.path_index),
      .cell_name = driver_port->libertyCell()->name(),
      .input_slew = context->target().input_slew,
      .cell_delay = current_cell_delay,
      .wire_delay = pathWireDelay(expanded, target.path_index),
      .load_cap = load_cap,
      .output_slew = context->target().current_slew,
      .extra_delay = current_fanin_penalty};
  profile.current_stages.push_back(current_row);

  StageProfileRow candidate_row = current_row;
  candidate_row.cell_name = replacement->name();
  candidate_row.cell_delay = candidate_cell_delay;
  candidate_row.output_slew = kMissingValue;
  candidate_row.extra_delay = candidate_fanin_penalty;
  profile.candidate_stages.push_back(candidate_row);

  return profile;
}

DelayEstimatorReporter::StageProfileRow
DelayEstimatorReporter::buildEstimatedCurrentStageRow(
    const sta::PathExpanded& expanded,
    const DelayStageState& stage,
    const int stage_index,
    const int target_index) const
{
  return {.path_index = stage.path_index,
          .role = roleFor(stage_index, target_index),
          .pin_name = stagePinName(expanded, stage.path_index),
          .cell_name = stage.arc.currentCell()->name(),
          .input_slew = stage.input_slew,
          .cell_delay = stage.current_delay,
          .wire_delay = pathWireDelay(expanded, stage.path_index),
          .load_cap = stage.load_cap,
          .output_slew = stage.current_slew,
          .extra_delay = 0.0f};
}

DelayEstimatorReporter::EstimatorProfile
DelayEstimatorReporter::buildDelayEstimatorProfile(
    const Target& target,
    sta::LibertyCell* replacement,
    const int delay_levels) const
{
  EstimatorProfile profile;

  FailReason fail_reason = FailReason::kNone;
  const std::optional<ArcDelayState> context
      = contextForTarget(target, delay_levels, &fail_reason);
  if (!context.has_value()) {
    profile.fail_reason = fail_reason;
    return profile;
  }

  // Pass `&trace` so candidate_stages reflects exactly what estimateWindow
  // would compute.
  std::vector<StageEvaluation> trace;
  const DelayEstimate estimate
      = DelayEstimator::estimate(*context, replacement, &trace);
  profile.candidate_delay = estimate.candidate_delay;
  profile.arrival_impr = estimate.arrival_impr;
  profile.legal = estimate.legal;
  profile.fail_reason = estimate.reason;

  // path_stages always non-empty by construction; target_stage_index points
  // to the stage being swapped.
  const std::vector<DelayStageState>& stages = context->path_stages;
  const int target_index = context->target_stage_index;

  // current_delay is the sum of pre-swap stage delays across the captured
  // window (delay_levels=0 collapses to the single target stage).
  profile.current_delay = 0.0f;
  for (const DelayStageState& stage : stages) {
    profile.current_delay += stage.current_delay;
  }

  // Mark a partial trace so the summary suppresses misleading partial sums
  // (the per-stage tables are still emitted because they help diagnose
  // which stage the estimator failed at).
  profile.candidate_stages_incomplete
      = !estimate.legal && trace.size() < stages.size();

  // If the estimator failed before any stage was computed, skip the
  // per-stage tables entirely.
  if (!estimate.legal && trace.empty()) {
    return profile;
  }

  sta::PathExpanded expanded(target.endpoint_path, resizer_.staState());

  // current_stages: per-stage snapshot of the pre-swap context.
  profile.current_stages.reserve(stages.size());
  for (int stage_index = 0; stage_index < static_cast<int>(stages.size());
       ++stage_index) {
    profile.current_stages.push_back(buildEstimatedCurrentStageRow(
        expanded, stages[stage_index], stage_index, target_index));
  }

  // candidate_stages: pull per-stage compute directly from the trace.
  profile.candidate_stages.reserve(trace.size());
  for (int stage_index = 0; stage_index < static_cast<int>(trace.size());
       ++stage_index) {
    const StageEvaluation& eval = trace[stage_index];
    profile.candidate_stages.push_back(
        {.path_index = eval.path_index,
         .role = roleFor(stage_index, target_index),
         .pin_name = stagePinName(expanded, eval.path_index),
         .cell_name = eval.cell != nullptr ? eval.cell->name() : "-",
         .input_slew = eval.input_slew,
         .cell_delay = eval.stage_delay,
         .wire_delay = pathWireDelay(expanded, eval.path_index),
         .load_cap = eval.load_cap,
         .output_slew = eval.output_slew,
         .extra_delay = 0.0f});
  }

  return profile;
}

std::vector<DelayEstimatorReporter::FixedStage>
DelayEstimatorReporter::captureFixedStages(const Target& target,
                                           const int delay_levels) const
{
  std::vector<FixedStage> fixed_stages;
  FailReason fail_reason = FailReason::kNone;
  const std::optional<ArcDelayState> context
      = contextForTarget(target, delay_levels, &fail_reason);
  if (!context.has_value()) {
    return fixed_stages;
  }

  // path_stages always non-empty by construction; target_stage_index points
  // to the stage being captured.
  const std::vector<DelayStageState>& stages = context->path_stages;
  const int target_index = context->target_stage_index;
  sta::PathExpanded expanded(target.endpoint_path, resizer_.staState());
  fixed_stages.reserve(stages.size());
  for (int stage_index = 0; stage_index < static_cast<int>(stages.size());
       ++stage_index) {
    const DelayStageState& stage = stages[stage_index];
    const sta::Path* driver_path = expanded.path(stage.path_index);
    const sta::Path* input_path
        = driver_path != nullptr ? driver_path->prevPath() : nullptr;
    const sta::Path* next_path
        = stage.path_index + 1 < static_cast<int>(expanded.size())
              ? expanded.path(stage.path_index + 1)
              : nullptr;

    FixedStage fixed_stage;
    fixed_stage.state = stage;
    fixed_stage.role = roleFor(stage_index, target_index);
    fixed_stage.driver_pin_name = stagePinName(expanded, stage.path_index);

    sta::Pin* input_pin = input_path != nullptr
                              ? input_path->pin(resizer_.staState())
                              : nullptr;
    if (input_pin != nullptr) {
      fixed_stage.input_pin_name = network_->pathName(input_pin);
    }

    sta::Edge* next_prev_edge = next_path != nullptr
                                    ? next_path->prevEdge(resizer_.staState())
                                    : nullptr;
    sta::Pin* next_pin
        = next_path != nullptr ? next_path->pin(resizer_.staState()) : nullptr;
    if (next_prev_edge != nullptr && next_prev_edge->isWire()
        && next_pin != nullptr) {
      fixed_stage.next_pin_name = network_->pathName(next_pin);
    }

    fixed_stages.push_back(fixed_stage);
  }
  return fixed_stages;
}

std::vector<DelayEstimatorReporter::StageProfileRow>
DelayEstimatorReporter::buildGoldenStageRows(
    const std::vector<FixedStage>& fixed_stages) const
{
  std::vector<StageProfileRow> rows;
  rows.reserve(fixed_stages.size());
  for (const FixedStage& fixed_stage : fixed_stages) {
    rows.push_back(buildGoldenStageRow(fixed_stage));
  }
  return rows;
}

DelayEstimatorReporter::StageProfileRow
DelayEstimatorReporter::buildGoldenStageRow(const FixedStage& fixed_stage) const
{
  StageProfileRow row{.path_index = fixed_stage.state.path_index,
                      .role = fixed_stage.role,
                      .pin_name = fixed_stage.driver_pin_name,
                      .cell_name = kDashCell,
                      .input_slew = kMissingValue,
                      .cell_delay = kMissingValue,
                      .wire_delay = fixedStageWireDelay(fixed_stage),
                      .load_cap = kMissingValue,
                      .output_slew = kMissingValue,
                      .extra_delay = 0.0f};

  sta::Pin* input_pin = network_->findPin(fixed_stage.input_pin_name);
  sta::Pin* driver_pin = network_->findPin(fixed_stage.driver_pin_name);
  if (input_pin == nullptr || driver_pin == nullptr) {
    return row;
  }

  sta::LibertyPort* driver_port = network_->libertyPort(driver_pin);
  if (driver_port != nullptr && driver_port->libertyCell() != nullptr) {
    row.cell_name = driver_port->libertyCell()->name();
  }

  sta::Vertex* input_vertex = graph_->pinLoadVertex(input_pin);
  sta::Vertex* driver_vertex = graph_->pinDrvrVertex(driver_pin);
  if (input_vertex == nullptr || driver_vertex == nullptr
      || fixed_stage.state.arc.scene == nullptr
      || fixed_stage.state.arc.min_max == nullptr) {
    return row;
  }

  // Resolve the same conditional/non-unate timing arc family captured before
  // the ECO.  RF-only graph lookup can pick the wrong XOR/XNOR table.
  sta::Edge* gate_edge = nullptr;
  const sta::TimingArc* gate_arc = nullptr;
  sta::VertexInEdgeIterator edge_iter(driver_vertex, graph_);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    if (edge->from(graph_) != input_vertex) {
      continue;
    }
    const sta::TimingArc* matching_arc = findMatchingTimingArc(
        fixed_stage.state.arc.ref_arc, edge->timingArcSet());
    if (matching_arc != nullptr) {
      gate_edge = edge;
      gate_arc = matching_arc;
      break;
    }
  }
  if (gate_edge == nullptr || gate_arc == nullptr) {
    return row;
  }

  const int dcalc_ap = fixed_stage.state.arc.scene->dcalcAnalysisPtIndex(
      fixed_stage.state.arc.min_max);
  row.input_slew = sta_->graphDelayCalc()->edgeFromSlew(
      gate_edge->from(graph_),
      fixed_stage.state.arc.inputRiseFall(),
      gate_edge,
      fixed_stage.state.arc.scene,
      fixed_stage.state.arc.min_max);
  row.cell_delay
      = sta::delayAsFloat(graph_->arcDelay(gate_edge, gate_arc, dcalc_ap));
  row.load_cap = sta_->graphDelayCalc()->loadCap(
      driver_pin, fixed_stage.state.arc.scene, fixed_stage.state.arc.min_max);
  row.output_slew = sta::delayAsFloat(graph_->slew(
      driver_vertex, fixed_stage.state.arc.outputRiseFall(), dcalc_ap));
  return row;
}

// === Path / context utilities =============================================

std::optional<ArcDelayState> DelayEstimatorReporter::contextForTarget(
    const Target& target,
    const int delay_levels,
    FailReason* fail_reason) const
{
  if (target.arc_delay.has_value()
      && target.arc_delay->delay_estimation_levels == delay_levels) {
    return target.arc_delay;
  }
  return DelayEstimator::buildContext(
      resizer_, target, delay_levels, fail_reason, dmpSlewBiasEnabled());
}

void DelayEstimatorReporter::prepareTargetContext(
    Target& target,
    sta::Instance* inst,
    sta::Pin* driver_pin,
    const sta::PathExpanded& expanded,
    const int path_index,
    const sta::Path* endpoint_path,
    const int delay_levels) const
{
  // Snapshot the selected path context while the PathExpanded that found it
  // is still alive.  PathExpanded must outlive the captured path handles.
  const sta::Scene* scene = endpoint_path != nullptr
                                ? endpoint_path->scene(resizer_.staState())
                                : nullptr;
  const sta::MinMax* min_max = endpoint_path != nullptr
                                   ? endpoint_path->minMax(resizer_.staState())
                                   : nullptr;
  if (min_max == nullptr) {
    min_max = max_min_max_;
  }

  FailReason ignored_reason = FailReason::kNone;
  target.arc_delay = DelayEstimator::buildContext(resizer_,
                                                  inst,
                                                  driver_pin,
                                                  expanded,
                                                  path_index,
                                                  scene,
                                                  min_max,
                                                  delay_levels,
                                                  &ignored_reason,
                                                  dmpSlewBiasEnabled());
}

float DelayEstimatorReporter::pathWireDelay(const sta::PathExpanded& expanded,
                                            const int path_index) const
{
  if (path_index + 1 >= expanded.size()) {
    return 0.0f;
  }
  const sta::Path* driver_path = expanded.path(path_index);
  const sta::Path* next_path = expanded.path(path_index + 1);
  sta::Edge* next_prev_edge = next_path->prevEdge(resizer_.staState());
  if (next_prev_edge == nullptr || !next_prev_edge->isWire()) {
    return 0.0f;
  }
  return sta::delayAsFloat(sta::delayDiff(
      next_path->arrival(), driver_path->arrival(), resizer_.staState()));
}

float DelayEstimatorReporter::fixedStageWireDelay(
    const FixedStage& fixed_stage) const
{
  if (fixed_stage.next_pin_name.empty()) {
    return 0.0f;
  }

  sta::Pin* driver_pin = network_->findPin(fixed_stage.driver_pin_name);
  sta::Pin* next_pin = network_->findPin(fixed_stage.next_pin_name);
  if (driver_pin == nullptr || next_pin == nullptr
      || fixed_stage.state.arc.scene == nullptr
      || fixed_stage.state.arc.min_max == nullptr) {
    return kMissingValue;
  }

  sta::Vertex* driver_vertex = graph_->pinDrvrVertex(driver_pin);
  sta::Vertex* next_vertex = graph_->pinLoadVertex(next_pin);
  if (driver_vertex == nullptr || next_vertex == nullptr) {
    return kMissingValue;
  }

  const int dcalc_ap = fixed_stage.state.arc.scene->dcalcAnalysisPtIndex(
      fixed_stage.state.arc.min_max);
  sta::VertexOutEdgeIterator edge_iter(driver_vertex, graph_);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    if (edge->isWire() && edge->to(graph_) == next_vertex) {
      return sta::delayAsFloat(graph_->wireArcDelay(
          edge, fixed_stage.state.arc.outputRiseFall(), dcalc_ap));
    }
  }

  return kMissingValue;
}

std::string DelayEstimatorReporter::stagePinName(
    const sta::PathExpanded& expanded,
    const int path_index) const
{
  const sta::Path* path = expanded.path(path_index);
  sta::Pin* pin = path->pin(resizer_.staState());
  if (pin == nullptr) {
    return fmt::format("path_index:{}", path_index);
  }
  return network_->pathName(pin);
}

float DelayEstimatorReporter::arrivalAtDriver(sta::Pin* driver_pin,
                                              const sta::Scene* scene) const
{
  sta::Vertex* driver_vertex = graph_->pinDrvrVertex(driver_pin);
  const sta::SceneSeq scenes
      = sta_->makeSceneSeq(const_cast<sta::Scene*>(scene));
  return sta_->arrival(
      driver_vertex, sta::RiseFallBoth::riseFall(), scenes, max_min_max_);
}

// === Print helpers ========================================================

const DelayEstimatorReporter::StageProfileRow&
DelayEstimatorReporter::rowOrEmpty(const std::vector<StageProfileRow>& rows,
                                   const size_t index)
{
  static const StageProfileRow kEmpty;
  return index < rows.size() ? rows[index] : kEmpty;
}

void DelayEstimatorReporter::printStageComparison(
    const std::string& title,
    const std::vector<StageProfileRow>& estimated,
    const std::vector<StageProfileRow>& golden) const
{
  logger_->report("");
  logger_->report("{}", title);
  logger_->report(
      "idx role    path_index pin                                      "
      "est_cell                 golden_cell              "
      "in_slew_est/gold/err(ps)   out_slew_est/gold/err(ps)  "
      "cell_est/gold/err(ps)      "
      "wire_est/gold/err(ps)      load_est/gold/err(fF)      "
      "extra_est(ps)");

  const size_t row_count = std::max(estimated.size(), golden.size());
  for (size_t row_index = 0; row_index < row_count; ++row_index) {
    const StageProfileRow& estimated_row = rowOrEmpty(estimated, row_index);
    const StageProfileRow& golden_row = rowOrEmpty(golden, row_index);
    const bool has_estimated = row_index < estimated.size();
    const bool has_golden = row_index < golden.size();

    const Role role = has_estimated ? estimated_row.role : golden_row.role;
    const int path_index
        = has_estimated ? estimated_row.path_index : golden_row.path_index;
    const std::string& pin_name
        = has_estimated ? estimated_row.pin_name : golden_row.pin_name;
    const std::string& estimated_cell
        = has_estimated ? estimated_row.cell_name : kDashCell;
    const std::string& golden_cell
        = has_golden ? golden_row.cell_name : kDashCell;

    const float estimated_slew
        = has_estimated ? estimated_row.input_slew : kMissingValue;
    const float golden_slew
        = has_golden ? golden_row.input_slew : kMissingValue;
    const float estimated_output_slew
        = has_estimated ? estimated_row.output_slew : kMissingValue;
    const float golden_output_slew
        = has_golden ? golden_row.output_slew : kMissingValue;
    const float estimated_cell_delay
        = has_estimated ? estimated_row.cell_delay : kMissingValue;
    const float golden_cell_delay
        = has_golden ? golden_row.cell_delay : kMissingValue;
    const float estimated_wire_delay
        = has_estimated ? estimated_row.wire_delay : kMissingValue;
    const float golden_wire_delay
        = has_golden ? golden_row.wire_delay : kMissingValue;
    const float estimated_load
        = has_estimated ? estimated_row.load_cap : kMissingValue;
    const float golden_load = has_golden ? golden_row.load_cap : kMissingValue;
    const float estimated_extra
        = has_estimated ? estimated_row.extra_delay : kMissingValue;

    logger_->report(
        "{:>3} {:<7} {:>10} {:<40} {:<24} {:<24} "
        "{:>8}/{:>8}/{:>8}  {:>8}/{:>8}/{:>8}  "
        "{:>8}/{:>8}/{:>8}  "
        "{:>8}/{:>8}/{:>8}  {:>8}/{:>8}/{:>8}  {:>8}",
        row_index,
        roleName(role),
        path_index,
        pin_name,
        estimated_cell,
        golden_cell,
        formatValue(estimated_slew, kSecondsToPicoseconds),
        formatValue(golden_slew, kSecondsToPicoseconds),
        formatDelta(estimated_slew, golden_slew, kSecondsToPicoseconds),
        formatValue(estimated_output_slew, kSecondsToPicoseconds),
        formatValue(golden_output_slew, kSecondsToPicoseconds),
        formatDelta(
            estimated_output_slew, golden_output_slew, kSecondsToPicoseconds),
        formatValue(estimated_cell_delay, kSecondsToPicoseconds),
        formatValue(golden_cell_delay, kSecondsToPicoseconds),
        formatDelta(
            estimated_cell_delay, golden_cell_delay, kSecondsToPicoseconds),
        formatValue(estimated_wire_delay, kSecondsToPicoseconds),
        formatValue(golden_wire_delay, kSecondsToPicoseconds),
        formatDelta(
            estimated_wire_delay, golden_wire_delay, kSecondsToPicoseconds),
        formatValue(estimated_load, kFaradsToFemtofarads),
        formatValue(golden_load, kFaradsToFemtofarads),
        formatDelta(estimated_load, golden_load, kFaradsToFemtofarads),
        formatValue(estimated_extra, kSecondsToPicoseconds));
  }
}

void DelayEstimatorReporter::printStageScoreBreakdown(
    const std::vector<StageProfileRow>& estimated_current,
    const std::vector<StageProfileRow>& estimated_candidate,
    const std::vector<StageProfileRow>& golden_before,
    const std::vector<StageProfileRow>& golden_after) const
{
  logger_->report("");
  logger_->report("Stage delay delta comparison");
  logger_->report(
      "  estimated delay = cell_delay + extra_est; "
      "golden delay = cell_delay + wire_delay");
  logger_->report(
      "idx role    pin                                      "
      "est_cur/est_cand/est_impr(ps)  "
      "gold_cur/gold_cand/gold_impr(ps)  impr_err(ps)");

  const size_t est_row_count
      = std::max(estimated_current.size(), estimated_candidate.size());
  const size_t golden_row_count
      = std::max(golden_before.size(), golden_after.size());
  const size_t row_count = std::max(est_row_count, golden_row_count);
  for (size_t row_index = 0; row_index < row_count; ++row_index) {
    const StageProfileRow& estimated_current_row
        = rowOrEmpty(estimated_current, row_index);
    const StageProfileRow& estimated_candidate_row
        = rowOrEmpty(estimated_candidate, row_index);
    const StageProfileRow& golden_before_row
        = rowOrEmpty(golden_before, row_index);
    const StageProfileRow& golden_after_row
        = rowOrEmpty(golden_after, row_index);
    const bool has_estimated_current = row_index < estimated_current.size();
    const bool has_estimated_candidate = row_index < estimated_candidate.size();
    const bool has_golden_before = row_index < golden_before.size();
    const bool has_golden_after = row_index < golden_after.size();

    const Role role = has_estimated_candidate ? estimated_candidate_row.role
                                              : golden_after_row.role;
    const std::string& pin_name = has_estimated_candidate
                                      ? estimated_candidate_row.pin_name
                                      : golden_after_row.pin_name;

    const float estimated_current_delay
        = has_estimated_current
              ? estimatedStageScoreDelay(estimated_current_row)
              : kMissingValue;
    const float estimated_candidate_delay
        = has_estimated_candidate
              ? estimatedStageScoreDelay(estimated_candidate_row)
              : kMissingValue;
    const float golden_before_delay
        = has_golden_before ? goldenStagePathDelay(golden_before_row)
                            : kMissingValue;
    const float golden_after_delay
        = has_golden_after ? goldenStagePathDelay(golden_after_row)
                           : kMissingValue;
    const float estimated_impr
        = estimated_current_delay - estimated_candidate_delay;
    const float golden_impr = golden_before_delay - golden_after_delay;

    logger_->report(
        "{:>3} {:<7} {:<40} {:>8}/{:>8}/{:>8}  "
        "{:>8}/{:>8}/{:>8}  {:>8}",
        row_index,
        roleName(role),
        pin_name,
        formatValue(estimated_current_delay, kSecondsToPicoseconds),
        formatValue(estimated_candidate_delay, kSecondsToPicoseconds),
        formatValue(estimated_impr, kSecondsToPicoseconds),
        formatValue(golden_before_delay, kSecondsToPicoseconds),
        formatValue(golden_after_delay, kSecondsToPicoseconds),
        formatValue(golden_impr, kSecondsToPicoseconds),
        formatDelta(estimated_impr, golden_impr, kSecondsToPicoseconds));
  }
}

}  // namespace rsz
