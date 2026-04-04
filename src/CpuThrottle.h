// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <vector>

namespace utl {
class Logger;
}

namespace ord {

// RAII guard that acquires CPU slots via per-CPU file locks.
// Multiple OpenROAD processes coordinate through /tmp/openroad_cpu_sem/
// so that total active threads never exceed available cores.
// On destruction (or process crash), the OS releases all held flocks.
class CpuThreadThrottle
{
 public:
  // Blocks until num_threads CPU slots are available.
  CpuThreadThrottle(int num_threads, utl::Logger* logger);
  ~CpuThreadThrottle();

  CpuThreadThrottle(const CpuThreadThrottle&) = delete;
  CpuThreadThrottle& operator=(const CpuThreadThrottle&) = delete;
  CpuThreadThrottle(CpuThreadThrottle&& other) noexcept;
  CpuThreadThrottle& operator=(CpuThreadThrottle&& other) noexcept;

 private:
  void ensureDirectory();
  bool tryAcquire(int num_threads, int total_cpus);

  std::vector<int> held_fds_;
  int coordinator_fd_ = -1;
  utl::Logger* logger_;

  static constexpr const char* kSemDir = "/tmp/openroad_cpu_sem";
  static constexpr const char* kCoordinatorLock
      = "/tmp/openroad_cpu_sem/coordinator.lock";
};

}  // namespace ord
