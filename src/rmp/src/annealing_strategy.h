// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2026, The OpenROAD Authors

#pragma once

#include <optional>
#include <random>
#include <vector>

#include "aig/gia/gia.h"
#include "cut/abc_library_factory.h"
#include "db_sta/dbSta.hh"
#include "gia.h"
#include "resynthesis_strategy.h"
#include "rsz/Resizer.hh"
#include "slack_tuning_strategy.h"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "utl/Logger.h"
#include "utl/unique_name.h"

namespace sta {
class Scene;
}  // namespace sta

namespace rmp {

class AnnealingStrategy final : public SlackTuningStrategy
{
 public:
  explicit AnnealingStrategy(sta::Scene* corner,
                             sta::Slack slack_threshold,
                             std::mt19937::result_type seed,
                             std::optional<float> temperature,
                             unsigned iterations,
                             std::optional<unsigned> revert_after,
                             unsigned initial_ops)
      : SlackTuningStrategy(corner,
                            slack_threshold,
                            seed,
                            iterations,
                            initial_ops),
        temperature_(temperature),
        revert_after_(revert_after)
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
  std::optional<float> temperature_;
  std::optional<unsigned> revert_after_;
};

}  // namespace rmp
