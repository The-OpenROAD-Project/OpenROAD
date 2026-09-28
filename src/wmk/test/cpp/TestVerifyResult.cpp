// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#include <cmath>
#include <stdexcept>

#include "gtest/gtest.h"
#include "wmk/VerifyResult.h"

namespace wmk {
namespace {

TEST(VerifyResult, EmptyAndFailingClaimsProvideNoEvidence)
{
  EXPECT_EQ(VerifyResult{}.pValue(), 1.0);
  EXPECT_EQ((VerifyResult{.checked = 100, .held = 0}).pValue(), 1.0);
}

TEST(VerifyResult, CountsDistinguishIdenticalExtractionRates)
{
  const VerifyResult one{.checked = 1, .held = 1};
  const VerifyResult thirteen{.checked = 13, .held = 13};
  const VerifyResult fourteen{.checked = 14, .held = 14};
  EXPECT_EQ(one.rate(), fourteen.rate());
  EXPECT_DOUBLE_EQ(one.pValue(), 0.5);
  EXPECT_GT(thirteen.pValue(), 1e-4);
  EXPECT_LE(fourteen.pValue(), 1e-4);
  EXPECT_DOUBLE_EQ(fourteen.pValue(), std::ldexp(1.0, -14));
}

TEST(VerifyResult, IncludesTiesInUpperTail)
{
  // P[X >= 3] = (4 + 1)/16, including equality at the observed count.
  EXPECT_DOUBLE_EQ((VerifyResult{.checked = 4, .held = 3}).pValue(), 5.0 / 16);
  EXPECT_DOUBLE_EQ((VerifyResult{.checked = 5, .held = 4}).pValue(), 6.0 / 32);
  EXPECT_DOUBLE_EQ((VerifyResult{.checked = 4, .held = 1}).pValue(), 15.0 / 16);
}

TEST(VerifyResult, LargeSamplesRetainSmallUpperTails)
{
  EXPECT_NEAR((VerifyResult{.checked = 64, .held = 48}).pValue(),
              3.866538440643513e-5,
              1e-17);
  const double p = (VerifyResult{.checked = 1000, .held = 1000}).pValue();
  EXPECT_GT(p, 0.0);
  EXPECT_NEAR(p / std::ldexp(1.0, -1000), 1.0, 1e-12);
  EXPECT_GE((VerifyResult{.checked = 100000, .held = 50000}).pValue(), 0.5);
}

TEST(VerifyResult, InvalidCountsAreErrors)
{
  for (const VerifyResult result : {VerifyResult{.checked = -1, .held = 0},
                                    VerifyResult{.checked = 1, .held = -1},
                                    VerifyResult{.checked = 1, .held = 2}}) {
    EXPECT_THROW(result.pValue(), std::invalid_argument);
  }
}

}  // namespace
}  // namespace wmk
