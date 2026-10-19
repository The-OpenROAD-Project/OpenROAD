// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Antenna rule behavior on small designs built in code.  Each net is a gate
// pin with one M1 wire whose area and side area are round numbers, so the
// expected ratios can be worked out by hand.

#include <optional>
#include <string>
#include <vector>

#include "AntennaReport.hh"
#include "AntennaTestUtil.h"
#include "ant/AntennaChecker.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "omp.h"
#include "tst/db_fixture.h"
#include "tst/logger_checks.h"
#include "tst/physical_builder.h"

namespace ant::test {
namespace {

using tst::AntennaRuleSpec;
using tst::constantPwl;
using tst::Pwl;

// Gate area of the GATE master's input pin, in um^2.
constexpr double kGateArea = 0.5;
// The M1 wire of a gate net, outside the pin: 10 um x 0.1 um.
constexpr double kWireArea = 1.0;
// Its perimeter (20.2 um) times the 1 um layer thickness.
constexpr double kWireSideArea = 20.2;
constexpr double kPar = kWireArea / kGateArea;      // 2.0
constexpr double kPsr = kWireSideArea / kGateArea;  // 40.4

class AntennaSyntheticTest : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    tech_ = tst::makeTech(getDb());
    addLayersBelowStack();
    stack_ = tst::makeLayerStack(tech_, 3, {.thickness = 1000});
    lib_ = tst::makeLib(getDb(), tech_);
    block_ = tst::makeBlock(getDb(), tech_, {0, 0, 200000, 200000});
    gate_ = tst::makeMaster(lib_,
                            "GATE",
                            100,
                            100,
                            {{.name = "A",
                              .layer = m1(),
                              .rect = {0, 0, 100, 100},
                              .gate_area = kGateArea}});
  }

  // Lets a fixture add layers below M1.
  virtual void addLayersBelowStack() {}

  odb::dbTechLayer* m1() const { return stack_.metal(1); }
  odb::dbTechLayer* m2() const { return stack_.metal(2); }

  // A driver master whose output pin has the given diffusion area.
  odb::dbMaster* driver(double diff_area)
  {
    const std::string name = "DRV" + std::to_string(next_master_++);
    return tst::makeMaster(lib_,
                           name.c_str(),
                           100,
                           100,
                           {{.name = "X",
                             .io_type = odb::dbIoType::OUTPUT,
                             .layer = m1(),
                             .rect = {0, 0, 100, 100},
                             .diff_area = diff_area}});
  }

  // Makes later gateNet calls place a new master like GATE whose pin A has
  // no antenna data, and returns that pin.  Tests add the per-layer LEF
  // values they need, as a block's LEF would give them.
  odb::dbMTerm* useBareGatePin()
  {
    const std::string name = "BLOCK" + std::to_string(next_master_++);
    gate_ = tst::makeMaster(
        lib_,
        name.c_str(),
        100,
        100,
        {{.name = "A", .layer = m1(), .rect = {0, 0, 100, 100}}});
    return gate_->findMTerm("A");
  }

  // A plain GATE instance on net at the far end of gateNet's wire, touching
  // it without overlapping.
  void addFarGate(odb::dbNet* net, odb::dbMaster* master)
  {
    const std::string name = net->getName() + "_far";
    makeInst(block_,
             master,
             name.c_str(),
             {.location = {10100, 0},
              .status = odb::dbPlacementStatus::PLACED,
              .iterms = {{net->getConstName(), "A"}}});
  }

  // Extends gateNet's wire from its far end up through V1 to a 10 um M2
  // wire.
  void routeUpToM2(odb::dbNet* net)
  {
    tst::RoutePath(net)
        .on(m1(), {50, 50})
        .to({10050, 50})
        .via(stack_.via(1))
        .to({10050, 10050});
  }

  // The checks for the named gate pin of net on layer.
  LayerCheck layerCheck(odb::dbNet* net,
                        const std::string& pin,
                        odb::dbTechLayer* layer)
  {
    const NetCheck net_check = checker_.checkNet(net);
    for (const PinCheck& pin_check : net_check.pins) {
      if (pinName(pin_check.gate) != pin) {
        continue;
      }
      for (const LayerCheck& check : pin_check.layers) {
        if (check.layer == layer) {
          return check;
        }
      }
    }
    ADD_FAILURE() << "No check for " << pin << " on " << layer->getName();
    return {};
  }

  // A gate at (0, y) with an M1 wire running 10 um to its right.  With
  // driver_master, a driver pin sits at the far end of the wire, touching
  // it without overlapping, so the wire's area is the same either way.
  odb::dbNet* gateNet(const char* name,
                      int y = 0,
                      odb::dbMaster* driver_master = nullptr)
  {
    const std::string gate_name = std::string(name) + "_g";
    makeInst(block_,
             gate_,
             gate_name.c_str(),
             {.location = {0, y},
              .status = odb::dbPlacementStatus::PLACED,
              .iterms = {{name, "A"}}});
    if (driver_master) {
      const std::string driver_name = std::string(name) + "_d";
      makeInst(block_,
               driver_master,
               driver_name.c_str(),
               {.location = {10100, y},
                .status = odb::dbPlacementStatus::PLACED,
                .iterms = {{name, "X"}}});
    }
    odb::dbNet* net = block_->findNet(name);
    tst::RoutePath(net).on(m1(), {50, y + 50}).to({10050, y + 50});
    return net;
  }

