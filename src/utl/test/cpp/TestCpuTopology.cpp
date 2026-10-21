// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "utl/cpu_topology.h"

namespace utl {

namespace {

// A fake sysfs cpu directory: cpus 0..n-1, cpu i on core (i % cores) of
// package (i / cpus_per_package), which is how Linux numbers SMT siblings
// on a one-package machine (cpu 0 and cpu 24 share core 0 on a 24-core,
// 48-thread part).
std::string makeTree(const std::string& name,
                     int cpus,
                     int cores,
                     int cpus_per_package,
                     bool with_topology = true)
{
  const std::filesystem::path root
      = std::filesystem::path(testing::TempDir()) / name;
  std::filesystem::remove_all(root);
  for (int cpu = 0; cpu < cpus; ++cpu) {
    const auto dir = root / ("cpu" + std::to_string(cpu)) / "topology";
    std::filesystem::create_directories(dir);
    if (with_topology) {
      std::ofstream(dir / "core_id") << (cpu % cores) << "\n";
      std::ofstream(dir / "physical_package_id")
          << (cpu / cpus_per_package) << "\n";
    }
  }
  return root.string();
}

std::vector<int> range(int begin, int end)
{
  std::vector<int> cpus;
  for (int cpu = begin; cpu < end; ++cpu) {
    cpus.push_back(cpu);
  }
  return cpus;
}

}  // namespace

TEST(CpuTopology, SmtSiblingsCountOnce)
{
  const auto root = makeTree("smt", 48, 24, 48);
  EXPECT_EQ(physicalCoreCount(root, range(0, 48)), 24);
}

TEST(CpuTopology, NoSmtCountsEveryCpu)
{
  const auto root = makeTree("nosmt", 16, 16, 16);
  EXPECT_EQ(physicalCoreCount(root, range(0, 16)), 16);
}

TEST(CpuTopology, CoreIdsRepeatAcrossPackages)
{
  // Two packages of 8 cores, 2 threads each: core_id 0..7 appears on both.
  const auto root = makeTree("twosocket", 32, 8, 16);
  EXPECT_EQ(physicalCoreCount(root, range(0, 32)), 16);
}

TEST(CpuTopology, AffinityRestrictsTheCount)
{
  const auto root = makeTree("affinity", 48, 24, 48);
  // Pinned to the first 12 cpus: 12 cores, no siblings among them.
  EXPECT_EQ(physicalCoreCount(root, range(0, 12)), 12);
  // Pinned to cpu 0 and its sibling cpu 24: one core.
  EXPECT_EQ(physicalCoreCount(root, {0, 24}), 1);
}

TEST(CpuTopology, NoTopologyFallsBackToTheCpuCount)
{
  const auto root = makeTree("bare", 8, 8, 8, /*with_topology=*/false);
  EXPECT_EQ(physicalCoreCount(root, range(0, 8)), 8);
}

TEST(CpuTopology, RealMachineIsSane)
{
  const int cores = physicalCoreCount();
  EXPECT_GE(cores, 1);
  EXPECT_LE(cores,
            static_cast<int>(std::thread::hardware_concurrency()) > 0
                ? static_cast<int>(std::thread::hardware_concurrency())
                : cores);
}

}  // namespace utl
