// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>
#include <vector>

#include "gtest/gtest.h"
#include "utl/random_bytes.h"

namespace utl {

namespace {

TEST(RandomBytes, DrawsTheRequestedCount)
{
  std::vector<std::uint8_t> first;
  ASSERT_TRUE(randomBytes(32, first));
  EXPECT_EQ(first.size(), 32u);
  // Not proof of entropy, but two 256-bit draws must not repeat.
  std::vector<std::uint8_t> second;
  ASSERT_TRUE(randomBytes(32, second));
  EXPECT_NE(first, second);
}

TEST(RandomBytes, ZeroBytesIsEmpty)
{
  std::vector<std::uint8_t> out{1, 2, 3};
  EXPECT_TRUE(randomBytes(0, out));
  EXPECT_TRUE(out.empty());
}

TEST(ToHex, LowercaseTwoDigitsPerByte)
{
  const std::uint8_t bytes[] = {0x00, 0x0f, 0xa5, 0xff};
  EXPECT_EQ(toHex(bytes, sizeof(bytes)), "000fa5ff");
  EXPECT_EQ(toHex(bytes, 0), "");
}

}  // namespace

}  // namespace utl
