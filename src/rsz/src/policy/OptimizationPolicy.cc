// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "OptimizationPolicy.hh"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "BufferGenerator.hh"
#include "CloneGenerator.hh"
#include "DelayEstimator.hh"
#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#include "RepairSetupContext.hh"
#include "RepairTargetCollector.hh"
#include "RerouteGenerator.hh"
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
#include "sta/Fuzzy.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/StaState.hh"
#include "utl/Logger.h"
#include "utl/ThreadPool.h"
#include "utl/env.h"
#include "utl/mem_stats.h"

namespace rsz {

RepairSetupContext::RepairSetupContext(Resizer& resizer)
    : target_collector(&resizer),
      initial_design_area(resizer.computeDesignArea()),
      initial_tns(resizer.sta()->totalNegativeSlack(resizer.maxAnalysisMode())),
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
                                       const OptimizerRunConfig& config)
    : resizer_(resizer),
      committer_(committer),
      setup_context_(setup_context),
      config_(config),
      target_collector_(&setup_context_.target_collector)
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
  loadPolicyEnvars();
  if (is_experimental) {
    logger_->warn(utl::RSZ,
                  2024,
                  "Experimental repair setup policy '{}' selected. Do not "
                  "use this for production.",
                  name());
  }
  return true;
}

bool OptimizationPolicy::finalizeAndReport(const double initial_design_area)
{
  RepairTargetCollector final_targets(&resizer_);
  final_targets.init(config_.setup_slack_margin,
                     /*collect_startpoints=*/true);
  printFinalProgress(final_targets, initial_design_area);
  committer_.printTrackerFinalReports(finalReportPins());
  return reportRepairSummary();
}

void OptimizationPolicy::printProgressHeader() const
{
  if (setup_context_.progress_header_printed) {
    return;
  }
  setup_context_.progress_header_printed = true;
  logger_->report(
      "   Iter   | Removed | Resized | Inserted | Cloned |  Pin  |"
      "   Area   |    WNS   |   StTNS    |   EnTNS    |  Viol  |  Worst");
  logger_->report(
      "          | Buffers |  Gates  | Buffers  |  Gates | Swaps |"
      "          |          |            |            | Endpts | St/EnPt");
  logger_->report(
      "---------------------------------------------------------------"
      "---------------------------------------------------------------");
}

void OptimizationPolicy::printFinalProgress(
    const RepairTargetCollector& target_collector,
    const double initial_design_area) const
{
  printProgressHeader();

  const sta::Slack wns = target_collector.getWns();
  const sta::Slack st_tns = target_collector.getTns(true);
  const sta::Slack en_tns = target_collector.getTns(false);
  const sta::Pin* worst_pin = target_collector.getWorstPin(false);

  const double design_area = resizer_.computeDesignArea();
  const double area_growth = design_area - initial_design_area;
  double area_growth_percent = std::numeric_limits<double>::infinity();
  if (std::abs(initial_design_area) > 0.0) {
    area_growth_percent = area_growth / initial_design_area * 100.0;
  }

  logger_->report(
      "{: >9s} | {: >7d} | {: >7d} | {: >8d} | {: >6d} | {: >5d} "
      "| {: >+7.1f}% | {: >8s} | {: >10s} | {: >10s} | {: >6d} | {}",
      "final",
      committer_.totalMoves(MoveType::kUnbuffer),
      committer_.totalMoves(MoveType::kSizeUp)
          + committer_.totalMoves(MoveType::kSizeDownFanout)
          + committer_.totalMoves(MoveType::kSizeUpMatch)
          + committer_.totalMoves(MoveType::kVtSwap),
      committer_.totalMoves(MoveType::kBuffer)
          + committer_.totalMoves(MoveType::kSplitLoad),
      committer_.totalMoves(MoveType::kClone),
      committer_.totalMoves(MoveType::kSwapPins),
      area_growth_percent,
      sta::delayAsString(wns, 3, sta_),
      sta::delayAsString(st_tns, 1, sta_),
      sta::delayAsString(en_tns, 1, sta_),
      std::max(0, target_collector.getNumViolatingEndpoints()),
      worst_pin != nullptr ? network_->pathName(worst_pin) : "");

  debugPrint(logger_, utl::RSZ, "memory", 1, "RSS = {}", utl::getCurrentRSS());
  logger_->report(
      "---------------------------------------------------------------"
      "---------------------------------------------------------------");
}