  // The checks for the first gate of net on layer.
  LayerCheck layerCheck(odb::dbNet* net,
                        odb::dbTechLayer* layer,
                        float ratio_margin = 0)
  {
    const NetCheck net_check = checker_.checkNet(net, ratio_margin);
    EXPECT_FALSE(net_check.pins.empty());
    if (!net_check.pins.empty()) {
      for (const LayerCheck& check : net_check.pins[0].layers) {
        if (check.layer == layer) {
          return check;
        }
      }
    }
    ADD_FAILURE() << "No check on layer " << layer->getName();
    return {};
  }

  odb::dbTech* tech_{nullptr};
  tst::LayerStack stack_;
  odb::dbLib* lib_{nullptr};
  odb::dbBlock* block_{nullptr};
  odb::dbMaster* gate_{nullptr};
  int next_master_{0};
  AntennaChecker checker_{getDb(), getLogger()};
};

TEST_F(AntennaSyntheticTest, PartialAreaRatio)
{
  tst::applyAntennaRule(m1(), {.par = 1.5, .car = 1.8});
  const LayerCheck check = layerCheck(gateNet("n1"), m1());

  EXPECT_NEAR(check.par.actual, kPar, 1e-9);
  EXPECT_DOUBLE_EQ(check.par.required, 1.5);
  EXPECT_TRUE(check.par.violated);
  EXPECT_TRUE(check.violated);
  // CAR is checked and reported even though PAR already violates.
  EXPECT_NEAR(check.car.actual, kPar, 1e-9);
  EXPECT_DOUBLE_EQ(check.car.required, 1.8);
  EXPECT_TRUE(check.car.violated);
}

TEST_F(AntennaSyntheticTest, PartialAreaRatioWithinLimit)
{
  tst::applyAntennaRule(m1(), {.par = 2.5});
  const LayerCheck check = layerCheck(gateNet("n1"), m1());

  EXPECT_FALSE(check.par.violated);
  EXPECT_FALSE(check.violated);
  EXPECT_NEAR(check.car.actual, kPar, 1e-9);
}

TEST_F(AntennaSyntheticTest, SideAreaRatio)
{
  tst::applyAntennaRule(m1(), {.psr = 40});
  const LayerCheck check = layerCheck(gateNet("n1"), m1());

  ASSERT_TRUE(check.psr.has_value());
  EXPECT_NEAR(check.psr->actual, kPsr, 1e-9);
  EXPECT_DOUBLE_EQ(check.psr->required, 40);
  EXPECT_TRUE(check.psr->violated);
  EXPECT_FALSE(check.par.violated);  // no PAR limit
  EXPECT_TRUE(check.violated);
}

TEST_F(AntennaSyntheticTest, AreaFactor)
{
  tst::applyAntennaRule(m1(), {.par = 100, .area_factor = 3});
  const LayerCheck check = layerCheck(gateNet("n1"), m1());
  EXPECT_NEAR(check.par.actual, 3 * kPar, 1e-9);
}

TEST_F(AntennaSyntheticTest, DiffuseOnlyAreaFactorIgnoredWithoutDiffusion)
{
  tst::applyAntennaRule(
      m1(), {.par = 100, .area_factor = 3, .area_factor_diffuse_only = true});
  const LayerCheck check = layerCheck(gateNet("n1"), m1());
  EXPECT_NEAR(check.par.actual, kPar, 1e-9);
}

TEST_F(AntennaSyntheticTest, DiffuseOnlyAreaFactorAppliedWithDiffusion)
{
  tst::applyAntennaRule(m1(),
                        {.diff_par = constantPwl(100),
                         .area_factor = 3,
                         .area_factor_diffuse_only = true});
  const LayerCheck check
      = layerCheck(gateNet("n1", 0, driver(/*diff_area=*/0.5)), m1());
  EXPECT_NEAR(check.par.actual, 3 * kPar, 1e-9);
}

TEST_F(AntennaSyntheticTest, SideAreaFactor)
{
  tst::applyAntennaRule(m1(), {.psr = 1000, .side_area_factor = 2});
  const LayerCheck check = layerCheck(gateNet("n1"), m1());
  ASSERT_TRUE(check.psr.has_value());
  EXPECT_NEAR(check.psr->actual, 2 * kPsr, 1e-9);
}

// With diffusion on the net, the required ratio comes from the diffusion
// PWL, evaluated at the net's diffusion area.
TEST_F(AntennaSyntheticTest, DiffusionRequiredRatioFromPwl)
{
  tst::applyAntennaRule(m1(), {.par = 1, .diff_par = Pwl{{0, 1}, {10, 30}}});
  const LayerCheck check
      = layerCheck(gateNet("n1", 0, driver(/*diff_area=*/0.5)), m1());

  EXPECT_NEAR(check.par.required, 20, 1e-9);
  EXPECT_NEAR(check.par.actual, kPar, 1e-9);
  EXPECT_FALSE(check.par.violated);
}

