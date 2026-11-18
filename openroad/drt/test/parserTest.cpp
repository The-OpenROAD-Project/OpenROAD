// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <utility>

#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
#include "fixture.h"
#include "io/io.h"

namespace drt {
namespace {

class DefaultViaTest : public Fixture
{
 protected:
  void SetUp() override
  {
    auto tech = design->getTech();
    addLayer(tech, "v1", odb::dbTechLayerType::CUT);
    addLayer(tech,
             "m2",
             odb::dbTechLayerType::ROUTING,
             odb::dbTechLayerDir::VERTICAL);
    router_cfg->BOTTOM_ROUTING_LAYER = 4;
    router_cfg->TOP_ROUTING_LAYER = 4;
    router_cfg->ENABLE_VIA_GEN = true;
  }

  frViaDef* makeDefaultCandidate(bool aligned = false)
  {
    auto via = std::make_unique<frViaDef>("native");
    auto lower = std::make_unique<frRect>();
    lower->setLayerNum(2);
    lower->setBBox(aligned ? odb::Rect(-20, -10, 20, 10)
                           : odb::Rect(-10, -20, 10, 20));
    via->addLayer1Fig(std::move(lower));
    auto upper = std::make_unique<frRect>();
    upper->setLayerNum(4);
    upper->setBBox(odb::Rect(-10, -20, 10, 20));
    via->addLayer2Fig(std::move(upper));
    auto cut = std::make_unique<frRect>();
    cut->setLayerNum(3);
    cut->setBBox(odb::Rect(-10, -10, 10, 10));
    via->addCutFig(std::move(cut));
    return design->getTech()->addVia(std::move(via));
  }

  void initDefaultVias()
  {
    io::Parser parser(
        db_tech->getDb(), design.get(), logger.get(), router_cfg.get());
    parser.initDefaultVias();
    EXPECT_EQ(router_cfg->BOTTOM_ROUTING_LAYER, 4);
    EXPECT_EQ(router_cfg->TOP_ROUTING_LAYER, 4);
  }
};

TEST_F(DefaultViaTest, AlignBelowRoutingFloor)
{
  const auto native = makeDefaultCandidate();
  initDefaultVias();
  const auto aligned = design->getTech()->getLayer(3)->getDefaultViaDef();
  ASSERT_NE(aligned, nullptr);
  EXPECT_NE(aligned, native);
  EXPECT_TRUE(aligned->isAddedByRouter());
  const frVia via(aligned);
  EXPECT_EQ(via.getLayer1BBox(), odb::Rect(-20, -10, 20, 10));
  EXPECT_EQ(via.getLayer2BBox(), odb::Rect(-10, -20, 10, 20));
  EXPECT_EQ(via.getCutBBox(), odb::Rect(-10, -10, 10, 10));
}

TEST_F(DefaultViaTest, RetainAlignedBelowRoutingFloor)
{
  const auto native = makeDefaultCandidate(true);
  initDefaultVias();
  EXPECT_EQ(design->getTech()->getLayer(3)->getDefaultViaDef(), native);
}

TEST_F(DefaultViaTest, RetainBelowRoutingFloorWhenGenerationDisabled)
{
  router_cfg->ENABLE_VIA_GEN = false;
  const auto native = makeDefaultCandidate();
  initDefaultVias();
  EXPECT_EQ(design->getTech()->getLayer(3)->getDefaultViaDef(), native);
}

TEST_F(DefaultViaTest, RetainAboveRoutingCeiling)
{
  router_cfg->BOTTOM_ROUTING_LAYER = 2;
  router_cfg->TOP_ROUTING_LAYER = 2;
  makeDefaultCandidate();
  io::Parser parser(
      db_tech->getDb(), design.get(), logger.get(), router_cfg.get());
  parser.initDefaultVias();
  EXPECT_EQ(design->getTech()->getLayer(3)->getDefaultViaDef(),
            design->getTech()->getVia("native"));
  EXPECT_EQ(router_cfg->BOTTOM_ROUTING_LAYER, 2);
  EXPECT_EQ(router_cfg->TOP_ROUTING_LAYER, 2);
}

TEST_F(DefaultViaTest, MissingSingleCutBelowRoutingFloor)
{
  auto via = makeDefaultCandidate();
  auto cut = std::make_unique<frRect>();
  cut->setLayerNum(3);
  cut->setBBox(odb::Rect(20, -10, 40, 10));
  via->addCutFig(std::move(cut));
  initDefaultVias();
  EXPECT_EQ(design->getTech()->getLayer(3)->getDefaultViaDef(), nullptr);
}

TEST_F(DefaultViaTest, MissingViasBelowRoutingFloor)
{
  initDefaultVias();
  EXPECT_EQ(design->getTech()->getLayer(3)->getDefaultViaDef(), nullptr);
}

}  // namespace
}  // namespace drt
