// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <random>
#include <vector>

#include "aig/gia/gia.h"
#include "cut/abc_library_factory.h"
#include "db_sta/dbSta.hh"
#include "resynthesis_strategy.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Scene.hh"
#include "utl/Logger.h"
#include "utl/unique_name.h"

namespace rmp {

using GiaOp = std::function<void(abc::Gia_Man_t*&)>;

class AnnealingStrategy : public ResynthesisStrategy
{
 public:
  explicit AnnealingStrategy(sta::Scene* corner,
                             sta::Slack slack_threshold,
                             std::optional<std::mt19937::result_type> seed,
                             std::optional<float> temperature,
                             unsigned iterations,
                             std::optional<unsigned> revert_after,
                             unsigned initial_ops)
      : corner_(corner),
        slack_threshold_(slack_threshold),
        temperature_(temperature),
        iterations_(iterations),
        revert_after_(revert_after),
        initial_ops_(initial_ops)
  {
    if (seed) {
      random_.seed(*seed);
    }
  }
  void OptimizeDesign(sta::dbSta* sta,
                      utl::UniqueName& name_generator,
                      rsz::Resizer* resizer,
                      utl::Logger* logger) override;
  void RunGia(sta::dbSta* sta,
              const std::vector<sta::Vertex*>& candidate_vertices,
              cut::AbcLibrary& abc_library,
              const std::vector<GiaOp>& gia_ops,
              size_t resize_iters,
              utl::UniqueName& name_generator,
              utl::Logger* logger);

 private:
  sta::Scene* corner_;
  sta::Slack slack_threshold_;
  std::optional<float> temperature_;
  unsigned iterations_;
  std::optional<unsigned> revert_after_;
  unsigned initial_ops_;
  std::mt19937 random_;
};

}  // namespace rmp
