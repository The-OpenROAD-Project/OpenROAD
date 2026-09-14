// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

namespace utl {

// The number of physical cores this process may run on.
//
// std::thread::hardware_concurrency() counts hardware threads, so on a
// machine with SMT it is twice the core count. Code whose parallel regions
// are memory-bound and end in a barrier runs slower with one thread per
// hardware thread than with one per core: two siblings share the core, and
// the one spinning in the barrier slows the one still working.
//
// On Linux the count is the number of distinct (package, core) pairs under
// sysfs_cpu_dir for the CPUs in the process affinity mask, so taskset and
// container CPU limits are honored. Where the topology cannot be read
// (including on macOS) it falls back to the hardware thread count, which is
// what callers used before and never worse than that.
int physicalCoreCount(const std::string& sysfs_cpu_dir
                      = "/sys/devices/system/cpu");

// The same, for an explicit set of logical CPU ids. Exposed for tests.
int physicalCoreCount(const std::string& sysfs_cpu_dir,
                      const std::vector<int>& cpus);

}  // namespace utl
