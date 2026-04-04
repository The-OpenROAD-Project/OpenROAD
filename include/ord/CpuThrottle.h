// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <vector>

namespace utl {
class Logger;
}

namespace ord {

// Cross-process CPU throttle using per-CPU file locks.
// Multiple OpenROAD processes coordinate through /tmp/openroad_cpu_sem/
// so that total active threads never exceed available cores.
// On destruction (or process crash), the OS releases all held flocks.
//
// Supports dynamic resize: starts holding 1 slot, can acquire more
// before parallel sections and release back to 1 afterward.
class CpuThreadThrottle
{
 public:
  // Acquires 1 CPU slot (for the main thread).
  CpuThreadThrottle(utl::Logger* logger);
  ~CpuThreadThrottle();

  CpuThreadThrottle(const CpuThreadThrottle&) = delete;
  CpuThreadThrottle& operator=(const CpuThreadThrottle&) = delete;
  CpuThreadThrottle(CpuThreadThrottle&& other) noexcept;
  CpuThreadThrottle& operator=(CpuThreadThrottle&& other) noexcept;

  // Adjust held slots to exactly num_threads.
  // Acquires more if needed, releases excess if shrinking.
  void resize(int num_threads);

  int heldCount() const { return static_cast<int>(held_fds_.size()); }

 private:
  void ensureDirectory();
  bool tryAcquireAdditional(int additional);

  std::vector<int> held_fds_;
  int total_cpus_ = 0;
  utl::Logger* logger_;

  // Stats for diagnostics
  int total_wait_ms_ = 0;
  int resize_count_ = 0;
  int contention_count_ = 0;

  static constexpr const char* kSemDir = "/tmp/openroad_cpu_sem";
  static constexpr const char* kCoordinatorLock
      = "/tmp/openroad_cpu_sem/coordinator.lock";
  static constexpr int kRetryDelayMs = 100;
  static constexpr int kMaxRetries = 300;  // 30 seconds
};

// RAII guard that acquires CPU slots on construction and releases
// back to 1 slot on destruction.  No-op if throttle is nullptr.
class CpuThreadGuard
{
 public:
  CpuThreadGuard(CpuThreadThrottle* throttle, int num_threads);
  ~CpuThreadGuard();

  CpuThreadGuard(const CpuThreadGuard&) = delete;
  CpuThreadGuard& operator=(const CpuThreadGuard&) = delete;
  CpuThreadGuard(CpuThreadGuard&& other) noexcept;
  CpuThreadGuard& operator=(CpuThreadGuard&& other) noexcept;

 private:
  CpuThreadThrottle* throttle_;
};

// Global throttle accessor — set by OpenRoad during init.
// Returns nullptr if throttle is disabled.
CpuThreadThrottle* globalCpuThrottle();
void setGlobalCpuThrottle(CpuThreadThrottle* throttle);
void setGlobalCpuTargetThreads(int threads);
int globalCpuTargetThreads();

// Convenience: acquire target threads from the global throttle.
// Returns a no-op guard if throttle is nullptr.
CpuThreadGuard acquireGlobalCpuThreads();
CpuThreadGuard acquireGlobalCpuThreads(int num_threads);

}  // namespace ord
