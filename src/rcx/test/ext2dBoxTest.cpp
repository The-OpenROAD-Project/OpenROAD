// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "gtest/gtest.h"
#include "rcx/ext2dBox.h"

namespace rcx {

TEST(rcx_suite, simple_instantiate_accessors)
{
  ext2dBox box({0, 1}, {2, 4});

  EXPECT_EQ(box.ll0(), 0);
  EXPECT_EQ(box.ll1(), 1);
  EXPECT_EQ(box.ur0(), 2);
  EXPECT_EQ(box.ur1(), 4);
}

}  // namespace rcx
