// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024, The OpenROAD Authors

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "utl/algorithms.h"

namespace utl {

TEST(Utl, SortAndUnique)
{
  std::vector<int> v{3, 1, 2, 1, 3, 2};

  sort_and_unique(v);

  const std::vector<int> expected{1, 2, 3};
  EXPECT_EQ(v, expected);
}

}  // namespace utl