const std::vector<const sta::Pin*>& OptimizationPolicy::finalReportPins() const
{
  return target_collector_->getViolatingPins();
}

bool OptimizationPolicy::reportRepairSummary() const
{
  bool repaired = false;

  const int buffer_moves = committer_.summaryCommittedMoves(MoveType::kBuffer);
  const int size_up_moves = committer_.summaryCommittedMoves(MoveType::kSizeUp);
  const int size_down_fanout_moves
      = committer_.summaryCommittedMoves(MoveType::kSizeDownFanout);
  const int swap_pins_moves
      = committer_.summaryCommittedMoves(MoveType::kSwapPins);
  const int clone_moves = committer_.summaryCommittedMoves(MoveType::kClone);
  const int split_load_moves
      = committer_.summaryCommittedMoves(MoveType::kSplitLoad);
  const int unbuffer_moves
      = committer_.summaryCommittedMoves(MoveType::kUnbuffer);
  const int vt_swap_moves = committer_.summaryCommittedMoves(MoveType::kVtSwap);
  const int size_up_match_moves
      = committer_.summaryCommittedMoves(MoveType::kSizeUpMatch);
  const int reroute_moves
      = committer_.summaryCommittedMoves(MoveType::kReroute);

  if (unbuffer_moves > 0) {
    repaired = true;
    logger_->info(utl::RSZ, 59, "Removed {} buffers.", unbuffer_moves);
  }
  if (buffer_moves > 0 || split_load_moves > 0) {
    repaired = true;
    if (split_load_moves == 0) {
      logger_->info(utl::RSZ, 40, "Inserted {} buffers.", buffer_moves);
    } else {
      logger_->info(utl::RSZ,
                    45,
                    "Inserted {} buffers, {} to split loads.",
                    buffer_moves + split_load_moves,
                    split_load_moves);
    }
  }
  logger_->metric("design__instance__count__setup_buffer",
                  buffer_moves + split_load_moves);
  if (size_up_moves + size_down_fanout_moves + size_up_match_moves
          + vt_swap_moves
      > 0) {
    repaired = true;
    logger_->info(utl::RSZ,
                  51,
                  "Resized {} instances: {} up, {} up match, {} down, {} VT",
                  size_up_moves + size_up_match_moves + size_down_fanout_moves
                      + vt_swap_moves,
                  size_up_moves,
                  size_up_match_moves,
                  size_down_fanout_moves,
                  vt_swap_moves);
  }
  if (swap_pins_moves > 0) {
    repaired = true;
    logger_->info(
        utl::RSZ, 43, "Swapped pins on {} instances.", swap_pins_moves);
  }
  if (clone_moves > 0) {
    repaired = true;
    logger_->info(utl::RSZ, 49, "Cloned {} instances.", clone_moves);
  }
  if (reroute_moves > 0) {
    repaired = true;
    logger_->info(
        utl::RSZ, 53, "Rerouted {} nets resistance-aware.", reroute_moves);
  }

  const sta::Slack worst_slack = sta_->worstSlack(max_);
  if (sta::fuzzyLess(worst_slack, config_.setup_slack_margin)) {
    repaired = true;
    logger_->warn(utl::RSZ, 62, "Unable to repair all setup violations.");
  }
  if (resizer_.overMaxArea()) {
    logger_->error(utl::RSZ, 25, "max utilization reached.");
  }

  return repaired;
}

