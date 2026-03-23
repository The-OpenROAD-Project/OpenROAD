// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2026, The OpenROAD Authors

#pragma once

#include <random>
#include <vector>

#include "cut/abc_library_factory.h"
#include "gia.h"
#include "slack_tuning_strategy.h"
#include "sta/Delay.hh"
#include "utl/unique_name.h"

namespace rmp {

class GeneticStrategy final : public SlackTuningStrategy
{
 public:
  explicit GeneticStrategy(sta::Scene* corner,
                           sta::Slack slack_threshold,
                           std::mt19937::result_type seed,
                           unsigned population_size,
                           float mutation_probability,
                           float crossover_probability,
                           unsigned tournament_size,
                           float tournament_probability,
                           unsigned iterations,
                           unsigned initial_ops)
      : SlackTuningStrategy(corner,
                            slack_threshold,
                            seed,
                            iterations,
                            initial_ops),
        population_size_(population_size),
        mutation_probability_(mutation_probability),
        crossover_probability_(crossover_probability),
        tournament_size_(tournament_size),
        tournament_probability_(tournament_probability)
  {
  }

  std::vector<GiaOp> RunStrategy(
      const std::vector<GiaOp>& all_ops,
      const std::vector<sta::Vertex*>& candidate_vertices,
      cut::AbcLibrary& abc_library,
      sta::dbSta* sta,
      utl::UniqueName& name_generator,
      rsz::Resizer* resizer,
      utl::Logger* logger) override;

 private:
  unsigned population_size_;
  float mutation_probability_;
  float crossover_probability_;
  unsigned tournament_size_;
  float tournament_probability_;
};

}  // namespace rmp
