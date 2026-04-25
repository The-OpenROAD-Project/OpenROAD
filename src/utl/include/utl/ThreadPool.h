// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace utl {

class ThreadPool;

// Shared lifetime token for a ThreadPool and any futures produced by it.
//
// Futures may outlive the pool object itself, so wait()/get() must be able to
// detect that the pool has already been destroyed before touching the raw
// ThreadPool* used for nested-worker assistance.
struct ThreadPoolLifetime
{
  std::atomic<bool> pool_alive{true};
};

// Wraps std::future so that a worker thread waiting on a nested task can
// keep draining the same pool's queue instead of blocking.
//
// This is the key mechanism that prevents deadlock and starvation in
// nested-parallelism patterns (e.g., parallelMap inside a submitted task).
// When get()/wait() is called from a thread that already belongs to the
// pool, waitUntilReady() routes through ThreadPool::helpUntilReady() to
// steal and execute queued tasks while the wrapped future is not ready.
// External callers (e.g., the main thread) fall through to a plain wait().
template <typename T>
class ThreadPoolFuture
{
 public:
  // === Construction =========================================================
  // Wrap one submitted task together with the pool that should assist nested
  // waits on this future.
  ThreadPoolFuture(ThreadPool* pool,
                   std::shared_ptr<ThreadPoolLifetime> lifetime,
                   std::future<T>&& future)
      : pool_(pool), lifetime_(std::move(lifetime)), future_(std::move(future))
  {
  }

  T get();
  void wait();
  bool valid() const { return future_.valid(); }

 private:
  // === Nested wait support ==================================================
  // Route nested waits back through the pool so worker threads keep making
  // forward progress instead of blocking on child tasks from the same pool.
  void waitUntilReady();

  // === Future state =========================================================
  ThreadPool* pool_{nullptr};
  std::shared_ptr<ThreadPoolLifetime> lifetime_;
  std::future<T> future_;
};

// Lightweight work-stealing thread pool for nested parallel work.
//
// Design choices:
//   - *Nested-safe*: workers that call submit() and then get() on a future
//     from the same pool run queued tasks while waiting (via helpUntilReady)
//     instead of sleeping, so nested fork-join patterns cannot self-deadlock.
//   - *Zero-thread fallback*: when constructed with thread_count==0 all
//     submitted tasks run inline on the calling thread, preserving
//     single-threaded determinism and making debug builds reproducible.
//   - *Thread-local pool marker* (active_pool_): each worker records which
//     pool currently owns it so nested waits only assist the correct pool.
//   - *Structured completion*: parallelFor() and parallelMap() always wait for
//     every submitted task before returning or rethrowing an exception, so
//     worker lambdas cannot outlive the input vectors or caller-owned captures
//     passed into the batch helper.
//
// Tasks are submitted via submit() (single item), parallelMap() (batch with
// results), or parallelFor() (batch without results). Results are collected by
// calling get() on the returned ThreadPoolFuture objects.
class ThreadPool
{
 public:
  // === Lifecycle ============================================================
  explicit ThreadPool(size_t thread_count);
  ~ThreadPool();

  // === Task submission ======================================================
  template <typename F>
  ThreadPoolFuture<typename std::invoke_result_t<F>> submit(F&& task)
  {
    using Result = typename std::invoke_result_t<F>;

    std::shared_ptr<std::packaged_task<Result()>> packaged_task
        = std::make_shared<std::packaged_task<Result()>>(std::forward<F>(task));
    std::future<Result> future = packaged_task->get_future();

    if (workers_.empty()) {
      // With zero worker threads, run the task immediately on the caller
      // thread instead of enqueueing work that no worker can consume.
      (*packaged_task)();
      return ThreadPoolFuture<Result>(this, lifetime_, std::move(future));
    }

    // Return a pool-aware future so callers can freely compose nested submit /
    // get() patterns without manually handling worker starvation.
    {
      std::lock_guard<std::mutex> lock(lock_);
      tasks_.push([packaged_task]() { (*packaged_task)(); });
    }
    cv_.notify_one();
    return ThreadPoolFuture<Result>(this, lifetime_, std::move(future));
  }

  // === Batch execution ======================================================
  // Run one void task per input item and wait for all tasks to finish.
  // Use this when the caller does not need a result vector.
  template <typename Item, typename F>
  void parallelFor(const std::vector<Item>& items, F&& func)
  {
    using Result = std::invoke_result_t<F, const Item&>;
    using Func = std::decay_t<F>;

    static_assert(std::is_void_v<Result>,
                  "ThreadPool::parallelFor requires a void result.");

    std::shared_ptr<Func> shared_func
        = std::make_shared<Func>(std::forward<F>(func));
    std::vector<ThreadPoolFuture<void>> futures;
    futures.reserve(items.size());
    for (size_t index = 0; index < items.size(); ++index) {
      const Item* item_ptr = &items[index];
      futures.push_back(submit(
          [shared_func, item_ptr]() { std::invoke(*shared_func, *item_ptr); }));
    }

    std::exception_ptr first_exception;
    for (ThreadPoolFuture<void>& future : futures) {
      try {
        future.get();
      } catch (...) {
        if (first_exception == nullptr) {
          first_exception = std::current_exception();
        }
      }
    }

    if (first_exception != nullptr) {
      std::rethrow_exception(first_exception);
    }
  }

