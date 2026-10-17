// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The antenna rule helpers: PWL evaluation and per-layer factor setup.

#include <vector>

#include "AntennaRules.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "tst/db_fixture.h"
#include "tst/logger_checks.h"
#include "tst/physical_builder.h"

namespace ant::test {
namespace {

double pwl(const std::vector<double>& indices,
           const std::vector<double>& ratios,
           double ref_value,
           double default_value = -1)
{
  return getPwlFactor(
      {.indices = indices, .ratios = ratios}, ref_value, default_value);
}

TEST(PwlFactorTest, EmptyTableUsesDefault)
{
  EXPECT_DOUBLE_EQ(pwl({}, {}, 0.5, 7), 7);
}

TEST(PwlFactorTest, SinglePointIsConstant)
{
  EXPECT_DOUBLE_EQ(pwl({0}, {3}, 0), 3);
  EXPECT_DOUBLE_EQ(pwl({0}, {3}, 100), 3);
}

TEST(PwlFactorTest, InterpolatesWithinSegments)
{
  const std::vector<double> indices{0, 1, 2};
  const std::vector<double> ratios{10, 20, 40};
  EXPECT_DOUBLE_EQ(pwl(indices, ratios, 0), 10);
  EXPECT_DOUBLE_EQ(pwl(indices, ratios, 0.5), 15);
  EXPECT_DOUBLE_EQ(pwl(indices, ratios, 1), 20);
  EXPECT_DOUBLE_EQ(pwl(indices, ratios, 1.5), 30);
  EXPECT_DOUBLE_EQ(pwl(indices, ratios, 2), 40);
}

TEST(PwlFactorTest, ExtrapolatesPastLastPoint)
{
  EXPECT_DOUBLE_EQ(pwl({0, 1, 2}, {10, 20, 40}, 3), 60);
}

// A reference below the first index extrapolates from the last segment.
// Diffusion areas are never negative and LEF tables start at 0, so this
// does not arise in practice; the test records the behavior.
TEST(PwlFactorTest, BelowFirstIndexUsesLastSegment)
{
  EXPECT_DOUBLE_EQ(pwl({1, 2}, {10, 20}, 0), 0);
}

// The step table ant_check.lef uses for ANTENNAAREADIFFREDUCEPWL.
TEST(PwlFactorTest, StepTable)
{
  const std::vector<double> indices{0, 0.0999, 0.1, 1000};
  const std::vector<double> ratios{1, 1, 0.2, 0.2};
  EXPECT_DOUBLE_EQ(pwl(indices, ratios, 0.05), 1);
  EXPECT_DOUBLE_EQ(pwl(indices, ratios, 0.1), 0.2);
  EXPECT_DOUBLE_EQ(pwl(indices, ratios, 0.5413), 0.2);
}

class AntennaModelTest : public tst::DbFixture
{
 protected:
  void SetUp() override { tech_ = tst::makeTech(getDb()); }

