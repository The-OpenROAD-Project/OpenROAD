// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "slack_tuning_strategy.h"

#include "utils.h"

namespace rmp {

// The magic numbers are defaults from abc/src/base/abci/abc.c
constexpr size_t SEARCH_RESIZE_ITERS = 100;
constexpr size_t FINAL_RESIZE_ITERS = 1000;

using utl::RMP;

sta::Vertex* SlackTuningStrategy::EvaluateSlack(
    SolutionSlack& sol_slack,
    const std::vector<sta::Vertex*>& candidate_vertices,
    cut::AbcLibrary& abc_library,
    sta::Corner* corner,
    sta::dbSta* sta,
    utl::UniqueName& name_generator,
    utl::Logger* logger)
{
  auto block = sta->db()->getChip()->getBlock();
  odb::dbDatabase::beginEco(block);

  RunGia(sta,
         candidate_vertices,
         abc_library,
         sol_slack.solution,
         SEARCH_RESIZE_ITERS,
         name_generator,
         logger);

  odb::dbDatabase::endEco(block);

  sta::Vertex* worst_vertex;
  float worst_slack;
  sta->worstSlack(corner, sta::MinMax::max(), worst_slack, worst_vertex);
  sol_slack.worst_slack = worst_slack;

  odb::dbDatabase::undoEco(block);
  return worst_vertex;
}

float SlackTuningStrategy::GetWorstSlack(sta::dbSta* sta, sta::Corner* corner)
{
  float worst_slack;
  sta::Vertex* worst_vertex_placeholder;
  sta->worstSlack(
      corner, sta::MinMax::max(), worst_slack, worst_vertex_placeholder);
  return worst_slack;
}

Solution SlackTuningStrategy::RandomNeighbor(Solution sol,
                                             const std::vector<GiaOp>& all_ops,
                                             utl::Logger* logger)
{
  enum Move
  {
    ADD,
    REMOVE,
    SWAP,
    COUNT
  };
  Move move = ADD;
  if (sol.size() > 1) {
    move = Move(random_() % (COUNT));
  }
  switch (move) {
    case ADD: {
      debugPrint(logger, RMP, "annealing", 2, "Adding a new GIA operation");
      size_t i = random_() % (sol.size() + 1);
      size_t j = random_() % all_ops.size();
      sol.insert(sol.begin() + i, all_ops[j]);
    } break;
    case REMOVE: {
      debugPrint(logger, RMP, "annealing", 2, "Removing a GIA operation");
      size_t i = random_() % sol.size();
      sol.erase(sol.begin() + i);
    } break;
    case SWAP: {
      debugPrint(
          logger, RMP, "annealing", 2, "Swapping adjacent GIA operations");
      size_t i = random_() % (sol.size() - 1);
      std::swap(sol[i], sol[i + 1]);
    } break;
    case COUNT:
      // unreachable
      std::abort();
  }
  return sol;
}

void SlackTuningStrategy::OptimizeDesign(sta::dbSta* sta,
                                         utl::UniqueName& name_generator,
                                         rsz::Resizer* resizer,
                                         utl::Logger* logger)
{
  sta->ensureGraph();
  sta->ensureLevelized();
  sta->searchPreamble();
  sta->ensureClkNetwork();

  auto candidate_vertices = GetEndpoints(sta, resizer, slack_threshold_);
  if (candidate_vertices.empty()) {
    logger->info(utl::RMP,
                 58,
                 "All endpoints have slack above threshold, nothing to do.");
    return;
  }

  cut::AbcLibraryFactory factory(logger);
  factory.AddDbSta(sta);
  factory.AddResizer(resizer);
  factory.SetCorner(corner_);
  cut::AbcLibrary abc_library = factory.Build();

  std::vector<GiaOp> all_ops = GiaOps(logger);

  const auto& best_ops = RunStrategy(all_ops,
                                     candidate_vertices,
                                     abc_library,
                                     sta,
                                     name_generator,
                                     resizer,
                                     logger);

  logger->info(
      RMP, 67, "Resynthesis: End of slack tuning, applying ABC operations");

  // Apply the ops
  RunGia(sta,
         candidate_vertices,
         abc_library,
         best_ops,
         FINAL_RESIZE_ITERS,
         name_generator,
         logger);
  logger->info(
      RMP, 68, "Resynthesis: Worst slack is {}", GetWorstSlack(sta, corner_));
}

}  // namespace rmp