// Without diffusion and without a plain limit, the diffusion PWL's value at
// zero diffusion is the limit.
TEST_F(AntennaSyntheticTest, DiffusionPwlUsedWithoutPlainLimit)
{
  tst::applyAntennaRule(m1(), {.diff_par = Pwl{{0, 10}, {1.5, 30}}});
  const LayerCheck check = layerCheck(gateNet("n1"), m1());

  EXPECT_NEAR(check.par.required, 1.5, 1e-9);
  EXPECT_NEAR(check.par.actual, kPar, 1e-9);
  EXPECT_TRUE(check.par.violated);
}

TEST_F(AntennaSyntheticTest, AreaMinusDiff)
{
  // (area - 1.0 * diff) / gate = (1.0 - 0.5) / 0.5
  tst::applyAntennaRule(m1(),
                        {.diff_par = constantPwl(100), .area_minus_diff = 1.0});
  const LayerCheck check
      = layerCheck(gateNet("n1", 0, driver(/*diff_area=*/0.5)), m1());
  EXPECT_NEAR(check.par.actual, 1.0, 1e-9);
}

TEST_F(AntennaSyntheticTest, GatePlusDiff)
{
  // area / (gate + 1.0 * diff) = 1.0 / (0.5 + 0.5)
  tst::applyAntennaRule(m1(),
                        {.diff_par = constantPwl(100), .gate_plus_diff = 1.0});
  const LayerCheck check
      = layerCheck(gateNet("n1", 0, driver(/*diff_area=*/0.5)), m1());
  EXPECT_NEAR(check.par.actual, 1.0, 1e-9);
}

TEST_F(AntennaSyntheticTest, AreaDiffReduce)
{
  // area * reduce(0.5) / gate, with reduce(0.5) = 0.5
  tst::applyAntennaRule(m1(),
                        {.diff_par = constantPwl(100),
                         .area_diff_reduce = Pwl{{0, 0.4, 10}, {1, 0.5, 0.5}}});
  const LayerCheck check
      = layerCheck(gateNet("n1", 0, driver(/*diff_area=*/0.5)), m1());
  EXPECT_NEAR(check.par.actual, kPar * 0.5, 1e-9);
}

TEST_F(AntennaSyntheticTest, RatioMargin)
{
  tst::applyAntennaRule(m1(), {.par = 2.1});
  odb::dbNet* net = gateNet("n1");

  EXPECT_FALSE(layerCheck(net, m1()).par.violated);
  const LayerCheck margin = layerCheck(net, m1(), /*ratio_margin=*/10);
  EXPECT_NEAR(margin.par.required, 2.1 * 0.9, 1e-6);
  EXPECT_TRUE(margin.par.violated);
}

// The margin tightens the cumulative limits too.
TEST_F(AntennaSyntheticTest, RatioMarginOnCumulativeRatios)
{
  tst::applyAntennaRule(m1(), {.car = 2.1, .csr = 42});
  odb::dbNet* net = gateNet("n1");

  const LayerCheck check = layerCheck(net, m1());
  ASSERT_TRUE(check.csr);
  EXPECT_FALSE(check.car.violated);
  EXPECT_FALSE(check.csr->violated);
  const LayerCheck margin = layerCheck(net, m1(), /*ratio_margin=*/10);
  ASSERT_TRUE(margin.csr);
  EXPECT_NEAR(margin.car.required, 2.1 * 0.9, 1e-6);
  EXPECT_TRUE(margin.car.violated);
  EXPECT_NEAR(margin.csr->required, 42 * 0.9, 1e-6);
  EXPECT_TRUE(margin.csr->violated);
}

// A gate wired up through a via accumulates the PAR/PSR of every layer
// below into CAR/CSR, and the cut layer gets a PAR of its own.
TEST_F(AntennaSyntheticTest, CumulativeRatiosAcrossLayers)
{
  for (odb::dbTechLayer* layer : {m1(), m2()}) {
    tst::applyAntennaRule(layer, {.car = 1000, .psr = 1000, .csr = 1000});
  }
  tst::applyAntennaRule(stack_.cut(1), {.par = 0.01});
  odb::dbNet* net = gateNet("n1");
  tst::RoutePath(net)
      .on(m1(), {50, 50})
      .to({10050, 50})
      .via(stack_.via(1))
      .to({10050, 10050});

  const LayerCheck m1_check = layerCheck(net, m1());
  const LayerCheck m2_check = layerCheck(net, m2());
  ASSERT_TRUE(m1_check.psr && m2_check.psr && m1_check.csr && m2_check.csr);
  EXPECT_NEAR(m1_check.car.actual, m1_check.par.actual, 1e-9);
  EXPECT_NEAR(
      m2_check.car.actual, m1_check.par.actual + m2_check.par.actual, 1e-9);
  EXPECT_NEAR(
      m2_check.csr->actual, m1_check.psr->actual + m2_check.psr->actual, 1e-9);

  // The cut is 0.1 um x 0.1 um.
  const LayerCheck cut_check = layerCheck(net, stack_.cut(1));
  EXPECT_NEAR(cut_check.par.actual, 0.01 / kGateArea, 1e-9);
  EXPECT_TRUE(cut_check.par.violated);
  EXPECT_FALSE(cut_check.psr.has_value());
  EXPECT_FALSE(cut_check.csr.has_value());
}

