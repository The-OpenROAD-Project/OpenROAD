// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <iterator>
#include <set>
#include <string>
#include <string_view>

#include "gtest/gtest.h"
#include "syn/lir/RegPinTypes.h"

namespace syn::test {
namespace {

using ::syn::lir::kNumRegPinTypes;
using ::syn::lir::regPinBit;
using ::syn::lir::RegPinType;
using ::syn::lir::regPinTypeName;

TEST(RegPinTypeTest, EveryPinTypeHasAUniqueName)
{
  std::set<std::string> names;
  for (int i = 0; i < kNumRegPinTypes; ++i) {
    const std::string_view name = regPinTypeName(static_cast<RegPinType>(i));
    ASSERT_FALSE(name.empty()) << "pin type " << i;
    EXPECT_NE(name, "?") << "pin type " << i;
    names.insert(std::string(name));
  }
  EXPECT_EQ(std::ssize(names), kNumRegPinTypes);
  EXPECT_EQ(regPinTypeName(RegPinType::kNumTypes), "?");
}

TEST(RegPinTypeTest, MaskBits)
{
  EXPECT_EQ(regPinBit(RegPinType::kData), 1);
  EXPECT_EQ(regPinBit(RegPinType::kClock), 2);
}

}  // namespace
}  // namespace syn::test
