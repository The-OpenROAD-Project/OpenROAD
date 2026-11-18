// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "db/obj/frAccess.h"
#include "db/obj/frInstTerm.h"
#include "db/obj/frTrackPattern.h"
#include "fixture.h"
#include "frRegionQuery.h"
#include "gtest/gtest.h"
#include "pa/AbstractPAGraphics.h"
#include "pa/FlexPA.h"

namespace drt {
namespace {

struct ViaProbe
{
  odb::Point point;
  const frViaDef* via;
  bool legal;
};

class PinAccessObserver : public AbstractPAGraphics
{
 public:
  std::vector<std::unique_ptr<frAccessPoint>> native_aps;
  std::vector<ViaProbe> supplemental;
  std::set<odb::Point> rejected_native_origins;

  void startPin(frBPin*, frInstTerm*, UniqueClass*) override {}
  void startPin(frMPin*, frInstTerm*, UniqueClass*) override {}
  void setAPs(const std::vector<std::unique_ptr<frAccessPoint>>& aps,
              frAccessPointEnum,
              frAccessPointEnum) override
  {
    for (const auto& probe : supplemental) {
      if (!probe.legal) {
        rejected_native_origins.insert(probe.point);
      }
    }
    supplemental.clear();
    for (const auto& ap : aps) {
      if (ap->hasAccess(frDirEnum::U)) {
        native_aps.push_back(std::make_unique<frAccessPoint>(*ap));
      }
    }
  }
  void setViaAP(const frAccessPoint* ap,
                const frVia* via,
                const std::vector<std::unique_ptr<frMarker>>& markers) override
  {
    supplemental.push_back({ap->getPoint(), via->getViaDef(), markers.empty()});
  }
  void setPlanarAP(const frAccessPoint*,
                   const frPathSeg*,
                   const std::vector<std::unique_ptr<frMarker>>&) override
  {
  }
  void setObjsAndMakers(
      const std::vector<std::pair<frConnFig*, frBlockObject*>>&,
      const std::vector<std::unique_ptr<frMarker>>&,
      FlexPA::PatternType) override
  {
  }
};

}  // namespace

class FlexPASparseViaTest : public Fixture
{
 protected:
  void SetUp() override
  {
    auto* tech = design->getTech();
    addLayer(tech, "v1", odb::dbTechLayerType::CUT);
    addLayer(tech,
             "m2",
             odb::dbTechLayerType::ROUTING,
             odb::dbTechLayerDir::VERTICAL);
    router_cfg->BOTTOM_ROUTING_LAYER = 2;
    router_cfg->TOP_ROUTING_LAYER = 4;
    router_cfg->VIA_ACCESS_LAYERNUM = 2;
    router_cfg->MINNUMACCESSPOINT_STDCELLPIN = 2;
    std::tie(master_, db_master_) = makeMacro("cell", 0, 0, 2000, 2000);
    auto* term = makeMacroPin(master_, "a", 450, 200, 550, 1800);
    pin_ = static_cast<frMTerm*>(term)->getPins().front().get();
    addTrack(2, false, 100, 2, 1800);
    addTrack(4, true, 500, 1, 200);
    addVia("large1", {-150, -150, 150, 150}, {-10, -10, 10, 10}, true);
    addVia("large2", {-200, -200, 200, 200}, {-10, -10, 10, 10}, true);
    block({350, 0, 430, 2000});
  }

  void addTrack(frLayerNum layer,
                bool vertical,
                int start,
                int count,
                int pitch)
  {
    design->getTopBlock()->addTrackPattern(
        std::make_unique<frTrackPattern>(vertical, start, count, pitch, layer));
  }

  frViaDef* addVia(const char* name,
                   const odb::Rect& lower,
                   const odb::Rect& cut = {-10, -10, 10, 10},
                   bool is_default = false,
                   const odb::Rect& upper = {-50, -50, 50, 50})
  {
    auto via = std::make_unique<frViaDef>(name);
    for (int layer = 2; layer <= 4; ++layer) {
      auto fig = std::make_unique<frRect>();
      fig->setLayerNum(layer);
      fig->setBBox(layer == 2 ? lower : layer == 3 ? cut : upper);
      if (layer == 2) {
        via->addLayer1Fig(std::move(fig));
      } else if (layer == 3) {
        via->addCutFig(std::move(fig));
      } else {
        via->addLayer2Fig(std::move(fig));
      }
    }
    via->setDefault(is_default);
    auto* result = design->getTech()->addVia(std::move(via));
    design->getTech()->getLayer(3)->addViaDef(result);
    return result;
  }

  void block(const odb::Rect& rect, int layer = 2)
  {
    makeMacroObs(
        master_, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax(), layer);
  }