TEST_F(AntennaSyntheticTest, CumulativeAreaRatioViolation)
{
  tst::applyAntennaRule(m1(), {.car = 1.5});
  const LayerCheck check = layerCheck(gateNet("n1"), m1());
  EXPECT_NEAR(check.car.actual, kPar, 1e-9);
  EXPECT_TRUE(check.car.violated);
  EXPECT_TRUE(check.violated);
}

// Only layers with an antenna rule are checked.
TEST_F(AntennaSyntheticTest, LayersWithoutRuleAreSkipped)
{
  const NetCheck net_check = checker_.checkNet(gateNet("n1"));
  ASSERT_EQ(net_check.pins.size(), 1);
  EXPECT_TRUE(net_check.pins[0].layers.empty());
  EXPECT_FALSE(net_check.violated);
}

TEST_F(AntennaSyntheticTest, OnlyInputPinsWithGateAreaAreGates)
{
  tst::applyAntennaRule(m1(), {.par = 1.5});
  const NetCheck net_check
      = checker_.checkNet(gateNet("n1", 0, driver(/*diff_area=*/0.5)));
  ASSERT_EQ(net_check.pins.size(), 1);
  EXPECT_EQ(pinName(net_check.pins[0].gate), "n1_g/A");
}

TEST_F(AntennaSyntheticTest, UnroutedNetHasNoChecks)
{
  tst::applyAntennaRule(m1(), {.par = 1.5});
  gateNet("routed");
  makeInst(block_,
           gate_,
           "u",
           {.location = {0, 5000},
            .status = odb::dbPlacementStatus::PLACED,
            .iterms = {{"unrouted", "A"}}});

  const NetCheck net_check = checker_.checkNet(block_->findNet("unrouted"));
  EXPECT_TRUE(net_check.pins.empty());
  EXPECT_FALSE(net_check.violated);
}

TEST_F(AntennaSyntheticTest, SpecialNet)
{
  tst::applyAntennaRule(m1(), {.par = 1.5});
  odb::dbNet* net = gateNet("n1");
  net->setSpecial();
  gateNet("n2", 1000);  // so the design counts as routed

  EXPECT_TRUE(checker_.checkNet(net).pins.empty());
  EXPECT_TRUE(checker_.getAntennaViolations(net, nullptr, 0).empty());
  EXPECT_LOGGER_ERROR(checker_.check(net), "ANT-0014");
  // A check of every net skips it.
  for (const NetCheck& net_check : checker_.check().nets) {
    EXPECT_NE(net_check.net, net);
  }
}

TEST_F(AntennaSyntheticTest, NoRoutingIsAnError)
{
  // A placed two-pin net with neither wires nor guides.
  for (const char* name : {"u1", "u2"}) {
    makeInst(block_,
             gate_,
             name,
             {.location = {0, 0},
              .status = odb::dbPlacementStatus::PLACED,
              .iterms = {{"n1", "A"}}});
  }
  EXPECT_LOGGER_ERROR(checker_.check(), "ANT-0008");
}

// A layer whose rules cannot be loaded fails every check, not just the
// first: the rule table is not left half built.
TEST_F(AntennaSyntheticTest, LayerRuleErrorRepeats)
{
  odb::dbTechLayer* m4 = tst::addRoutingLayer(tech_, "M4");  // no thickness
  tst::applyAntennaRule(m4, {.psr = 100});
  gateNet("n1");

  EXPECT_LOGGER_ERROR(checker_.check(), "ANT-0013");
  EXPECT_LOGGER_ERROR(checker_.check(), "ANT-0013");
}

// The thread count applies to the check only, not to later OpenMP work.
TEST_F(AntennaSyntheticTest, CheckKeepsProcessThreadCount)
{
  tst::applyAntennaRule(m1(), {.par = 1.5});
  odb::dbNet* net = gateNet("n1");

  omp_set_num_threads(3);
  checker_.check(nullptr, 2);
  checker_.check(net, 1);
  EXPECT_EQ(omp_get_max_threads(), 3);
}

TEST_F(AntennaSyntheticTest, Counts)
{
  tst::applyAntennaRule(m1(), {.par = 1.5});
  gateNet("bad1", 0);
  gateNet("bad2", 1000);
  tst::RoutePath(gateNet("ok", 2000)).on(m1(), {50, 2050}).to({550, 2050});

  const DesignCheck design = checker_.check();
  EXPECT_EQ(design.nets.size(), 3);
  EXPECT_EQ(design.violatingNets(), 2);
  EXPECT_EQ(design.violatingPins(), 2);
  EXPECT_FALSE(netCheck(design, "ok").violated);
}

// check_antennas reports the counts of the check as metrics, and keeps them
// for antenna_violation_count.
TEST_F(AntennaSyntheticTest, CheckAntennasCountsAndMetrics)
{
  tst::applyAntennaRule(m1(), {.par = 1.5});
  gateNet("bad1", 0);
  gateNet("bad2", 1000);

  tst::MetricsCapture metrics(logger_);
  EXPECT_EQ(checker_.checkAntennas(), 2);
  EXPECT_EQ(checker_.antennaViolationCount(), 2);
  EXPECT_EQ(checker_.pinViolationCount(), 2);
  EXPECT_EQ(metrics.value("antenna__violating__nets"), "2");
  EXPECT_EQ(metrics.value("antenna__violating__pins"), "2");
}

