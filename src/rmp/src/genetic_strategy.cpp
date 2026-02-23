// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2026, The OpenROAD Authors

#include "genetic_strategy.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <ranges>
#include <unordered_set>
#include <utility>
#include <vector>

#include "absl/hash/hash.h"
#include "absl/random/distributions.h"
#include "cut/abc_library_factory.h"
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
  std::unordered_set<SolutionSlack::ResultOps,
                     absl::Hash<SolutionSlack::ResultOps>>
      taken;
  population.erase(
      std::ranges::begin(std::ranges::remove_if(
          population,
          [&taken, logger](const SolutionSlack& s) {
            if (!taken.insert(s.Solution()).second) {
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
  if (population_size_ <= 0) {
    logger->error(RMP, 64, "Population size should be greater than 0");
  }

  // Initial solution and slack
  debugPrint(logger,
             RMP,
             "population",
             1,
             "Generating and evaluating the initial population");
  std::vector<SolutionSlack> population(population_size_);
  for (auto& ind : population) {
    ind.Solution().reserve(initial_ops_);
    for (size_t i = 0; i < initial_ops_; i++) {
      const auto idx = absl::Uniform<int>(random_, 0, all_ops.size());
      ind.Solution().push_back(all_ops[idx]);
    }
  }

  for (auto& candidate : population) {
    (void) candidate.Evaluate(
        candidate_vertices, abc_library, corner_, sta, name_generator, logger);

    debugPrint(logger, RMP, "genetic", 1, candidate.toString());
  }

  for (unsigned i = 0; i < iterations_; i++) {
    logger->info(RMP, 65, "Resynthesis: Iteration {} of genetic algorithm", i);
    unsigned generation_size = population.size();
    // Crossover
    unsigned cross_size
        = std::max<unsigned>(crossover_probability_ * generation_size, 1);
    for (unsigned j = 0; j < cross_size; j++) {
      auto rand1 = absl::Uniform<int>(random_, 0, generation_size);
      auto rand2 = absl::Uniform<int>(random_, 0, generation_size);
      if (rand1 == rand2) {
        continue;
      }
      SolutionSlack::ResultOps& parent1_sol = population[rand1].Solution();
      SolutionSlack::ResultOps& parent2_sol = population[rand2].Solution();
      SolutionSlack::ResultOps child_sol(
          parent1_sol.begin(), parent1_sol.begin() + parent1_sol.size() / 2);
      child_sol.insert(child_sol.end(),
                       parent2_sol.begin() + parent2_sol.size() / 2,
                       parent2_sol.end());
      population.emplace_back(std::move(child_sol));
    }
    // Mutations
    unsigned mut_size
        = std::max<unsigned>(mutation_probability_ * generation_size, 1);
    for (unsigned j = 0; j < mut_size; j++) {
      auto rand = absl::Uniform<int>(random_, 0, generation_size);
      population.emplace_back(
          population[rand].RandomNeighbor(all_ops, logger, random_));
    }
    removeDuplicates(population, logger);
    // Evaluation
    for (auto& sol_slack : population) {
      if (sol_slack.WorstSlack()) {
        continue;
      }
      (void) sol_slack.Evaluate(candidate_vertices,
                                abc_library,
                                corner_,
                                sta,
                                name_generator,
                                logger);
    }
    // Selection
    std::ranges::stable_sort(
        population, std::ranges::greater{}, &SolutionSlack::WorstSlack);
    std::vector<SolutionSlack> newPopulation;
    newPopulation.reserve(population_size_);
    for (int j = 0; j < population_size_; j++) {
      std::vector<size_t> tournament(tournament_size_);
      std::generate_n(tournament.begin(), tournament_size_, [&]() {
        return absl::Uniform<int>(random_, 0, population.size());
      });
      std::ranges::stable_sort(tournament);
      tournament.erase(std::ranges::begin(std::ranges::unique(tournament)),
                       tournament.end());
      auto winner = std::ranges::find_if(tournament, [&](auto const& _) {
        return absl::Bernoulli(random_,
                               static_cast<double>(tournament_probability_));
      });
      if (winner != tournament.end()) {
        newPopulation.emplace_back(population[*winner]);
      }
    }
    removeDuplicates(newPopulation, logger);

    if (newPopulation.empty()) {
      newPopulation.emplace_back(population.front());
    }

    population = std::move(newPopulation);

    for (const auto& candidate : population) {
      debugPrint(logger, RMP, "genetic", 1, candidate.toString());
    }
  }

  auto best_it
      = std::ranges::max_element(population, {}, &SolutionSlack::WorstSlack);
  logger->info(RMP,
               66,
               "Resynthesis: Best result is of individual {}",
               std::distance(population.begin(), best_it));
  return best_it->Solution();
}
}  // namespace rmp