  // Run one non-void task per input item and return the results in input order.
  // Use this when each task produces a value. Void tasks should use
  // parallelFor() so callers cannot accidentally request std::vector<void>.
  template <typename Item, typename F>
  std::vector<std::invoke_result_t<F, const Item&>> parallelMap(
      const std::vector<Item>& items,
      F&& func)
  {
    using Result = std::invoke_result_t<F, const Item&>;
    using Func = std::decay_t<F>;

    static_assert(!std::is_void_v<Result>,
                  "ThreadPool::parallelMap requires a non-void result.");

    std::shared_ptr<Func> shared_func
        = std::make_shared<Func>(std::forward<F>(func));
    std::vector<ThreadPoolFuture<Result>> futures;
    futures.reserve(items.size());
    // Keep one future per input item so results can be collected in the same
    // order that the caller provided.
    for (size_t index = 0; index < items.size(); ++index) {
      const Item* item_ptr = &items[index];
      futures.push_back(submit([shared_func, item_ptr]() -> Result {
        return std::invoke(*shared_func, *item_ptr);
      }));
    }

    std::vector<std::optional<Result>> staged_results;
    staged_results.reserve(futures.size());
    std::exception_ptr first_exception;
    for (ThreadPoolFuture<Result>& future : futures) {
      try {
        staged_results.emplace_back(future.get());
      } catch (...) {
        if (first_exception == nullptr) {
          first_exception = std::current_exception();
        }
        staged_results.emplace_back(std::nullopt);
      }
    }

    if (first_exception != nullptr) {
      std::rethrow_exception(first_exception);
    }

    std::vector<Result> results;
    results.reserve(staged_results.size());
    for (std::optional<Result>& staged_result : staged_results) {
      results.push_back(std::move(*staged_result));
    }
    return results;
  }

  size_t threadCount() const;

  // === Non-copyable pool ownership =========================================
  ThreadPool(const ThreadPool& rhs) = delete;
  ThreadPool& operator=(const ThreadPool& rhs) = delete;
  ThreadPool(ThreadPool&& rhs) = delete;
  ThreadPool& operator=(ThreadPool&& rhs) = delete;

 private:
  // === Worker execution =====================================================
  static bool isWorkerThreadFor(const ThreadPool* pool);
  bool tryRunPendingTask();
  void workerLoop();

  // === Nested wait helpers ==================================================
  template <typename T>
  static bool futureIsReady(std::future<T>& future)
  {
    // Probe readiness without blocking so nested waits can first try to help
    // the pool run queued child tasks.
    return future.wait_for(std::chrono::milliseconds(0))
           == std::future_status::ready;
  }

  template <typename T>
  void helpUntilReady(std::future<T>& future)
  {
    // A worker that waits on a future from the same pool should help execute
    // queued tasks instead of sleeping. This is the core nested-threading
    // support path that prevents pool starvation and self-deadlock when one
    // task fans out into more work and then waits for it.
    while (!futureIsReady(future)) {
      if (tryRunPendingTask()) {
        continue;
      }

      // No queued work is ready to steal right now, so back off briefly before
      // polling again. The short wait avoids a hot spin while still keeping the
      // nested wait responsive.
      future.wait_for(std::chrono::milliseconds(1));
    }
  }

  // === Queue state ==========================================================
  std::mutex lock_;
  std::condition_variable cv_;
  std::queue<std::function<void()>> tasks_;
  std::vector<std::thread> workers_;
  std::shared_ptr<ThreadPoolLifetime> lifetime_;
  bool stop_{false};
  // Keep one active-pool marker per thread so nested waits only assist the
  // pool that currently owns that worker thread.
  static thread_local ThreadPool* active_pool_;

  template <typename T>
  friend class ThreadPoolFuture;
};

template <typename T>
T ThreadPoolFuture<T>::get()
{
  waitUntilReady();

  if constexpr (std::is_void_v<T>) {
    future_.get();
  } else {
    return future_.get();
  }
}

template <typename T>
void ThreadPoolFuture<T>::wait()
{
  waitUntilReady();
}

template <typename T>
void ThreadPoolFuture<T>::waitUntilReady()
{
  // Guard against get()/wait() being called more than once, or against a
  // moved-from future. std::future::wait() on a non-valid future is undefined
  // behavior, so fail loudly instead.
  if (!future_.valid()) {
    throw std::logic_error(
        "ThreadPoolFuture::get()/wait() called on an invalid future.");
  }

  if (lifetime_->pool_alive.load(std::memory_order_acquire)
      && ThreadPool::isWorkerThreadFor(pool_)) {
    // Only workers that already belong to this pool need the nested-threading
    // assist path. External callers can block normally on the wrapped future.
    pool_->helpUntilReady(future_);
    return;
  }

  future_.wait();
}

}  // namespace utl
