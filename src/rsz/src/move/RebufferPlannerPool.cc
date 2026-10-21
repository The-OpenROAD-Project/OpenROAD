// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "RebufferPlannerPool.hh"

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <utility>

#include "Rebuffer.hh"
#include "rsz/Resizer.hh"

namespace rsz {

RebufferPlannerPool::RebufferPlannerPool(Resizer& resizer) : resizer_(resizer)
{
}

RebufferPlannerPool::~RebufferPlannerPool() = default;

void RebufferPlannerPool::reset(sta::Scene* scene, const size_t count)
{
  planners_.clear();
  idle_.clear();
  for (size_t index = 0; index < count; ++index) {
    std::unique_ptr<Rebuffer> planner = std::make_unique<Rebuffer>(&resizer_);
    planner->initForPlanning(scene, /* worker */ true);
    idle_.push_back(planner.get());
    planners_.push_back(std::move(planner));
  }
}

std::map<int, int> RebufferPlannerPool::workerWarningCounts() const
{
  std::map<int, int> counts;
  for (const std::unique_ptr<Rebuffer>& planner : planners_) {
    for (const std::pair<const int, int>& id_count :
         planner->workerWarningCounts()) {
      counts[id_count.first] += id_count.second;
    }
  }
  return counts;
}

Rebuffer& RebufferPlannerPool::acquire()
{
  // Inline tasks keep the Resizer's planner.
  if (planners_.empty()) {
    return resizer_.rebuffer();
  }

  const std::lock_guard<std::mutex> lock(mutex_);
  Rebuffer* planner = idle_.back();
  idle_.pop_back();
  return *planner;
}

void RebufferPlannerPool::release(Rebuffer& planner)
{
  if (planners_.empty()) {
    return;
  }

  const std::lock_guard<std::mutex> lock(mutex_);
  idle_.push_back(&planner);
}

}  // namespace rsz
