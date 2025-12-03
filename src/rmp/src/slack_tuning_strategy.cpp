// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "slack_tuning_strategy.h"

#include <absl/base/optimization.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/random/random.h"
#include "cut/abc_library_factory.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gia.h"
#include "odb/db.h"
#include "sta/Delay.hh"
#include "sta/MinMax.hh"
#include "utils.h"
#include "utl/Logger.h"
#include "utl/unique_name.h"

namespace rmp {

// The magic numbers are defaults from abc/src/base/abci/abc.c
constexpr size_t SEARCH_RESIZE_ITERS = 100;
constexpr size_t FINAL_RESIZE_ITERS = 1000;

using utl::RMP;

std::string SolutionSlack::toString() const
{
  if (solution_.empty()) {
    return "[]";
  }

  std::ostringstream resStream;
  resStream << '[' << std::to_string(solution_.front().id);
  for (int i = 1; i < solution_.size(); i++) {
    resStream << ", " << std::to_string(solution_[i].id);
  }
  resStream << "], worst slack: ";

  if (worst_slack_) {
    resStream << *worst_slack_;
  } else {
    resStream << "not computed";
  }
  return resStream.str();
}

SolutionSlack::Type SolutionSlack::RandomNeighbor(
    const SolutionSlack::Type& all_ops,
    utl::Logger* logger,
    std::mt19937& random) const
{
  SolutionSlack::Type sol = solution_;
  enum class Move : uint8_t
  {
    ADD,
    REMOVE,
    SWAP,
    COUNT
  };
  Move move = Move::ADD;
  if (sol.size() > 1) {
    const auto count = static_cast<std::underlying_type_t<Move>>(Move::COUNT);
    move = Move(absl::Uniform<int>(random, 0, count));
  }
  switch (move) {
    case Move::ADD: {
      debugPrint(logger, RMP, "slack_tunning", 2, "Adding a new GIA operation");
      size_t i = absl::Uniform<size_t>(random, 0, sol.size() + 1);
      size_t j = absl::Uniform<size_t>(random, 0, all_ops.size());
      sol.insert(sol.begin() + i, all_ops[j]);
    } break;
    case Move::REMOVE: {
      debugPrint(logger, RMP, "slack_tunning", 2, "Removing a GIA operation");
      size_t i = absl::Uniform<size_t>(random, 0, sol.size());
      sol.erase(sol.begin() + i);
    } break;
    case Move::SWAP: {
      debugPrint(
          logger, RMP, "slack_tunning", 2, "Swapping adjacent GIA operations");
      assert(sol.size() > 1);
      size_t i = absl::Uniform<size_t>(random, 0, sol.size() - 1);
      std::swap(sol[i], sol[i + 1]);
    } break;
    case Move::COUNT:
      // TODO replace with std::unreachable() once we reach c++23
      ABSL_UNREACHABLE();
      break;
  }
  return sol;
}

static std::pair<sta::Slack, sta::Vertex*> GetWorstSlack(sta::dbSta* sta,
                                                         sta::Corner* corner)
{
  sta::Slack worst_slack;
  sta::Vertex* worst_vertex = nullptr;
  sta->worstSlack(corner, sta::MinMax::max(), worst_slack, worst_vertex);
  return {worst_slack, worst_vertex};
}

sta::Vertex* SolutionSlack::Evaluate(
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
         solution_,
         SEARCH_RESIZE_ITERS,
         name_generator,
         logger);

  odb::dbDatabase::endEco(block);

  auto [worst_slack, worst_vertex] = GetWorstSlack(sta, corner);
  worst_slack_ = worst_slack;

  odb::dbDatabase::undoEco(block);
  return worst_vertex;
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

  logger->info(RMP,
               59,
               "Resynthesis: starting tuning algorithm, Worst slack is {}",
               GetWorstSlack(sta, corner_).first);

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
  logger->info(RMP,
               68,
               "Resynthesis: Worst slack is {}",
               GetWorstSlack(sta, corner_).first);
}

}  // namespace rmp
