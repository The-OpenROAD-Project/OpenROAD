// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <limits>
#include <map>
#include <memory>
#include <vector>

#include "boost/polygon/polygon.hpp"
#include "db/obj/frBPin.h"
#include "db/obj/frBTerm.h"
#include "fixture.h"
#include "gtest/gtest.h"
#include "pa/FlexPA.h"

namespace drt {

class ViaInPinTest : public Fixture
{
 protected:
  void SetUp() override
  {
    addLayer(design->getTech(), "v1", odb::dbTechLayerType::CUT);
    addLayer(design->getTech(),
             "m2",
             odb::dbTechLayerType::ROUTING,
             odb::dbTechLayerDir::VERTICAL);
    router_cfg->BOTTOM_ROUTING_LAYER = 2;
    router_cfg->TOP_ROUTING_LAYER = 4;
    pa_ = std::make_unique<FlexPA>(
        design.get(), logger.get(), nullptr, router_cfg.get());
  }

  gtl::polygon_90_set_data<frCoord> pinShapes(const odb::Rect& box)
  {
    gtl::polygon_90_set_data<frCoord> shapes;
    shapes.insert(gtl::rectangle_data<frCoord>(
        box.xMin(), box.yMin(), box.xMax(), box.yMax()));
    return shapes;
  }

  frViaDef* makeAsymmetricVia()
  {
    auto via = std::make_unique<frViaDef>("asymmetric");
    auto lower = std::make_unique<frRect>();
    lower->setLayerNum(2);
    lower->setBBox(odb::Rect(-400, -100, 100, 100));
    via->addLayer1Fig(std::move(lower));
    auto upper = std::make_unique<frRect>();
    upper->setLayerNum(4);
    upper->setBBox(odb::Rect(-100, -100, 100, 100));
    via->addLayer2Fig(std::move(upper));
    auto cut = std::make_unique<frRect>();
    cut->setLayerNum(3);
    cut->setBBox(odb::Rect(-50, -50, 50, 50));
    via->addCutFig(std::move(cut));
    return design->getTech()->addVia(std::move(via));
  }

  frCoord maxExtension(frAccessPoint& ap,
                       const odb::Rect& pin,
                       const frViaDef* via,
                       frInstTerm* inst_term = nullptr)
  {
    return pa_->viaMaxExt(inst_term, &ap, pinShapes(pin), via);
  }

  void filter(frAccessPoint& ap,
              const odb::Rect& box,
              const frViaDef* via,
              const bool required)
  {
    ap.setAllowVia(true);
    router_cfg->VIAINPIN_BOTTOMLAYERNUM = required ? 2 : 6;
    router_cfg->VIAINPIN_TOPLAYERNUM = required ? 4 : 6;
    pa_->layer_num_to_via_defs_[3][1][ViaRawPriorityTuple{}] = via;
    auto term = std::make_unique<frBTerm>("pin");
    auto pin = std::make_unique<frBPin>();
    auto* pin_ptr = pin.get();
    auto rect = std::make_unique<frRect>();
    rect->setBBox(box);
    rect->setLayerNum(ap.getLayerNum());
    rect->addToPin(pin_ptr);
    pin->addPinFig(std::move(rect));
    term->addPin(std::move(pin));
    auto* net = makeNet("net");
    term->addToNet(net);
    net->addBTerm(term.get());
    design->getTopBlock()->addTerm(std::move(term));
    initRegionQuery();
    const auto shapes = pinShapes(box);
    std::vector<gtl::polygon_90_data<frCoord>> polygons;
    shapes.get_polygons(polygons);
    pa_->filterViaAccess(&ap, polygons, shapes, pin_ptr, nullptr);
  }

