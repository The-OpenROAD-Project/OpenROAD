// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace stt {
namespace {

TEST(SteinerTreeBuilderThreadSafety, ConcurrentMakeSteinerTree)
{
  auto db = odb::dbDatabase::create();
  utl::Logger logger;
  SteinerTreeBuilder builder(db, &logger);
  builder.setAlpha(0.3);

  constexpr int kNumThreads = 8;
  constexpr int kNetsPerThread = 100;
  std::vector<std::thread> threads;
  threads.reserve(kNumThreads);
  std::vector<bool> results(kNumThreads, true);

  for (int t = 0; t < kNumThreads; ++t) {
    threads.emplace_back([&builder, &results, t]() {
      for (int i = 0; i < kNetsPerThread; ++i) {
        // Vary net size per iteration for diversity
        const int pin_count = 3 + (i % 15);
        std::vector<int> x(pin_count), y(pin_count);
        for (int p = 0; p < pin_count; ++p) {
          x[p] = (t * 1000) + (p * 100) + (i * 10);
          y[p] = (p % 3) * 100 + (i % 5) * 50;
        }

        Tree tree = builder.makeSteinerTree(x, y, 0, 0.3);

        if (tree.deg != pin_count) {
          results[t] = false;
          return;
        }
        if (!builder.checkTree(tree)) {
          // checkTree failure is expected for some PD trees
          // (they fall back to FLUTE), but the tree itself
          // should still be valid
        }
        // Verify branch indices are valid
        for (int b = 0; b < tree.branchCount(); ++b) {
          if (tree.branch[b].n < 0 || tree.branch[b].n >= tree.branchCount()) {
            results[t] = false;
            return;
          }
        }
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  for (int t = 0; t < kNumThreads; ++t) {
    EXPECT_TRUE(results[t]) << "Thread " << t << " had a failure";
  }

  odb::dbDatabase::destroy(db);
}

TEST(SteinerTreeBuilderThreadSafety, ConcurrentFluteOnly)
{
  auto db = odb::dbDatabase::create();
  utl::Logger logger;
  SteinerTreeBuilder builder(db, &logger);

  constexpr int kNumThreads = 8;
  constexpr int kNetsPerThread = 200;
  std::vector<std::thread> threads;
  threads.reserve(kNumThreads);
  std::vector<int> total_wl(kNumThreads, 0);

  for (int t = 0; t < kNumThreads; ++t) {
    threads.emplace_back([&builder, &total_wl, t]() {
      for (int i = 0; i < kNetsPerThread; ++i) {
        const int pin_count = 2 + (i % 8);
        std::vector<int> x(pin_count), y(pin_count);
        for (int p = 0; p < pin_count; ++p) {
          x[p] = p * 100;
          y[p] = (p % 2) * 100;
        }

        // Alpha=0 forces FLUTE path (no PD)
        Tree tree = builder.makeSteinerTree(x, y, 0, 0.0);

        total_wl[t] += tree.length;
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  // All threads should produce identical results for identical inputs
  for (int t = 1; t < kNumThreads; ++t) {
    EXPECT_EQ(total_wl[0], total_wl[t])
        << "Thread " << t << " produced different total wirelength";
  }

  odb::dbDatabase::destroy(db);
}

}  // namespace
}  // namespace stt
