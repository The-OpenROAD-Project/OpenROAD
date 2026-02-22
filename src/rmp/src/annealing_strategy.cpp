// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2026, The OpenROAD Authors

#include "annealing_strategy.h"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <utility>
#include <vector>

#include "cut/abc_library_factory.h"
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

  SolutionSlack sol_slack{ops};
  auto* worst_vertex = sol_slack.Evaluate(
      candidate_vertices, abc_library, corner_, sta, name_generator, logger);

  if (!sol_slack.WorstSlack()) {
    logger->error(RMP, 51, "Should be evaluated");
  }
  sta::Slack worst_slack = *sol_slack.WorstSlack();

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

  SolutionSlack best_sol{ops, worst_slack};
  size_t worse_iters = 0;

  for (unsigned i = 0; i < iterations_; i++) {
    float current_temp
        = *temperature_ * (static_cast<float>(iterations_ - i) / iterations_);

    if (revert_after_ && worse_iters >= *revert_after_) {
      logger->info(RMP, 57, "Reverting to the best found solution");
      ops = best_sol.Solution();
      worst_slack = *best_sol.WorstSlack();
      worse_iters = 0;
    }

    if ((i + 1) % 10 == 0) {
      logger->info(RMP,
                   54,
                   "Iteration: {}, temperature: {}, best worst slack: {}",
                   i + 1,
                   current_temp,
                   *best_sol.WorstSlack());
    } else {
      debugPrint(logger,
                 RMP,
                 "annealing",
                 1,
                 "Iteration: {}, temperature: {}, best worst slack: {}",
                 i + 1,
                 current_temp,
                 best_sol.WorstSlack() ? *best_sol.WorstSlack() : 0);
    }

    SolutionSlack s{ops};
    auto new_ops = s.RandomNeighbor(all_ops, logger, random_);
    sol_slack = SolutionSlack{new_ops};

    sol_slack.Evaluate(
        candidate_vertices, abc_library, corner_, sta, name_generator, logger);

    if (!sol_slack.WorstSlack()) {
      logger->error(RMP, 55, "Should be evaluated");
    }
    sta::Slack worst_slack_new = *sol_slack.WorstSlack();

    if (best_sol.WorstSlack() && worst_slack_new < *best_sol.WorstSlack()) {
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

    if (best_sol.WorstSlack() && worst_slack > *best_sol.WorstSlack()) {
      best_sol = SolutionSlack{ops, worst_slack};
    }
  }
  return best_sol.Solution();
}

}  // namespace rmp