  std::unique_ptr<FlexPA> pa_;
};

struct ContactCase
{
  odb::Rect pin;
  frCoord cost;
};

class ViaContactTest : public ViaInPinTest,
                       public testing::WithParamInterface<ContactCase>
{
};

TEST_P(ViaContactTest, MaximumExtension)
{
  const auto& test = GetParam();
  const auto* via = makeViaDef("via", 3, {-100, -100}, {100, 100});
  frAccessPoint ap({0, 0}, 2);
  EXPECT_EQ(maxExtension(ap, test.pin, via), test.cost);
}

INSTANTIATE_TEST_SUITE_P(
    Contact,
    ViaContactTest,
    testing::Values(ContactCase{odb::Rect(-100, -100, 100, 100), 0},
                    ContactCase{odb::Rect(-50, -100, 100, 100), 50},
                    ContactCase{odb::Rect(100, -100, 200, 100),
                                std::numeric_limits<frCoord>::max()},
                    ContactCase{odb::Rect(100, 100, 200, 200),
                                std::numeric_limits<frCoord>::max()},
                    ContactCase{odb::Rect(200, -100, 300, 100),
                                std::numeric_limits<frCoord>::max()}));

TEST_F(ViaInPinTest, DownUsesUpperLanding)
{
  const auto* via = makeAsymmetricVia();
  frAccessPoint ap({0, 0}, 4);
  EXPECT_EQ(maxExtension(ap, odb::Rect(-100, -100, 100, 100), via), 0);
  EXPECT_EQ(maxExtension(ap, odb::Rect(-50, -100, 100, 100), via), 50);
  filter(ap, odb::Rect(-100, -100, 100, 100), via, true);
  EXPECT_TRUE(ap.hasAccess(frDirEnum::D));
}

TEST_F(ViaInPinTest, UpUsesLowerLanding)
{
  const auto* via = makeAsymmetricVia();
  frAccessPoint ap({0, 0}, 2);
  EXPECT_EQ(maxExtension(ap, odb::Rect(-400, -100, -200, 100), via), 300);
}

TEST_F(ViaInPinTest, RequiredEnclosureRejectsEdgeContact)
{
  const auto* via = makeViaDef("via", 3, {-100, -100}, {100, 100});
  frAccessPoint ap({0, 0}, 2);
  filter(ap, odb::Rect(100, -100, 300, 100), via, true);
  EXPECT_FALSE(ap.hasViaDef());
}

TEST_F(ViaInPinTest, OptionalEnclosureRanksEdgeContactLast)
{
  const auto* edge = makeViaDef("edge", 3, {-100, -100}, {100, 100});
  const auto* partial = makeViaDef("partial", 3, {50, -100}, {250, 100});
  const auto* enclosed = makeViaDef("enclosed", 3, {100, -100}, {300, 100});
  frAccessPoint ap({0, 0}, 2);
  const odb::Rect pin(100, -100, 300, 100);
  filter(ap, pin, edge, false);
  ASSERT_TRUE(ap.hasViaDef());
  ap.addViaDef(partial);
  ap.addViaDef(enclosed);
  std::map<const frViaDef*, int> costs;
  for (const auto* via : {edge, partial, enclosed}) {
    costs[via] = maxExtension(ap, pin, via);
  }
  ap.sortViaDefs(costs);
  EXPECT_EQ(ap.getViaDef(1, 0), enclosed);
  EXPECT_EQ(ap.getViaDef(1, 1), partial);
  EXPECT_EQ(ap.getViaDef(1, 2), edge);
}

TEST_F(ViaInPinTest, SideBoundaryIgnoresOnlyYExtension)
{
  const auto [master, db_master] = makeMacro("cell", -1000, -1000, 1000, 1000);
  auto* inst = makeInst("inst", master, db_master);
  frInstTerm term(inst, nullptr);
  const auto* via = makeViaDef("via", 3, {-100, -100}, {100, 100});
  frAccessPoint ap({-900, 0}, 2);
  EXPECT_EQ(maxExtension(ap, odb::Rect(-1000, -50, -800, 50), via, &term), 0);
  EXPECT_EQ(maxExtension(ap, odb::Rect(-1000, -50, -800, 50), via), 50);
  EXPECT_EQ(maxExtension(ap, odb::Rect(-950, -50, -800, 50), via, &term), 50);
  EXPECT_EQ(maxExtension(ap, odb::Rect(-800, -100, -600, 100), via, &term),
            std::numeric_limits<frCoord>::max());
}

}  // namespace drt
