// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Multi-threaded tests of the pools, in their own target so that
// --config=tsan --test_tag_filters=tsan runs exactly these.

#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/Ids.h"
#include "syn/lir/NameInterner.h"

namespace syn::test {
namespace {

using ::syn::lir::ConstId;
using ::syn::lir::ConstPool;
using ::syn::lir::NameId;
using ::syn::lir::NameInterner;
using ::syn::lir::TableMode;

TEST(ConstPoolTest, ParallelLookupsInReadOnlyMode)
{
  ConstPool pool;
  pool.setMode(TableMode::kSingleWrite);
  constexpr uint64_t kCount = 5000;
  std::vector<ConstId> ids;
  ids.reserve(kCount);
  for (uint64_t i = 0; i < kCount; ++i) {
    ids.push_back(pool.internUint(32, i));
  }
  pool.setMode(TableMode::kReadOnly);
  std::vector<std::thread> threads;
  threads.reserve(8);
  for (int t = 0; t < 8; ++t) {
    threads.emplace_back([&] {
      for (uint64_t i = 0; i < kCount; ++i) {
        ASSERT_EQ(pool.get(ids[i]).toUint64(), i);
        ASSERT_EQ(pool.internUint(32, i), ids[i]);
      }
    });
  }
  for (std::thread& thread : threads) {
    thread.join();
  }
}

// Every thread interns the same pooled constants in its
// own order; all agree on the ids and each constant is stored once.
TEST(ConstPoolTest, ConcurrentInterningAgrees)
{
  constexpr int kThreads = 8;
  constexpr uint64_t kCount = 5000;
  ConstPool pool;
  pool.setMode(TableMode::kParallelWrite);
  std::vector<std::vector<ConstId>> ids(kThreads);
  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([&, t] {
      for (uint64_t k = 0; k < kCount; ++k) {
        const uint64_t i
            = (k * 7919 + static_cast<uint64_t>(t) * 104729) % kCount;
        ids[t].push_back(pool.internUint(40, i));
      }
    });
  }
  for (std::thread& thread : threads) {
    thread.join();
  }
  pool.setMode(TableMode::kReadOnly);
  EXPECT_EQ(pool.size(), kCount);
  for (int t = 0; t < kThreads; ++t) {
    for (uint64_t k = 0; k < kCount; ++k) {
      const uint64_t i
          = (k * 7919 + static_cast<uint64_t>(t) * 104729) % kCount;
      ASSERT_EQ(pool.internUint(40, i), ids[t][k]);
      ASSERT_EQ(pool.get(ids[t][k]).toUint64(), i);
    }
  }
}

TEST(NameInternerTest, ConcurrentInterningAgrees)
{
  constexpr int kThreads = 4;
  constexpr int kNames = 5000;
  NameInterner names;
  names.setMode(TableMode::kParallelWrite);
  std::vector<std::vector<NameId>> ids(kThreads);
  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([&, t] {
      // Every thread interns the same names, in a different order.
      for (int k = 0; k < kNames; ++k) {
        const int i = (k * 7919 + t * 104729) % kNames;
        ids[t].push_back(names.intern(absl::StrCat("n", i)));
      }
    });
  }
  for (std::thread& thread : threads) {
    thread.join();
  }
  EXPECT_EQ(names.size(), kNames);
  names.setMode(TableMode::kReadOnly);
  for (int i = 0; i < kNames; ++i) {
    const NameId id = names.find(absl::StrCat("n", i));
    ASSERT_TRUE(id.isValid());
    EXPECT_EQ(names.name(id), absl::StrCat("n", i));
  }
  for (int t = 0; t < kThreads; ++t) {
    for (int k = 0; k < kNames; ++k) {
      const int i = (k * 7919 + t * 104729) % kNames;
      ASSERT_EQ(ids[t][k], names.find(absl::StrCat("n", i)));
    }
  }
}

TEST(NameInternerTest, ParallelLookupsInReadOnlyMode)
{
  NameInterner names;
  names.setMode(TableMode::kSingleWrite);
  constexpr int kNames = 5000;
  std::vector<NameId> ids;
  ids.reserve(kNames);
  for (int i = 0; i < kNames; ++i) {
    ids.push_back(names.intern(absl::StrCat("n", i)));
  }
  names.setMode(TableMode::kReadOnly);
  constexpr int kThreads = 8;
  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([&] {
      for (int i = 0; i < kNames; ++i) {
        const std::string text = absl::StrCat("n", i);
        ASSERT_EQ(names.name(ids[i]), text);
        ASSERT_EQ(names.find(text), ids[i]);
        ASSERT_EQ(names.intern(text), ids[i]);
      }
    });
  }
  for (std::thread& thread : threads) {
    thread.join();
  }
  names.setMode(TableMode::kSingleWrite);
}

}  // namespace
}  // namespace syn::test
