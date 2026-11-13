// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "db/drObj/drShape.h"
#include "db/obj/frShape.h"
#include "gtest/gtest.h"
#include "odb/geom.h"

namespace drt {
namespace {

class PathSegCopyTest : public testing::TestWithParam<bool>
{
};

TEST_P(PathSegCopyTest, PreservesAccessPoint)
{
  frPathSeg source;
  source.setPoints({900, 1000}, {1100, 1000});
  if (GetParam()) {
    source.setApPathSeg({900, 1000});
  }

  const frPathSeg copied(source);
  const drPathSeg converted(source);
  const frPathSeg round_trip(converted);

  EXPECT_EQ(copied.isApPathSeg(), GetParam());
  EXPECT_EQ(converted.isApPathSeg(), GetParam());
  EXPECT_EQ(round_trip.isApPathSeg(), GetParam());

  if (GetParam()) {
    EXPECT_EQ(copied.getApLoc(), odb::Point(900, 1000));
    EXPECT_EQ(converted.getApLoc(), odb::Point(900, 1000));
    EXPECT_EQ(round_trip.getApLoc(), odb::Point(900, 1000));
  }
}

INSTANTIATE_TEST_SUITE_P(PathSeg, PathSegCopyTest, testing::Bool());

}  // namespace
}  // namespace drt
