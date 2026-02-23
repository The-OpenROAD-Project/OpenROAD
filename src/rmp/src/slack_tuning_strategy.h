// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2026, The OpenROAD Authors

#pragma once

#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "cut/abc_library_factory.h"
#include "gia.h"
#include "resynthesis_strategy.h"
#include "sta/Delay.hh"
#include "utl/unique_name.h"

namespace rsz {
class Resizer;
}  // namespace rsz

namespace sta {
class dbSta;
class Scene;
class Vertex;
}  // namespace sta

namespace utl {
class Logger;
}  // namespace utl

namespace rmp {

class SolutionSlack final
{
 public:
  using ResultOps = std::vector<GiaOp>;

 private:
  ResultOps solution_;
  std::optional<sta::Slack> worst_slack_ = std::nullopt;

 public:
  explicit SolutionSlack() = default;
  explicit SolutionSlack(ResultOps solution) : solution_{std::move(solution)} {}
  explicit SolutionSlack(ResultOps& solution, sta::Slack worst_slack)
      : solution_{solution}, worst_slack_{worst_slack}
  {
  }

  std::string toString() const;
  ResultOps RandomNeighbor(const ResultOps& all_ops,
                           utl::Logger* logger,
                           std::mt19937& random) const;

  std::pair<sta::Slack, sta::Vertex*> Evaluate(
      const std::vector<sta::Vertex*>& candidate_vertices,
      cut::AbcLibrary& abc_library,
      sta::Scene* corner,
      sta::dbSta* sta,
      utl::UniqueName& name_generator,
      utl::Logger* logger);
  ResultOps Solution() const { return solution_; }
  ResultOps& Solution() { return solution_; }
  std::optional<sta::Slack> WorstSlack() const { return worst_slack_; }
};

class SlackTuningStrategy : public ResynthesisStrategy
{
 public:
  explicit SlackTuningStrategy(sta::Scene* corner,
                               sta::Slack slack_threshold,
                               std::optional<std::mt19937::result_type> seed,
                               unsigned iterations,
                               unsigned initial_ops)
      : corner_(corner),
        slack_threshold_(slack_threshold),
        iterations_(iterations),
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

  virtual std::vector<GiaOp> RunStrategy(
      const std::vector<GiaOp>& all_ops,
      const std::vector<sta::Vertex*>& candidate_vertices,
      cut::AbcLibrary& abc_library,
      sta::dbSta* sta,
      utl::UniqueName& name_generator,
      rsz::Resizer* resizer,
      utl::Logger* logger)
      = 0;

 protected:
  sta::Scene* corner_;
  sta::Slack slack_threshold_;
  unsigned iterations_;
  unsigned initial_ops_;
  std::mt19937 random_;
};

}  // namespace rmp
