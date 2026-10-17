// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "OptimizationPolicy.hh"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "BufferGenerator.hh"
#include "BufferToInvertersGenerator.hh"
#include "CloneGenerator.hh"
#include "DelayEstimator.hh"
#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#include "Rebuffer.hh"
#include "RepairSetupContext.hh"
#include "RepairSetupProfiler.hh"
#include "RepairTargetCollector.hh"
#include "RerouteGenerator.hh"
#include "SequentialTimingEstimator.hh"
#include "SizeDownFanoutGenerator.hh"
#include "SizeUpGenerator.hh"
#include "SizeUpMatchGenerator.hh"
#include "SplitLoadGenerator.hh"
#include "SwapPinsGenerator.hh"
#include "UnbufferGenerator.hh"
#include "VtSwapGenerator.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/StaState.hh"
#include "utl/Logger.h"
#include "utl/ThreadPool.h"

namespace rsz {

RepairSetupContext::RepairSetupContext(Resizer& resizer)
    : target_collector(&resizer),
      initial_design_area(resizer.computeDesignArea()),
      initial_tns(target_collector.totalNegativeSlack()),
      previous_tns(initial_tns)
{
}

char phaseMarkerForIndex(int phase_index)
{
  constexpr char special_markers[] = "*+^&@!-=";
  constexpr int num_special = 8;
  if (phase_index < num_special) {
    return special_markers[phase_index];
  }
  phase_index -= num_special;
  if (phase_index < 26) {
    return 'a' + phase_index;
  }
  phase_index -= 26;
  if (phase_index < 26) {
    return 'A' + phase_index;
  }
  return '?';
}

OptimizationPolicy::OptimizationPolicy(Resizer& resizer,
                                       MoveCommitter& committer,
                                       RepairSetupContext& setup_context,
                                       const OptimizerRunConfig& config,
                                       OptimizationPolicyConfig policy_config)
    : resizer_(resizer),
      committer_(committer),
      setup_context_(setup_context),
      config_(config),
      policy_config_(std::move(policy_config)),
      target_collector_(&setup_context_.target_collector),
      progress_(resizer, committer, setup_context)
{
}

OptimizationPolicy::~OptimizationPolicy() = default;

bool OptimizationPolicy::start()
{
  logger_ = resizer_.logger();
  sta_ = resizer_.sta();
  network_ = resizer_.network();
  graph_ = resizer_.graph();
  estimate_parasitics_ = resizer_.estimateParasitics();
  max_ = resizer_.maxAnalysisMode();
  resetRun();
  estimator_cache_.clear();
  equiv_pin_cache_.clear();
  // Start every phase with the progress table's area cache invalid. The
  // cache is keyed on MoveCommitter::netlistEdits(), which only counts
  // edits made through the committer; GlobalSizingPolicy replaces cells
  // directly (applyPresize/applyDecisions), so a GLOBAL_SIZING phase
  // between two legacy-derived phases would otherwise leave the later
  // phase printing the pre-sizing area. One area walk per phase.
  setup_context_.progress_area_at_edit = -1;
  if (is_experimental) {
    logger_->warn(utl::RSZ,
                  2024,
                  "Experimental repair setup policy '{}' selected. Do not "
                  "use this for production.",
                  name());
  }
  return true;
}

GeneratorContext OptimizationPolicy::makeGeneratorContext() const
{
  return GeneratorContext{
      .resizer = resizer_,
      .committer = committer_,
      .run_config = config_,
      .policy_config = policy_config_,
      .critical_load_pins = &target_collector_->critPathLoadPins(),
      .equiv_pin_cache = &equiv_pin_cache_};
}

void OptimizationPolicy::buildMoveGenerators(
    const std::vector<MoveType>& move_types,
    const GeneratorContext& context)
{
  // Default move generator creation
  move_generators_.clear();
  move_generators_.reserve(move_types.size());
  for (const MoveType type : move_types) {
    std::unique_ptr<MoveGenerator> generator;
    switch (type) {
      case MoveType::kVtSwap:
        generator = std::make_unique<VtSwapGenerator>(context);
        break;
      case MoveType::kSizeUp:
        generator = std::make_unique<SizeUpGenerator>(context);
        break;
      case MoveType::kSizeUpMatch:
        generator = std::make_unique<SizeUpMatchGenerator>(context);
        break;
      case MoveType::kBuffer:
        generator = std::make_unique<BufferGenerator>(context);
        break;
      case MoveType::kClone:
        generator = std::make_unique<CloneGenerator>(context);
        break;
      case MoveType::kSplitLoad:
        generator = std::make_unique<SplitLoadGenerator>(context);
        break;
      case MoveType::kSizeDownFanout:
        generator = std::make_unique<SizeDownFanoutGenerator>(context);
        break;
      case MoveType::kSwapPins:
        generator = std::make_unique<SwapPinsGenerator>(context);
        break;
      case MoveType::kUnbuffer:
        generator = std::make_unique<UnbufferGenerator>(context);
        break;
      case MoveType::kBufferToInverters:
        generator = std::make_unique<BufferToInvertersGenerator>(context);
        break;
      case MoveType::kReroute:
        generator = std::make_unique<RerouteGenerator>(context);
        break;
      case MoveType::kCount:
        break;
    }
    if (generator != nullptr) {
      move_generators_.push_back(std::move(generator));
    }
  }
}

PrepareCacheMask OptimizationPolicy::accumulatePrepareRequirements() const
{
  PrepareCacheMask prepare_mask = kNoPrepareCache;
  for (const std::unique_ptr<MoveGenerator>& generator : move_generators_) {
    if (!generatorEnabled(generator->type())) {
      continue;
    }
    prepare_mask |= generator->prepareRequirements();
  }
  return prepare_mask;
}

MoveGenerator* OptimizationPolicy::findGenerator(const MoveType type) const
{
  for (const std::unique_ptr<MoveGenerator>& generator : move_generators_) {
    if (generator->type() == type) {
      return generator.get();
    }
  }
  return nullptr;
}

void OptimizationPolicy::prepareTargets(std::vector<Target>& targets) const
{
  const ProfileScope profile("prepare");
  RepairSetupProfiler::event("targets_prepared", targets.size());
  const PrepareCacheMask mask = accumulatePrepareRequirements();
  for (Target& target : targets) {
    prepareTarget(target, mask);
  }
}

Target OptimizationPolicy::prepareTarget(const Target& target) const
{
  const ProfileScope profile("prepare");
  RepairSetupProfiler::event("targets_prepared");
  Target prepared_target = target;
  prepareTarget(prepared_target, accumulatePrepareRequirements());
  return prepared_target;
}

void OptimizationPolicy::prewarmTargets(
    const std::vector<Target>& targets) const
{
  const ProfileScope profile("prepare");
  const bool prewarm_swappable_cells
      = findGenerator(MoveType::kSizeUp) != nullptr;
  const bool prewarm_vt_equiv_cells
      = findGenerator(MoveType::kVtSwap) != nullptr;
  prewarmTargetLibertyCaches(
      targets, prewarm_swappable_cells, prewarm_vt_equiv_cells);
}

bool OptimizationPolicy::generatorEnabled(const MoveType) const
{
  return true;
}

void OptimizationPolicy::prepareTarget(Target& target,
                                       const PrepareCacheMask mask) const
{
  if ((mask & kEstimatorContextCache) != 0) {
    prepareEstimatorContext(target);
  }
  if ((mask & kSequentialEstimatorCache) != 0) {
    prepareSequentialEstimator(target);
  }
}

std::optional<EstimatorContext> OptimizationPolicy::buildPolicyContext(
    Resizer& resizer,
    const Target& target,
    const OptimizationPolicyConfig& policy_config,
    const std::unordered_set<const sta::Pin*>* critical_load_pins,
    const bool include_latch_fanin,
    DelayEstimatorCache* cache,
    FailReason* fail_reason)
{
  const FaninSideLoadOptions fanin_side_loads{
      .critical_load_pins = critical_load_pins,
      .off_path_weight = policy_config.fanin_side_offpath_weight};
  std::optional<EstimatorContext> context = DelayEstimator::buildContext(
      resizer,
      target,
      policy_config.delay_estimation_levels,
      fail_reason,
      policy_config.sta_slew_bias,
      include_latch_fanin,
      cache,
      policy_config.fanin_side_loss ? &fanin_side_loads : nullptr,
      policy_config.dual_rf_eval);

  if ((policy_config.fanin_side_loss || policy_config.dual_rf_eval
       || policy_config.rebuffer_mt_side_loads)
      && context.has_value()) {
    // Side paths may not be pushed below the current global worst slack.
    context->side_slack_floor = sta::delayAsFloat(
        resizer.sta()->worstSlack(resizer.maxAnalysisMode()));
    context->side_floor_margin = policy_config.side_floor_margin;
    context->fanin_side_loss_weight = policy_config.fanin_side_loss_weight;
  }
  return context;
}

void OptimizationPolicy::prepareEstimatorContext(Target& target) const
{
  if (target.estimator_context.has_value() || !target.canBePathDriver()) {
    return;
  }

  FailReason fail_reason = FailReason::kNone;
  {
    const ProfileScope profile("prepare_context");
    target.estimator_context
        = buildPolicyContext(resizer_,
                             target,
                             policy_config_,
                             &target_collector_->critPathLoadPins(),
                             /* include_latch_fanin */ true,
                             &estimator_cache_,
                             &fail_reason);
  }

  if (!target.estimator_context.has_value() && RepairSetupProfiler::enabled()) {
    RepairSetupProfiler::event(std::string("prepare_reject_")
                               + failReasonName(fail_reason));
  }
}

void OptimizationPolicy::prepareSequentialEstimator(Target& target) const
{
  // Only SizeUpMt and VtSwapMt read it, so a run without them skips the
  // flip-flop snapshots.
  if (target.sequential_timing_estimator != nullptr
      || !target.estimator_context.has_value()) {
    return;
  }

  target.sequential_timing_estimator
      = SequentialTimingEstimator::prepare(resizer_, target);
  if (target.sequential_timing_estimator != nullptr) {
    RepairSetupProfiler::event("sequential_targets_prepared");
  }
}

void OptimizationPolicy::prewarmTargetLibertyCaches(
    const std::vector<Target>& targets,
    const bool prewarm_swappable_cells,
    const bool prewarm_vt_equiv_cells) const
{
  if (!prewarm_swappable_cells && !prewarm_vt_equiv_cells) {
    return;
  }

  std::unordered_set<sta::LibertyCell*> cells_to_prewarm;
  for (const Target& target : targets) {
    sta::Instance* inst = target.inst(resizer_);
    if (inst == nullptr) {
      continue;
    }
    sta::LibertyCell* current_cell = resizer_.network()->libertyCell(inst);
    if (current_cell != nullptr) {
      cells_to_prewarm.insert(current_cell);
    }
  }

  for (sta::LibertyCell* current_cell : cells_to_prewarm) {
    odb::dbMaster* current_master = resizer_.dbNetwork()->staToDb(current_cell);
    if (current_master != nullptr) {
      static_cast<void>(resizer_.cellVTType(current_master));
    }

    if (prewarm_swappable_cells) {
      const sta::LibertyCellSeq swappable_cells
          = resizer_.getSwappableCells(current_cell);
      for (sta::LibertyCell* swappable_cell : swappable_cells) {
        odb::dbMaster* swappable_master
            = resizer_.dbNetwork()->staToDb(swappable_cell);
        if (swappable_master != nullptr) {
          static_cast<void>(resizer_.cellVTType(swappable_master));
        }
      }
    }

    if (prewarm_vt_equiv_cells) {
      const sta::LibertyCellSeq vt_equiv_cells
          = resizer_.getVTEquivCells(current_cell);
      for (sta::LibertyCell* equiv_cell : vt_equiv_cells) {
        odb::dbMaster* equiv_master = resizer_.dbNetwork()->staToDb(equiv_cell);
        if (equiv_master != nullptr) {
          static_cast<void>(resizer_.cellVTType(equiv_master));
        }
      }
    }
  }
}

sta::Slack OptimizationPolicy::totalNegativeSlack() const
{
  const ProfileScope profile("qor_collection");
  return target_collector_->totalNegativeSlack();
}

void OptimizationPolicy::prewarmStaForPrepareStage() const
{
  sta::dbSta* sta = resizer_.sta();
  // Capacitance checks do not run their own preamble, so refresh only that
  // service before prepare-stage snapshots are rebuilt.
  sta->checkCapacitancesPreamble(sta->scenes());
}

std::unique_ptr<utl::ThreadPool> OptimizationPolicy::makeWorkerThreadPool()
    const
{
  // Honor OpenROAD thread budget and keep the main thread for orchestration.
  // Zero workers fall back to caller-thread execution.
  const int thread_count = static_cast<int>(resizer_.staState()->threadCount());
  const int worker_thread_count = std::max(0, thread_count - 1);
  return std::make_unique<utl::ThreadPool>(worker_thread_count);
}

}  // namespace rsz
