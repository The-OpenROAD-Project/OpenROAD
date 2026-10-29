// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <unordered_set>

#include "absl/container/flat_hash_set.h"
#include "gtest/gtest.h"
#include "syn/lir/Ids.h"

namespace syn::test {
namespace {

using ::syn::lir::ConstId;
using ::syn::lir::kObjIdBits;
using ::syn::lir::LibCellId;
using ::syn::lir::ModId;
using ::syn::lir::NameId;
using ::syn::lir::ObjId;
using ::syn::lir::ObjIndex;
using ::syn::lir::OpSpecId;
using ::syn::lir::RegClassId;

template <typename T>
concept HasValue = requires(T id) { id.value(); };
template <typename T>
concept HasRawIndex = requires(T id) { id.rawIndex(); };

TEST(IdTest, DefaultIsInvalid)
{
  ObjId id;
  EXPECT_FALSE(id.isValid());
  EXPECT_EQ(id, ObjId::invalid());
  EXPECT_TRUE(ObjId(0).isValid());
}

TEST(IdTest, OrderingAndEquality)
{
  EXPECT_LT(ObjId(1), ObjId(2));
  EXPECT_EQ(ObjId(7), ObjId(7));
  EXPECT_NE(ObjId(7), ObjId(8));
  EXPECT_LT(ObjId(0), ObjId::invalid());
  EXPECT_LT(ModId(1), ModId(2));

  EXPECT_TRUE(std::equality_comparable<ObjId>);
  EXPECT_TRUE(std::equality_comparable<ModId>);
  EXPECT_TRUE(std::equality_comparable<NameId>);
  EXPECT_TRUE(std::equality_comparable<ConstId>);
  EXPECT_TRUE(std::equality_comparable<RegClassId>);
  EXPECT_TRUE(std::equality_comparable<LibCellId>);
  EXPECT_TRUE(std::equality_comparable<OpSpecId>);

  // Pool handles have no relational ordering (<, <=, >, >=) so code cannot
  // accidentally sort or order by pool insertion order.
  EXPECT_TRUE(std::three_way_comparable<ObjId>);
  EXPECT_TRUE(std::three_way_comparable<ModId>);
  EXPECT_FALSE(std::three_way_comparable<NameId>);
  EXPECT_FALSE(std::three_way_comparable<ConstId>);
  EXPECT_FALSE(std::three_way_comparable<RegClassId>);
  EXPECT_FALSE(std::three_way_comparable<LibCellId>);
  EXPECT_FALSE(std::three_way_comparable<OpSpecId>);

  // Nor a value(): only an explicit rawIndex() for bounds checks and
  // payload fields, so the raw number cannot leak into results by accident.
  EXPECT_TRUE(HasValue<ObjId>);
  EXPECT_TRUE(HasValue<ModId>);
  EXPECT_FALSE(HasValue<NameId>);
  EXPECT_FALSE(HasValue<ConstId>);
  EXPECT_FALSE(HasValue<RegClassId>);
  EXPECT_FALSE(HasValue<LibCellId>);
  EXPECT_FALSE(HasValue<OpSpecId>);
  EXPECT_TRUE(HasRawIndex<NameId>);
  EXPECT_FALSE(HasRawIndex<ObjId>);
}

TEST(IdTest, Hashable)
{
  absl::flat_hash_set<ModId> absl_set = {ModId(1), ModId(2), ModId(1)};
  EXPECT_EQ(absl_set.size(), 2u);
  std::unordered_set<NameId> std_set = {NameId(3), NameId(3)};
  EXPECT_EQ(std_set.size(), 1u);
}

TEST(IdTest, ObjectIdWidth)
{
  // 32 by default; 64 with --define=lir_obj_id_bits=64.
  EXPECT_EQ(sizeof(ObjIndex) * 8, static_cast<size_t>(kObjIdBits));
  EXPECT_EQ(sizeof(ObjId), sizeof(ObjIndex));
  EXPECT_EQ(ObjId::kInvalidValue, std::numeric_limits<ObjIndex>::max());
}

TEST(IdTest, OtherHandlesStayFourBytes)
{
  EXPECT_EQ(sizeof(ModId), 4u);
  EXPECT_EQ(sizeof(NameId), 4u);
  EXPECT_EQ(sizeof(ConstId), 4u);
  EXPECT_EQ(sizeof(RegClassId), 4u);
  EXPECT_EQ(sizeof(LibCellId), 4u);
  EXPECT_EQ(sizeof(OpSpecId), 4u);
}

TEST(IdTest, ObjIdsAboveFourBillion)
{
  if constexpr (kObjIdBits == 64) {
    const uint64_t big = uint64_t{1} << 33;
    const ObjId id(big + 1);
    EXPECT_TRUE(id.isValid());
    EXPECT_EQ(id.value(), big + 1);
    EXPECT_LT(ObjId(big), id);
    EXPECT_NE(ObjId(1), id);  // no truncation to 32 bits
    absl::flat_hash_set<ObjId> set = {id, ObjId(1), ObjId(big + 1)};
    EXPECT_EQ(set.size(), 2u);
  } else {
    GTEST_SKIP() << "32-bit object ids";
  }
}

}  // namespace
}  // namespace syn::test
