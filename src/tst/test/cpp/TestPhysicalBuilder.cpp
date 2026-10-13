// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"
#include "tst/db_fixture.h"
#include "tst/physical_builder.h"

namespace tst {
namespace {

class PhysicalBuilderTest : public DbFixture
{
 protected:
  void SetUp() override
  {
    tech_ = makeTech(getDb());
    stack_ = makeLayerStack(tech_, 3, {.thickness = 500});
    lib_ = makeLib(getDb(), tech_);
    block_ = makeBlock(getDb(), tech_);
  }

  odb::dbTech* tech_{nullptr};
  LayerStack stack_;
  odb::dbLib* lib_{nullptr};
  odb::dbBlock* block_{nullptr};
};

TEST_F(PhysicalBuilderTest, TechUnits)
{
  EXPECT_EQ(tech_->getDbUnitsPerMicron(), 1000);
  EXPECT_EQ(tech_->getLefUnits(), 1000);
  EXPECT_EQ(block_->getDbUnitsPerMicron(), 1000);
  EXPECT_EQ(block_->getDieArea(), odb::Rect(0, 0, 100000, 100000));
}

TEST_F(PhysicalBuilderTest, LayerStackOrderAndDirections)
{
  ASSERT_EQ(stack_.metals.size(), 3);
  ASSERT_EQ(stack_.cuts.size(), 2);
  ASSERT_EQ(stack_.vias.size(), 2);

  const std::vector<std::string> expected_order
      = {"M1", "V1", "M2", "V2", "M3"};
  std::vector<std::string> order;
  for (odb::dbTechLayer* layer : tech_->getLayers()) {
    order.emplace_back(layer->getName());
  }
  EXPECT_EQ(order, expected_order);

  EXPECT_EQ(stack_.metal(1)->getRoutingLevel(), 1);
  EXPECT_EQ(stack_.metal(3)->getRoutingLevel(), 3);
  EXPECT_EQ(stack_.cut(1)->getType(), odb::dbTechLayerType::CUT);
  EXPECT_EQ(stack_.metal(1)->getDirection(), odb::dbTechLayerDir::HORIZONTAL);
  EXPECT_EQ(stack_.metal(2)->getDirection(), odb::dbTechLayerDir::VERTICAL);
  EXPECT_EQ(stack_.metal(3)->getDirection(), odb::dbTechLayerDir::HORIZONTAL);

  uint32_t thickness = 0;
  EXPECT_TRUE(stack_.metal(2)->getThickness(thickness));
  EXPECT_EQ(thickness, 500);
}

TEST_F(PhysicalBuilderTest, ThicknessUnsetByDefault)
{
  odb::dbTechLayer* layer = addRoutingLayer(tech_, "M4");
  uint32_t thickness = 0;
  EXPECT_FALSE(layer->getThickness(thickness));
}

TEST_F(PhysicalBuilderTest, ViaConnectsAdjacentMetals)
{
  odb::dbTechVia* via = stack_.via(2);
  EXPECT_EQ(via->getName(), "VIA23");
  EXPECT_EQ(via->getBottomLayer(), stack_.metal(2));
  EXPECT_EQ(via->getTopLayer(), stack_.metal(3));

  int cut_boxes = 0;
  for (odb::dbBox* box : via->getBoxes()) {
    if (box->getTechLayer() == stack_.cut(2)) {
      ++cut_boxes;
      EXPECT_EQ(box->getBox(), odb::Rect(-50, -50, 50, 50));
    }
  }
  EXPECT_EQ(cut_boxes, 1);
}

TEST_F(PhysicalBuilderTest, AntennaRuleRoundTrip)
{
  AntennaRuleSpec spec;
  spec.par = 400;
  spec.psr = 100;
  spec.diff_psr = Pwl{{0, 0.1, 1}, {10, 15, 20}};
  spec.diff_par = constantPwl(3);
  spec.area_factor = 3;
  spec.area_factor_diffuse_only = true;
  spec.side_area_factor = 2;
  spec.area_minus_diff = 0.5;
  spec.gate_plus_diff = 0.3;
  const odb::dbTechLayerAntennaRule* rule
      = applyAntennaRule(stack_.metal(1), spec);

  ASSERT_TRUE(stack_.metal(1)->hasDefaultAntennaRule());
  EXPECT_EQ(stack_.metal(1)->getDefaultAntennaRule(), rule);
  EXPECT_DOUBLE_EQ(rule->getPAR(), 400);
  EXPECT_DOUBLE_EQ(rule->getPSR(), 100);
  EXPECT_DOUBLE_EQ(rule->getCAR(), 0);  // unset

  const auto diff_psr = rule->getDiffPSR();
  EXPECT_EQ(diff_psr.indices, (std::vector<double>{0, 0.1, 1}));
  EXPECT_EQ(diff_psr.ratios, (std::vector<double>{10, 15, 20}));
  const auto diff_par = rule->getDiffPAR();
  EXPECT_EQ(diff_par.indices, (std::vector<double>{0}));
  EXPECT_EQ(diff_par.ratios, (std::vector<double>{3}));

  EXPECT_DOUBLE_EQ(rule->getAreaFactor(), 3);
  EXPECT_TRUE(rule->isAreaFactorDiffUseOnly());
  EXPECT_DOUBLE_EQ(rule->getSideAreaFactor(), 2);
  EXPECT_FALSE(rule->isSideAreaFactorDiffUseOnly());
  EXPECT_DOUBLE_EQ(rule->getAreaMinusDiffFactor(), 0.5);
  EXPECT_DOUBLE_EQ(rule->getGatePlusDiffFactor(), 0.3);
}

TEST_F(PhysicalBuilderTest, AntennaRuleUpdatesExistingRule)
{
  applyAntennaRule(stack_.metal(2), {.par = 100});
  applyAntennaRule(stack_.metal(2), {.psr = 50});
  const odb::dbTechLayerAntennaRule* rule
      = stack_.metal(2)->getDefaultAntennaRule();
  EXPECT_DOUBLE_EQ(rule->getPAR(), 100);
  EXPECT_DOUBLE_EQ(rule->getPSR(), 50);
}

TEST_F(PhysicalBuilderTest, AntennaRuleKeepsExistingTables)
{
  applyAntennaRule(stack_.metal(1), {.diff_psr = Pwl{{0, 1}, {10, 20}}});
  applyAntennaRule(stack_.metal(1), {.psr = 50});
  const auto diff_psr = stack_.metal(1)->getDefaultAntennaRule()->getDiffPSR();
  EXPECT_EQ(diff_psr.ratios, (std::vector<double>{10, 20}));
}

TEST_F(PhysicalBuilderTest, AntennaRuleRejectsReplacingTable)
{
  applyAntennaRule(stack_.metal(1), {.diff_psr = constantPwl(10)});
  EXPECT_THROW(applyAntennaRule(stack_.metal(1), {.diff_psr = constantPwl(20)}),
               std::invalid_argument);
  // A different table that is still unset can be added.
  EXPECT_NO_THROW(applyAntennaRule(
      stack_.metal(1), {.area_diff_reduce = Pwl{{0, 1}, {1, 0.5}}}));
}

TEST_F(PhysicalBuilderTest, MasterPinsAndAntennaAreas)
{
  odb::dbMaster* buf = makeMaster(lib_,
                                  "BUF",
                                  1000,
                                  2000,
                                  {{.name = "A",
                                    .layer = stack_.metal(1),
                                    .rect = {100, 100, 200, 300},
                                    .gate_area = 0.25},
                                   {.name = "X",
                                    .io_type = odb::dbIoType::OUTPUT,
                                    .layer = stack_.metal(1),
                                    .rect = {800, 100, 900, 300},
                                    .diff_area = 0.5}});

  EXPECT_TRUE(buf->isFrozen());
  EXPECT_EQ(buf->getWidth(), 1000);
  EXPECT_EQ(buf->getHeight(), 2000);
  EXPECT_EQ(buf->getType(), odb::dbMasterType::CORE);

  odb::dbMTerm* a = buf->findMTerm("A");
  ASSERT_NE(a, nullptr);
  EXPECT_EQ(a->getIoType(), odb::dbIoType::INPUT);
  EXPECT_EQ(a->getBBox(), odb::Rect(100, 100, 200, 300));
  ASSERT_TRUE(a->hasDefaultAntennaModel());
  std::vector<std::pair<double, odb::dbTechLayer*>> gate_areas;
  a->getDefaultAntennaModel()->getGateArea(gate_areas);
  ASSERT_EQ(gate_areas.size(), 1);
  EXPECT_DOUBLE_EQ(gate_areas[0].first, 0.25);

  odb::dbMTerm* x = buf->findMTerm("X");
  ASSERT_NE(x, nullptr);
  EXPECT_EQ(x->getIoType(), odb::dbIoType::OUTPUT);
  EXPECT_FALSE(x->hasDefaultAntennaModel());
  std::vector<std::pair<double, odb::dbTechLayer*>> diff_areas;
  x->getDiffArea(diff_areas);
  ASSERT_EQ(diff_areas.size(), 1);
  EXPECT_DOUBLE_EQ(diff_areas[0].first, 0.5);
}

// Collects (layer, is_via, box) for each shape of a wire.
struct WireShape
{
  odb::dbTechLayer* layer;
  bool is_via;
  odb::Rect box;
};

std::vector<WireShape> wireShapes(odb::dbWire* wire)
{
  std::vector<WireShape> shapes;
  odb::dbWireShapeItr itr;
  odb::dbShape shape;
  for (itr.begin(wire); itr.next(shape);) {
    if (shape.isVia()) {
      shapes.push_back(
          {shape.getTechVia()->getBottomLayer(), true, shape.getBox()});
    } else {
      shapes.push_back({shape.getTechLayer(), false, shape.getBox()});
    }
  }
  return shapes;
}

TEST_F(PhysicalBuilderTest, RoutePathSegmentsAndVias)
{
  odb::dbNet* net = odb::dbNet::create(block_, "n1");
  RoutePath(net)
      .on(stack_.metal(1), {0, 0})
      .to({5000, 0})
      .via(stack_.via(1))
      .to({5000, 8000})
      .on(stack_.metal(1), {9000, 0})
      .to({12000, 0})
      .build();

  EXPECT_EQ(net->getWireType(), odb::dbWireType::ROUTED);
  ASSERT_NE(net->getWire(), nullptr);
  const std::vector<WireShape> shapes = wireShapes(net->getWire());
  ASSERT_EQ(shapes.size(), 4);

  EXPECT_EQ(shapes[0].layer, stack_.metal(1));
  EXPECT_FALSE(shapes[0].is_via);
  EXPECT_EQ(shapes[0].box, odb::Rect(-50, -50, 5050, 50));

  EXPECT_TRUE(shapes[1].is_via);
  EXPECT_EQ(shapes[1].layer, stack_.metal(1));

  EXPECT_EQ(shapes[2].layer, stack_.metal(2));
  EXPECT_FALSE(shapes[2].is_via);
  EXPECT_EQ(shapes[2].box, odb::Rect(4950, -50, 5050, 8050));

  EXPECT_EQ(shapes[3].layer, stack_.metal(1));
  EXPECT_EQ(shapes[3].box, odb::Rect(8950, -50, 12050, 50));
}

TEST_F(PhysicalBuilderTest, RoutePathViaDownSwitchesToBottomLayer)
{
  odb::dbNet* net = odb::dbNet::create(block_, "n1");
  RoutePath(net)
      .on(stack_.metal(2), {0, 0})
      .to({0, 4000})
      .via(stack_.via(1))
      .to({3000, 4000});

  const std::vector<WireShape> shapes = wireShapes(net->getWire());
  ASSERT_EQ(shapes.size(), 3);
  EXPECT_EQ(shapes[0].layer, stack_.metal(2));
  EXPECT_TRUE(shapes[1].is_via);
  EXPECT_EQ(shapes[2].layer, stack_.metal(1));
}

TEST_F(PhysicalBuilderTest, RoutePathReplacesExistingWire)
{
  odb::dbNet* net = odb::dbNet::create(block_, "n1");
  RoutePath(net).on(stack_.metal(1), {0, 0}).to({5000, 0});
  RoutePath(net).on(stack_.metal(3), {0, 0}).to({1000, 0});

  const std::vector<WireShape> shapes = wireShapes(net->getWire());
  ASSERT_EQ(shapes.size(), 1);
  EXPECT_EQ(shapes[0].layer, stack_.metal(3));
}

TEST_F(PhysicalBuilderTest, RoutePathRejectsDiagonalStep)
{
  odb::dbNet* net = odb::dbNet::create(block_, "n1");
  RoutePath path(net);
  path.on(stack_.metal(1), {0, 0});
  EXPECT_THROW(path.to({100, 100}), std::invalid_argument);
}

TEST_F(PhysicalBuilderTest, RoutePathRejectsUnconnectedVia)
{
  odb::dbNet* net = odb::dbNet::create(block_, "n1");
  RoutePath path(net);
  path.on(stack_.metal(1), {0, 0});
  EXPECT_THROW(path.via(stack_.via(2)), std::invalid_argument);
}

TEST_F(PhysicalBuilderTest, Guide)
{
  odb::dbNet* net = odb::dbNet::create(block_, "n1");
  odb::dbGuide* guide = addGuide(net, stack_.metal(2), {0, 0, 1000, 5000});
  ASSERT_NE(guide, nullptr);
  EXPECT_EQ(net->getGuides().size(), 1);
  EXPECT_EQ(guide->getLayer(), stack_.metal(2));
  EXPECT_EQ(guide->getViaLayer(), stack_.metal(2));
  EXPECT_EQ(guide->getBox(), odb::Rect(0, 0, 1000, 5000));

  odb::dbGuide* via_guide
      = addGuide(net, stack_.metal(2), {0, 0, 1000, 1000}, stack_.metal(3));
  EXPECT_EQ(via_guide->getViaLayer(), stack_.metal(3));
  EXPECT_EQ(net->getGuides().size(), 2);
}

class ReadDefTest : public DbFixture
{
 protected:
  static std::string writeFile(const std::string& name,
                               const std::string& content)
  {
    const std::string path = ::testing::TempDir() + name;
    std::ofstream(path) << content;
    return path;
  }
};

TEST_F(ReadDefTest, LefAndDefWithoutSta)
{
  const std::string lef = writeFile("tst_read_def.lef", R"(
VERSION 5.8 ;
UNITS DATABASE MICRONS 1000 ; END UNITS
LAYER M1 TYPE ROUTING ; DIRECTION HORIZONTAL ; WIDTH 0.1 ; PITCH 0.2 ; END M1
MACRO BUF
  CLASS CORE ;
  SIZE 1 BY 2 ;
  PIN A DIRECTION INPUT ;
    PORT LAYER M1 ; RECT 0.1 0.1 0.2 0.3 ; END
  END A
END BUF
END LIBRARY
)");
  const std::string def = writeFile("tst_read_def.def", R"(
VERSION 5.8 ;
DESIGN top ;
UNITS DISTANCE MICRONS 1000 ;
DIEAREA ( 0 0 ) ( 10000 10000 ) ;
COMPONENTS 1 ;
  - u1 BUF + PLACED ( 1000 1000 ) N ;
END COMPONENTS
NETS 1 ;
  - n1 ( u1 A ) ;
END NETS
END DESIGN
)");

  odb::dbLib* lib = loadTechAndLibLef("tech", "lib", lef);
  ASSERT_NE(lib, nullptr);
  ASSERT_NE(lib->findMaster("BUF"), nullptr);

  odb::dbBlock* block = readDef(def);
  ASSERT_NE(block, nullptr);
  EXPECT_EQ(block->getName(), "top");
  odb::dbInst* u1 = block->findInst("u1");
  ASSERT_NE(u1, nullptr);
  EXPECT_TRUE(u1->isPlaced());
  EXPECT_EQ(u1->getLocation(), odb::Point(1000, 1000));
  odb::dbNet* n1 = block->findNet("n1");
  ASSERT_NE(n1, nullptr);
  EXPECT_EQ(n1->getITermCount(), 1);
}

}  // namespace
}  // namespace tst
