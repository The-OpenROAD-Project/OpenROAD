// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "db/tech/frLookupTbl.h"
#include "frBaseTypes.h"
#include "gtest/gtest.h"

namespace drt {
namespace {

TEST(LookupTblTest, OneDLowerBoundTableClampsBelowAtAndAboveBounds)
{
  const fr1DLookupTbl<frCoord, frCoord> tbl(
      "WIDTH", {100, 200, 400}, {10, 20, 40}, /*lowerBoundIn=*/true);

  EXPECT_EQ(tbl.find(50), 10);
  EXPECT_EQ(tbl.find(100), 10);
  EXPECT_EQ(tbl.find(150), 10);
  EXPECT_EQ(tbl.find(200), 10);
  EXPECT_EQ(tbl.find(201), 20);
  EXPECT_EQ(tbl.find(400), 20);
  EXPECT_EQ(tbl.find(500), 40);
}

TEST(LookupTblTest, OneDUpperBoundTableClampsAtLastRowWithoutOverflow)
{
  const fr1DLookupTbl<frCoord, frCoord> tbl(
      "WIDTH", {100, 200, 400}, {10, 20, 40}, /*lowerBoundIn=*/false);

  EXPECT_EQ(tbl.find(50), 10);
  EXPECT_EQ(tbl.find(100), 20);
  EXPECT_EQ(tbl.find(150), 20);
  EXPECT_EQ(tbl.find(200), 40);
  EXPECT_EQ(tbl.find(350), 40);
  EXPECT_EQ(tbl.find(400), 40);
  EXPECT_EQ(tbl.find(500), 40);

  const fr1DLookupTbl<frCoord, frCoord> single_entry_tbl(
      "WIDTH", {101}, {50}, /*lowerBoundIn=*/false);
  EXPECT_EQ(single_entry_tbl.find(80), 50);
  EXPECT_EQ(single_entry_tbl.find(101), 50);
  EXPECT_EQ(single_entry_tbl.find(120), 50);
}

TEST(LookupTblTest, TwoDTableHandlesValuesBelowAndAtFirstRowAndCol)
{
  const fr2DLookupTbl<frCoord, frCoord, frCoord> tbl(
      "WIDTH",
      {100, 200, 400},
      "PARALLELRUNLENGTH",
      {0, 300, 600},
      {{11, 12, 13}, {21, 22, 23}, {31, 32, 33}});

  // Below and equal to rows_.front() / cols_.front() (lower_bound == begin()).
  EXPECT_EQ(tbl.find(50, -10), 11);
  EXPECT_EQ(tbl.find(100, 0), 11);
  EXPECT_EQ(tbl.find(100, 300), 11);
  EXPECT_EQ(tbl.find(200, 0), 11);

  // Strictly above thresholds steps to subsequent buckets.
  EXPECT_EQ(tbl.find(201, 0), 21);
  EXPECT_EQ(tbl.find(201, 301), 22);
  EXPECT_EQ(tbl.find(400, 600), 22);
  EXPECT_EQ(tbl.find(401, 601), 33);
}

}  // namespace
}  // namespace drt
