// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "DelayEstimatorReporter.hh"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "DelayEstimator.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizationPolicy.hh"
#include "OptimizationPolicyConfig.hh"
#include "OptimizerTypes.hh"
#include "Rebuffer.hh"
#include "RebufferMtCandidate.hh"
#include "RebufferMtGenerator.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/SearchClass.hh"
#include "sta/Sta.hh"
#include "sta/StringUtil.hh"
#include "sta/TimingArc.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

namespace {

// === Diagnostic-mode constants ============================================
constexpr float kSecondsToPicoseconds = 1.0e12f;
constexpr float kFaradsToFemtofarads = 1.0e15f;
// Reporter accepts at most 2 fanin/fanout levels.  Beyond that the table
// cascade approximation accumulates error faster than the diagnostic value.
constexpr int kMaxDelayLevels = 2;
// Diagnostic path search is filtered by the target output pins, so a generous
// path count is affordable and avoids missing the worst endpoint path through
// the selected instance.
constexpr int kReporterPathSearchCount = 1000;
constexpr float kTinyStageDelayComparison = 1.0e-12f;
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

// The stages of the plan's target load path; empty when the tree does not
// reach the target load.
const std::vector<BufferTreePathStage>& targetPathStages(
    const RebufferTreePlan& plan)
{
  static const std::vector<BufferTreePathStage> kNoStages;
  const BufferTreeLoadPath* target_path = plan.targetPath();
  return target_path != nullptr ? target_path->stages : kNoStages;
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

std::string formatWithUnit(float value, float scale, std::string_view unit)
{
  if (!std::isfinite(value)) {
    return "-";
  }
  return fmt::format("{:.3f} {}", value * scale, unit);
}

std::string formatDeltaWithUnit(float estimated,
                                float golden,
                                float scale,
                                std::string_view unit)
{
  if (!std::isfinite(estimated) || !std::isfinite(golden)) {
    return "-";
  }
  return fmt::format("{:+.3f} {}", (estimated - golden) * scale, unit);
}

// The report measures the estimator with the policies' STA slew bias.
constexpr bool kStaSlewBias = OptimizationPolicyConfig{}.sta_slew_bias;

bool isTinyDelayComparisonMetric(const std::string& metric)
{
  return metric.find("wire delay") != std::string::npos
         || metric.find("delay delta") != std::string::npos;
}

void printMetricRow(utl::Logger* logger,
                    const std::string& metric,
                    const float estimated,
                    const float golden,
                    const float scale)
{
  const bool tiny_delay_comparison
      = isTinyDelayComparisonMetric(metric) && std::isfinite(estimated)
        && std::isfinite(golden)
        && std::abs(estimated) < kTinyStageDelayComparison
        && std::abs(golden) < kTinyStageDelayComparison;
  if (tiny_delay_comparison) {
    logger->report(
        "    {:<31} {:>10}   {:>10}   {:>10}", metric, "-", "-", "-");
    return;
  }

  logger->report("    {:<31} {:>10}   {:>10}   {:>10}",
                 metric,
                 formatValue(estimated, scale),
                 formatValue(golden, scale),
                 formatDelta(estimated, golden, scale));
}

// The setup path ends through any of through_pins, up to count per path
// group, sorted by slack.
sta::PathEndSeq findPathEndsThrough(sta::dbSta* sta,
                                    const std::vector<sta::Pin*>& through_pins,
                                    const sta::MinMaxAll* min_max_all,
                                    const int count)
{
  // The search owns the exception objects.
  sta::PinSet* pins = new sta::PinSet(sta->network());
  for (sta::Pin* pin : through_pins) {
    pins->insert(pin);
  }

  sta::ExceptionThru* through = sta->makeExceptionThru(
      pins, nullptr, nullptr, sta::RiseFallBoth::riseFall(), sta->cmdSdc());

  sta::ExceptionThruSeq* thrus = new sta::ExceptionThruSeq;
  thrus->push_back(through);
  sta::StringSeq group_names;

  return sta->findPathEnds(nullptr,
                           thrus,
                           nullptr,
                           false,
                           sta->scenes(),
                           min_max_all,
                           count,
                           count,
                           false,
                           false,
                           -sta::INF,
                           sta::INF,
                           true,
                           group_names,
                           true,
                           false,
                           false,
                           false,
                           false,
                           false);
}

}  // namespace

// === Public name parsing (single source of truth for Tcl + C++) ===========

const std::vector<std::string_view>&
DelayEstimatorReporter::knownEstimatorNames()
{
  static const std::vector<std::string_view> kNames
      = {"legacy", "stage_window"};
  return kNames;
}

std::optional<DelayEstimatorReporter::EstimatorKind>
DelayEstimatorReporter::parseEstimatorKind(const std::string& estimator)
{
  if (estimator == "legacy") {
    return EstimatorKind::kLegacy;
  }

  if (estimator == "stage_window") {
    return EstimatorKind::kStageWindow;
  }
  return std::nullopt;
}

// === Stage-row scalar helpers =============================================

float DelayEstimatorReporter::estimatedStageScoreDelay(
    const StageProfileRow& row)
{
  const float extra_delay = finiteOrZero(row.extra_delay);
  const float wire_delay
      = extra_delay != 0.0f ? extra_delay : finiteOrZero(row.wire_delay);
  return finiteOrZero(row.cell_delay) + wire_delay;
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
    if (!row.valid) {
      continue;
    }
    total += estimatedStageScoreDelay(row);
  }
  return total;
}

