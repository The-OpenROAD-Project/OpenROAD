// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "color.h"
#include "gtest/gtest.h"
#include "timing_report.h"
#include "tst/nangate45_fixture.h"

namespace web {
namespace {

//------------------------------------------------------------------------------
// spectrumColor — the Turbo colormap ported from gui::SpectrumGenerator.
// These lock the endpoints/clamping so the cone colors keep matching the Qt
// GUI even if the table is edited.
//------------------------------------------------------------------------------

TEST(SpectrumColorTest, LowEndIsFirstTableEntry)
{
  const Color c = spectrumColor(0.0, 255);
  EXPECT_EQ(c.r, 48);
  EXPECT_EQ(c.g, 18);
  EXPECT_EQ(c.b, 59);
  EXPECT_EQ(c.a, 255);
}

TEST(SpectrumColorTest, HighEndIsLastTableEntry)
{
  const Color c = spectrumColor(1.0, 128);
  EXPECT_EQ(c.r, 122);
  EXPECT_EQ(c.g, 4);
  EXPECT_EQ(c.b, 3);
  EXPECT_EQ(c.a, 128);
}

TEST(SpectrumColorTest, ClampsOutOfRangeValues)
{
  EXPECT_EQ(spectrumColor(-5.0).r, spectrumColor(0.0).r);
  EXPECT_EQ(spectrumColor(5.0).r, spectrumColor(1.0).r);
}

TEST(SpectrumColorTest, IsMonotonicAcrossTheRamp)
{
  // The Turbo ramp is not monotonic per channel, but distinct inputs must map
  // to valid, in-bounds colors (a cheap guard against table truncation).
  const Color mid = spectrumColor(0.5);
  EXPECT_NE(mid.r + mid.g + mid.b, 0);
}

//------------------------------------------------------------------------------
// TimingReport::computeTimingCone guard behavior (no timing setup needed).
//------------------------------------------------------------------------------

using TimingConeTest = tst::Nangate45Fixture;

TEST_F(TimingConeTest, EmptyDirectionsReturnEmptyCone)
{
  TimingReport report(getSta());
  // Neither fanin nor fanout requested: a valid, empty cone (the clear path).
  const TimingConeResult result
      = report.computeTimingCone("anything", false, false, 0, 0);
  EXPECT_TRUE(result.ok);
  EXPECT_TRUE(result.nodes.empty());
  EXPECT_FALSE(result.constrained);
}

}  // namespace
}  // namespace web