TEST_F(AntennaSyntheticTest, ReportListsViolatingNetsUnlessAskedForAll)
{
  tst::applyAntennaRule(m1(), {.par = 1.5});
  gateNet("bad", 0);
  tst::RoutePath(gateNet("ok", 1000)).on(m1(), {50, 1050}).to({550, 1050});

  const DesignCheck design = checker_.check();
  const std::string violating = formatReport(design, false);
  EXPECT_NE(violating.find("Net: bad"), std::string::npos);
  EXPECT_EQ(violating.find("Net: ok"), std::string::npos);
  const std::string all = formatReport(design, true);
  EXPECT_NE(all.find("Net: bad"), std::string::npos);
  EXPECT_NE(all.find("Net: ok"), std::string::npos);
}

TEST_F(AntennaSyntheticTest, SingleNetCheck)
{
  tst::applyAntennaRule(m1(), {.par = 1.5});
  gateNet("bad", 0);
  odb::dbNet* ok = gateNet("ok", 1000);
  tst::RoutePath(ok).on(m1(), {50, 1050}).to({550, 1050});

  const DesignCheck design = checker_.check(ok);
  ASSERT_EQ(design.nets.size(), 1);
  EXPECT_EQ(design.nets[0].net, ok);
  EXPECT_EQ(design.violatingNets(), 0);
}

TEST_F(AntennaSyntheticTest, ThreadCountDoesNotChangeResults)
{
  tst::applyAntennaRule(m1(), {.par = 1.5});
  for (int i = 0; i < 12; ++i) {
    const std::string name = "n" + std::to_string(i);
    odb::dbNet* net = gateNet(name.c_str(), i * 1000);
    if (i % 3 == 0) {
      tst::RoutePath(net)
          .on(m1(), {50, i * 1000 + 50})
          .to({550, i * 1000 + 50});
    }
  }

  const DesignCheck one = checker_.check(nullptr, 1);
  EXPECT_EQ(one.violatingNets(), 8);
  // Four threads, and a second run on the same design, give the same
  // answer net by net.
  for (const DesignCheck& again :
       {checker_.check(nullptr, 4), checker_.check(nullptr, 4)}) {
    ASSERT_EQ(again.nets.size(), one.nets.size());
    for (size_t i = 0; i < one.nets.size(); ++i) {
      EXPECT_EQ(again.nets[i].net, one.nets[i].net);
      EXPECT_EQ(again.nets[i].violated, one.nets[i].violated);
      EXPECT_EQ(again.nets[i].pins.size(), one.nets[i].pins.size());
    }
  }
}

// getAntennaViolations loads the layer rules itself, as repair may call it
// on a new checker.
TEST_F(AntennaSyntheticTest, ViolationsWithoutDiode)
{
  tst::applyAntennaRule(m1(), {.par = 1.5});
  odb::dbNet* net = gateNet("n1");

  const Violations violations = checker_.getAntennaViolations(net, nullptr, 0);
  ASSERT_EQ(violations.size(), 1);
  EXPECT_EQ(violations[0].routing_level, 1);
  ASSERT_EQ(violations[0].gates.size(), 1);
  EXPECT_EQ(pinName(violations[0].gates[0]), "n1_g/A");
  EXPECT_EQ(violations[0].diode_count_per_gate, 0);
  EXPECT_NEAR(violations[0].excess_ratio, kPar / 1.5, 1e-9);
}

// Repair asks how many diodes of a given diffusion area each violating gate
// needs; each diode adds its diffusion to the gate's denominator.
TEST_F(AntennaSyntheticTest, DiodeCount)
{
  // Without diffusion: 1.0 / 0.5 = 2.0 > 1.5.
  // One diode: 1.0 / (0.5 + 1.0 * 0.25) = 1.33 < 1.5.
  tst::applyAntennaRule(m1(),
                        {.diff_par = constantPwl(1.5), .gate_plus_diff = 1.0});
  odb::dbNet* net = gateNet("n1");
  odb::dbMaster* diode = driver(/*diff_area=*/0.25);

  const Violations violations
      = checker_.getAntennaViolations(net, diode->findMTerm("X"), 0);
  ASSERT_EQ(violations.size(), 1);
  EXPECT_EQ(violations[0].diode_count_per_gate, 1);
  EXPECT_NEAR(violations[0].excess_ratio, kPar / 1.5, 1e-9);
}

// The diode search stops one past kMaxDiodeCountPerGate (10).
TEST_F(AntennaSyntheticTest, DiodeCountCapped)
{
  tst::applyAntennaRule(m1(),
                        {.diff_par = constantPwl(1.5), .gate_plus_diff = 1.0});
  odb::dbNet* net = gateNet("n1");
  odb::dbMaster* diode = driver(/*diff_area=*/0.0001);

  const Violations violations
      = checker_.getAntennaViolations(net, diode->findMTerm("X"), 0);
  ASSERT_EQ(violations.size(), 1);
  EXPECT_EQ(violations[0].diode_count_per_gate, 11);
}

// A pin's gate area holds from the layer LEF gives it on, so a gate
// checked on M1 uses its M1 area rather than a larger one given for M2.
TEST_F(AntennaSyntheticTest, GateAreaPerLayer)
{
  tst::applyAntennaRule(m1(), {.par = 100});
  odb::dbTechAntennaPinModel* model
      = useBareGatePin()->createDefaultAntennaModel();
  model->addGateAreaEntry(kGateArea, m1());
  model->addGateAreaEntry(4 * kGateArea, m2());

  const LayerCheck check = layerCheck(gateNet("n1"), m1());
  EXPECT_NEAR(check.par.actual, kPar, 1e-9);
}

