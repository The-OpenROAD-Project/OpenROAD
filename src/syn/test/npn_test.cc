// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "flow/combinational_mapper_npn.h"
#include "gtest/gtest.h"

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

namespace syn {

TEST(NpnTest, ClassifyK4)
{
  int K = 4;

  std::set<Truth6> seen;
  int unique_classes = 0;
  for (Truth6 f = 0; f < (1 << (1 << K)); f++) {
    NPN map;
    Truth6 semiclass;
    semiclass = npnSemiclass(f, K, map);

    ASSERT_EQ(semiclass, map(f));
    NPN map_inv = map.inv();
    ASSERT_EQ(map_inv(semiclass), f);
    ASSERT_TRUE((map_inv * map).isIdentity());
    ASSERT_TRUE((map * map_inv).isIdentity());

    if (seen.contains(semiclass)) {
      continue;
    }

    unique_classes++;

    npnSemiclassAllRepr(f, K, [&](const Truth6 repr, const NPN& npn) {
      seen.insert(repr);
      (void) npn;
    });
  }

  // See https://oeis.org/A000370
  ASSERT_EQ(unique_classes, 222);
}

}  // namespace syn
