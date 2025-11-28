// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <optional>
#include <random>
#include <vector>

#include "cut/abc_library_factory.h"
#include "gia.h"
#include "slack_tuning_strategy.h"
#include "sta/Delay.hh"
#include "utl/unique_name.h"

namespace rsz {
class Resizer;
}  // namespace rsz

namespace sta {
class dbSta;
class Corner;
class Vertex;
}  // namespace sta

namespace utl {
class Logger;
}  // namespace utl

namespace rmp {

// TODO docs
class GeneticStrategy final : public SlackTuningStrategy
{
 public:
  explicit GeneticStrategy(sta::Corner* corner,
                           sta::Slack slack_threshold,
                           std::optional<std::mt19937::result_type> seed,
                           unsigned pop_size,
                           float mut_prob,
                           float cross_prob,
                           unsigned tourn_size,
                           float tourn_prob,
                           unsigned iterations,
                           unsigned initial_ops)
      : SlackTuningStrategy(corner,
                            slack_threshold,
                            seed,
                            iterations,
                            initial_ops),
        pop_size_(pop_size),
        mut_prob_(mut_prob),
        cross_prob_(cross_prob),
        tourn_size_(tourn_size),
        tourn_prob_(tourn_prob)
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
  unsigned pop_size_;
  float mut_prob_;
  float cross_prob_;
  unsigned tourn_size_;
  float tourn_prob_;
};

}  // namespace rmp
