// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "annealing_strategy.h"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <utility>
#include <vector>

#include "cut/abc_library_factory.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gia.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "slack_tuning_strategy.h"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "utl/Logger.h"
#include "utl/deleter.h"
#include "utl/unique_name.h"

namespace rmp {
using utl::RMP;

std::vector<GiaOp> AnnealingStrategy::RunStrategy(
    const std::vector<GiaOp>& all_ops,
    const std::vector<sta::Vertex*>& candidate_vertices,
    cut::AbcLibrary& abc_library,
    sta::dbSta* sta,
    utl::UniqueName& name_generator,
    rsz::Resizer* resizer,
    utl::Logger* logger)
{
  // Initial solution and slack
  debugPrint(logger,
             RMP,
             "annealing",
             1,
             "Generating and evaluating the initial solution");
  std::vector<GiaOp> ops;
  ops.reserve(initial_ops_);
  for (size_t i = 0; i < initial_ops_; i++) {
    ops.push_back(all_ops[random_() % all_ops.size()]);
  }

  SolutionSlack sol_slack;
  sol_slack.solution_ = ops;
  auto* worst_vertex = sol_slack.Evaluate(
      candidate_vertices, abc_library, corner_, sta, name_generator, logger);

  if (!sol_slack.worst_slack_) {
    logger->error(RMP, 51, "Should be evaluated");
  }
  float worst_slack = *sol_slack.worst_slack_;

  if (!temperature_) {
    sta::Delay required = sta->vertexRequired(worst_vertex, sta::MinMax::max());
    temperature_ = required;
  }

  logger->info(RMP, 52, "Resynthesis: starting simulated annealing");
  logger->info(RMP,
               53,
               "Initial temperature: {}, worst slack: {}",
               *temperature_,
               worst_slack);

  SolutionSlack best_sol{.solution_ = ops, .worst_slack_ = worst_slack};
  size_t worse_iters = 0;

  for (unsigned i = 0; i < iterations_; i++) {
    float current_temp
        = *temperature_ * (static_cast<float>(iterations_ - i) / iterations_);

    if (revert_after_ && worse_iters >= *revert_after_) {
      logger->info(RMP, 57, "Reverting to the best found solution");
      ops = best_sol.solution_;
      worst_slack = *best_sol.worst_slack_;
      worse_iters = 0;
    }

    if ((i + 1) % 10 == 0) {
      logger->info(RMP,
                   54,
                   "Iteration: {}, temperature: {}, best worst slack: {}",
                   i + 1,
                   current_temp,
                   *best_sol.worst_slack_);
    } else {
      debugPrint(logger,
                 RMP,
                 "annealing",
                 1,
                 "Iteration: {}, temperature: {}, best worst slack: {}",
                 i + 1,
                 current_temp,
                 best_sol.worst_slack_ ? *best_sol.worst_slack_ : 0);
    }

    SolutionSlack s;
    s.solution_ = ops;
    auto new_ops = s.RandomNeighbor(all_ops, logger, random_);
    sol_slack.solution_ = new_ops;

    sol_slack.Evaluate(
        candidate_vertices, abc_library, corner_, sta, name_generator, logger);

    if (!sol_slack.worst_slack_) {
      logger->error(RMP, 55, "Should be evaluated");
    }
    float worst_slack_new = *sol_slack.worst_slack_;

    if (best_sol.worst_slack_ && worst_slack_new < *best_sol.worst_slack_) {
      worse_iters++;
    } else {
      worse_iters = 0;
    }

    if (worst_slack_new < worst_slack) {
      float accept_prob
          = current_temp == 0
                ? 0
                : std::exp((worst_slack_new - worst_slack) / current_temp);
      debugPrint(
          logger,
          RMP,
          "annealing",
          1,
          "Current worst slack: {}, new: {}, accepting new ABC script with "
          "probability {}",
          worst_slack,
          worst_slack_new,
          accept_prob);
      if (std::uniform_real_distribution<float>(0, 1)(random_) < accept_prob) {
        debugPrint(logger,
                   RMP,
                   "annealing",
                   1,
                   "Accepting new ABC script with worse slack");
      } else {
        debugPrint(logger,
                   RMP,
                   "annealing",
                   1,
                   "Rejecting new ABC script with worse slack");
        continue;
      }
    } else {
      debugPrint(logger,
                 RMP,
                 "annealing",
                 1,

                 "Current worst slack: {}, new: {}, accepting new ABC script",
                 worst_slack,
                 worst_slack_new);
    }

    ops = std::move(new_ops);
    worst_slack = worst_slack_new;

    if (best_sol.worst_slack_ && worst_slack > *best_sol.worst_slack_) {
      *best_sol.worst_slack_ = worst_slack;
      best_sol.solution_ = ops;
    }
  }
  return best_sol.solution_;
}

}  // namespace rmp
