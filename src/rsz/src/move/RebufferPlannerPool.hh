// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace sta {
class Scene;
}  // namespace sta

namespace rsz {

class Rebuffer;
class Resizer;

// One Rebuffer planner per worker thread, because planning keeps per-call
// scratch state.  An empty pool hands out the Resizer's planner to tasks
// that run inline.
class RebufferPlannerPool
{
 public:
  // A planner borrowed for one task.  The lease returns it also when planning
  // throws, because the other tasks of the batch still run.
  class Lease
  {
   public:
    explicit Lease(RebufferPlannerPool& pool)
        : pool_(pool), planner_(pool.acquire())
    {
    }
    ~Lease() { pool_.release(planner_); }
    Lease(const Lease&) = delete;
    Lease& operator=(const Lease&) = delete;

    Rebuffer& planner() const { return planner_; }

   private:
    RebufferPlannerPool& pool_;
    Rebuffer& planner_;
  };

  explicit RebufferPlannerPool(Resizer& resizer);
  ~RebufferPlannerPool();

  // Make `count` worker planners on `scene`; zero leaves the pool empty.
  void reset(sta::Scene* scene, size_t count);
  // The warnings the worker planners counted instead of logging, by message
  // ID.
  std::map<int, int> workerWarningCounts() const;

 private:
  // Borrow a worker planner, or the shared planner for an inline task.
  Rebuffer& acquire();
  // Return a planner after its task has finished.
  void release(Rebuffer& planner);

  Resizer& resizer_;
  // Planners with independent delay-calculator scratch state.
  std::vector<std::unique_ptr<Rebuffer>> planners_;
  // Available planners protected by mutex_.
  std::vector<Rebuffer*> idle_;
  std::mutex mutex_;
};

}  // namespace rsz
