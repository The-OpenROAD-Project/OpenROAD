// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <array>
#include <cstdint>
#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gtest/gtest.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/ConstView.h"
#include "syn/lir/Ids.h"

namespace syn::test {
namespace {

using ::syn::lir::ConstId;
using ::syn::lir::ConstPool;
using ::syn::lir::ConstView;
using ::syn::lir::LogicValue;
using ::syn::lir::TableMode;

TEST(ConstPoolTest, DefaultsToReadOnly)
{
  ConstPool pool;
  EXPECT_TRUE(pool.isReadOnly());
}

TEST(ConstPoolTest, EqualValuesShareAnId)
{
  ConstPool pool;
  pool.setMode(TableMode::kSingleWrite);
  const ConstId five = pool.internUint(8, 5);
  EXPECT_EQ(pool.internUint(8, 5), five);
  EXPECT_NE(pool.internUint(9, 5), five);  // width is part of the value
  EXPECT_NE(pool.internUint(8, 6), five);
  EXPECT_EQ(*pool.internBitString("00000101"), five);
  EXPECT_EQ(pool.size(), 3u);
}

TEST(ConstPoolTest, ThreeStateBits)
{
  ConstPool pool;
  pool.setMode(TableMode::kSingleWrite);
  const ConstView value = pool.get(*pool.internBitString("10x1"));
  EXPECT_EQ(value.width(), 4);
  EXPECT_EQ(value.bit(0), LogicValue::kOne);
  EXPECT_EQ(value.bit(1), LogicValue::kX);
  EXPECT_EQ(value.bit(2), LogicValue::kZero);
  EXPECT_EQ(value.bit(3), LogicValue::kOne);
  // Planes: aval = 1 -> 1; bval = x -> 1.
  EXPECT_EQ(value.aval()[0], 0b1001u);
  EXPECT_EQ(value.bval()[0], 0b0010u);
  EXPECT_FALSE(value.isTwoState());
  EXPECT_FALSE(value.toUint64().has_value());
  EXPECT_EQ(value.toString(), "4'b10x1");
  EXPECT_EQ(*pool.internBitString("10X1"), *pool.internBitString("10x1"));
  // bval = 1 with aval = 1 is normalized to x (aval = 0, bval = 1).
  const std::array<uint64_t, 1> aval = {0b1011u};
  const std::array<uint64_t, 1> bval = {0b0010u};
  EXPECT_EQ(pool.intern(4, aval, bval), *pool.internBitString("10x1"));
  EXPECT_FALSE(pool.internBitString("10z").ok());
  EXPECT_FALSE(pool.internBitString("10Z").ok());
  EXPECT_FALSE(pool.internBitString("10?").ok());
  EXPECT_FALSE(pool.internBitString("10a").ok());
}

// A new constant in read-only mode is an invalid id (internBitString():
// FailedPrecondition), in every build mode; existing ones are found.
TEST(ConstPoolTest, ReadOnlyModeCannotAddConstants)
{
  ConstPool pool;
  pool.setMode(TableMode::kSingleWrite);
  const ConstId five = pool.internUint(8, 5);
  pool.setMode(TableMode::kReadOnly);
  EXPECT_EQ(pool.internUint(8, 5), five);
  EXPECT_EQ(*pool.internBitString("00000101"), five);
  EXPECT_FALSE(pool.internUint(8, 6).isValid());
  EXPECT_FALSE(pool.internFill(70, LogicValue::kOne).isValid());
  const absl::StatusOr<ConstId> fresh = pool.internBitString("0110");
  EXPECT_EQ(fresh.status().code(), absl::StatusCode::kFailedPrecondition);
  EXPECT_EQ(pool.size(), 1u);
}

TEST(ConstPoolTest, BitsAboveTheWidthAreIgnored)
{
  ConstPool pool;
  pool.setMode(TableMode::kSingleWrite);
  const std::array<uint64_t, 1> aval = {0xF5};
  const std::array<uint64_t, 1> bval = {0xF0};
  EXPECT_EQ(pool.intern(4, aval, bval), pool.internUint(4, 5));
  EXPECT_EQ(pool.internUint(4, 0xFF), *pool.internBitString("1111"));
}

TEST(ConstPoolTest, WideAndFilledConstants)
{
  ConstPool pool;
  pool.setMode(TableMode::kSingleWrite);
  const ConstView wide = pool.get(pool.internUint(100, 5));
  EXPECT_EQ(wide.numWords(), 2);
  EXPECT_EQ(wide.toUint64(), 5);
  EXPECT_TRUE(wide.isTwoState());

  const ConstId ones = pool.internFill(130, LogicValue::kOne);
  EXPECT_TRUE(pool.get(ones).isFill(LogicValue::kOne));
  EXPECT_FALSE(pool.get(ones).isFill(LogicValue::kZero));
  EXPECT_FALSE(pool.get(ones).toUint64().has_value());
  EXPECT_EQ(pool.internFill(130, LogicValue::kZero), pool.internUint(130, 0));
  EXPECT_EQ(pool.get(pool.internFill(130, LogicValue::kX)).bit(129),
            LogicValue::kX);

  std::string digits;
  for (int i = 0; i < 1000; ++i) {
    digits.push_back("01x"[i % 3]);
  }
  const ConstView pattern = pool.get(*pool.internBitString(digits));
  EXPECT_EQ(pattern.toString(), "1000'b" + digits);
}

TEST(ConstPoolTest, ViewsStayValidAsThePoolGrows)
{
  ConstPool pool;
  pool.setMode(TableMode::kSingleWrite);
  const ConstView small = pool.get(pool.internUint(16, 0xBEEF));
  // A constant larger than a chunk gets its own storage.
  const ConstView huge = pool.get(pool.internFill(40000, LogicValue::kX));
  for (uint64_t i = 0; i < 20000; ++i) {
    pool.internUint(64, i);
  }
  EXPECT_EQ(small.toUint64(), 0xBEEF);
  EXPECT_TRUE(huge.isFill(LogicValue::kX));
  EXPECT_EQ(pool.size(), 20002u);
  EXPECT_GT(pool.memoryBytes(), 20000 * 16);
}

TEST(ConstViewTest, SmallViewsHoldTheirValue)
{
  // 4'b10x1, with junk above the width.
  const ConstView original
      = ConstView::fromInline(4, 0xF0u | 0b1001u, 0xF0u | 0b0010u);
  ConstView copy;
  {
    const ConstView temp = original;
    copy = temp;  // must not point into `temp`
  }
  EXPECT_EQ(copy.toString(), "4'b10x1");
  EXPECT_EQ(copy.aval()[0], 0b1001u);
  EXPECT_EQ(copy.bval()[0], 0b0010u);
  EXPECT_FALSE(copy.isTwoState());
  // bval = 1 with aval = 1 is normalized to x (aval = 0, bval = 1).
  const ConstView norm = ConstView::fromInline(4, 0b1011, 0b0010);
  EXPECT_EQ(norm.toString(), "4'b10x1");
  EXPECT_EQ(norm.aval()[0], 0b1001u);
  EXPECT_EQ(ConstView::fromInline(32, 0xFFFFFFFF, 0).toUint64(),
            uint64_t{0xFFFFFFFF});
  EXPECT_TRUE(ConstView::fromInline(0, 1, 1).isTwoState());
  EXPECT_EQ(ConstView::fromInline(0, 1, 1).toString(), "0'b");
}

}  // namespace
}  // namespace syn::test
