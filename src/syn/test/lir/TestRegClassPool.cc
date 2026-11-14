// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <array>
#include <cstdint>

#include "gtest/gtest.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/ConstView.h"
#include "syn/lir/Ids.h"
#include "syn/lir/RegClassPool.h"
#include "syn/lir/RegPinTypes.h"

namespace syn::test {
namespace {

using ::syn::lir::ConstId;
using ::syn::lir::ConstPool;
using ::syn::lir::ConstView;
using ::syn::lir::LibCellId;
using ::syn::lir::LogicValue;
using ::syn::lir::RegClass;
using ::syn::lir::RegClassId;
using ::syn::lir::RegClassPool;
using ::syn::lir::RegFlags;
using ::syn::lir::RegInit;
using ::syn::lir::regPinBit;
using ::syn::lir::RegPinMask;
using ::syn::lir::RegPinType;
using ::syn::lir::TableMode;

RegClass asyncResetFlop(RegInit init = {})
{
  return RegClass::generic(
      RegFlags(),
      static_cast<RegPinMask>(uint32_t{regPinBit(RegPinType::kData)}
                              | regPinBit(RegPinType::kClock)
                              | regPinBit(RegPinType::kAsyncReset)),
      init);
}

TEST(RegClassTest, PinIndexFollowsPinTypeOrder)
{
  const RegClass flop = asyncResetFlop();
  EXPECT_TRUE(flop.isGeneric());
  EXPECT_EQ(flop.numPins(), 3);
  EXPECT_EQ(flop.pinIndex(RegPinType::kData), 0);
  EXPECT_EQ(flop.pinIndex(RegPinType::kClock), 1);
  EXPECT_EQ(flop.pinIndex(RegPinType::kAsyncReset), 2);
  EXPECT_EQ(flop.pinIndex(RegPinType::kEnable), -1);
  EXPECT_FALSE(flop.has(RegPinType::kSyncReset));
  EXPECT_TRUE(flop.init().isNone());
}

TEST(RegClassTest, MappedClassesHoldTheCell)
{
  const RegClass mapped = RegClass::mapped(LibCellId(7));
  EXPECT_TRUE(mapped.isMapped());
  EXPECT_EQ(mapped.cell(), LibCellId(7));
  EXPECT_NE(mapped, RegClass::mapped(LibCellId(8)));
}

TEST(RegClassTest, SameKindIgnoresTheInitialState)
{
  const RegClass a = asyncResetFlop();
  const RegClass b = asyncResetFlop(RegInit::inlineValue(0b01, 0b11));
  EXPECT_NE(a, b);
  EXPECT_TRUE(a.sameKindIgnoringInit(b));
  EXPECT_EQ(a.withInit(b.init()), b);
  EXPECT_FALSE(a.sameKindIgnoringInit(
      a.withPins(a.pins() | regPinBit(RegPinType::kEnable))));
}

TEST(RegInitTest, InlineAndPooledBits)
{
  ConstPool consts;
  consts.setMode(TableMode::kSingleWrite);
  const RegInit narrow = RegInit::inlineValue(0b010, 0b011);  // x10
  EXPECT_FALSE(narrow.isNone());
  EXPECT_EQ(narrow.bit(0, consts), LogicValue::kZero);
  EXPECT_EQ(narrow.bit(1, consts), LogicValue::kOne);
  EXPECT_EQ(narrow.bit(2, consts), LogicValue::kX);

  // From a 3-state constant: narrow values stay inline.
  const ConstView v4 = ConstView::fromInline(4, 0b0010, 0b1100);  // x x 1 0
  const RegInit from4 = RegInit::fromConst(v4, consts);
  EXPECT_FALSE(from4.isPooled());
  EXPECT_EQ(from4, RegInit::inlineValue(0b10, 0b11));

  // Wide values go to the pool.
  const ConstId wide = consts.internUint(40, uint64_t{1} << 39u);
  const RegInit pooled = RegInit::fromConst(consts.get(wide), consts);
  EXPECT_TRUE(pooled.isPooled());
  EXPECT_EQ(pooled.constId(), wide);
  EXPECT_EQ(pooled.bit(39, consts), LogicValue::kOne);
  EXPECT_EQ(pooled.bit(0, consts), LogicValue::kZero);

  // All x is no initial state.
  const ConstId all_x = consts.internFill(40, LogicValue::kX);
  EXPECT_TRUE(RegInit::fromConst(consts.get(all_x), consts).isNone());
}

// A new wide initial state in a read-only pool is an invalid RegInit, and
// RegClassPool::intern() refuses a class holding it.
TEST(RegInitTest, NewWideValueInAReadOnlyPoolIsInvalid)
{
  ConstPool consts;
  consts.setMode(TableMode::kSingleWrite);
  const ConstId wide = consts.internUint(40, 1);
  consts.setMode(TableMode::kReadOnly);
  EXPECT_TRUE(RegInit::fromConst(consts.get(wide), consts).isValid());

  const std::array<uint64_t, 2> words = {2, 0};  // bits, x_mask
  const RegInit init = RegInit::fromConst(ConstView(40, words.data()), consts);
  EXPECT_FALSE(init.isValid());
  EXPECT_EQ(consts.size(), 1u);
  RegClassPool pool;
  EXPECT_FALSE(pool.intern(asyncResetFlop(init)).isValid());
  EXPECT_TRUE(pool.empty());
}

TEST(RegClassPoolTest, EqualClassesShareAnId)
{
  RegClassPool pool;
  EXPECT_TRUE(pool.empty());
  const RegClassId flop = pool.intern(asyncResetFlop());
  EXPECT_EQ(pool.intern(asyncResetFlop()), flop);
  EXPECT_EQ(pool.find(asyncResetFlop()), flop);

  const RegClassId with_init
      = pool.intern(asyncResetFlop(RegInit::inlineValue(1, 1)));
  EXPECT_NE(with_init, flop);
  const RegClassId bound = pool.intern(RegClass::mapped(LibCellId(7)));
  EXPECT_NE(bound, flop);
  EXPECT_EQ(pool.get(bound).cell(), LibCellId(7));
  EXPECT_EQ(pool.size(), 3u);
  EXPECT_FALSE(pool.find(RegClass::mapped(LibCellId(8))).isValid());

  // Pools are values: a copy keeps the ids (copyModule, rebuilds).
  const RegClassPool copy = pool;
  EXPECT_EQ(copy.get(flop), asyncResetFlop());
  EXPECT_EQ(copy.find(RegClass::mapped(LibCellId(7))), bound);
}

}  // namespace
}  // namespace syn::test