TEST_F(AntennaSyntheticTest, DiffusionAreaPerLayer)
{
  // 1.0 / (0.5 + 1.0 * 1.0) on M1, where the driver has 1.0 of diffusion.
  tst::applyAntennaRule(m1(),
                        {.diff_par = constantPwl(100), .gate_plus_diff = 1.0});
  odb::dbMaster* drv = driver(/*diff_area=*/1.0);
  drv->findMTerm("X")->addDiffAreaEntry(3.0, m2());

  const LayerCheck check = layerCheck(gateNet("n1", 0, drv), m1());
  EXPECT_NEAR(check.par.actual, kWireArea / (kGateArea + 1.0), 1e-9);
}

// A block pin whose gate is reached only through upper metal, like the
// OpenRAM-style blocks whose pins are on met1 but whose gate area is given
// on met4, is not a gate below that layer: neither checked nor protecting
// the gates it touches there.  Its partial metal still charges them.
TEST_F(AntennaSyntheticTest, BlockGateOnlyFromItsLayerUp)
{
  tst::applyAntennaRule(m1(), {.par = 100});
  odb::dbMaster* plain_gate = gate_;
  odb::dbMTerm* pin = useBareGatePin();
  pin->createDefaultAntennaModel()->addGateAreaEntry(kGateArea, m2());
  pin->addPartialMetalAreaEntry(1.5, m1());
  odb::dbNet* net = gateNet("n1");
  addFarGate(net, plain_gate);

  const NetCheck net_check = checker_.checkNet(net);
  ASSERT_EQ(net_check.pins.size(), 1);
  EXPECT_EQ(pinName(net_check.pins[0].gate), "n1_far/A");
  ASSERT_EQ(net_check.pins[0].layers.size(), 1);
  EXPECT_NEAR(net_check.pins[0].layers[0].par.actual,
              (kWireArea + 1.5) / kGateArea,
              1e-9);
}

// Diffusion reached only through upper metal doesn't protect lower layers.
TEST_F(AntennaSyntheticTest, DiffusionOnlyFromItsLayerUp)
{
  tst::applyAntennaRule(m1(), {.par = 100, .diff_par = constantPwl(200)});
  odb::dbMaster* drv = tst::makeMaster(lib_,
                                       "BLOCK_DRV",
                                       100,
                                       100,
                                       {{.name = "X",
                                         .io_type = odb::dbIoType::OUTPUT,
                                         .layer = m1(),
                                         .rect = {0, 0, 100, 100}}});
  drv->findMTerm("X")->addDiffAreaEntry(1.0, m2());

  const LayerCheck check = layerCheck(gateNet("n1", 0, drv), m1());
  EXPECT_DOUBLE_EQ(check.par.required, 100);
}

// ANTENNAPARTIALMETALAREA and ANTENNAPARTIALMETALSIDEAREA are metal inside
// a block, connected to its pin, that the router doesn't see.  They add to
// the pin layer's area and side area.
TEST_F(AntennaSyntheticTest, PartialMetalArea)
{
  tst::applyAntennaRule(m1(), {.par = 100, .psr = 1000});
  odb::dbMTerm* pin = useBareGatePin();
  pin->createDefaultAntennaModel()->addGateAreaEntry(kGateArea, m1());
  pin->addPartialMetalAreaEntry(1.5, m1());
  pin->addPartialMetalSideAreaEntry(10, m1());
  pin->addPartialMetalAreaEntry(100, m2());  // no M2 metal on the net

  const LayerCheck check = layerCheck(gateNet("n1"), m1());
  EXPECT_NEAR(check.par.actual, (kWireArea + 1.5) / kGateArea, 1e-9);
  ASSERT_TRUE(check.psr);
  EXPECT_NEAR(check.psr->actual, (kWireSideArea + 10) / kGateArea, 1e-9);
}

// The partial metal is part of the node, so it charges every gate on it.
TEST_F(AntennaSyntheticTest, PartialMetalAreaChargesOtherGates)
{
  tst::applyAntennaRule(m1(), {.par = 100});
  odb::dbMaster* plain_gate = gate_;
  odb::dbMTerm* pin = useBareGatePin();
  pin->createDefaultAntennaModel()->addGateAreaEntry(kGateArea, m1());
  pin->addPartialMetalAreaEntry(1.5, m1());
  odb::dbNet* net = gateNet("n1");
  addFarGate(net, plain_gate);

  const double par = (kWireArea + 1.5) / (2 * kGateArea);
  EXPECT_NEAR(layerCheck(net, "n1_g/A", m1()).par.actual, par, 1e-9);
  const LayerCheck far = layerCheck(net, "n1_far/A", m1());
  EXPECT_NEAR(far.par.actual, par, 1e-9);
  EXPECT_NEAR(far.car.actual, par, 1e-9);
}

