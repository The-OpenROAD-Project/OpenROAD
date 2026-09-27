// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "utl/cpu_topology.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef __linux__
#include <sched.h>
#endif

namespace utl {

namespace {

// The CPUs this process may run on; empty when that cannot be determined.
std::vector<int> affinityCpus()
{
  std::vector<int> cpus;
#ifdef __linux__
  cpu_set_t set;
  CPU_ZERO(&set);
  if (sched_getaffinity(0, sizeof(set), &set) == 0) {
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
      if (CPU_ISSET(cpu, &set)) {
        cpus.push_back(cpu);
      }
    }
  }
#endif
  return cpus;
}

bool readInt(const std::filesystem::path& path, int& value)
{
  std::ifstream in(path);
  return static_cast<bool>(in >> value);
}

}  // namespace

int physicalCoreCount(const std::string& sysfs_cpu_dir,
                      const std::vector<int>& cpus)
{
  std::set<std::pair<int, int>> cores;
  const std::filesystem::path root(sysfs_cpu_dir);
  for (const int cpu : cpus) {
    const auto topology = root / ("cpu" + std::to_string(cpu)) / "topology";
    int package = 0;
    int core = 0;
    if (readInt(topology / "physical_package_id", package)
        && readInt(topology / "core_id", core)) {
      cores.emplace(package, core);
    }
  }
  if (!cores.empty()) {
    return static_cast<int>(cores.size());
  }
  // No topology to read: the hardware thread count is the previous answer
  // and the safe one.
  if (!cpus.empty()) {
    return static_cast<int>(cpus.size());
  }
  const int threads = static_cast<int>(std::thread::hardware_concurrency());
  return threads > 0 ? threads : 1;
}

int physicalCoreCount(const std::string& sysfs_cpu_dir)
{
  static const int count = [&] {
    std::vector<int> cpus = affinityCpus();
    if (cpus.empty()) {
      // No affinity information: consider every cpuN sysfs entry.
      std::error_code ec;
      for (const auto& entry :
           std::filesystem::directory_iterator(sysfs_cpu_dir, ec)) {
        const std::string name = entry.path().filename().string();
        if (name.size() > 3 && name.rfind("cpu", 0) == 0
            && std::isdigit(static_cast<unsigned char>(name[3]))) {
          cpus.push_back(std::stoi(name.substr(3)));
        }
      }
    }
    return physicalCoreCount(sysfs_cpu_dir, cpus);
  }();
  return count;
}

}  // namespace utl
