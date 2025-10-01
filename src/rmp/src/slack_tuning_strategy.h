// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <functional>
#include <optional>
#include <random>

#include "cut/abc_library_factory.h"
#include "db_sta/dbSta.hh"
#include "gia.h"
#include "resynthesis_strategy.h"
#include "sta/Corner.hh"

namespace rmp {

using Solution = std::vector<GiaOp>;
struct SolutionSlack final
{
  Solution solution;
  std::optional<float> worst_slack = std::nullopt;
  std::string toString() const
  {
    std::ostringstream resStream;
    resStream << '['
              << (solution.size() > 0 ? std::to_string(solution[0].id) : "");
    for (int i = 1; i < solution.size(); i++) {
      resStream << ", " << std::to_string(solution[i].id);
    }
    resStream << "], worst slack: ";
    if (worst_slack) {
      resStream << *worst_slack;
    } else {
      resStream << "not computed";
    }
    return resStream.str();
  }
  bool operator<(const SolutionSlack& other) const
  {
    // Highest slack first
    return worst_slack > other.worst_slack;
  }
};

// TODO docs
class SlackTuningStrategy : public ResynthesisStrategy
{
 public:
  explicit SlackTuningStrategy(sta::Corner* corner,
                               sta::Slack slack_threshold,
                               std::optional<std::mt19937::result_type> seed,
                               unsigned iterations,
                               unsigned initial_ops)
      : corner_(corner),
        slack_threshold_(slack_threshold),
        seed_(seed),
        iterations_(iterations),
        initial_ops_(initial_ops)
  {
    if (seed_) {
      random_.seed(*seed_);
    }
  }

  void OptimizeDesign(sta::dbSta* sta,
                      utl::UniqueName& name_generator,
                      rsz::Resizer* resizer,
                      utl::Logger* logger) override;

  virtual std::vector<GiaOp> RunStrategy(
      const std::vector<GiaOp>& all_ops,
      const std::vector<sta::Vertex*>& candidate_vertices,
      cut::AbcLibrary& abc_library,
      sta::dbSta* sta,
      utl::UniqueName& name_generator,
      rsz::Resizer* resizer,
      utl::Logger* logger)
      = 0;

  sta::Vertex* EvaluateSlack(
      SolutionSlack& sol_slack,
      const std::vector<sta::Vertex*>& candidate_vertices,
      cut::AbcLibrary& abc_library,
      sta::Corner* corner,
      sta::dbSta* sta,
      utl::UniqueName& name_generator,
      utl::Logger* logger);

  Solution RandomNeighbor(Solution sol,
                          const std::vector<GiaOp>& all_ops,
                          utl::Logger* logger);

  float GetWorstSlack(sta::dbSta* sta, sta::Corner* corner);

 protected:
  sta::Corner* corner_;
  sta::Slack slack_threshold_;
  std::optional<std::mt19937::result_type> seed_;
  unsigned iterations_;
  unsigned initial_ops_;
  std::mt19937 random_;
};

}  // namespace rmp