  void blockNativeOrigin() { block({440, 950, 560, 1050}, 4); }

  void setUpperWidth(int width)
  {
    auto* tech = design->getTech();
    auto* layer = tech->getLayer(4);
    layer->setWidth(width);
    layer->setMinWidth(width);
    auto min_width = std::make_unique<frMinWidthConstraint>(width);
    layer->setMinWidthConstraint(min_width.get());
    tech->addUConstraint(std::move(min_width));
  }

  void mapVia(const frViaDef* via)
  {
    auto* rule = odb::dbMetalWidthViaMap::create(db_tech);
    rule->setCutLayer(design->getTech()->getLayer(3)->getDbLayer());
    rule->setBelowLayerWidthLow(0);
    rule->setBelowLayerWidthHigh(2000);
    rule->setAboveLayerWidthLow(0);
    rule->setAboveLayerWidthHigh(2000);
    rule->setViaName(via->getName());
  }

  void run(bool stubborn = false,
           const frViaDef* avoid = nullptr,
           bool ndr = false)
  {
    auto* inst = makeInst("u", master_, db_master_);
    inst_term_ = inst->getInstTerms().front().get();
    inst_term_->setStubborn(stubborn);
    if (ndr) {
      auto* net = makeNet("n");
      net->updateNondefaultRule(&ndr_);
      inst_term_->addToNet(net);
    }
    initRegionQuery();
    pa_ = std::make_unique<FlexPA>(
        design.get(), logger.get(), nullptr, router_cfg.get());
    if (avoid) {
      pa_->addAvoidViaDef(avoid);
    }
    pa_->init();
    auto observer = std::make_unique<PinAccessObserver>();
    observer_ = observer.get();
    pa_->setDebug(std::move(observer));
    representatives_ = pa_->getPriorityViaDefs(3, inst_term_, 100);
    pa_->genPinAccess(pin_, inst_term_);
    EXPECT_EQ(pa_->getPriorityViaDefs(3, inst_term_, 100), representatives_);
  }

  const auto& aps() const { return pin_->getPinAccess(0)->getAccessPoints(); }

  bool nativeSufficient()
  {
    FlexPA::pa_requirements_met reqs;
    return pa_->EnoughAccessPoints(observer_->native_aps, inst_term_, reqs);
  }

  void expectRecovered()
  {
    FlexPA::pa_requirements_met reqs;
    EXPECT_FALSE(
        pa_->EnoughAccessPoints(observer_->native_aps, inst_term_, reqs));
    std::vector<std::unique_ptr<frAccessPoint>> copies;
    for (const auto& ap : aps()) {
      copies.push_back(std::make_unique<frAccessPoint>(*ap));
      EXPECT_TRUE(ap->hasAccess(frDirEnum::U));
      EXPECT_FALSE(ap->hasPlanarAccess());
      EXPECT_EQ(ap->getType(false), frAccessPointEnum::OnGrid);
      EXPECT_EQ(ap->getPoint().x(), 500);
      EXPECT_EQ(ap->getPoint().y() % 10, 0);
      const frVia via(ap->getViaDefs().front(), ap->getPoint());
      EXPECT_TRUE(inst_term_->getInst()->getBoundaryBBox().contains(
          via.getLayer1BBox()));
      EXPECT_TRUE(inst_term_->getInst()->getBoundaryBBox().contains(
          via.getLayer2BBox()));
      EXPECT_TRUE(odb::Rect(450, 200, 550, 1800).contains(via.getLayer1BBox()));
    }
    EXPECT_TRUE(pa_->EnoughAccessPoints(copies, inst_term_, reqs));
    EXPECT_FALSE(observer_->supplemental.empty());
    EXPECT_LE(observer_->supplemental.size(), 256);
  }