void OptimizationPolicy::loadPolicyEnvars()
{
  // VtSwap candidate cap (per target, per generator); 0 means unlimited.
  policy_config_.max_candidate_generation
      = utl::readEnvarNonNegativeInt("RSZ_VTSWAP_CANDIDATES", 0);
  // Hard cap on accepted moves; 0 means unlimited (per OptimizationPolicyConfig
  // docs).
  policy_config_.max_committed_moves
      = utl::readEnvarNonNegativeInt("RSZ_VTSWAP_MAX_MOVES", 0);
  // Number of fanin/fanout stages included in MT delay estimation; 0 keeps
  // target-stage-only scoring.
  policy_config_.delay_estimation_levels
      = utl::readEnvarInt("RSZ_MT_DELAY_LEVELS", 1);
  // Experimental STA slew-bias sampling. Negative values are treated the same
  // as 0.
  policy_config_.delay_estimator_sta_slew_bias
      = utl::readEnvarInt("RSZ_MT_SLEW_BIAS", 1) > 0;
}

GeneratorContext OptimizationPolicy::makeGeneratorContext() const
{
  return GeneratorContext{.resizer = resizer_,
                          .committer = committer_,
                          .run_config = config_,
                          .policy_config = policy_config_};
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
  const PrepareCacheMask mask = accumulatePrepareRequirements();
  for (Target& target : targets) {
    prepareTarget(target, mask);
  }
}

Target OptimizationPolicy::prepareTarget(const Target& target) const
{
  Target prepared_target = target;
  prepareTarget(prepared_target, accumulatePrepareRequirements());
  return prepared_target;
}

void OptimizationPolicy::prewarmTargets(
    const std::vector<Target>& targets) const
{
  const bool prewarm_swappable_cells
      = findGenerator(MoveType::kSizeUp) != nullptr;
  const bool prewarm_vt_equiv_cells
      = findGenerator(MoveType::kVtSwap) != nullptr;
  prewarmTargetLibertyCaches(
      targets, prewarm_swappable_cells, prewarm_vt_equiv_cells);
  if (prewarm_swappable_cells || prewarm_vt_equiv_cells) {
    prewarmTargetDriverCaches(targets);
  }
}

bool OptimizationPolicy::targetPrewarmEnabled() const
{
  return false;
}

bool OptimizationPolicy::generatorEnabled(const MoveType) const
{
  return true;
}

void OptimizationPolicy::prepareTarget(Target& target,
                                       const PrepareCacheMask mask) const
{
  if ((mask & kArcDelayStateCache) != 0) {
    prepareArcDelayState(target);
  }
}

void OptimizationPolicy::prepareArcDelayState(Target& target) const
{
  if (target.isPrepared(kArcDelayStateCache) || !target.canBePathDriver()) {
    return;
  }

  FailReason fail_reason = FailReason::kNone;
  target.arc_delay = DelayEstimator::buildContext(
      resizer_,
      target,
      policy_config_.delay_estimation_levels,
      &fail_reason,
      policy_config_.delay_estimator_sta_slew_bias);
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

bool OptimizationPolicy::hasSetupViolations(const OptimizerRunConfig& config,
                                            const sta::MinMax* max) const
{
  return sta::fuzzyLess(resizer_.sta()->worstSlack(max),
                        config.setup_slack_margin);
}

sta::Slack OptimizationPolicy::totalNegativeSlack(const sta::MinMax* max) const
{
  return resizer_.sta()->totalNegativeSlack(max);
}

void OptimizationPolicy::prewarmTargetDriverCaches(
    const std::vector<Target>& targets) const
{
  std::unordered_set<const sta::Pin*> pins;
  for (const Target& target : targets) {
    sta::Instance* inst = target.inst(resizer_);
    if (inst == nullptr) {
      continue;
    }

    std::unique_ptr<sta::InstancePinIterator> pin_iter(
        network_->pinIterator(inst));
    while (pin_iter->hasNext()) {
      pins.insert(pin_iter->next());
    }
  }

  // Warm up dbNetwork's net-driver cache on the caller thread so MT workers do
  // not populate net_drvr_pin_map_ while checking replacement legality.
  for (const sta::Pin* pin : pins) {
    static_cast<void>(network_->drivers(pin));
  }
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
