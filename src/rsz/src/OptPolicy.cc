// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "OptPolicy.hh"

#include <algorithm>
#include <unordered_set>

#include "BufferGenerator.hh"
#include "CloneGenerator.hh"
#include "DelayEstimator.hh"
#include "MoveCommitter.hh"
#include "SizeDownGenerator.hh"
#include "SizeUpGenerator.hh"
#include "SizeUpMatchGenerator.hh"
#include "SplitLoadGenerator.hh"
#include "SwapPinsGenerator.hh"
#include "UnbufferGenerator.hh"
#include "VtSwapGenerator.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Fuzzy.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/StaState.hh"
#include "utl/Logger.h"
#include "utl/ThreadPool.h"

namespace rsz {

OptPolicy::OptPolicy(Resizer& resizer, MoveCommitter& committer)
    : resizer_(resizer), committer_(committer)
{
}

OptPolicy::~OptPolicy() = default;

void OptPolicy::start(const OptimizerRunConfig& config)
{
  config_ = config;
  logger_ = resizer_.logger();
  sta_ = resizer_.sta();
  network_ = resizer_.network();
  graph_ = resizer_.graph();
  estimate_parasitics_ = resizer_.estimateParasitics();
  max_ = resizer_.maxAnalysisMode();
  resetRun();
}

GeneratorContext OptPolicy::makeGeneratorContext(
    const OptimizerRunConfig& run_config) const
{
  return GeneratorContext{.resizer = resizer_,
                          .committer = committer_,
                          .run_config = run_config,
                          .policy_config = policy_config_};
}

void OptPolicy::buildMoveGenerators(const std::vector<MoveType>& move_types,
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
      case MoveType::kSizeDown:
        generator = std::make_unique<SizeDownGenerator>(context);
        break;
      case MoveType::kSwapPins:
        generator = std::make_unique<SwapPinsGenerator>(context);
        break;
      case MoveType::kUnbuffer:
        generator = std::make_unique<UnbufferGenerator>(context);
        break;
      case MoveType::kCount:
        break;
    }
    if (generator != nullptr) {
      move_generators_.push_back(std::move(generator));
    }
  }
}

PrepareCacheMask OptPolicy::accumulatePrepareRequirements() const
{
  PrepareCacheMask prepare_mask = prepareCacheMask(PrepareCacheKind::kNone);
  for (const std::unique_ptr<MoveGenerator>& generator : move_generators_) {
    if (!generatorEnabled(generator->type())) {
      continue;
    }
    prepare_mask |= generator->prepareRequirements();
  }
  return prepare_mask;
}

MoveGenerator* OptPolicy::findGenerator(const MoveType type) const
{
  for (const std::unique_ptr<MoveGenerator>& generator : move_generators_) {
    if (generator->type() == type) {
      return generator.get();
    }
  }
  return nullptr;
}

void OptPolicy::prepareTargets(std::vector<Target>& targets) const
{
  const PrepareCacheMask mask = accumulatePrepareRequirements();
  for (Target& target : targets) {
    prepareTarget(target, mask);
  }
}

Target OptPolicy::prepareTarget(const Target& target) const
{
  Target prepared_target = target;
  prepareTarget(prepared_target, accumulatePrepareRequirements());
  return prepared_target;
}

void OptPolicy::prewarmTargets(const std::vector<Target>& targets) const
{
  prewarmTargetLibertyCaches(targets,
                             findGenerator(MoveType::kSizeUp) != nullptr,
                             findGenerator(MoveType::kVtSwap) != nullptr);
}

bool OptPolicy::targetPrewarmEnabled() const
{
  return false;
}

bool OptPolicy::generatorEnabled(const MoveType) const
{
  return true;
}

void OptPolicy::prepareTarget(Target& target, const PrepareCacheMask mask) const
{
  if (needToCache(mask, PrepareCacheKind::kArcDelayState)) {
    prepareArcDelayState(target);
  }
}

void OptPolicy::prepareArcDelayState(Target& target) const
{
  if (target.isPrepared(PrepareCacheKind::kArcDelayState)
      || !target.isKind(TargetKind::kPathDriver) || !target.isValid()) {
    return;
  }

  FailReason fail_reason = FailReason::kNone;
  target.arc_delay
      = DelayEstimator::buildContext(resizer_, target, &fail_reason);
}

void OptPolicy::prewarmTargetLibertyCaches(
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

bool OptPolicy::hasSetupViolations(const OptimizerRunConfig& config,
                                   const sta::MinMax* max) const
{
  return sta::fuzzyLess(resizer_.sta()->worstSlack(max),
                        config.setup_slack_margin);
}

sta::Slack OptPolicy::totalNegativeSlack(const sta::MinMax* max) const
{
  return resizer_.sta()->totalNegativeSlack(max);
}

void OptPolicy::prewarmStaForPrepareStage() const
{
  sta::dbSta* sta = resizer_.sta();
  // Capacitance checks do not run their own preamble, so refresh only that
  // service before prepare-stage snapshots are rebuilt.
  sta->checkCapacitancesPreamble(sta->scenes());
}

std::unique_ptr<utl::ThreadPool> OptPolicy::makeWorkerThreadPool() const
{
  // Honor OpenROAD thread budget and keep the main thread for orchestration.
  // Zero workers fall back to caller-thread execution.
  const int thread_count = static_cast<int>(resizer_.staState()->threadCount());
  const int worker_thread_count = std::max(0, thread_count - 1);
  return std::make_unique<utl::ThreadPool>(worker_thread_count);
}

}  // namespace rsz