  frMaster* master_ = nullptr;
  odb::dbMaster* db_master_ = nullptr;
  frMPin* pin_ = nullptr;
  frInstTerm* inst_term_ = nullptr;
  std::unique_ptr<FlexPA> pa_;
  PinAccessObserver* observer_ = nullptr;
  std::vector<const frViaDef*> representatives_;
  frNonDefaultRule ndr_;
};

TEST_F(FlexPASparseViaTest, RecoversTwoSparseOriginsOnOnePreferredTrack)
{
  addVia("small", {-50, -50, 50, 50});
  blockNativeOrigin();
  run();
  expectRecovered();
  ASSERT_EQ(aps().size(), 2);
  EXPECT_GT(std::abs(aps()[0]->getPoint().y() - aps()[1]->getPoint().y()), 50);
  for (const auto& ap : aps()) {
    EXPECT_EQ(ap->getType(true), frAccessPointEnum::EncOpt);
  }
}

TEST_F(FlexPASparseViaTest, TriesEqualPriorityIdentityAfterRejectedIdentity)
{
  const auto* legal = addVia("B_legal",
                             {-50, -50, 50, 50},
                             {-40, -10, -20, 10},
                             false,
                             {-150, -50, 50, 50});
  const auto* blocked = addVia("A_blocked",
                               {-50, -50, 50, 50},
                               {20, -10, 40, 10},
                               false,
                               {-50, -50, 150, 50});
  block({560, 0, 600, 2000}, 4);
  run();
  expectRecovered();
  EXPECT_EQ(std::ranges::find(representatives_, legal), representatives_.end());
  EXPECT_NE(std::ranges::find(representatives_, blocked),
            representatives_.end());
  EXPECT_TRUE(std::ranges::any_of(aps(), [&](const auto& ap) {
    return observer_->rejected_native_origins.contains(ap->getPoint());
  }));
  for (const auto& ap : aps()) {
    EXPECT_EQ(ap->getViaDefs().front(), legal);
    EXPECT_TRUE(
        std::ranges::any_of(observer_->supplemental, [&](const auto& probe) {
          return probe.point == ap->getPoint() && probe.via == blocked
                 && !probe.legal;
        }));
  }
}

TEST_F(FlexPASparseViaTest, FindsInteriorWhenEndpointsAreBlocked)
{
  addVia("small",
         {-50, -40, 50, 60},
         {-10, -10, 10, 10},
         false,
         {-10, -10, 10, 10});
  setUpperWidth(20);
  router_cfg->MINNUMACCESSPOINT_STDCELLPIN = 1;
  block({480, 230, 520, 250}, 4);
  block({480, 1730, 520, 1750}, 4);
  block({480, 1005, 520, 1015}, 4);
  run();
  expectRecovered();
  ASSERT_EQ(aps().size(), 1);
  EXPECT_GT(aps().front()->getPoint().y(), 250);
  EXPECT_LT(aps().front()->getPoint().y(), 1730);
}

TEST_F(FlexPASparseViaTest, DenseSamplingFindsAnInteriorBeyondTheMidpoint)
{
  static_cast<frRect*>(pin_->getFigs().front().get())
      ->setBBox({450, 200, 550, 400});
  addVia("B_legal",
         {-50, -50, 50, 50},
         {-40, -10, -20, 10},
         false,
         {-150, -10, 50, 10});
  addVia("A_blocked",
         {-50, -50, 50, 50},
         {20, -10, 40, 10},
         false,
         {-50, -10, 150, 10});
  setUpperWidth(20);
  router_cfg->MINNUMACCESSPOINT_STDCELLPIN = 1;
  block({560, 0, 600, 2000}, 4);
  for (const int y : {250, 300, 350}) {
    block({350, y - 5, 400, y + 5}, 4);
  }
  run();
  expectRecovered();
  ASSERT_EQ(aps().size(), 1);
  EXPECT_GT(aps().front()->getPoint().y(), 260);
  EXPECT_LT(aps().front()->getPoint().y(), 290);
}

TEST_F(FlexPASparseViaTest, SufficientNativeAccessNeedsNoSupplementalChecks)
{
  addVia("small", {-50, -50, 50, 50}, {-10, -10, 10, 10}, true);
  addTrack(2, false, 500, 2, 1000);
  run();
  EXPECT_TRUE(nativeSufficient());
  EXPECT_EQ(aps().size(), observer_->native_aps.size());
  EXPECT_TRUE(observer_->supplemental.empty());
  for (size_t i = 0; i < aps().size(); ++i) {
    EXPECT_EQ(aps()[i]->getPoint(), observer_->native_aps[i]->getPoint());
    EXPECT_EQ(aps()[i]->getViaDefs(), observer_->native_aps[i]->getViaDefs());
    EXPECT_EQ(aps()[i]->getType(true), observer_->native_aps[i]->getType(true));
  }
}

struct SparseControl
{
  bool mapped;
  bool stubborn;
  bool ndr;
  bool recovered;
};

class SparseViaControlTest : public FlexPASparseViaTest,
                             public testing::WithParamInterface<SparseControl>
{
};

TEST_P(SparseViaControlTest, RespectsViaSelectionControls)
{
  const auto control = GetParam();
  const auto* small = addVia("small", {-50, -50, 50, 50});
  if (control.mapped) {
    mapVia(design->getTech()->getVia("large1"));
  }
  blockNativeOrigin();
  run(control.stubborn, small, control.ndr);
  if (control.recovered) {
    expectRecovered();
  } else {
    EXPECT_TRUE(aps().empty());
    EXPECT_TRUE(observer_->supplemental.empty());
  }
}

INSTANTIATE_TEST_SUITE_P(
    Selection,
    SparseViaControlTest,
    testing::Values(SparseControl{false, false, false, true},
                    SparseControl{true, false, false, false},
                    SparseControl{false, true, false, false},
                    SparseControl{false, false, true, false}));

TEST_F(FlexPASparseViaTest, AllBlockedSearchHasBoundedDirectionalChecks)
{
  for (int i = 0; i < 100; ++i) {
    const auto name = "small" + std::to_string(i);
    addVia(name.c_str(), {-50, -50, 50, 50});
  }
  block({440, 0, 560, 2000}, 4);
  run();
  EXPECT_TRUE(aps().empty());
  EXPECT_FALSE(observer_->supplemental.empty());
  EXPECT_LE(observer_->supplemental.size(), 256);
  EXPECT_TRUE(std::ranges::none_of(
      observer_->supplemental, [](const auto& probe) { return probe.legal; }));
}

TEST_F(FlexPASparseViaTest,
       VisitsAnotherRectangleBeforeExhaustingBlockedAlternatives)
{
  router_cfg->MINNUMACCESSPOINT_STDCELLPIN = 1;
  auto fig = std::make_unique<frRect>();
  fig->setLayerNum(2);
  fig->setBBox({1450, 200, 1550, 1800});
  pin_->addPinFig(std::move(fig));
  addTrack(4, true, 1500, 1, 200);
  for (int i = 0; i < 100; ++i) {
    const auto name = "small" + std::to_string(i);
    addVia(name.c_str(), {-50, -50, 50, 50});
  }
  block({440, 0, 560, 2000}, 4);
  block({1350, 0, 1430, 2000});
  block({1440, 950, 1560, 1050}, 4);
  run();
  EXPECT_FALSE(nativeSufficient());
  ASSERT_EQ(aps().size(), 1);
  for (const auto& ap : aps()) {
    EXPECT_EQ(ap->getPoint().x(), 1500);
    EXPECT_TRUE(ap->hasAccess(frDirEnum::U));
  }
  EXPECT_LE(observer_->supplemental.size(), 256);
}

class SparseUpperTrackTest : public FlexPASparseViaTest,
                             public testing::WithParamInterface<bool>
{
};

TEST_P(SparseUpperTrackTest, RequiresPreferredManufacturingGridTracks)
{
  addVia("small", {-50, -50, 50, 50});
  blockNativeOrigin();
  if (GetParam()) {
    design->getTech()->getLayer(4)->getDbLayer()->setDirection(
        odb::dbTechLayerDir::HORIZONTAL);
  } else {
    design->getTech()->setManufacturingGrid(40);
    static_cast<frRect*>(pin_->getFigs().front().get())
        ->setBBox({440, 200, 560, 1800});
  }
  run();
  EXPECT_TRUE(aps().empty());
  EXPECT_TRUE(observer_->supplemental.empty());
}

INSTANTIATE_TEST_SUITE_P(UpperTracks, SparseUpperTrackTest, testing::Bool());

struct SparseLayerCase
{
  bool below_floor;
  bool same_direction;
  int lower_track;
  frAccessPointEnum lower_type;
};

class SparseViaLayerTest : public FlexPASparseViaTest,
                           public testing::WithParamInterface<SparseLayerCase>
{
};

TEST_P(SparseViaLayerTest, UsesRoutingMetadataAndTruthfulTrackTypes)
{
  const auto test = GetParam();
  addVia("B_legal",
         {-50, -50, 50, 50},
         {-40, -10, -20, 10},
         false,
         {-150, -50, 50, 50});
  addVia("A_blocked",
         {-50, -50, 50, 50},
         {20, -10, 40, 10},
         false,
         {-50, -50, 150, 50});
  block({560, 0, 600, 2000}, 4);
  blockNativeOrigin();
  if (test.below_floor) {
    router_cfg->BOTTOM_ROUTING_LAYER = 4;
  }
  if (test.same_direction) {
    design->getTech()->getLayer(2)->getDbLayer()->setDirection(
        odb::dbTechLayerDir::VERTICAL);
  } else if (test.lower_track) {
    addTrack(2, false, test.lower_track, 2, 1000);
  }
  run();
  expectRecovered();
  EXPECT_TRUE(std::ranges::any_of(aps(), [&](const auto& ap) {
    return ap->getType(true) == test.lower_type;
  }));
}

INSTANTIATE_TEST_SUITE_P(
    Metadata,
    SparseViaLayerTest,
    testing::Values(
        SparseLayerCase{true, false, 0, frAccessPointEnum::EncOpt},
        SparseLayerCase{false, true, 0, frAccessPointEnum::EncOpt},
        SparseLayerCase{false, false, 250, frAccessPointEnum::OnGrid},
        SparseLayerCase{false, false, 400, frAccessPointEnum::HalfGrid}));

}  // namespace drt
