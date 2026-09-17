// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2026, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace drt {

struct TaskRecord {
  std::string name;
  std::atomic<uint64_t> call_count{0};
  std::atomic<uint64_t> wall_ns{0};
  std::atomic<uint64_t> cpu_ns{0};
};

class ProfileRegistry {
 public:
  static ProfileRegistry& get() {
    static ProfileRegistry instance;
    return instance;
  }

  TaskRecord* getOrCreateRecord(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_map_.find(name);
    if (it != records_map_.end()) {
      return it->second;
    }
    auto record = std::make_unique<TaskRecord>();
    record->name = name;
    TaskRecord* ptr = record.get();
    records_vec_.push_back(std::move(record));
    records_map_[name] = ptr;
    return ptr;
  }

  void report(const std::string& phase_title = "TRITONROUTE") {
    std::lock_guard<std::mutex> lock(mutex_);
    if (records_vec_.empty()) {
      return;
    }

    std::ostringstream oss;
    oss << "\n========================================================================================================\n";
    oss << "=== " << phase_title << " DETAILED PROFILING REPORT ===\n";
    oss << "========================================================================================================\n";
    oss << std::left << std::setw(34) << "Task / Region Name"
        << std::right << std::setw(12) << "Calls"
        << std::setw(18) << "Wall Time (s)"
        << std::setw(18) << "CPU Time (s)"
        << std::setw(14) << "Avg Wall(ms)"
        << std::setw(12) << "CPU/Wall\n";
    oss << "--------------------------------------------------------------------------------------------------------\n";

    std::vector<TaskRecord*> sorted_records;
    for (const auto& rec : records_vec_) {
      if (rec->call_count.load(std::memory_order_relaxed) > 0) {
        sorted_records.push_back(rec.get());
      }
    }
    std::sort(sorted_records.begin(), sorted_records.end(),
              [](const TaskRecord* a, const TaskRecord* b) {
                return a->wall_ns.load(std::memory_order_relaxed) >
                       b->wall_ns.load(std::memory_order_relaxed);
              });

    for (const auto* rec : sorted_records) {
      uint64_t count = rec->call_count.load(std::memory_order_relaxed);
      uint64_t wall = rec->wall_ns.load(std::memory_order_relaxed);
      uint64_t cpu = rec->cpu_ns.load(std::memory_order_relaxed);

      double wall_s = static_cast<double>(wall) / 1e9;
      double cpu_s = static_cast<double>(cpu) / 1e9;
      double avg_ms = count > 0 ? (static_cast<double>(wall) / 1e6) / count : 0.0;
      double cpu_ratio = wall_s > 0.001 ? (cpu_s / wall_s) : 0.0;

      oss << std::left << std::setw(34) << rec->name
          << std::right << std::setw(12) << count
          << std::fixed << std::setprecision(3)
          << std::setw(18) << wall_s
          << std::setw(18) << cpu_s
          << std::setw(14) << avg_ms
          << std::setprecision(2)
          << std::setw(12) << cpu_ratio << "\n";
    }
    oss << "========================================================================================================\n\n";
    std::cout << oss.str() << std::flush;
  }

  void reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& rec : records_vec_) {
      rec->call_count = 0;
      rec->wall_ns = 0;
      rec->cpu_ns = 0;
    }
  }

 private:
  ProfileRegistry() = default;
  std::mutex mutex_;
  std::vector<std::unique_ptr<TaskRecord>> records_vec_;
  std::unordered_map<std::string, TaskRecord*> records_map_;
};

class ProfileTask {
 public:
  explicit ProfileTask(const char* name) : done_(false), record_(nullptr) {
    if (name) {
      record_ = ProfileRegistry::get().getOrCreateRecord(name);
      wall_start_ = std::chrono::steady_clock::now();
      struct timespec ts;
      clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
      cpu_start_ns_ = static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + ts.tv_nsec;
    }
  }

  explicit ProfileTask(const std::string& name) : ProfileTask(name.c_str()) {}

  ~ProfileTask() {
    done();
  }

  void done() {
    if (!done_ && record_) {
      done_ = true;
      auto wall_now = std::chrono::steady_clock::now();
      uint64_t wall_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(wall_now - wall_start_).count();

      struct timespec ts;
      clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
      uint64_t cpu_now_ns = static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + ts.tv_nsec;
      uint64_t cpu_ns = (cpu_now_ns >= cpu_start_ns_) ? (cpu_now_ns - cpu_start_ns_) : 0;

      record_->call_count.fetch_add(1, std::memory_order_relaxed);
      record_->wall_ns.fetch_add(wall_ns, std::memory_order_relaxed);
      record_->cpu_ns.fetch_add(cpu_ns, std::memory_order_relaxed);
    }
  }

 private:
  bool done_;
  TaskRecord* record_;
  std::chrono::steady_clock::time_point wall_start_;
  uint64_t cpu_start_ns_{0};
};

}  // namespace drt
