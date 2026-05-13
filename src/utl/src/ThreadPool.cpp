// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "utl/ThreadPool.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

namespace utl {

// Each thread keeps its own active-pool pointer so worker-only nested-wait
// assists never leak across unrelated threads or pools.
thread_local ThreadPool* ThreadPool::active_pool_ = nullptr;

ThreadPool::ThreadPool(const size_t thread_count)
    : workers_(thread_count), lifetime_(std::make_shared<ThreadPoolLifetime>())
{
  for (size_t index = 0; index < thread_count; ++index) {
    workers_[index] = std::thread(&ThreadPool::workerLoop, this);
  }
}

ThreadPool::~ThreadPool()
{
  {
    std::lock_guard<std::mutex> lock(lock_);
    stop_ = true;
  }
  cv_.notify_all();

  for (std::thread& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }

  lifetime_->pool_alive.store(false, std::memory_order_release);
}

size_t ThreadPool::threadCount() const
{
  return workers_.size();
}

bool ThreadPool::isWorkerThreadFor(const ThreadPool* pool)
{
  return active_pool_ == pool;
}

bool ThreadPool::tryRunPendingTask()
{
  std::function<void()> task;
  {
    std::lock_guard<std::mutex> lock(lock_);
    if (tasks_.empty()) {
      return false;
    }

    task = std::move(tasks_.front());
    tasks_.pop();
  }

  task();
  return true;
}

void ThreadPool::workerLoop()
{
  ThreadPool* previous_pool = active_pool_;
  // Mark this thread as belonging to the current pool while it runs worker
  // tasks so pool-aware futures can detect same-pool nested waits.
  active_pool_ = this;
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(lock_);
      cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
      if (stop_ && tasks_.empty()) {
        active_pool_ = previous_pool;
        return;
      }

      task = std::move(tasks_.front());
      tasks_.pop();
    }
    task();
  }
}

}  // namespace utl