// ANTENNAMAXAREACAR and ANTENNAMAXSIDEAREACAR are the cumulative ratios
// the block's own metal has given the pin's gates up to a layer.  The
// checker continues the sum from there, as in LEF/DEF Appendix C Example 1:
// CAR(M1,G1) = PAR(M1,G1) + pin A's CAR.
TEST_F(AntennaSyntheticTest, MaxCarStartsCumulativeRatio)
{
  for (odb::dbTechLayer* layer : {m1(), m2()}) {
    tst::applyAntennaRule(layer, {.car = 1000, .psr = 1000, .csr = 1000});
  }
  odb::dbTechAntennaPinModel* model
      = useBareGatePin()->createDefaultAntennaModel();
  model->addGateAreaEntry(kGateArea, m1());
  model->addMaxAreaCAREntry(3.0, m1());
  model->addMaxSideAreaCAREntry(7.0, m1());
  odb::dbNet* net = gateNet("n1");
  routeUpToM2(net);

  const LayerCheck m1_check = layerCheck(net, m1());
  const LayerCheck m2_check = layerCheck(net, m2());
  ASSERT_TRUE(m1_check.psr && m1_check.csr && m2_check.psr && m2_check.csr);
  EXPECT_NEAR(m1_check.car.actual, 3.0 + m1_check.par.actual, 1e-9);
  EXPECT_NEAR(m1_check.csr->actual, 7.0 + m1_check.psr->actual, 1e-9);
  EXPECT_NEAR(m2_check.car.actual,
              3.0 + m1_check.par.actual + m2_check.par.actual,
              1e-9);
  EXPECT_NEAR(m2_check.csr->actual,
              7.0 + m1_check.psr->actual + m2_check.psr->actual,
              1e-9);
}

// Each MAX*CAR is cumulative through its layer, so the M2 value already
// includes the M1 one and replaces it in the sum.
TEST_F(AntennaSyntheticTest, MaxCarOnSuccessiveLayersReplaces)
{
  for (odb::dbTechLayer* layer : {m1(), m2()}) {
    tst::applyAntennaRule(layer, {.car = 1000, .psr = 1000, .csr = 1000});
  }
  odb::dbTechAntennaPinModel* model
      = useBareGatePin()->createDefaultAntennaModel();
  model->addGateAreaEntry(kGateArea, m1());
  model->addMaxAreaCAREntry(3.0, m1());
  model->addMaxAreaCAREntry(4.0, m2());
  model->addMaxSideAreaCAREntry(7.0, m1());
  model->addMaxSideAreaCAREntry(9.0, m2());
  odb::dbNet* net = gateNet("n1");
  routeUpToM2(net);

  const LayerCheck m1_check = layerCheck(net, m1());
  const LayerCheck m2_check = layerCheck(net, m2());
  ASSERT_TRUE(m1_check.psr && m1_check.csr && m2_check.psr && m2_check.csr);
  EXPECT_NEAR(m1_check.car.actual, 3.0 + m1_check.par.actual, 1e-9);
  EXPECT_NEAR(m2_check.car.actual,
              4.0 + m1_check.par.actual + m2_check.par.actual,
              1e-9);
  EXPECT_NEAR(m2_check.csr->actual,
              9.0 + m1_check.psr->actual + m2_check.psr->actual,
              1e-9);
}

// MAX*CAR counts the pin's partial area on its layer already (it covers
// the metal "at or below the current pin layer"), so the pin's own gates
// don't count it twice.  Other gates on the node do see it.
TEST_F(AntennaSyntheticTest, MaxCarIncludesPartialArea)
{
  tst::applyAntennaRule(m1(), {.par = 100, .car = 100});
  odb::dbMaster* plain_gate = gate_;
  odb::dbMTerm* pin = useBareGatePin();
  odb::dbTechAntennaPinModel* model = pin->createDefaultAntennaModel();
  model->addGateAreaEntry(kGateArea, m1());
  model->addMaxAreaCAREntry(4.0, m1());
  pin->addPartialMetalAreaEntry(1.5, m1());
  odb::dbNet* net = gateNet("n1");
  addFarGate(net, plain_gate);

  const double par = (kWireArea + 1.5) / (2 * kGateArea);
  const LayerCheck block = layerCheck(net, "n1_g/A", m1());
  EXPECT_NEAR(block.par.actual, par, 1e-9);
  EXPECT_NEAR(block.car.actual, 4.0 + kWireArea / (2 * kGateArea), 1e-9);
  const LayerCheck far = layerCheck(net, "n1_far/A", m1());
  EXPECT_NEAR(far.car.actual, par, 1e-9);
}

// ANTENNAPARTIALCUTAREA and ANTENNAMAXCUTCAR do the same for the cut layer
// above the pin.
TEST_F(AntennaSyntheticTest, PartialCutAreaAndMaxCutCar)
{
  tst::applyAntennaRule(stack_.cut(1), {.par = 100, .car = 100});
  odb::dbMTerm* pin = useBareGatePin();
  odb::dbTechAntennaPinModel* model = pin->createDefaultAntennaModel();
  model->addGateAreaEntry(kGateArea, m1());
  model->addMaxCutCAREntry(1.0, stack_.cut(1));
  pin->addPartialCutAreaEntry(0.01, stack_.cut(1));
  odb::dbNet* net = gateNet("n1");
  routeUpToM2(net);

  // The net's own cut is 0.1 um x 0.1 um.
  const LayerCheck check = layerCheck(net, stack_.cut(1));
  EXPECT_NEAR(check.par.actual, (0.01 + 0.01) / kGateArea, 1e-9);
  EXPECT_NEAR(check.car.actual, 1.0 + 0.01 / kGateArea, 1e-9);
}

