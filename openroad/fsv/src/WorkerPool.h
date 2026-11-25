// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <barrier>
#include <functional>
#include <thread>
#include <vector>

namespace fsv {

// Persistent threads that run one job at a time on every thread, including
// the caller. Between jobs the workers wait at a barrier instead of being
// recreated, so short solves do not pay for thread creation.
class WorkerPool
{
 public:
  explicit WorkerPool(int threads);
  ~WorkerPool();

  WorkerPool(const WorkerPool&) = delete;
  WorkerPool& operator=(const WorkerPool&) = delete;

  // Runs job() on all threads and returns when every call has finished.
  // The job divides the work itself (e.g. by claiming batches atomically).
  void run(const std::function<void()>& job);

 private:
  const std::function<void()>* job_ = nullptr;
  bool stop_ = false;
  // `start` releases the workers into a job (or to exit); `done` waits for
  // them all to finish it. Each barrier also publishes the writes made
  // before it, so job_ and stop_ need no atomics.
  std::barrier<> start_;
  std::barrier<> done_;
  // Declared last: destroyed (joined) before the barriers.
  std::vector<std::jthread> workers_;
};

}  // namespace fsv