  odb::dbTech* tech_{nullptr};
};

TEST_F(AntennaModelTest, NoRuleUsesDefaults)
{
  odb::dbTechLayer* m1 = tst::addRoutingLayer(tech_, "M1");
  const AntennaModel model = makeAntennaModel(m1, getLogger());
  EXPECT_EQ(model.layer, m1);
  EXPECT_DOUBLE_EQ(model.metal_factor, 1);
  EXPECT_DOUBLE_EQ(model.diff_metal_factor, 1);
  EXPECT_DOUBLE_EQ(model.cut_factor, 1);
  EXPECT_DOUBLE_EQ(model.diff_cut_factor, 1);
  EXPECT_DOUBLE_EQ(model.side_metal_factor, 1);
  EXPECT_DOUBLE_EQ(model.diff_side_metal_factor, 1);
  EXPECT_DOUBLE_EQ(model.minus_diff_factor, 0);
  EXPECT_DOUBLE_EQ(model.plus_diff_factor, 0);
  EXPECT_TRUE(model.gate_plus_diff_idx.empty());
}

TEST_F(AntennaModelTest, AreaFactorAppliesToAll)
{
  odb::dbTechLayer* m1 = tst::addRoutingLayer(tech_, "M1");
  tst::applyAntennaRule(m1, {.area_factor = 3, .side_area_factor = 2});
  const AntennaModel model = makeAntennaModel(m1, getLogger());
  EXPECT_DOUBLE_EQ(model.metal_factor, 3);
  EXPECT_DOUBLE_EQ(model.diff_metal_factor, 3);
  EXPECT_DOUBLE_EQ(model.cut_factor, 3);
  EXPECT_DOUBLE_EQ(model.diff_cut_factor, 3);
  EXPECT_DOUBLE_EQ(model.side_metal_factor, 2);
  EXPECT_DOUBLE_EQ(model.diff_side_metal_factor, 2);
}

TEST_F(AntennaModelTest, DiffuseOnlyFactorsApplyToDiffusionOnly)
{
  odb::dbTechLayer* m1 = tst::addRoutingLayer(tech_, "M1");
  tst::applyAntennaRule(m1,
                        {.area_factor = 3,
                         .area_factor_diffuse_only = true,
                         .side_area_factor = 2,
                         .side_area_factor_diffuse_only = true});
  const AntennaModel model = makeAntennaModel(m1, getLogger());
  EXPECT_DOUBLE_EQ(model.metal_factor, 1);
  EXPECT_DOUBLE_EQ(model.diff_metal_factor, 3);
  EXPECT_DOUBLE_EQ(model.cut_factor, 1);
  EXPECT_DOUBLE_EQ(model.diff_cut_factor, 3);
  EXPECT_DOUBLE_EQ(model.side_metal_factor, 1);
  EXPECT_DOUBLE_EQ(model.diff_side_metal_factor, 2);
}

TEST_F(AntennaModelTest, DiffusionTerms)
{
  odb::dbTechLayer* m1 = tst::addRoutingLayer(tech_, "M1");
  tst::applyAntennaRule(m1,
                        {.area_minus_diff = 0.5,
                         .gate_plus_diff = 0.3,
                         .gate_plus_diff_pwl = tst::Pwl{{0, 1}, {2, 4}}});
  const AntennaModel model = makeAntennaModel(m1, getLogger());
  EXPECT_DOUBLE_EQ(model.minus_diff_factor, 0.5);
  EXPECT_DOUBLE_EQ(model.plus_diff_factor, 0.3);
  EXPECT_EQ(model.gate_plus_diff_idx, (std::vector<double>{0, 1}));
  EXPECT_EQ(model.gate_plus_diff_ratios, (std::vector<double>{2, 4}));
}

// Side area is perimeter times thickness, so a side area rule needs one.
TEST_F(AntennaModelTest, SideRuleWithoutThicknessIsAnError)
{
  odb::dbTechLayer* m1 = tst::addRoutingLayer(tech_, "M1");
  odb::dbTechLayer* m2 = tst::addRoutingLayer(tech_, "M2");
  tst::applyAntennaRule(m1, {.psr = 100});
  tst::applyAntennaRule(m2, {.diff_psr = tst::constantPwl(100)});

  EXPECT_LOGGER_ERROR(makeAntennaModel(m1, getLogger()), "ANT-0013");
  EXPECT_LOGGER_ERROR(makeAntennaModel(m2, getLogger()), "ANT-0013");
}

TEST_F(AntennaModelTest, SideRuleWithThickness)
{
  odb::dbTechLayer* m1 = tst::addRoutingLayer(tech_, "M1", {.thickness = 1000});
  odb::dbTechLayer* v1 = tst::addCutLayer(tech_, "V1");
  tst::applyAntennaRule(m1, {.psr = 100});
  tst::applyAntennaRule(v1, {.diff_psr = tst::constantPwl(100)});

  EXPECT_NO_THROW(makeAntennaModel(m1, getLogger()));
  EXPECT_NO_THROW(makeAntennaModel(v1, getLogger()));  // cut: no side area
}

}  // namespace
}  // namespace ant::test