float DelayEstimatorReporter::sumGoldenStagePathDelays(
    const std::vector<StageProfileRow>& rows)
{
  float total = 0.0f;
  for (const StageProfileRow& row : rows) {
    if (!row.valid) {
      continue;
    }
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

DelayEstimatorReporter::EcoJournalScope::EcoJournalScope(
    DelayEstimatorReporter& reporter)
    : reporter_(reporter)
{
  odb::dbDatabase::beginEco(reporter_.block_);
}

DelayEstimatorReporter::EcoJournalScope::~EcoJournalScope()
{
  try {
    reporter_.resizer_.initForJournalRestore();
    odb::dbDatabase::undoEco(reporter_.block_);
    reporter_.resizer_.updateParasiticsAndTiming();
  } catch (...) {
    // Destructors must not throw during diagnostic ECO cleanup, but the trial
    // edits may still be in the design.
    reporter_.logger_->warn(RSZ,
                            3212,
                            "The accuracy report could not undo its trial "
                            "edits; the design may keep them.");
  }
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
      = kind == EstimatorKind::kStageWindow ? normalized_levels : 0;

  // Phase 1: locate the worst path stage driven by `inst`.
  const std::optional<TargetMatch> before_match
      = findWorstTargetForInstance(inst, report_levels);
  if (!before_match.has_value()) {
    errorNoSetupPathStage(inst);
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

  // Phase 3: golden-before snapshot (still no DB mutation).  Capture fixed
  // path stages before arrivalAtDriver(), which may trigger a STA search
  // preamble and invalidate through-filtered PathEnd handles.
  const std::vector<FixedStage> fixed_stages
      = captureFixedStages(before_target, report_levels);
  const sta::Scene* scene = before_target.activeScene(resizer_);
  GoldenProfile golden_profile;
  golden_profile.arrival_reference_pin_name
      = fixed_stages.empty() ? network_->pathName(before_target.driver_pin)
                             : fixed_stages.back().driver_pin_name;
  const float before_arrival
      = arrivalAtPinName(golden_profile.arrival_reference_pin_name, scene);
  golden_profile.before_arrival = before_arrival;
  golden_profile.after_arrival = before_arrival;
  golden_profile.before_stages = buildGoldenStageRows(fixed_stages);

  // Phase 4: apply the candidate inside an ECO journal so golden STA can be
  // measured, then unconditionally undo.  Resolved instance/pin handles must
  // be re-found post-swap because replaceCell may invalidate the old ones.
  const std::string inst_name = network_->pathName(inst);
  sta::LibertyCell* current_cell = network_->libertyCell(inst);
  bool replaced = false;
  {
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

void DelayEstimatorReporter::reportAccuracyForRebufferTree(
    sta::Instance* inst,
    sta::Pin* driver_pin,
    const int delay_levels)
{
  // -driver_pin names an output pin of -inst.
  if (driver_pin != nullptr
      && (network_->instance(driver_pin) != inst
          || !network_->isDriver(driver_pin))) {
    logger_->error(RSZ,
                   3211,
                   "-driver_pin {} is not an output pin of instance {}.",
                   network_->pathName(driver_pin),
                   network_->pathName(inst));
  }

  resizer_.init();
  resizer_.resizePreamble();
  block_ = resizer_.block_;
  sta_->findRequireds();

  const int report_levels = std::clamp(delay_levels, 0, kMaxDelayLevels);
  const std::optional<TargetMatch> before_match
      = findWorstTargetForInstance(inst, report_levels, driver_pin);
  if (!before_match.has_value()) {
    errorNoSetupPathStage(inst);
  }

  // The match is a through-filtered path end, which the next search frees,
  // so the stages come off the path before the context build runs STA.
  const std::vector<FixedStage> fixed_stages
      = captureFixedStages(before_match->target, report_levels);
  if (fixed_stages.empty()) {
    logger_->error(
        RSZ, 3235, "Could not capture fixed stages for RebufferMt accuracy.");
  }

  const sta::Pin* target_load_pin = before_match->target.load_pin;
  if (target_load_pin == nullptr) {
    logger_->error(RSZ,
                   3241,
                   "Could not find target load pin for Rebuffer tree "
                   "accuracy on {}.",
                   network_->pathName(before_match->target.driver_pin));
  }

  const std::string target_load_pin_name = network_->pathName(target_load_pin);

  OptimizationPolicyConfig policy_config
      = OptimizationPolicyConfig::forParallelEval(RepairEffort::kHigh);
  policy_config.delay_estimation_levels = report_levels;
  const std::optional<EstimatorContext> context
      = rebufferReportContext(before_match->target, policy_config);

  RebufferReportPlan report_plan;
  planRebufferReport(
      before_match->target, *context, policy_config, report_plan);
  const RebufferTreePlan& plan = report_plan.best->plan();
  const std::vector<BufferTreeSidePathContext>& side_contexts
      = report_plan.best->sideContexts();

  const sta::Scene* scene = before_match->target.activeScene(resizer_);
  std::vector<SideGoldenRow> side_golden_rows = captureSideGoldenRows(
      before_match->target.driver_pin, side_contexts, report_levels, scene);

  std::vector<StageEvaluation> trace;
  std::vector<BufferTreeSidePathEvaluation> side_evaluations;
  const DelayEstimate estimate = RebufferMtCandidate::estimateTree(
      *context, plan, side_contexts, &trace, &side_evaluations);

  const int target_index = context->target_stage_index;
  const RebufferRun run{
      .inst = inst,
      .driver_pin = before_match->target.driver_pin,
      .driver_pin_name = network_->pathName(before_match->target.driver_pin),
      .endpoint_name = before_match->endpoint_name,
      .report_levels = report_levels,
      .target_load_pin = target_load_pin,
      .target_load_pin_name = target_load_pin_name,
      .target_index = target_index,
      .plan = &plan,
      .fixed_stages = &fixed_stages,
      .split_target_load_wire
      = target_index + 1 >= static_cast<int>(context->path_stages.size()),
      .scene = scene};
  EstimatorProfile estimator_profile;
  GoldenProfile golden_profile;
  buildRebufferProfiles(
      run, *context, estimate, trace, estimator_profile, golden_profile);

  const int inserted_count
      = applyRebufferTreeForGolden(run,
                                   estimator_profile.candidate_stages,
                                   golden_profile,
                                   side_golden_rows);
  if (inserted_count <= 0) {
    logger_->error(RSZ,
                   3237,
                   "Could not apply Rebuffer tree after {} for golden STA.",
                   fixed_stages[target_index].driver_pin_name);
  }

  const std::vector<SidePathReport> side_paths
      = sidePathReports(side_contexts, side_evaluations, side_golden_rows);
  const float golden_delta
      = golden_profile.before_arrival - golden_profile.after_arrival;
  printRebufferSummary(run,
                       estimate,
                       golden_delta,
                       worstSidePath(side_paths),
                       estimator_profile,
                       golden_profile);
  printRebufferSidePaths(run.driver_pin_name, *context, plan, side_paths);
}

std::optional<EstimatorContext> DelayEstimatorReporter::rebufferReportContext(
    const Target& target,
    const OptimizationPolicyConfig& policy_config) const
{
  // The PARALLEL_EVAL context of the target: the window with the side models
  // and the floors RebufferMt scores against.  Every side load counts fully,
  // as one on the worst path of a violating endpoint, and no latch fanin
  // stage is stitched in, since the stage tables index the path from its
  // start.
  FailReason fail_reason = FailReason::kNone;
  const std::optional<EstimatorContext> context
      = OptimizationPolicy::buildPolicyContext(resizer_,
                                               target,
                                               policy_config,
                                               nullptr,
                                               /* include_latch_fanin */ false,
                                               nullptr,
                                               &fail_reason);
  if (!context.has_value()) {
    logger_->error(RSZ,
                   3234,
                   "Could not build rebuffer-tree estimator context for {}: "
                   "{}.",
                   network_->pathName(target.driver_pin),
                   failReasonName(fail_reason));
  }
  return context;
}

void DelayEstimatorReporter::planRebufferReport(
    const Target& report_target,
    const EstimatorContext& context,
    const OptimizationPolicyConfig& policy_config,
    RebufferReportPlan& report_plan) const
{
  // Plan and score the trees as RebufferMt does in PARALLEL_EVAL: the
  // generator stages the net and the load timing, and the candidate with the
  // best legal estimate is the tree the policy would commit; without a legal
  // one the first tree stands in.
  report_plan.target = report_target;
  report_plan.target.estimator_context = context;
  MoveCommitter committer(resizer_);
  const OptimizerRunConfig run_config;
  const GeneratorContext generator_context{.resizer = resizer_,
                                           .committer = committer,
                                           .run_config = run_config,
                                           .policy_config = policy_config};
  RebufferMtGenerator generator(generator_context);
  // The generator readies the Resizer's planner, as in a policy phase
  // without worker threads.
  generator.startPhase(0);
  generator.prepareInputs(report_plan.target);
  report_plan.candidates = generator.generate(report_plan.target);

  const RebufferMtCandidate* first_tree = nullptr;
  float best_score = 0.0f;
  for (const std::unique_ptr<MoveCandidate>& candidate :
       report_plan.candidates) {
    // RebufferMtGenerator makes RebufferMtCandidate objects only.
    RebufferMtCandidate& tree = static_cast<RebufferMtCandidate&>(*candidate);
    if (first_tree == nullptr) {
      first_tree = &tree;
    }
    const Estimate estimate = tree.estimate();
    if (estimate.legal
        && (report_plan.best == nullptr || estimate.score > best_score)) {
      report_plan.best = &tree;
      best_score = estimate.score;
    }
  }
  if (report_plan.best == nullptr) {
    report_plan.best = first_tree;
  }
  generator.finishPhase();
  if (report_plan.best == nullptr) {
    logger_->error(RSZ,
                   3236,
                   "Could not build a Rebuffer tree plan for {}.",
                   network_->pathName(report_target.driver_pin));
  }
}

void DelayEstimatorReporter::buildRebufferProfiles(
    const RebufferRun& run,
    const EstimatorContext& context,
    const DelayEstimate& estimate,
    const std::vector<StageEvaluation>& trace,
    EstimatorProfile& estimator_profile,
    GoldenProfile& golden_profile) const
{
  estimator_profile.legal = estimate.legal;
  estimator_profile.fail_reason = estimate.reason;
  estimator_profile.current_stages = buildRebufferCurrentStages(
      context, *run.plan, run.target_load_pin_name, run.split_target_load_wire);
  estimator_profile.candidate_stages
      = buildRebufferCandidateStages(context,
                                     *run.plan,
                                     trace,
                                     run.target_load_pin_name,
                                     run.split_target_load_wire);

  const std::vector<FixedStage>& fixed_stages = *run.fixed_stages;
  const FixedStage& reference_fixed_stage = fixed_stages.back();
  const std::string reference_pin_name
      = !reference_fixed_stage.next_pin_name.empty()
            ? reference_fixed_stage.next_pin_name
            : reference_fixed_stage.driver_pin_name;
  const float before_arrival = arrivalAtPinName(reference_pin_name, run.scene);
  golden_profile.arrival_reference_pin_name = reference_pin_name;
  golden_profile.before_arrival = before_arrival;
  golden_profile.after_arrival = before_arrival;
  golden_profile.before_stages = buildGoldenStageRows(fixed_stages);
  if (run.split_target_load_wire) {
    splitFinalLoadWire(golden_profile.before_stages,
                       run.target_load_pin_name,
                       "target-load wire");
  }
  insertRebufferTreePlaceholders(
      golden_profile.before_stages, run.target_index + 1, *run.plan);
}

std::vector<DelayEstimatorReporter::StageProfileRow>
DelayEstimatorReporter::buildRebufferCurrentStages(
    const EstimatorContext& context,
    const RebufferTreePlan& plan,
    const std::string& target_load_pin_name,
    const bool split_target_load_wire) const
{
  const std::vector<DelayStageState>& stages = context.path_stages;
  const int target_index = context.target_stage_index;
  std::vector<StageProfileRow> rows;
  rows.reserve(stages.size());
  for (int stage_index = 0; stage_index < static_cast<int>(stages.size());
       ++stage_index) {
    StageProfileRow current_row = buildEstimatedCurrentStageRow(
        stages[stage_index], stage_index, target_index);
    if (stage_index == target_index) {
      current_row.extra_delay = stages[stage_index].current_wire_delay;
    }

    rows.push_back(current_row);
  }

  if (split_target_load_wire) {
    splitFinalLoadWire(rows, target_load_pin_name, "target-load wire");
  }

  insertRebufferTreePlaceholders(rows, target_index + 1, plan);
  return rows;
}

DelayEstimatorReporter::WorstSidePath DelayEstimatorReporter::worstSidePath(
    const std::vector<SidePathReport>& side_paths)
{
  WorstSidePath worst{.golden_delta = kMissingValue,
                      .estimated_delta = kMissingValue,
                      .golden_slack_after = kMissingValue,
                      .estimated_slack_after = kMissingValue};

  for (const SidePathReport& side_path : side_paths) {
    if (side_path.evaluation == nullptr) {
      continue;
    }

    const BufferTreeSidePathEvaluation& eval = *side_path.evaluation;
    if (!std::isfinite(worst.estimated_delta)
        || eval.arrival_impr < worst.estimated_delta) {
      worst.estimated_delta = eval.arrival_impr;
      worst.golden_delta = side_path.goldenDelta();
    }

    if (!std::isfinite(worst.estimated_slack_after)
        || eval.slack_after < worst.estimated_slack_after) {
      worst.estimated_slack_after = eval.slack_after;
      worst.golden_slack_after = side_path.goldenSlackAfter();
    }
  }

  return worst;
}

std::vector<DelayEstimatorReporter::SidePathReport>
DelayEstimatorReporter::sidePathReports(
    const std::vector<BufferTreeSidePathContext>& side_contexts,
    const std::vector<BufferTreeSidePathEvaluation>& side_evaluations,
    const std::vector<SideGoldenRow>& side_golden_rows)
{
  std::vector<SidePathReport> side_paths;
  side_paths.reserve(side_contexts.size());
  for (size_t index = 0; index < side_contexts.size(); ++index) {
    side_paths.push_back({.context = &side_contexts[index],
                          .evaluation = index < side_evaluations.size()
                                            ? &side_evaluations[index]
                                            : nullptr,
                          .golden = &side_golden_rows[index]});
  }
  return side_paths;
}

void DelayEstimatorReporter::printRebufferSummary(
    const RebufferRun& run,
    const DelayEstimate& estimate,
    const float golden_delta,
    const WorstSidePath& worst_side,
    const EstimatorProfile& estimator_profile,
    const GoldenProfile& golden_profile) const
{
  logger_->report("Delay estimator Rebuffer tree accuracy");
  logger_->report("  instance:          {}", network_->pathName(run.inst));
  logger_->report("  driver pin:        {}", run.driver_pin_name);
  logger_->report("  endpoint path:     {}", run.endpoint_name);
  logger_->report("  delay levels:      {}", run.report_levels);
  logger_->report("  estimator legal:   {} ({})",
                  estimator_profile.legal ? "true" : "false",
                  failReasonName(estimator_profile.fail_reason));
  logger_->report("  tree buffers:      {}", run.plan->buffer_count);
  logger_->report("  tree loads:        {}", run.plan->load_count);
  logger_->report("  target load pin:   {}", run.target_load_pin_name);
  logger_->report(
      "  side loss:         {}",
      formatWithUnit(estimate.side_loss, kSecondsToPicoseconds, "ps"));
  logger_->report("");
  logger_->report("Rebuffer tree impact summary");
  logger_->report(
      "  {:<30} {:>14} {:>14} {:>14}", "", "estimated", "golden", "error");
  logger_->report(
      "  {:<30} {:>14} {:>14} {:>14}",
      "pre-post arrival delta:",
      formatWithUnit(estimate.arrival_impr, kSecondsToPicoseconds, "ps"),
      formatWithUnit(golden_delta, kSecondsToPicoseconds, "ps"),
      formatDeltaWithUnit(
          estimate.arrival_impr, golden_delta, kSecondsToPicoseconds, "ps"));
  logger_->report(
      "  {:<30} {:>14} {:>14} {:>14}",
      "worst side improvement:",
      formatWithUnit(worst_side.estimated_delta, kSecondsToPicoseconds, "ps"),
      formatWithUnit(worst_side.golden_delta, kSecondsToPicoseconds, "ps"),
      formatDeltaWithUnit(worst_side.estimated_delta,
                          worst_side.golden_delta,
                          kSecondsToPicoseconds,
                          "ps"));
  logger_->report(
      "  {:<30} {:>14} {:>14} {:>14}",
      "worst side slack after:",
      formatWithUnit(
          worst_side.estimated_slack_after, kSecondsToPicoseconds, "ps"),
      formatWithUnit(
          worst_side.golden_slack_after, kSecondsToPicoseconds, "ps"),
      formatDeltaWithUnit(worst_side.estimated_slack_after,
                          worst_side.golden_slack_after,
                          kSecondsToPicoseconds,
                          "ps"));
  logger_->report("");
  logger_->report(
      "============================================================");
  logger_->report("Critical path ({} -> {})",
                  run.driver_pin_name,
                  run.target_load_pin_name);
  logger_->report(
      "============================================================");
  logger_->report("Stage-window output arrival reference");
  logger_->report("  reference pin:                   {}",
                  golden_profile.arrival_reference_pin_name);
  logger_->report("  golden before arrival:           {:>10.3f} ps",
                  golden_profile.before_arrival * kSecondsToPicoseconds);
  logger_->report("  golden after arrival:            {:>10.3f} ps",
                  golden_profile.after_arrival * kSecondsToPicoseconds);
  logger_->report("  golden reference arrival delta:  {:>10.3f} ps",
                  golden_profile.arrival_impr * kSecondsToPicoseconds);
  printStageWindowDetail(estimator_profile.current_stages,
                         estimator_profile.candidate_stages,
                         golden_profile.before_stages,
                         golden_profile.after_stages);
}

std::vector<DelayEstimatorReporter::SideGoldenRow>
DelayEstimatorReporter::captureSideGoldenRows(
    sta::Pin* driver_pin,
    const std::vector<BufferTreeSidePathContext>& side_contexts,
    const int report_levels,
    const sta::Scene* scene) const
{
  std::vector<SideGoldenRow> side_golden_rows;
  side_golden_rows.reserve(side_contexts.size());
  for (const BufferTreeSidePathContext& side_context : side_contexts) {
    const std::string load_pin_name = network_->pathName(side_context.load_pin);
    const std::vector<FixedStage> before_stages = captureSidePathFixedStages(
        driver_pin, side_context.load_pin, report_levels);
    // The arrival before the tree; applyRebufferTreeForGolden() measures it
    // again with the tree.
    const float arrival = arrivalAtPinName(load_pin_name, scene);
    side_golden_rows.push_back(
        {.load_pin = side_context.load_pin,
         .load_pin_name = load_pin_name,
         .before_stage_rows = buildGoldenStageRows(before_stages),
         .after_stage_rows = {},
         .before_arrival = arrival,
         .after_arrival = arrival,
         .current_slack = side_context.current_slack});
  }

  return side_golden_rows;
}

std::vector<DelayEstimatorReporter::StageProfileRow>
DelayEstimatorReporter::buildRebufferCandidateStages(
    const EstimatorContext& context,
    const RebufferTreePlan& plan,
    const std::vector<StageEvaluation>& trace,
    const std::string& target_load_pin_name,
    const bool split_target_load_wire) const
{
  const std::vector<DelayStageState>& stages = context.path_stages;
  const int target_index = context.target_stage_index;
  std::vector<StageProfileRow> rows;
  rows.reserve(trace.size());
  const int tree_stage_count = static_cast<int>(targetPathStages(plan).size());
  int buffer_stage_index = 0;
  int trace_index = 0;
  int target_load_wire_row_index = -1;
  while (trace_index < static_cast<int>(trace.size())) {
    const StageEvaluation& eval = trace[trace_index];
    int path_index = eval.path_index;
    Role role = Role::kTarget;
    std::string pin_name;
    if (trace_index <= target_index) {
      const int stage_index = trace_index;
      path_index = stages[stage_index].path_index;
      role = roleFor(stage_index, target_index);
      pin_name = stageDriverPinName(stages[stage_index]);
    } else if (trace_index <= target_index + tree_stage_count) {
      const int tree_stage_index = trace_index - target_index - 1;
      path_index = -1;
      role = Role::kTarget;
      const bool is_wire_stage
          = isRebufferTreeWireStage(targetPathStages(plan)[tree_stage_index]);
      if (is_wire_stage) {
        if (split_target_load_wire
            && !plan.targetPath()->hasBufferAfter(tree_stage_index)) {
          if (target_load_wire_row_index < 0) {
            target_load_wire_row_index = static_cast<int>(rows.size());
            rows.push_back(loadWireRow(
                fmt::format("{} <target-load wire>", target_load_pin_name),
                eval.path_index,
                eval.input_slew,
                eval.wire_delay,
                rows.back().load_cap,
                eval.arc_match_relaxed));
          } else {
            foldWire(rows[target_load_wire_row_index], eval);
          }
          ++trace_index;
          continue;
        }

        // Fold contiguous tree-wire/via stages into the preceding row.
        StageProfileRow& active_row = rows.back();
        foldWire(active_row, eval);
        ++trace_index;
        while (trace_index < static_cast<int>(trace.size())
               && trace_index <= target_index + tree_stage_count) {
          const int next_tree_stage_index = trace_index - target_index - 1;
          if (!isRebufferTreeWireStage(
                  targetPathStages(plan)[next_tree_stage_index])) {
            break;
          }

          foldWire(active_row, trace[trace_index]);
          ++trace_index;
        }
        continue;
      }

      pin_name = rebufferTreeStageName(plan, buffer_stage_index);
      ++buffer_stage_index;
    } else {
      // The trace holds one entry per path stage and per tree stage, so the
      // entries after the tree are the fanout stages.
      const int stage_index = trace_index - tree_stage_count;
      path_index = stages[stage_index].path_index;
      role = roleFor(stage_index, target_index);
      pin_name = stageDriverPinName(stages[stage_index]);
    }

    rows.push_back(
        {.path_index = path_index,
         .role = role,
         .pin_name = pin_name,
         .cell_name = eval.cell != nullptr ? eval.cell->name() : "tree-wire",
         .input_slew = eval.input_slew,
         .cell_delay = eval.stage_delay,
         .wire_delay = eval.wire_delay,
         .load_cap = eval.load_cap,
         .output_slew = eval.output_slew,
         .extra_delay = 0.0f,
         .arc_match_relaxed = eval.arc_match_relaxed});
    ++trace_index;
  }

  return rows;
}

int DelayEstimatorReporter::applyRebufferTreeForGolden(
    const RebufferRun& run,
    std::vector<StageProfileRow>& candidate_stages,
    GoldenProfile& golden_profile,
    std::vector<SideGoldenRow>& side_golden_rows)
{
  const std::vector<FixedStage>& fixed_stages = *run.fixed_stages;
  const FixedStage& target_fixed_stage = fixed_stages[run.target_index];
  const std::string& reference_pin_name
      = golden_profile.arrival_reference_pin_name;

  int inserted_count = 0;

  {
    EcoJournalScope eco(*this);
    inserted_count = resizer_.rebuffer().applyRebufferTreePlan(
        *run.plan, "delay_estimator_rebuffer");
    if (inserted_count > 0) {
      resizer_.updateParasiticsAndTiming();
      golden_profile.after_arrival
          = arrivalAtPinName(reference_pin_name, run.scene);

      // Re-capture the post-ECO target path while inserted-buffer pins exist
      // so the stage table can report golden values for each new buffer.
      const std::vector<FixedStage> after_target_stages
          = captureSidePathFixedStages(run.driver_pin,
                                       run.target_load_pin,
                                       run.report_levels,
                                       &target_fixed_stage,
                                       &fixed_stages);
      golden_profile.after_stages
          = after_target_stages.empty()
                ? buildGoldenStageRows(fixed_stages)
                : buildGoldenStageRows(after_target_stages);
      if (after_target_stages.empty()) {
        insertRebufferTreePlaceholders(
            golden_profile.after_stages, run.target_index + 1, *run.plan);
      } else {
        if (run.split_target_load_wire) {
          splitFinalLoadWire(golden_profile.after_stages,
                             run.target_load_pin_name,
                             "target-load wire");
        }
        alignSidePathCandidateNames(candidate_stages,
                                    golden_profile.after_stages);
      }

      golden_profile.arrival_impr
          = golden_profile.before_arrival - golden_profile.after_arrival;
      for (SideGoldenRow& side_row : side_golden_rows) {
        side_row.after_arrival
            = arrivalAtPinName(side_row.load_pin_name, run.scene);
        const std::vector<FixedStage> after_stages
            = captureSidePathFixedStages(run.driver_pin,
                                         side_row.load_pin,
                                         run.report_levels,
                                         &target_fixed_stage);

        // Materialize rows before undoEco(), because inserted-buffer pins
        // only exist while the rebuffer-tree ECO journal is active.
        side_row.after_stage_rows = buildGoldenStageRows(after_stages);
      }
    }
  }

  return inserted_count;
}

void DelayEstimatorReporter::printSidePathImpactTable(
    const std::vector<SidePathReport>& side_paths) const
{
  logger_->report("  {:>12} {:>12} {:>12} {:>12} {:>12} {:>12} {:>10}  {}",
                  "est delta",
                  "golden",
                  "error",
                  "est slack",
                  "golden slack",
                  "slack err",
                  "result",
                  "load pin");

  for (const SidePathReport& side_path : side_paths) {
    const BufferTreeSidePathEvaluation* side_eval = side_path.evaluation;
    const float golden_side_delta = side_path.goldenDelta();
    const float estimated_side_delta
        = side_eval != nullptr ? side_eval->arrival_impr : kMissingValue;
    const float estimated_slack_after
        = side_eval != nullptr ? side_eval->slack_after : kMissingValue;

    const float golden_slack_after = side_path.goldenSlackAfter();
    logger_->report(
        "  {:>12} {:>12} {:>12} {:>12} {:>12} {:>12} {:>10}  {}",
        formatValue(estimated_side_delta, kSecondsToPicoseconds),
        formatValue(golden_side_delta, kSecondsToPicoseconds),
        formatDelta(
            estimated_side_delta, golden_side_delta, kSecondsToPicoseconds),
        formatValue(estimated_slack_after, kSecondsToPicoseconds),
        formatValue(golden_slack_after, kSecondsToPicoseconds),
        formatDelta(
            estimated_slack_after, golden_slack_after, kSecondsToPicoseconds),
        side_eval != nullptr && side_eval->legal ? "legal" : "reject",
        side_path.golden->load_pin_name);
  }
}

void DelayEstimatorReporter::printRebufferSidePaths(
    const std::string& rebuffer_driver_pin_name,
    const EstimatorContext& context,
    const RebufferTreePlan& plan,
    const std::vector<SidePathReport>& side_paths) const
{
  if (!side_paths.empty()) {
    logger_->report("");
    logger_->report(
        "============================================================");
    logger_->report("Side-path aggregate summary");
    logger_->report(
        "============================================================");
    logger_->report("Rebuffer tree side-path impact summary");
    printSidePathImpactTable(side_paths);

    logger_->report("");
    logger_->report("Rebuffer tree side-path stage details");
    int side_path_index = 1;
    for (const SidePathReport& side_path : side_paths) {
      if (side_path.evaluation == nullptr) {
        continue;
      }

      const SideGoldenRow& side_row = *side_path.golden;
      const BufferTreeSidePathEvaluation* side_eval = side_path.evaluation;
      std::vector<StageProfileRow> current_rows
          = buildSidePathCurrentRows(context, *side_path.context, plan);
      std::vector<StageProfileRow> candidate_rows
          = buildSidePathCandidateRows(*side_path.context, *side_eval, plan);
      std::vector<StageProfileRow> golden_before_rows
          = side_row.before_stage_rows;
      std::vector<StageProfileRow> golden_after_rows
          = side_row.after_stage_rows;

      if (candidate_rows.empty()) {
        continue;
      }

      splitFinalLoadWire(
          current_rows, side_row.load_pin_name, "side-load wire");
      splitFinalLoadWire(
          golden_before_rows, side_row.load_pin_name, "side-load wire");
      splitFinalLoadWire(
          golden_after_rows, side_row.load_pin_name, "side-load wire");

      int branch_row_count = 0;
      for (const StageProfileRow& row : candidate_rows) {
        if (row.role == Role::kFanout) {
          break;
        }
        ++branch_row_count;
      }

      insertSidePathPlaceholders(current_rows, branch_row_count);
      insertSidePathPlaceholders(golden_before_rows, branch_row_count);
      alignSidePathCandidateNames(candidate_rows, golden_after_rows);

      const float golden_side_delta = side_path.goldenDelta();

      logger_->report("");
      logger_->report(
          "============================================================");
      logger_->report("Side path #{} ({} -> {})",
                      side_path_index,
                      rebuffer_driver_pin_name,
                      side_row.load_pin_name);
      logger_->report(
          "============================================================");
      logger_->report("  reference pin:   {}", side_row.load_pin_name);
      logger_->report(
          "  estimated delta: {:>10} ps",
          formatValue(side_eval->arrival_impr, kSecondsToPicoseconds));
      logger_->report("  golden delta:    {:>10} ps",
                      formatValue(golden_side_delta, kSecondsToPicoseconds));
      logger_->report("  error:           {:>10} ps",
                      formatDelta(side_eval->arrival_impr,
                                  golden_side_delta,
                                  kSecondsToPicoseconds));

      printStageWindowDetail(
          current_rows, candidate_rows, golden_before_rows, golden_after_rows);
      ++side_path_index;
    }
  }
}

// === Phase implementations ================================================

void DelayEstimatorReporter::measureGoldenAfterSwap(
    const TargetMatch& before_match,
    const std::vector<FixedStage>& fixed_stages,
    GoldenProfile& golden_profile) const
{
  const sta::Scene* scene = before_match.target.activeScene(resizer_);

  if (golden_profile.arrival_reference_pin_name.empty()) {
    golden_profile.arrival_reference_pin_name
        = fixed_stages.empty()
              ? network_->pathName(before_match.target.driver_pin)
              : fixed_stages.back().driver_pin_name;
  }

  const float after_arrival
      = arrivalAtPinName(golden_profile.arrival_reference_pin_name, scene);

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

  const size_t stage_count
      = std::max({estimator_profile.current_stages.size(),
                  estimator_profile.candidate_stages.size(),
                  golden_profile.before_stages.size(),
                  golden_profile.after_stages.size()});
  const std::string stage_count_label
      = fmt::format("{}-stage", static_cast<int>(stage_count));

  const float candidate_stage_delay_for_table
      = estimator_profile.candidate_stages_incomplete
            ? kMissingValue
            : estimated_candidate_stage_delay;
  const float estimated_stage_impr_for_table
      = estimator_profile.candidate_stages_incomplete ? kMissingValue
                                                      : estimated_stage_impr;

  const std::string arrival_reference_pin_name
      = golden_profile.arrival_reference_pin_name.empty()
            ? driver_pin_name
            : golden_profile.arrival_reference_pin_name;
  const char* arrival_reference_role
      = !golden_profile.before_stages.empty()
            ? roleName(golden_profile.before_stages.back().role)
            : roleName(Role::kTarget);

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
  logger_->report("Stage window delay summary");
  logger_->report(
      "  {:<30} {:>14} {:>14} {:>14}", "", "estimated", "golden", "error");
  logger_->report(
      "  {:<30} {:>14} {:>14} {:>14}",
      fmt::format("pre-ECO {} delay:", stage_count_label),
      formatWithUnit(
          estimated_current_stage_delay, kSecondsToPicoseconds, "ps"),
      formatWithUnit(golden_before_stage_delay, kSecondsToPicoseconds, "ps"),
      formatDeltaWithUnit(estimated_current_stage_delay,
                          golden_before_stage_delay,
                          kSecondsToPicoseconds,
                          "ps"));
  logger_->report(
      "  {:<30} {:>14} {:>14} {:>14}",
      fmt::format("post-ECO {} delay:", stage_count_label),
      formatWithUnit(
          candidate_stage_delay_for_table, kSecondsToPicoseconds, "ps"),
      formatWithUnit(golden_after_stage_delay, kSecondsToPicoseconds, "ps"),
      formatDeltaWithUnit(candidate_stage_delay_for_table,
                          golden_after_stage_delay,
                          kSecondsToPicoseconds,
                          "ps"));
  logger_->report(
      "  {:<30} {:>14} {:>14} {:>14}",
      fmt::format("pre-post {} delta:", stage_count_label),
      formatWithUnit(
          estimated_stage_impr_for_table, kSecondsToPicoseconds, "ps"),
      formatWithUnit(golden_stage_impr, kSecondsToPicoseconds, "ps"),
      formatDeltaWithUnit(estimated_stage_impr_for_table,
                          golden_stage_impr,
                          kSecondsToPicoseconds,
                          "ps"));

  if (estimator_profile.candidate_stages_incomplete) {
    // Estimator failed mid-walk; partial sums would mislead.  Show explicit
    // marker; per-stage tables below still display the prefix that succeeded.
    logger_->report(
        "  estimated post-ECO stage delay is incomplete: {} of {} stages",
        estimator_profile.candidate_stages.size(),
        estimator_profile.current_stages.size());
  }

  logger_->report("");
  logger_->report("Stage-window output arrival reference");
  logger_->report("  reference pin:                   {} ({})",
                  arrival_reference_pin_name,
                  arrival_reference_role);
  logger_->report("  golden before arrival:           {:>10.3f} ps",
                  golden_profile.before_arrival * kSecondsToPicoseconds);
  logger_->report("  golden after arrival:            {:>10.3f} ps",
                  golden_profile.after_arrival * kSecondsToPicoseconds);
  logger_->report("  golden reference arrival delta:  {:>10.3f} ps",
                  golden_profile.arrival_impr * kSecondsToPicoseconds);

  printStageWindowDetail(estimator_profile.current_stages,
                         estimator_profile.candidate_stages,
                         golden_profile.before_stages,
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

void DelayEstimatorReporter::errorNoSetupPathStage(sta::Instance* inst) const
{
  logger_->error(RSZ,
                 3201,
                 "Could not find a setup timing path stage driven by "
                 "instance {}.",
                 network_->pathName(inst));
}

std::optional<DelayEstimatorReporter::TargetMatch>
DelayEstimatorReporter::findWorstTargetForInstance(
    sta::Instance* inst,
    const int delay_levels,
    sta::Pin* target_driver_pin) const
{
  // Use a through-filtered endpoint path rather than vertexWorstSlackPath().
  // vertexWorstSlackPath() stops at the target output vertex, which hides
  // downstream stages from the fanout diagnostic table.
  std::vector<sta::Pin*> output_pins;
  if (target_driver_pin != nullptr) {
    output_pins.push_back(target_driver_pin);
  } else {
    output_pins = outputPins(inst);
  }

  if (output_pins.empty()) {
    return std::nullopt;
  }

  sta::PathEndSeq path_ends = findPathEndsThrough(
      sta_, output_pins, max_min_max_->asMinMaxAll(), kReporterPathSearchCount);

  std::optional<TargetMatch> best_match;
  for (const sta::PathEnd* path_end : path_ends) {
    const sta::Path* path = path_end->path();
    if (path == nullptr) {
      continue;
    }

    sta::PathExpanded expanded(path, resizer_.staState());

    // Locate the requested instance output stage on the full endpoint path.
    // The previous timing arc/edge must be valid so the stage can drive a
    // sizing target.
    sta::Pin* driver_pin = nullptr;
    int driver_path_index = -1;
    for (int i = expanded.startIndex(); i < expanded.size(); ++i) {
      const sta::Path* stage_path = expanded.path(i);
      sta::Pin* stage_pin = stage_path->pin(resizer_.staState());
      if ((target_driver_pin != nullptr
               ? stage_pin == target_driver_pin
               : std::find(output_pins.begin(), output_pins.end(), stage_pin)
                     != output_pins.end())
          && stage_path->prevArc(resizer_.staState()) != nullptr
          && stage_path->prevEdge(resizer_.staState()) != nullptr) {
        driver_pin = stage_pin;
        driver_path_index = i;
        break;
      }
    }

    if (driver_pin == nullptr || driver_path_index < 0) {
      continue;
    }

    const sta::Slack slack = path_end->slack(resizer_.staState());
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

  const float current_delay = current_cell_delay + current_fanin_penalty;
  const float candidate_delay = candidate_cell_delay + candidate_fanin_penalty;
  const float arrival_impr = current_delay - candidate_delay;
  profile.legal = arrival_impr > 0.0f;
  profile.fail_reason = profile.legal ? FailReason::kEstimateLegal
                                      : FailReason::kEstimateNonImproving;

  // Per-stage rows are best-effort.  If the context for input_slew capture
  // is unavailable, return the legal/fail_reason already computed above
  // and skip the optional stage tables instead of overwriting the result.
  const std::optional<EstimatorContext> context
      = contextForTarget(target, 0, nullptr);
  if (!context.has_value()) {
    return profile;
  }

  sta::PathExpanded expanded(target.endpoint_path, resizer_.staState());
  const std::string target_pin_name = stagePinName(expanded, target.path_index);
  const float target_wire_delay = pathWireDelay(expanded, target.path_index);
  StageProfileRow current_row{.path_index = target.path_index,
                              .role = Role::kTarget,
                              .pin_name = target_pin_name,
                              .cell_name = driver_port->libertyCell()->name(),
                              .input_slew = context->target().input_slew,
                              .cell_delay = current_cell_delay,
                              .wire_delay = target_wire_delay,
                              .load_cap = load_cap,
                              .output_slew = context->target().current_slew,
                              .extra_delay = current_fanin_penalty};
  profile.current_stages.push_back(std::move(current_row));

  StageProfileRow candidate_row{.path_index = target.path_index,
                                .role = Role::kTarget,
                                .pin_name = target_pin_name,
                                .cell_name = replacement->name(),
                                .input_slew = context->target().input_slew,
                                .cell_delay = candidate_cell_delay,
                                .wire_delay = target_wire_delay,
                                .load_cap = load_cap,
                                .output_slew = kMissingValue,
                                .extra_delay = candidate_fanin_penalty};
  profile.candidate_stages.push_back(std::move(candidate_row));

  return profile;
}

DelayEstimatorReporter::StageProfileRow
DelayEstimatorReporter::buildEstimatedCurrentStageRow(
    const DelayStageState& stage,
    const int stage_index,
    const int target_index) const
{
  return {.path_index = stage.path_index,
          .role = roleFor(stage_index, target_index),
          .pin_name = stageDriverPinName(stage),
          .cell_name = stage.arc.currentCell()->name(),
          .input_slew = stage.input_slew,
          .cell_delay = stage.current_delay,
          .wire_delay = stage.current_wire_delay,
          .load_cap = stage.load_cap,
          .output_slew = stage.current_slew,
          .extra_delay = 0.0f};
}

DelayEstimatorReporter::StageProfileRow
DelayEstimatorReporter::makeMissingStageRow(const Role role,
                                            const std::string& pin_name,
                                            const int path_index) const
{
  StageProfileRow row;
  row.valid = false;
  row.path_index = path_index;
  row.role = role;
  row.pin_name = pin_name;
  row.cell_name = kDashCell;
  row.input_slew = kMissingValue;
  row.cell_delay = kMissingValue;
  row.wire_delay = kMissingValue;
  row.load_cap = kMissingValue;
  row.output_slew = kMissingValue;
  row.extra_delay = kMissingValue;

  return row;
}

std::string DelayEstimatorReporter::stageDriverPinName(
    const DelayStageState& stage) const
{
  return network_->pathName(stage.driver_pin);
}

std::string DelayEstimatorReporter::rebufferTreeStageName(
    const RebufferTreePlan& plan,
    const int buffer_stage_index) const
{
  return rebufferTreeStageName(plan, plan.target_load_pin, buffer_stage_index);
}

std::string DelayEstimatorReporter::rebufferTreeStageName(
    const RebufferTreePlan& plan,
    const sta::Pin* load_pin,
    const int buffer_stage_index) const
{
  const std::string driver_name = network_->pathName(plan.driver_pin);
  const std::string load_name
      = load_pin != nullptr ? network_->pathName(load_pin) : "<target_load>";
  return fmt::format("{} -> {} <inserted buffer {}>",
                     driver_name,
                     load_name,
                     buffer_stage_index + 1);
}

bool DelayEstimatorReporter::isRebufferTreeWireStage(
    const BufferTreePathStage& tree_stage) const
{
  return tree_stage.type == BufferTreeStageType::kWire
         || tree_stage.type == BufferTreeStageType::kVia;
}

std::vector<DelayEstimatorReporter::StageProfileRow>
DelayEstimatorReporter::buildRebufferTreePlaceholderRows(
    const RebufferTreePlan& plan) const
{
  std::vector<StageProfileRow> placeholders;
  int buffer_stage_index = 0;
  for (const BufferTreePathStage& tree_stage : targetPathStages(plan)) {
    if (!isRebufferTreeWireStage(tree_stage)) {
      placeholders.push_back(makeMissingStageRow(
          Role::kTarget, rebufferTreeStageName(plan, buffer_stage_index)));
      ++buffer_stage_index;
    }
  }

  return placeholders;
}

void DelayEstimatorReporter::insertRebufferTreePlaceholders(
    std::vector<StageProfileRow>& rows,
    const int insert_index,
    const RebufferTreePlan& plan) const
{
  if (targetPathStages(plan).empty()) {
    return;
  }

  const std::vector<StageProfileRow> placeholders
      = buildRebufferTreePlaceholderRows(plan);

  const int clamped_insert_index
      = std::clamp(insert_index, 0, static_cast<int>(rows.size()));
  rows.insert(rows.begin() + clamped_insert_index,
              placeholders.begin(),
              placeholders.end());
}

std::vector<DelayEstimatorReporter::FixedStage>
DelayEstimatorReporter::captureSidePathFixedStages(
    sta::Pin* driver_pin,
    const sta::Pin* load_pin,
    const int delay_levels,
    const FixedStage* reference_stage,
    const std::vector<FixedStage>* reference_path_stages) const
{
  std::vector<FixedStage> best_fixed_stages;
  if (driver_pin == nullptr || load_pin == nullptr) {
    return best_fixed_stages;
  }

  sta::PathEndSeq path_ends
      = findPathEndsThrough(sta_,
                            {const_cast<sta::Pin*>(load_pin)},
                            max_min_max_->asMinMaxAll(),
                            kReporterPathSearchCount);

  bool found_path = false;
  sta::Slack best_slack = sta::INF;
  for (const sta::PathEnd* path_end : path_ends) {
    const sta::Path* path = path_end != nullptr ? path_end->path() : nullptr;
    if (path == nullptr) {
      continue;
    }

    sta::PathExpanded expanded(path, resizer_.staState());
    int driver_path_index = -1;
    int load_path_index = -1;
    for (int path_index = expanded.startIndex(); path_index < expanded.size();
         ++path_index) {
      const sta::Path* stage_path = expanded.path(path_index);
      sta::Pin* stage_pin = stage_path != nullptr
                                ? stage_path->pin(resizer_.staState())
                                : nullptr;
      if (stage_pin == driver_pin) {
        driver_path_index = path_index;
      }

      if (stage_pin == load_pin) {
        load_path_index = path_index;
      }
    }

    if (driver_path_index < 0 || load_path_index < 0
        || driver_path_index >= load_path_index) {
      continue;
    }

    std::vector<FixedStage> fixed_stages = collectSidePathStages(
        expanded, path, driver_path_index, load_path_index, delay_levels);
    assignNextPins(fixed_stages, expanded, load_pin, load_path_index);

    const sta::Slack slack = path_end->slack(resizer_.staState());
    if (!found_path || slack < best_slack) {
      found_path = true;
      best_slack = slack;
      best_fixed_stages = std::move(fixed_stages);
    }
  }

  const std::vector<FixedStage> connectivity_fixed_stages
      = captureSidePathConnectivityFixedStages(driver_pin,
                                               load_pin,
                                               best_fixed_stages,
                                               reference_stage,
                                               reference_path_stages);
  return connectivity_fixed_stages.empty() ? best_fixed_stages
                                           : connectivity_fixed_stages;
}

std::vector<DelayEstimatorReporter::FixedStage>
DelayEstimatorReporter::collectSidePathStages(const sta::PathExpanded& expanded,
                                              const sta::Path* path,
                                              const int driver_path_index,
                                              const int load_path_index,
                                              const int delay_levels) const
{
  std::vector<FixedStage> fixed_stages;
  int fanout_stage_count = 0;
  for (int path_index = driver_path_index; path_index < expanded.size();
       ++path_index) {
    if (path_index > load_path_index && fanout_stage_count >= delay_levels) {
      break;
    }

    const sta::Path* stage_path = expanded.path(path_index);
    sta::Pin* stage_pin = stage_path != nullptr
                              ? stage_path->pin(resizer_.staState())
                              : nullptr;
    if (stage_pin == nullptr || !network_->isDriver(stage_pin)
        || stage_path->prevArc(resizer_.staState()) == nullptr
        || stage_path->prevEdge(resizer_.staState()) == nullptr) {
      continue;
    }

    sta::Instance* stage_inst = network_->instance(stage_pin);
    if (stage_inst == nullptr) {
      continue;
    }

    const sta::Scene* scene = path->scene(resizer_.staState());
    const sta::MinMax* min_max = path->minMax(resizer_.staState());
    if (min_max == nullptr) {
      min_max = max_min_max_;
    }

    FailReason ignored_reason = FailReason::kNone;
    const std::optional<EstimatorContext> stage_context
        = DelayEstimator::buildContext(resizer_,
                                       stage_inst,
                                       stage_pin,
                                       expanded,
                                       path_index,
                                       scene,
                                       min_max,
                                       0,
                                       &ignored_reason,
                                       kStaSlewBias);
    if (!stage_context.has_value()) {
      continue;
    }

    const sta::Path* input_path = stage_path->prevPath();
    sta::Pin* input_pin = input_path != nullptr
                              ? input_path->pin(resizer_.staState())
                              : nullptr;

    FixedStage fixed_stage;
    fixed_stage.state = stage_context->target();
    fixed_stage.role = fixed_stages.empty() ? Role::kFanin : Role::kTarget;
    if (path_index > load_path_index) {
      fixed_stage.role = Role::kFanout;
      ++fanout_stage_count;
    }

    fixed_stage.driver_pin = stage_pin;
    fixed_stage.driver_pin_name = network_->pathName(stage_pin);
    if (input_pin != nullptr) {
      fixed_stage.input_pin = input_pin;
      fixed_stage.input_pin_name = network_->pathName(input_pin);
    }

    fixed_stages.push_back(std::move(fixed_stage));
  }

  return fixed_stages;
}

void DelayEstimatorReporter::assignNextPins(
    std::vector<FixedStage>& fixed_stages,
    const sta::PathExpanded& expanded,
    const sta::Pin* load_pin,
    const int load_path_index) const
{
  // Prefer the next captured stage input so fanout/load semantics follow the
  // filtered stage window instead of raw path hops such as HA out->out arcs.
  for (int stage_index = 0; stage_index < static_cast<int>(fixed_stages.size());
       ++stage_index) {
    FixedStage& fixed_stage = fixed_stages[stage_index];
    if (stage_index + 1 < static_cast<int>(fixed_stages.size())
        && !fixed_stages[stage_index + 1].input_pin_name.empty()) {
      fixed_stage.next_pin = fixed_stages[stage_index + 1].input_pin;
      fixed_stage.next_pin_name = fixed_stages[stage_index + 1].input_pin_name;
      continue;
    }

    if (load_pin != nullptr && fixed_stage.state.path_index < load_path_index) {
      fixed_stage.next_pin = const_cast<sta::Pin*>(load_pin);
      fixed_stage.next_pin_name = network_->pathName(load_pin);
      continue;
    }

    // Else use the first downstream receiver outside the instance.  This skips
    // multi-output hops and keeps the last fanout stage's output wire.
    sta::Pin* driver_pin = network_->findPin(fixed_stage.driver_pin_name);
    sta::Instance* driver_inst
        = driver_pin != nullptr ? network_->instance(driver_pin) : nullptr;

    for (int path_index = fixed_stage.state.path_index + 1;
         path_index < static_cast<int>(expanded.size());
         ++path_index) {
      const sta::Path* next_path = expanded.path(path_index);
      sta::Edge* next_prev_edge = next_path != nullptr
                                      ? next_path->prevEdge(resizer_.staState())
                                      : nullptr;
      sta::Pin* next_pin = next_path != nullptr
                               ? next_path->pin(resizer_.staState())
                               : nullptr;
      if (next_prev_edge == nullptr || !next_prev_edge->isWire()
          || next_pin == nullptr || network_->isDriver(next_pin)) {
        continue;
      }

      sta::Instance* next_inst = network_->instance(next_pin);
      if (driver_inst != nullptr && next_inst != nullptr
          && driver_inst == next_inst) {
        continue;
      }

      fixed_stage.next_pin = next_pin;
      fixed_stage.next_pin_name = network_->pathName(next_pin);
      break;
    }
  }
}

std::vector<DelayEstimatorReporter::FixedStage>
DelayEstimatorReporter::captureSidePathConnectivityFixedStages(
    sta::Pin* driver_pin,
    const sta::Pin* load_pin,
    const std::vector<FixedStage>& path_fixed_stages,
    const FixedStage* reference_stage,
    const std::vector<FixedStage>* reference_path_stages) const
{
  std::vector<FixedStage> fixed_stages;
  if (driver_pin == nullptr || load_pin == nullptr
      || path_fixed_stages.empty()) {
    return fixed_stages;
  }

  std::vector<sta::Pin*> reverse_driver_chain;
  std::vector<sta::Pin*> reverse_input_chain;
  sta::Pin* current_load_pin = const_cast<sta::Pin*>(load_pin);
  for (int depth = 0; depth < 32; ++depth) {
    sta::Net* net = network_->net(current_load_pin);
    sta::Pin* net_driver = dbNetDriver(current_load_pin);
    if (net_driver == nullptr) {
      net_driver = netDriver(net);
    }

    if (net_driver == nullptr) {
      return fixed_stages;
    }

    reverse_driver_chain.push_back(net_driver);
    reverse_input_chain.push_back(current_load_pin);
    if (net_driver == driver_pin) {
      break;
    }

    sta::Instance* driver_inst = network_->instance(net_driver);
    sta::LibertyCell* driver_cell
        = driver_inst != nullptr ? network_->libertyCell(driver_inst) : nullptr;
    if (driver_cell == nullptr || !driver_cell->isBuffer()) {
      return fixed_stages;
    }

    sta::LibertyPort* buffer_input = nullptr;
    sta::LibertyPort* buffer_output = nullptr;
    driver_cell->bufferPorts(buffer_input, buffer_output);
    if (buffer_input == nullptr || buffer_output == nullptr) {
      return fixed_stages;
    }

    current_load_pin = network_->findPin(driver_inst, buffer_input);
    if (current_load_pin == nullptr) {
      return fixed_stages;
    }
  }

  if (reverse_driver_chain.empty() || reverse_driver_chain.back() != driver_pin
      || reverse_driver_chain.size() <= 1) {
    return fixed_stages;
  }

  fixed_stages.reserve(path_fixed_stages.size() + reverse_driver_chain.size());
  const FixedStage& timing_reference = reference_stage != nullptr
                                           ? *reference_stage
                                           : path_fixed_stages.front();

  FixedStage driver_stage = reference_stage != nullptr
                                ? *reference_stage
                                : path_fixed_stages.front();
  driver_stage.role = Role::kFanin;
  driver_stage.driver_pin = driver_pin;
  driver_stage.next_pin = reverse_input_chain.back();
  driver_stage.driver_pin_name = network_->pathName(driver_pin);
  driver_stage.next_pin_name = network_->pathName(reverse_input_chain.back());
  fixed_stages.push_back(driver_stage);

  for (int chain_index = static_cast<int>(reverse_driver_chain.size()) - 2;
       chain_index >= 0;
       --chain_index) {
    sta::Pin* buffer_output_pin = reverse_driver_chain[chain_index];
    sta::Pin* buffer_input_pin = reverse_input_chain[chain_index + 1];
    if (buffer_output_pin == nullptr || buffer_input_pin == nullptr) {
      return {};
    }

    const sta::TimingArc* arc
        = firstGraphArc(buffer_input_pin,
                        buffer_output_pin,
                        timing_reference.state.arc.outputRiseFall(),
                        timing_reference.state.arc.outputRiseFall());
    if (arc == nullptr) {
      return {};
    }

    FixedStage buffer_stage;
    buffer_stage.state = timing_reference.state;
    buffer_stage.state.driver_pin = buffer_output_pin;
    buffer_stage.state.path_index = -1;
    buffer_stage.state.arc.ref_arc = arc;
    buffer_stage.role = Role::kTarget;
    buffer_stage.input_pin = buffer_input_pin;
    buffer_stage.driver_pin = buffer_output_pin;
    buffer_stage.next_pin = reverse_input_chain[chain_index];
    buffer_stage.input_pin_name = network_->pathName(buffer_input_pin);
    buffer_stage.driver_pin_name = network_->pathName(buffer_output_pin);
    buffer_stage.next_pin_name
        = network_->pathName(reverse_input_chain[chain_index]);
    fixed_stages.push_back(std::move(buffer_stage));
  }

  for (const FixedStage& path_stage : path_fixed_stages) {
    if (path_stage.role == Role::kFanout) {
      FixedStage fanout_stage = path_stage;
      if (reference_path_stages != nullptr) {
        for (const FixedStage& reference_path_stage : *reference_path_stages) {
          if (reference_path_stage.role == Role::kFanout
              && reference_path_stage.driver_pin_name
                     == path_stage.driver_pin_name) {
            fanout_stage.state = reference_path_stage.state;
            break;
          }
        }
      }

      fixed_stages.push_back(fanout_stage);
    }
  }

  return fixed_stages;
}

sta::Pin* DelayEstimatorReporter::netDriver(sta::Net* net) const
{
  if (net == nullptr) {
    return nullptr;
  }

  std::unique_ptr<sta::NetPinIterator> pin_iter(network_->pinIterator(net));
  while (pin_iter->hasNext()) {
    sta::Pin* pin = const_cast<sta::Pin*>(pin_iter->next());
    if (pin != nullptr && network_->isDriver(pin)) {
      return pin;
    }
  }

  return nullptr;
}

sta::Pin* DelayEstimatorReporter::dbNetDriver(sta::Pin* load_pin) const
{
  if (load_pin == nullptr) {
    return nullptr;
  }

  odb::dbITerm* load_iterm = nullptr;
  odb::dbBTerm* load_bterm = nullptr;
  odb::dbModITerm* load_moditerm = nullptr;
  sta::dbNetwork* db_network = resizer_.dbNetwork();
  db_network->staToDb(load_pin, load_iterm, load_bterm, load_moditerm);

  odb::dbNet* db_net = nullptr;
  if (load_iterm != nullptr) {
    db_net = load_iterm->getNet();
  } else if (load_bterm != nullptr) {
    db_net = load_bterm->getNet();
  }

  if (db_net == nullptr) {
    return nullptr;
  }

  // Use OpenDB connectivity after ECO so the side-path golden rows follow
  // newly inserted buffers even before STA path selection exposes them.
  odb::dbSet<odb::dbITerm> iterms = db_net->getITerms();
  for (odb::dbITerm* iterm : iterms) {
    if (iterm == nullptr) {
      continue;
    }

    const odb::dbIoType io_type = iterm->getIoType();
    if (io_type == odb::dbIoType::OUTPUT || io_type == odb::dbIoType::INOUT) {
      return db_network->dbToSta(iterm);
    }
  }

  odb::dbSet<odb::dbBTerm> bterms = db_net->getBTerms();
  for (odb::dbBTerm* bterm : bterms) {
    if (bterm == nullptr) {
      continue;
    }

    const odb::dbIoType io_type = bterm->getIoType();
    if (io_type == odb::dbIoType::INPUT || io_type == odb::dbIoType::INOUT) {
      return db_network->dbToSta(bterm);
    }
  }

  return nullptr;
}

const sta::TimingArc* DelayEstimatorReporter::firstGraphArc(
    sta::Pin* input_pin,
    sta::Pin* driver_pin,
    const sta::RiseFall* input_rf,
    const sta::RiseFall* output_rf) const
{
  sta::Vertex* input_vertex = graph_->pinLoadVertex(input_pin);
  sta::Vertex* driver_vertex = graph_->pinDrvrVertex(driver_pin);
  if (input_vertex == nullptr || driver_vertex == nullptr) {
    return nullptr;
  }

  sta::VertexInEdgeIterator edge_iter(driver_vertex, graph_);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    if (edge->from(graph_) != input_vertex) {
      continue;
    }

    for (const sta::TimingArc* arc : edge->timingArcSet()->arcs()) {
      if (arc->fromEdge()->asRiseFall() == input_rf
          && arc->toEdge()->asRiseFall() == output_rf) {
        return arc;
      }
    }
  }

  return nullptr;
}

std::vector<DelayEstimatorReporter::StageProfileRow>
DelayEstimatorReporter::buildSidePathCurrentRows(
    const EstimatorContext& target_context,
    const BufferTreeSidePathContext& side_context,
    const RebufferTreePlan& plan) const
{
  std::vector<StageProfileRow> rows;
  const DelayStageState& target_stage = target_context.target();
  const std::string driver_name = network_->pathName(plan.driver_pin);
  const std::string load_name = side_context.load_pin != nullptr
                                    ? network_->pathName(side_context.load_pin)
                                    : "<load>";
  rows.push_back({.path_index = target_stage.path_index,
                  .role = Role::kFanin,
                  .pin_name = fmt::format(
                      "{} -> {} <pre-ECO side branch>", driver_name, load_name),
                  .cell_name = target_stage.arc.currentCell()->name(),
                  .input_slew = target_stage.input_slew,
                  .cell_delay = target_stage.current_delay,
                  .wire_delay = side_context.current_driver_to_load_delay,
                  .load_cap = target_stage.load_cap,
                  .output_slew = target_stage.current_slew,
                  .extra_delay = side_context.current_driver_to_load_delay});
  return rows;
}

std::vector<DelayEstimatorReporter::StageProfileRow>
DelayEstimatorReporter::buildSidePathCandidateRows(
    const BufferTreeSidePathContext& side_context,
    const BufferTreeSidePathEvaluation& side_eval,
    const RebufferTreePlan& plan) const
{
  std::vector<StageProfileRow> rows;
  const BufferTreeLoadPath* load_path = plan.loadPath(side_context.load_pin);
  if (load_path == nullptr || side_eval.trace.empty()) {
    return rows;
  }

  const StageEvaluation& driver_eval = side_eval.trace.front();
  rows.push_back({.path_index = driver_eval.path_index,
                  .role = Role::kFanin,
                  .pin_name = plan.driver_pin != nullptr
                                  ? network_->pathName(plan.driver_pin)
                                  : "<driver>",
                  .cell_name = driver_eval.cell != nullptr
                                   ? driver_eval.cell->name()
                                   : kDashCell,
                  .input_slew = driver_eval.input_slew,
                  .cell_delay = driver_eval.stage_delay,
                  .wire_delay = 0.0f,
                  .load_cap = driver_eval.load_cap,
                  .output_slew = driver_eval.output_slew,
                  .extra_delay = 0.0f,
                  .arc_match_relaxed = driver_eval.arc_match_relaxed});

  int trace_index = 1;
  int buffer_index = 0;
  int active_output_row_index = 0;
  bool has_side_load_wire_row = false;
  StageProfileRow side_load_wire_row;
  for (size_t stage_index = 0; stage_index < load_path->stages.size();
       ++stage_index) {
    if (trace_index >= static_cast<int>(side_eval.trace.size())) {
      break;
    }

    const BufferTreePathStage& tree_stage = load_path->stages[stage_index];
    const StageEvaluation& eval = side_eval.trace[trace_index];
    ++trace_index;

    if (isRebufferTreeWireStage(tree_stage)) {
      if (!load_path->hasBufferAfter(stage_index)) {
        if (!has_side_load_wire_row) {
          side_load_wire_row = loadWireRow(
              fmt::format("{} <side-load wire>",
                          network_->pathName(side_context.load_pin)),
              eval.path_index,
              eval.input_slew,
              eval.wire_delay,
              rows[active_output_row_index].load_cap,
              eval.arc_match_relaxed);
          has_side_load_wire_row = true;
        } else {
          foldWire(side_load_wire_row, eval);
          side_load_wire_row.load_cap = rows[active_output_row_index].load_cap;
        }
        continue;
      }

      foldWire(rows[active_output_row_index], eval);
      continue;
    }

    rows.push_back(
        {.path_index = eval.path_index,
         .role = Role::kTarget,
         .pin_name
         = rebufferTreeStageName(plan, side_context.load_pin, buffer_index),
         .cell_name = eval.cell != nullptr ? eval.cell->name() : kDashCell,
         .input_slew = eval.input_slew,
         .cell_delay = eval.stage_delay,
         .wire_delay = 0.0f,
         .load_cap = eval.load_cap,
         .output_slew = eval.output_slew,
         .extra_delay = 0.0f,
         .arc_match_relaxed = eval.arc_match_relaxed});
    active_output_row_index = static_cast<int>(rows.size()) - 1;
    ++buffer_index;
  }

  if (has_side_load_wire_row) {
    rows.push_back(side_load_wire_row);
  }

  return rows;
}

DelayEstimatorReporter::StageProfileRow DelayEstimatorReporter::loadWireRow(
    const std::string& pin_name,
    const int path_index,
    const float input_slew,
    const float wire_delay,
    const float load_cap,
    const bool arc_match_relaxed)
{
  return {.path_index = path_index,
          .role = Role::kFanout,
          .pin_name = pin_name,
          .cell_name = "tree-wire",
          .input_slew = input_slew,
          .cell_delay = kMissingValue,
          .wire_delay = wire_delay,
          .load_cap = load_cap,
          .output_slew = kMissingValue,
          .extra_delay = wire_delay,
          .arc_match_relaxed = arc_match_relaxed};
}

void DelayEstimatorReporter::foldWire(StageProfileRow& row,
                                      const StageEvaluation& eval)
{
  row.wire_delay += eval.wire_delay;
  row.extra_delay += eval.wire_delay;
  row.arc_match_relaxed = row.arc_match_relaxed || eval.arc_match_relaxed;
}

void DelayEstimatorReporter::splitFinalLoadWire(
    std::vector<StageProfileRow>& rows,
    const std::string& load_pin_name,
    const char* row_label) const
{
  if (rows.empty()) {
    return;
  }

  int insert_index = static_cast<int>(rows.size());
  for (int row_index = 1; row_index < static_cast<int>(rows.size());
       ++row_index) {
    if (rows[row_index].role == Role::kFanout) {
      insert_index = row_index;
      break;
    }
  }

  const int source_index = insert_index - 1;
  if (source_index < 0 || !rows[source_index].valid
      || !std::isfinite(rows[source_index].wire_delay)
      || rows[source_index].wire_delay == 0.0f) {
    return;
  }

  // Keep the final load segment visible as a separate fanout stage
  // instead of folding it into the upstream driver or inserted buffer row.
  StageProfileRow& source = rows[source_index];
  const StageProfileRow load_wire_row
      = loadWireRow(fmt::format("{} <{}>", load_pin_name, row_label),
                    source.path_index,
                    source.output_slew,
                    source.wire_delay,
                    source.load_cap,
                    source.arc_match_relaxed);
  source.wire_delay = 0.0f;
  source.extra_delay = 0.0f;
  rows.insert(rows.begin() + insert_index, load_wire_row);
}

void DelayEstimatorReporter::insertSidePathPlaceholders(
    std::vector<StageProfileRow>& rows,
    const int branch_row_count) const
{
  if (branch_row_count <= 1 || rows.empty()) {
    return;
  }

  std::vector<StageProfileRow> placeholders;
  placeholders.reserve(branch_row_count - 1);
  for (int placeholder_index = 1; placeholder_index < branch_row_count;
       ++placeholder_index) {
    placeholders.push_back(makeMissingStageRow(
        Role::kTarget,
        fmt::format("<pre-ECO no inserted buffer {}>", placeholder_index)));
  }

  rows.insert(rows.begin() + 1, placeholders.begin(), placeholders.end());
}

void DelayEstimatorReporter::alignSidePathCandidateNames(
    std::vector<StageProfileRow>& candidate_rows,
    const std::vector<StageProfileRow>& golden_after_rows) const
{
  const size_t row_count
      = std::min(candidate_rows.size(), golden_after_rows.size());
  for (size_t row_index = 1; row_index < row_count; ++row_index) {
    StageProfileRow& candidate_row = candidate_rows[row_index];
    const StageProfileRow& golden_after_row = golden_after_rows[row_index];
    if (candidate_row.role != Role::kTarget || !golden_after_row.valid
        || golden_after_row.pin_name.empty()) {
      continue;
    }

    candidate_row.pin_name = golden_after_row.pin_name;
  }
}

DelayEstimatorReporter::EstimatorProfile
DelayEstimatorReporter::buildDelayEstimatorProfile(
    const Target& target,
    sta::LibertyCell* replacement,
    const int delay_levels) const
{
  EstimatorProfile profile;

  FailReason fail_reason = FailReason::kNone;
  const std::optional<EstimatorContext> context
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
  profile.legal = estimate.legal;
  profile.fail_reason = estimate.reason;

  // path_stages always non-empty by construction; target_stage_index points
  // to the stage being swapped.
  const std::vector<DelayStageState>& stages = context->path_stages;
  const int target_index = context->target_stage_index;

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
  for (int stage_index = 0; std::cmp_less(stage_index, stages.size());
       ++stage_index) {
    profile.current_stages.push_back(buildEstimatedCurrentStageRow(
        stages[stage_index], stage_index, target_index));
  }

  // candidate_stages: pull per-stage compute directly from the trace.
  profile.candidate_stages.reserve(trace.size());
  for (int stage_index = 0; std::cmp_less(stage_index, trace.size());
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
         .extra_delay = 0.0f,
         .arc_match_relaxed = eval.arc_match_relaxed});
  }

  return profile;
}

std::vector<DelayEstimatorReporter::FixedStage>
DelayEstimatorReporter::captureFixedStages(const Target& target,
                                           const int delay_levels) const
{
  std::vector<FixedStage> fixed_stages;
  FailReason fail_reason = FailReason::kNone;
  const std::optional<EstimatorContext> context
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
  for (int stage_index = 0; std::cmp_less(stage_index, stages.size());
       ++stage_index) {
    const DelayStageState& stage = stages[stage_index];
    const sta::Path* driver_path = expanded.path(stage.path_index);
    const sta::Path* input_path
        = driver_path != nullptr ? driver_path->prevPath() : nullptr;

    FixedStage fixed_stage;
    fixed_stage.state = stage;
    fixed_stage.role = roleFor(stage_index, target_index);
    fixed_stage.driver_pin = stage.driver_pin;
    fixed_stage.driver_pin_name = stageDriverPinName(stage);

    sta::Pin* input_pin = input_path != nullptr
                              ? input_path->pin(resizer_.staState())
                              : nullptr;
    if (input_pin != nullptr) {
      fixed_stage.input_pin = input_pin;
      fixed_stage.input_pin_name = network_->pathName(input_pin);
    }

    fixed_stages.push_back(std::move(fixed_stage));
  }

  assignNextPins(fixed_stages, expanded, nullptr, -1);
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

  sta::Pin* input_pin = fixed_stage.input_pin != nullptr
                            ? fixed_stage.input_pin
                            : network_->findPin(fixed_stage.input_pin_name);
  sta::Pin* driver_pin = fixed_stage.driver_pin != nullptr
                             ? fixed_stage.driver_pin
                             : network_->findPin(fixed_stage.driver_pin_name);
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

  const int dcalc_ap = fixed_stage.state.arc.scene->dcalcAnalysisPtIndex(
      fixed_stage.state.arc.min_max);

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

  // For the target replacement only, mirror DelayEstimator's relaxed fallback
  // so default current arcs can be compared against conditional replacement
  // arcs in the golden post-ECO table.
  // Try a relaxed match first, then any arc.
  bool arc_match_relaxed = false;
  float selected_cell_delay = 0.0f;
  if (gate_edge == nullptr && fixed_stage.role == Role::kTarget) {
    arc_match_relaxed = pickWorstArc(fixed_stage.state,
                                     input_vertex,
                                     driver_vertex,
                                     dcalc_ap,
                                     /* relaxed_only */ true,
                                     gate_edge,
                                     gate_arc,
                                     selected_cell_delay)
                        || pickWorstArc(fixed_stage.state,
                                        input_vertex,
                                        driver_vertex,
                                        dcalc_ap,
                                        /* relaxed_only */ false,
                                        gate_edge,
                                        gate_arc,
                                        selected_cell_delay);
  }
  if (gate_edge == nullptr || gate_arc == nullptr) {
    if (fixed_stage.role == Role::kTarget) {
      row.input_slew = sta::delayAsFloat(graph_->slew(
          input_vertex, fixed_stage.state.arc.inputRiseFall(), dcalc_ap));
      row.cell_delay
          = arrivalAtDriver(driver_pin, fixed_stage.state.arc.scene)
            - arrivalAtDriver(input_pin, fixed_stage.state.arc.scene);
      row.load_cap
          = sta_->graphDelayCalc()->loadCap(driver_pin,
                                            fixed_stage.state.arc.scene,
                                            fixed_stage.state.arc.min_max);
      row.output_slew = sta::delayAsFloat(graph_->slew(
          driver_vertex, fixed_stage.state.arc.outputRiseFall(), dcalc_ap));
      row.arc_match_relaxed = true;
      return row;
    }
    return row;
  }

  row.input_slew = sta_->graphDelayCalc()->edgeFromSlew(
      gate_edge->from(graph_),
      fixed_stage.state.arc.inputRiseFall(),
      gate_edge,
      fixed_stage.state.arc.scene,
      fixed_stage.state.arc.min_max);
  row.cell_delay = arc_match_relaxed ? selected_cell_delay
                                     : sta::delayAsFloat(graph_->arcDelay(
                                           gate_edge, gate_arc, dcalc_ap));
  row.load_cap = sta_->graphDelayCalc()->loadCap(
      driver_pin, fixed_stage.state.arc.scene, fixed_stage.state.arc.min_max);
  row.output_slew = sta::delayAsFloat(graph_->slew(
      driver_vertex, fixed_stage.state.arc.outputRiseFall(), dcalc_ap));
  row.arc_match_relaxed = arc_match_relaxed;
  return row;
}

// === Path / context utilities =============================================

bool DelayEstimatorReporter::pickWorstArc(const DelayStageState& stage,
                                          sta::Vertex* input_vertex,
                                          sta::Vertex* driver_vertex,
                                          const int dcalc_ap,
                                          const bool relaxed_only,
                                          sta::Edge*& gate_edge,
                                          const sta::TimingArc*& gate_arc,
                                          float& delay) const
{
  bool found = false;
  sta::VertexInEdgeIterator edge_iter(driver_vertex, graph_);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    if (edge->from(graph_) != input_vertex) {
      continue;
    }

    for (const sta::TimingArc* arc : edge->timingArcSet()->arcs()) {
      if (relaxed_only
          && matchTimingArc(
                 stage.arc.ref_arc, arc, ArcMatchMode::kRelaxedCandidate)
                 != ArcMatchType::kRelaxed) {
        continue;
      }

      const float arc_delay
          = sta::delayAsFloat(graph_->arcDelay(edge, arc, dcalc_ap));
      if (!found || stage.arc.min_max->compare(arc_delay, delay)) {
        gate_edge = edge;
        gate_arc = arc;
        delay = arc_delay;
        found = true;
      }
    }
  }

  return found;
}

std::optional<EstimatorContext> DelayEstimatorReporter::contextForTarget(
    const Target& target,
    const int delay_levels,
    FailReason* fail_reason) const
{
  if (target.estimator_context.has_value()
      && target.estimator_context->delay_estimation_levels == delay_levels) {
    return target.estimator_context;
  }
  return DelayEstimator::buildContext(
      resizer_, target, delay_levels, fail_reason, kStaSlewBias);
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
  target.estimator_context = DelayEstimator::buildContext(resizer_,
                                                          inst,
                                                          driver_pin,
                                                          expanded,
                                                          path_index,
                                                          scene,
                                                          min_max,
                                                          delay_levels,
                                                          &ignored_reason,
                                                          kStaSlewBias);
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

  sta::Pin* driver_pin = fixed_stage.driver_pin != nullptr
                             ? fixed_stage.driver_pin
                             : network_->findPin(fixed_stage.driver_pin_name);
  sta::Pin* next_pin = fixed_stage.next_pin != nullptr
                           ? fixed_stage.next_pin
                           : network_->findPin(fixed_stage.next_pin_name);
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

  return arrivalAtDriver(next_pin, fixed_stage.state.arc.scene)
         - arrivalAtDriver(driver_pin, fixed_stage.state.arc.scene);
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

float DelayEstimatorReporter::arrivalAtPinName(const std::string& pin_name,
                                               const sta::Scene* scene) const
{
  sta::Pin* pin = network_->findPin(pin_name);
  if (pin == nullptr) {
    logger_->error(
        RSZ, 3213, "Could not find arrival reference pin {}.", pin_name);
  }
  return arrivalAtDriver(pin, scene);
}

float DelayEstimatorReporter::arrivalAtDriver(sta::Pin* driver_pin,
                                              const sta::Scene* scene) const
{
  sta::Vertex* driver_vertex = graph_->pinDrvrVertex(driver_pin);
  if (driver_vertex == nullptr) {
    driver_vertex = graph_->pinLoadVertex(driver_pin);
  }

  if (driver_vertex == nullptr) {
    logger_->error(RSZ,
                   3224,
                   "Could not find arrival vertex for {}.",
                   network_->pathName(driver_pin));
  }
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

const DelayEstimatorReporter::StageProfileRow* DelayEstimatorReporter::validRow(
    const std::vector<StageProfileRow>& rows,
    const size_t index)
{
  return index < rows.size() && rows[index].valid ? &rows[index] : nullptr;
}

void DelayEstimatorReporter::printStageWindowDetail(
    const std::vector<StageProfileRow>& estimated_current,
    const std::vector<StageProfileRow>& estimated_candidate,
    const std::vector<StageProfileRow>& golden_before,
    const std::vector<StageProfileRow>& golden_after) const
{
  // The per-stage metrics, each printed before and after the ECO.
  struct StageMetric
  {
    const char* name;
    float StageProfileRow::* field;
    float scale;
  };

  const std::array<StageMetric, 5> metrics{{
      {"input slew(ps)", &StageProfileRow::input_slew, kSecondsToPicoseconds},
      {"cell delay(ps)", &StageProfileRow::cell_delay, kSecondsToPicoseconds},
      {"output slew(ps)", &StageProfileRow::output_slew, kSecondsToPicoseconds},
      {"wire delay(ps)", &StageProfileRow::wire_delay, kSecondsToPicoseconds},
      {"load cap(fF)", &StageProfileRow::load_cap, kFaradsToFemtofarads},
  }};

  logger_->report("");
  logger_->report("Stage window detail");
  logger_->report(
      "  estimated delay = cell_delay + extra_est; "
      "golden delay = cell_delay + wire_delay");

  const size_t row_count = std::max({estimated_current.size(),
                                     estimated_candidate.size(),
                                     golden_before.size(),
                                     golden_after.size()});
  for (size_t row_index = 0; row_index < row_count; ++row_index) {
    // The valid row of each profile, or null.
    const StageProfileRow* estimated_pre
        = validRow(estimated_current, row_index);
    const StageProfileRow* estimated_post
        = validRow(estimated_candidate, row_index);
    const StageProfileRow* golden_pre = validRow(golden_before, row_index);
    const StageProfileRow* golden_post = validRow(golden_after, row_index);

    // The estimated rows name the stage first, then the golden rows.
    const StageProfileRow& reference
        = estimated_pre    ? *estimated_pre
          : estimated_post ? *estimated_post
          : golden_pre     ? *golden_pre
                           : rowOrEmpty(golden_after, row_index);
    const std::string& pre_cell = estimated_pre ? estimated_pre->cell_name
                                  : golden_pre  ? golden_pre->cell_name
                                                : kDashCell;
    const std::string& post_cell = estimated_post ? estimated_post->cell_name
                                   : golden_post  ? golden_post->cell_name
                                                  : kDashCell;

    logger_->report("");
    logger_->report("[{}] {}  path_index={}  pin={}",
                    row_index,
                    roleName(reference.role),
                    reference.path_index,
                    reference.pin_name);
    const bool post_arc_relaxed
        = estimated_post != nullptr && estimated_post->arc_match_relaxed;
    logger_->report("    cell: pre={}  post={}{}",
                    pre_cell,
                    post_cell,
                    post_arc_relaxed ? "  arc_match: relaxed" : "");
    logger_->report("");
    logger_->report("    {:<31} {:>10}   {:>10}   {:>10}",
                    "metric",
                    "estimated",
                    "golden",
                    "error");

    // A missing row prints a missing value.
    for (const StageMetric& metric : metrics) {
      printMetricRow(
          logger_,
          std::string("pre-ECO ") + metric.name,
          estimated_pre ? estimated_pre->*metric.field : kMissingValue,
          golden_pre ? golden_pre->*metric.field : kMissingValue,
          metric.scale);
      printMetricRow(
          logger_,
          std::string("post-ECO ") + metric.name,
          estimated_post ? estimated_post->*metric.field : kMissingValue,
          golden_post ? golden_post->*metric.field : kMissingValue,
          metric.scale);
    }
    logger_->report(
        "    "
        "------------------------------------------------------------------");

    // Stage delays: cell plus extra estimate, against cell plus wire.
    const float estimated_pre_delay
        = estimated_pre ? estimatedStageScoreDelay(*estimated_pre)
                        : kMissingValue;
    const float estimated_post_delay
        = estimated_post ? estimatedStageScoreDelay(*estimated_post)
                         : kMissingValue;
    const float golden_pre_delay
        = golden_pre ? goldenStagePathDelay(*golden_pre) : kMissingValue;
    const float golden_post_delay
        = golden_post ? goldenStagePathDelay(*golden_post) : kMissingValue;
    const float estimated_impr
        = std::isfinite(estimated_pre_delay)
                  && std::isfinite(estimated_post_delay)
              ? estimated_pre_delay - estimated_post_delay
              : kMissingValue;
    printMetricRow(logger_,
                   "pre-ECO stage delay(ps)",
                   estimated_pre_delay,
                   golden_pre_delay,
                   kSecondsToPicoseconds);
    printMetricRow(logger_,
                   "post-ECO stage delay(ps)",
                   estimated_post_delay,
                   golden_post_delay,
                   kSecondsToPicoseconds);
    printMetricRow(logger_,
                   "pre-post delay delta(ps)",
                   estimated_impr,
                   golden_pre_delay - golden_post_delay,
                   kSecondsToPicoseconds);
  }
}

}  // namespace rsz
