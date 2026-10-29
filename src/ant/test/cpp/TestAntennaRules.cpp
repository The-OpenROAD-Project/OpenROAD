// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The antenna rule helpers: PWL evaluation and per-layer factor setup.

#include <vector>

#include "AntennaRules.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
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

// LEF gives pin areas per layer ("ANTENNAGATEAREA 0.5 LAYER M1").
class PinValueTest : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    tech_ = tst::makeTech(getDb());
    stack_ = tst::makeLayerStack(tech_, 3);
  }

  odb::dbTech* tech_{nullptr};
  tst::LayerStack stack_;
};

TEST_F(PinValueTest, NoEntriesIsZero)
{
  EXPECT_DOUBLE_EQ(valueAtOrBelow({}, stack_.metal(1)), 0);
  EXPECT_DOUBLE_EQ(valueOnLayer({}, stack_.metal(1)), 0);
}

TEST_F(PinValueTest, EntryWithoutLayerAppliesEverywhere)
{
  const LayerValues entries{{0.5, nullptr}};
  for (odb::dbTechLayer* layer : {stack_.metal(1), stack_.cut(1)}) {
    EXPECT_DOUBLE_EQ(valueAtOrBelow(entries, layer), 0.5);
    EXPECT_DOUBLE_EQ(valueOnLayer(entries, layer), 0.5);
  }
}

// The nearest entry at or below a layer holds on it, cut layers included.
TEST_F(PinValueTest, AtOrBelowUsesNearestLowerEntry)
{
  const LayerValues entries{{2.0, stack_.metal(2)}, {1.0, stack_.metal(1)}};
  EXPECT_DOUBLE_EQ(valueAtOrBelow(entries, stack_.metal(1)), 1.0);
  EXPECT_DOUBLE_EQ(valueAtOrBelow(entries, stack_.cut(1)), 1.0);
  EXPECT_DOUBLE_EQ(valueAtOrBelow(entries, stack_.metal(2)), 2.0);
  EXPECT_DOUBLE_EQ(valueAtOrBelow(entries, stack_.metal(3)), 2.0);
}

TEST_F(PinValueTest, AtOrBelowPrefersLayerEntryOverUnlayered)
{
  const LayerValues entries{{2.0, stack_.metal(2)}, {0.5, nullptr}};
  EXPECT_DOUBLE_EQ(valueAtOrBelow(entries, stack_.metal(1)), 0.5);
  EXPECT_DOUBLE_EQ(valueAtOrBelow(entries, stack_.metal(3)), 2.0);
}

// A device a block reaches only through upper metal is not connected while
// the layers below are built.
TEST_F(PinValueTest, BelowLowestEntryIsZero)
{
  const LayerValues entries{{3.0, stack_.metal(3)}, {2.0, stack_.metal(2)}};
  EXPECT_DOUBLE_EQ(valueAtOrBelow(entries, stack_.metal(1)), 0);
  EXPECT_DOUBLE_EQ(valueAtOrBelow(entries, stack_.cut(1)), 0);
}

// LEF uses the last of repeated statements.
TEST_F(PinValueTest, LastRepeatedEntryWins)
{
  const LayerValues entries{{1.0, stack_.metal(1)}, {4.0, stack_.metal(1)}};
  EXPECT_DOUBLE_EQ(valueAtOrBelow(entries, stack_.metal(2)), 4.0);
  EXPECT_DOUBLE_EQ(valueOnLayer(entries, stack_.metal(1)), 4.0);
}

// Partial areas and MAX*CAR describe one layer, so they don't carry up.
TEST_F(PinValueTest, OnLayerMatchesOnlyThatLayer)
{
  const LayerValues entries{{1.5, stack_.metal(1)}, {0.1, stack_.cut(1)}};
  EXPECT_DOUBLE_EQ(valueOnLayer(entries, stack_.metal(1)), 1.5);
  EXPECT_DOUBLE_EQ(valueOnLayer(entries, stack_.cut(1)), 0.1);
  EXPECT_DOUBLE_EQ(valueOnLayer(entries, stack_.metal(2)), 0);
}

TEST_F(PinValueTest, OnLayerPrefersLayerEntryOverUnlayered)
{
  const LayerValues entries{{1.5, stack_.metal(1)}, {0.5, nullptr}};
  EXPECT_DOUBLE_EQ(valueOnLayer(entries, stack_.metal(1)), 1.5);
  EXPECT_DOUBLE_EQ(valueOnLayer(entries, stack_.metal(2)), 0.5);
}

TEST_F(PinValueTest, ReadPinAntenna)
{
  odb::dbLib* lib = tst::makeLib(getDb(), tech_);
  odb::dbMaster* master = tst::makeMaster(
      lib,
      "BLOCK",
      100,
      100,
      {{.name = "A"},
       {.name = "Y", .io_type = odb::dbIoType::OUTPUT, .gate_area = 0.5},
       {.name = "Z", .diff_area = 0.5}});
  odb::dbTechLayer* m1 = stack_.metal(1);
  odb::dbTechLayer* v1 = stack_.cut(1);
  odb::dbMTerm* a = master->findMTerm("A");
  a->addDiffAreaEntry(0.25, m1);
  a->addPartialMetalAreaEntry(1.5, m1);
  a->addPartialMetalSideAreaEntry(2.5, m1);
  a->addPartialCutAreaEntry(0.01, v1);
  odb::dbTechAntennaPinModel* model = a->createDefaultAntennaModel();
  model->addGateAreaEntry(0.5, m1);
  model->addMaxAreaCAREntry(10, m1);
  model->addMaxSideAreaCAREntry(20, m1);
  model->addMaxCutCAREntry(1, v1);

  const PinAntenna pin = readPinAntenna(a);
  EXPECT_TRUE(pin.is_input);
  EXPECT_EQ(pin.gate_area, (LayerValues{{0.5, m1}}));
  EXPECT_EQ(pin.diff_area, (LayerValues{{0.25, m1}}));
  EXPECT_EQ(pin.partial_metal_area, (LayerValues{{1.5, m1}}));
  EXPECT_EQ(pin.partial_metal_side_area, (LayerValues{{2.5, m1}}));
  EXPECT_EQ(pin.partial_cut_area, (LayerValues{{0.01, v1}}));
  EXPECT_EQ(pin.max_area_car, (LayerValues{{10, m1}}));
  EXPECT_EQ(pin.max_side_area_car, (LayerValues{{20, m1}}));
  EXPECT_EQ(pin.max_cut_car, (LayerValues{{1, v1}}));

  // Only input pins with gate area are gates, from their gate's layer up.
  EXPECT_TRUE(isGateOn(pin, m1));
  EXPECT_FALSE(isGateOn(readPinAntenna(master->findMTerm("Y")), m1));
  EXPECT_FALSE(isGateOn(readPinAntenna(master->findMTerm("Z")), m1));
}

TEST_F(PinValueTest, GateFromItsLayerUp)
{
  PinAntenna pin;
  pin.is_input = true;
  pin.gate_area = {{0.5, stack_.metal(2)}};
  EXPECT_FALSE(isGateOn(pin, stack_.metal(1)));
  EXPECT_FALSE(isGateOn(pin, stack_.cut(1)));
  EXPECT_TRUE(isGateOn(pin, stack_.metal(2)));
  EXPECT_TRUE(isGateOn(pin, stack_.metal(3)));
}

}  // namespace
}  // namespace ant::test
