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

// The failure mode the `precision <= 0` guard covers.
//
// std::fixed emits no decimal point at a precision of 0, so the trailing-zero
// strip used to run across the integer part instead of a fractional one.  For a
// value that formats to all zeros it consumed the whole string, and the back()
// that follows then read an empty string: undefined behavior, an abort under a
// hardened libstdc++ (which is on by default at -O0, hence a CMake debug CI
// build), silently "" in an optimized one.  Reached in practice through a
// DBU-per-micron scale of 1, which makes web::dbuPrecision return 0.
TEST(Utl, ToNumericStringPrecisionZeroFormatsZeroAsZero)
{
  EXPECT_EQ(to_numeric_string(0.0, 0), "0");
  EXPECT_EQ(to_numeric_string(0.4, 0), "0");
  EXPECT_EQ(to_numeric_string(0.49999, 0), "0");
  // Negative values that round to zero keep the sign std::fixed gives them,
  // rather than being stripped down to a bare "-".
  EXPECT_EQ(to_numeric_string(-0.0, 0), "-0");
  EXPECT_EQ(to_numeric_string(-0.4, 0), "-0");
}

// An integer part ending in zeros must survive: there is no fractional part for
// the strip to work on, so it must not run at all.
TEST(Utl, ToNumericStringPrecisionZeroKeepsTheIntegerPart)
{
  EXPECT_EQ(to_numeric_string(10.0, 0), "10");
  EXPECT_EQ(to_numeric_string(100.0, 0), "100");
  EXPECT_EQ(to_numeric_string(1200.0, 0), "1200");
  EXPECT_EQ(to_numeric_string(-1200.0, 0), "-1200");
}

// Precision 0 rounds to the nearest whole number and emits no point.
TEST(Utl, ToNumericStringPrecisionZeroRoundsToWholeNumbers)
{
  EXPECT_EQ(to_numeric_string(7.0, 0), "7");
  EXPECT_EQ(to_numeric_string(123.0, 0), "123");
  EXPECT_EQ(to_numeric_string(-7.0, 0), "-7");
  EXPECT_EQ(to_numeric_string(0.6, 0), "1");
  EXPECT_EQ(to_numeric_string(3.7, 0), "4");
}

// A negative precision cannot reach the strip either; std::setprecision treats
// it as 6, so the output keeps whatever std::fixed produced.
TEST(Utl, ToNumericStringNegativePrecisionDoesNotStrip)
{
  EXPECT_EQ(to_numeric_string(1200.0, -1), "1200.000000");
  EXPECT_EQ(to_numeric_string(0.0, -1), "0.000000");
}

// The guard must not disturb any precision the callers actually use.  Integer
// parts ending in zeros are safe here precisely because the '.' stops the
// strip.
TEST(Utl, ToNumericStringDropsTrailingFractionalZeros)
{
  EXPECT_EQ(to_numeric_string(10.20, 2), "10.2");
  EXPECT_EQ(to_numeric_string(11.00, 2), "11");
  EXPECT_EQ(to_numeric_string(0.0, 3), "0");
  EXPECT_EQ(to_numeric_string(-0.0001, 3), "-0");
  EXPECT_EQ(to_numeric_string(0.001, 3), "0.001");
  EXPECT_EQ(to_numeric_string(974.4, 3), "974.4");
  EXPECT_EQ(to_numeric_string(-5.76, 3), "-5.76");
  EXPECT_EQ(to_numeric_string(100.0, 3), "100");
  EXPECT_EQ(to_numeric_string(1200.0, 1), "1200");
  EXPECT_EQ(to_numeric_string(0.0001, 4), "0.0001");
  EXPECT_EQ(to_numeric_string(0.0005, 4), "0.0005");
}

}  // namespace utl