// ANTENNACUMROUTINGPLUSCUT joins the metal and cut sums into one chain:
// CAR(M2) = PAR(M2) + CAR(V1) and CAR(V1) = PAR(V1) + CAR(M1).  Cuts have
// no side area, so CSR still sums the metal layers.
TEST_F(AntennaSyntheticTest, CumRoutingPlusCut)
{
  for (odb::dbTechLayer* layer : {m1(), m2()}) {
    tst::applyAntennaRule(
        layer,
        {.car = 1000, .psr = 1000, .csr = 1000, .cum_routing_plus_cut = true});
  }
  tst::applyAntennaRule(stack_.cut(1),
                        {.car = 1000, .cum_routing_plus_cut = true});
  odb::dbNet* net = gateNet("n1");
  routeUpToM2(net);

  const LayerCheck m1_check = layerCheck(net, m1());
  const LayerCheck v1_check = layerCheck(net, stack_.cut(1));
  const LayerCheck m2_check = layerCheck(net, m2());
  ASSERT_TRUE(m1_check.psr && m2_check.psr && m2_check.csr);
  EXPECT_NEAR(m1_check.car.actual, m1_check.par.actual, 1e-9);
  EXPECT_NEAR(
      v1_check.car.actual, v1_check.par.actual + m1_check.car.actual, 1e-9);
  EXPECT_NEAR(
      m2_check.car.actual, m2_check.par.actual + v1_check.car.actual, 1e-9);
  EXPECT_NEAR(
      m2_check.csr->actual, m1_check.psr->actual + m2_check.psr->actual, 1e-9);
}

// A stack with a contact layer CO below M1, like layer C in LEF/DEF
// Appendix C's Example 1, where a block pin's ANTENNAMAXCUTCAR is given on
// the contact.
class AntennaContactTest : public AntennaSyntheticTest
{
 protected:
  void addLayersBelowStack() override
  {
    contact_ = tst::addCutLayer(tech_, "CO");
  }

  // A gate net routed up to M2 whose pin has MAXCUTCAR 1.0 on CO.
  odb::dbNet* netWithContactCar()
  {
    odb::dbTechAntennaPinModel* model
        = useBareGatePin()->createDefaultAntennaModel();
    model->addGateAreaEntry(kGateArea, m1());
    model->addMaxCutCAREntry(1.0, contact_);
    odb::dbNet* net = gateNet("n1");
    routeUpToM2(net);
    return net;
  }

  odb::dbTechLayer* contact_{nullptr};
};

// By default the contact's ratio accumulates with the other cuts.
TEST_F(AntennaContactTest, MaxCutCarCarriesToCuts)
{
  tst::applyAntennaRule(m1(), {.car = 1000});
  tst::applyAntennaRule(stack_.cut(1), {.car = 1000});
  odb::dbNet* net = netWithContactCar();

  const LayerCheck m1_check = layerCheck(net, m1());
  const LayerCheck v1_check = layerCheck(net, stack_.cut(1));
  EXPECT_NEAR(m1_check.car.actual, m1_check.par.actual, 1e-9);
  EXPECT_NEAR(v1_check.car.actual, 1.0 + v1_check.par.actual, 1e-9);
}

// With ANTENNACUMROUTINGPLUSCUT, Example 1's CAR(M1,G1) = PAR(M1,G1) + pin
// A's CAR(C,G1), and the sum continues up through V1.
TEST_F(AntennaContactTest, MaxCutCarCarriesToMetalWithRoutingPlusCut)
{
  tst::applyAntennaRule(m1(), {.car = 1000, .cum_routing_plus_cut = true});
  tst::applyAntennaRule(stack_.cut(1),
                        {.car = 1000, .cum_routing_plus_cut = true});
  odb::dbNet* net = netWithContactCar();

  const LayerCheck m1_check = layerCheck(net, m1());
  const LayerCheck v1_check = layerCheck(net, stack_.cut(1));
  EXPECT_NEAR(m1_check.car.actual, 1.0 + m1_check.par.actual, 1e-9);
  EXPECT_NEAR(
      v1_check.car.actual, v1_check.par.actual + m1_check.car.actual, 1e-9);
}

// Example 1 notes that MAXCUTCAR 1.0 on C and MAXAREACAR 1.0 on M1 give the
// same result: in one chain, the M1 value already includes the contact's.
TEST_F(AntennaContactTest, MaxAreaCarReplacesMaxCutCarWithRoutingPlusCut)
{
  tst::applyAntennaRule(m1(), {.car = 1000, .cum_routing_plus_cut = true});
  odb::dbMTerm* pin = useBareGatePin();
  odb::dbTechAntennaPinModel* model = pin->createDefaultAntennaModel();
  model->addGateAreaEntry(kGateArea, m1());
  model->addMaxCutCAREntry(1.0, contact_);
  model->addMaxAreaCAREntry(1.0, m1());

  const LayerCheck check = layerCheck(gateNet("n1"), m1());
  EXPECT_NEAR(check.car.actual, 1.0 + kPar, 1e-9);
}

}  // namespace
}  // namespace ant::test
