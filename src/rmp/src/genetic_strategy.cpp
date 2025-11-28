// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "genetic_strategy.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <random>
#include <ranges>
#include <unordered_set>
#include <utility>
#include <vector>

#include "boost/container_hash/hash.hpp"
#include "cut/abc_library_factory.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gia.h"
#include "rsz/Resizer.hh"
#include "slack_tuning_strategy.h"
#include "utl/Logger.h"
#include "utl/deleter.h"
#include "utl/unique_name.h"

namespace rmp {
using utl::RMP;

static void removeDuplicates(std::vector<SolutionSlack>& population,
                             utl::Logger* logger)
{
  struct HashVector final
  {
    size_t operator()(const SolutionSlack::Type& sol) const
    {
      size_t res = 0;
      for (const auto& item : sol) {
        size_t hash = boost::hash<size_t>()(item.id);
        // TODO set seed
        boost::hash_combine(hash, res);
      }
      return res;
    }
  };

  std::unordered_set<SolutionSlack::Type, HashVector> taken;
  population.erase(
      std::ranges::begin(std::ranges::remove_if(
          population,
          [&taken, logger](const SolutionSlack& s) {
            if (!taken.insert(s.solution_).second) {
              debugPrint(
                  logger, RMP, "genetic", 2, "Removing: " + s.toString());
              return true;
            }
            debugPrint(logger, RMP, "genetic", 2, "Keeping: " + s.toString());
            return false;
          })),
      population.end());
}

std::vector<GiaOp> GeneticStrategy::RunStrategy(
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
             "population",
             1,
             "Generating and evaluating the initial population");
  std::vector<SolutionSlack> population(pop_size_);
  for (auto& ind : population) {
    ind.solution_.reserve(initial_ops_);
    for (size_t i = 0; i < initial_ops_; i++) {
      ind.solution_.push_back(all_ops[random_() % all_ops.size()]);
    }
  }

  for (auto& candidate : population) {
    candidate.Evaluate(
        candidate_vertices, abc_library, corner_, sta, name_generator, logger);

    debugPrint(logger, RMP, "genetic", 1, candidate.toString());
  }

  for (unsigned i = 0; i < iterations_; i++) {
    logger->info(RMP, 65, "Resynthesis: Iteration {} of genetic algorithm", i);
    unsigned generation_size = population.size();
    // Crossover
    unsigned cross_size = std::max<unsigned>(cross_prob_ * generation_size, 1);
    for (unsigned j = 0; j < cross_size; j++) {
      auto rand1 = random_() % generation_size;
      auto rand2 = random_() % generation_size;
      if (rand1 == rand2) {
        continue;
      }
      SolutionSlack::Type& parent1_sol = population[rand1].solution_;
      SolutionSlack::Type& parent2_sol = population[rand2].solution_;
      SolutionSlack::Type child_sol(
          parent1_sol.begin(), parent1_sol.begin() + parent1_sol.size() / 2);
      child_sol.insert(child_sol.end(),
                       parent2_sol.begin() + parent2_sol.size() / 2,
                       parent2_sol.end());
      SolutionSlack child_sol_slack;
      child_sol_slack.solution_ = std::move(child_sol);
      population.emplace_back(child_sol_slack);
    }
    // Mutations
    unsigned mut_size = std::max<unsigned>(mut_prob_ * generation_size, 1);
    for (unsigned j = 0; j < mut_size; j++) {
      SolutionSlack sol_slack;
      auto rand = random_() % generation_size;
      sol_slack.solution_
          = population[rand].RandomNeighbor(all_ops, logger, random_);
      population.emplace_back(sol_slack);
    }
    removeDuplicates(population, logger);
    // Evaluation
    for (auto& sol_slack : population) {
      if (sol_slack.worst_slack_) {
        continue;
      }
      sol_slack.Evaluate(candidate_vertices,
                         abc_library,
                         corner_,
                         sta,
                         name_generator,
                         logger);
    }
    // Selection
    std::ranges::sort(population, std::less{});
    std::vector<SolutionSlack> newPopulation;
    newPopulation.reserve(pop_size_);
    for (int j = 0; j < pop_size_; j++) {
      std::vector<size_t> tournament(tourn_size_);
      std::generate_n(tournament.begin(), tourn_size_, [&]() {
        return random_() % population.size();
      });
      std::ranges::sort(tournament);
      tournament.erase(std::ranges::begin(std::ranges::unique(tournament)),
                       tournament.end());
      std::bernoulli_distribution bern_dist{tourn_prob_};
      for (const auto& candidateId : tournament) {
        if (bern_dist(random_)) {
          newPopulation.push_back(population[candidateId]);
          break;
        }
      }
    }
    removeDuplicates(newPopulation, logger);
    population = newPopulation;

    for (const auto& candidate : population) {
      debugPrint(logger, RMP, "genetic", 1, candidate.toString());
    }
  }

  auto best_it = std::ranges::min_element(population, std::less{});
  logger->info(RMP,
               66,
               "Resynthesis: Best result is of individual {}",
               std::distance(population.begin(), best_it));
  return best_it->solution_;
}
}  // namespace rmp
