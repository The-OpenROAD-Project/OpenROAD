// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "WorkerPool.h"

#include <algorithm>
#include <functional>

namespace fsv {

WorkerPool::WorkerPool(const int threads)
    : start_(std::max(1, threads)), done_(std::max(1, threads))
{
  for (int t = 1; t < threads; t++) {
    workers_.emplace_back([this]() {
      while (true) {
        start_.arrive_and_wait();
        if (stop_) {
          return;
        }
        (*job_)();
        done_.arrive_and_wait();
      }
    });
  }
}

WorkerPool::~WorkerPool()
{
  stop_ = true;
  start_.arrive_and_wait();
}

void WorkerPool::run(const std::function<void()>& job)
{
  job_ = &job;
  start_.arrive_and_wait();
  job();
  done_.arrive_and_wait();
  job_ = nullptr;
}

}  // namespace fsv
