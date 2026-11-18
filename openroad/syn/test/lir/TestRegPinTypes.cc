// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <set>
#include <string>

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
    const char* name = regPinTypeName(static_cast<RegPinType>(i));
    ASSERT_NE(name, nullptr) << "pin type " << i;
    EXPECT_STRNE(name, "?") << "pin type " << i;
    names.insert(name);
  }
  EXPECT_EQ(names.size(), kNumRegPinTypes);
  EXPECT_STREQ(regPinTypeName(RegPinType::kNumTypes), "?");
}

TEST(RegPinTypeTest, MaskBits)
{
  EXPECT_EQ(regPinBit(RegPinType::kData), 1);
  EXPECT_EQ(regPinBit(RegPinType::kClock), 2);
}

}  // namespace
}  // namespace syn::test
