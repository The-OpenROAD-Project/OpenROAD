// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"
#include "syn/lir/Ids.h"
#include "syn/lir/NameInterner.h"

namespace syn::test {
namespace {

using ::syn::lir::NameId;
using ::syn::lir::NameInterner;
using ::syn::lir::TableMode;

TEST(NameInternerTest, DefaultsToReadOnly)
{
  NameInterner names;
  EXPECT_TRUE(names.isReadOnly());
  EXPECT_FALSE(names.find("clk").isValid());
}

TEST(NameInternerTest, InternIsIdempotent)
{
  NameInterner names;
  names.setMode(TableMode::kSingleWrite);
  const NameId a = names.intern("clk");
  const NameId b = names.intern("rst");
  EXPECT_TRUE(a.isValid());
  EXPECT_NE(a, b);
  EXPECT_EQ(names.intern("clk"), a);
  EXPECT_EQ(names.name(a), "clk");
  EXPECT_EQ(names.name(b), "rst");
  EXPECT_EQ(names.size(), 2u);
}

TEST(NameInternerTest, FindDoesNotAdd)
{
  NameInterner names;
  names.setMode(TableMode::kSingleWrite);
  EXPECT_FALSE(names.find("missing").isValid());
  EXPECT_EQ(names.size(), 0u);
  const NameId id = names.intern("present");
  EXPECT_EQ(names.find("present"), id);
}

TEST(NameInternerTest, EmptyAndLargeStrings)
{
  NameInterner names;
  names.setMode(TableMode::kSingleWrite);
  const std::string large(100000, 'x');
  const NameId empty = names.intern("");
  const NameId big = names.intern(large);
  const NameId small = names.intern("after_large");
  EXPECT_EQ(names.name(empty), "");
  EXPECT_EQ(names.name(big), large);
  EXPECT_EQ(names.name(small), "after_large");
}

TEST(NameInternerTest, ManyNamesStayValid)
{
  // Long paths: 12000 names fill more than 1 MB of text, past the chunk
  // size cap.
  constexpr int kNames = 12000;
  const std::string path = absl::StrCat("top/", std::string(80, 'h'), "/u");
  NameInterner names;
  names.setMode(TableMode::kSingleWrite);
  std::vector<NameId> ids;
  for (int i = 0; i < kNames; ++i) {
    ids.push_back(names.intern(absl::StrCat(path, i, "/net")));
  }
  for (int i = 0; i < kNames; ++i) {
    ASSERT_EQ(names.name(ids[i]), absl::StrCat(path, i, "/net"));
  }
  const absl::flat_hash_set<NameId> unique(ids.begin(), ids.end());
  EXPECT_EQ(unique.size(), ids.size());
  EXPECT_GT(names.memoryBytes(), kNames * 10);
}

TEST(NameInternerTest, ConcurrentInterningAgrees)
{
  constexpr int kThreads = 4;
  constexpr int kNames = 5000;
  NameInterner names;
  names.setMode(TableMode::kParallelWrite);
  std::vector<std::vector<NameId>> ids(kThreads);
  std::vector<std::thread> threads;
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

TEST(NameInternerTest, ReadOnlyModeAllowsLookupsOnly)
{
  NameInterner names;
  names.setMode(TableMode::kSingleWrite);
  const NameId clk = names.intern("clk");
  names.setMode(TableMode::kReadOnly);
  EXPECT_TRUE(names.isReadOnly());
  EXPECT_EQ(names.intern("clk"), clk);  // existing name: a lookup
  EXPECT_EQ(names.find("clk"), clk);
  EXPECT_FALSE(names.find("rst").isValid());
  EXPECT_EQ(names.name(clk), "clk");
  EXPECT_EQ(names.size(), 1u);
  names.setMode(TableMode::kSingleWrite);
  EXPECT_TRUE(names.intern("rst").isValid());
  EXPECT_EQ(names.size(), 2u);
}

// Run under TSan.
TEST(NameInternerTest, ParallelLookupsInReadOnlyMode)
{
  NameInterner names;
  names.setMode(TableMode::kSingleWrite);
  constexpr int kNames = 5000;
  std::vector<NameId> ids;
  for (int i = 0; i < kNames; ++i) {
    ids.push_back(names.intern(absl::StrCat("n", i)));
  }
  names.setMode(TableMode::kReadOnly);
  std::vector<std::thread> threads;
  for (int t = 0; t < 8; ++t) {
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

TEST(NameInternerTest, IdsAreDenseInInsertionOrder)
{
  NameInterner names;
  names.setMode(TableMode::kSingleWrite);
  for (uint32_t i = 0; i < 1000; ++i) {
    ASSERT_EQ(names.intern(absl::StrCat("n", i)), NameId(i));
  }
  EXPECT_EQ(names.intern("n500"), NameId(500));  // repeats get no new id
  EXPECT_EQ(names.intern("new"), NameId(1000));
}

TEST(NameInternerTest, ContainsOnlyIssuedIds)
{
  NameInterner names;
  names.setMode(TableMode::kSingleWrite);
  EXPECT_FALSE(names.contains(NameId(0)));
  const NameId a = names.intern("a");
  EXPECT_TRUE(names.contains(a));
  EXPECT_FALSE(names.contains(NameId(1)));
  EXPECT_FALSE(names.contains(NameId()));
}

// Lengths around the varint boundaries (1, 2 and 3 length bytes), mixed
// with strings that need their own chunk.
TEST(NameInternerTest, LengthPrefixesAndChunkBoundaries)
{
  NameInterner names;
  names.setMode(TableMode::kSingleWrite);
  std::vector<std::string> texts;
  for (const size_t length :
       {0, 1, 127, 128, 129, 1000, 16383, 16384, 16385, 300000}) {
    texts.push_back(std::string(length, static_cast<char>('a' + length % 26)));
    texts.push_back(absl::StrCat("small", length));
  }
  std::vector<NameId> ids;
  for (const std::string& text : texts) {
    ids.push_back(names.intern(text));
  }
  for (size_t i = 0; i < texts.size(); ++i) {
    ASSERT_EQ(names.name(ids[i]), texts[i]) << i;
    ASSERT_EQ(names.find(texts[i]), ids[i]) << i;
  }
}

TEST(NameInternerTest, InternPrefixedJoinsPrefixAndLeaf)
{
  NameInterner names;
  names.setMode(TableMode::kSingleWrite);
  const NameId leaf = names.intern("q");
  const NameId path = names.internPrefixed("u1/u2/", leaf);
  EXPECT_EQ(names.name(path), "u1/u2/q");
  EXPECT_EQ(names.find("u1/u2/q"), path);
  EXPECT_EQ(names.internPrefixed("u1/u2/", leaf), path);  // idempotent
  EXPECT_EQ(names.internPrefixed("", leaf), leaf);
  // A prefix longer than the leaf, and a leaf interned after the path.
  const NameId longer = names.internPrefixed(std::string(300, 'p'), leaf);
  EXPECT_EQ(names.name(longer), std::string(300, 'p') + "q");

  names.setMode(TableMode::kReadOnly);  // existing paths are still found
  EXPECT_EQ(names.internPrefixed("u1/u2/", leaf), path);
}

TEST(NameInternerTest, SmallFootprint)
{
  NameInterner empty;
  EXPECT_LT(empty.memoryBytes(), 1024);  // no chunk until the first name

  NameInterner names;
  names.setMode(TableMode::kSingleWrite);
  constexpr int kNames = 16000;
  size_t text_bytes = 0;
  for (int i = 0; i < kNames; ++i) {
    const std::string text = absl::StrCat("top/u", i, "/net");
    text_bytes += text.size();
    names.intern(text);
  }
  // About 21 B per name plus the string and its length byte; allow for the
  // table's load factor and the chunk slack.
  const size_t overhead = names.memoryBytes() - text_bytes;
  EXPECT_LT(overhead / kNames, 40) << names.memoryBytes();
}

}  // namespace
}  // namespace syn::test
