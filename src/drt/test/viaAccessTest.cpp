// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "db/obj/frAccess.h"
#include "db/obj/frBPin.h"
#include "db/obj/frBTerm.h"
#include "db/obj/frInstBlockage.h"
#include "db/obj/frMPin.h"
#include "db/obj/frMTerm.h"
#include "db/obj/frShape.h"
#include "db/obj/frTrackPattern.h"
#include "fixture.h"
#include "gtest/gtest.h"
#include "pa/AbstractPAGraphics.h"
#include "pa/FlexPA.h"
#include "pa/ViaAccess.h"

namespace drt {

// Observe actual native checker calls through the existing graphics interface.
class ViaCheckObserver : public AbstractPAGraphics
{
 public:
  void startPin(frBPin*, frInstTerm*, UniqueClass*) override {}
  void startPin(frMPin*, frInstTerm*, UniqueClass*) override {}
  void setAPs(const std::vector<std::unique_ptr<frAccessPoint>>&,
              frAccessPointEnum,
              frAccessPointEnum) override
  {
  }
  void setViaAP(const frAccessPoint*,
                const frVia*,
                const std::vector<std::unique_ptr<frMarker>>&) override
  {
    ++via_checks;
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
  int via_checks = 0;
};

// Snapshot the ordinary, filtered pool before automatic via additions.
class OrdinaryAccessObserver : public ViaCheckObserver
{
 public:
  explicit OrdinaryAccessObserver(frLayerNum via_access_layer = 2)
      : via_access_layer_(via_access_layer)
  {
  }

  void setAPs(const std::vector<std::unique_ptr<frAccessPoint>>& aps,
              frAccessPointEnum,
              frAccessPointEnum) override
  {
    for (const auto& ap : aps) {
      if (ap->hasAccess()
          && (ap->getLayerNum() > via_access_layer_
              || ap->hasAccess(frDirEnum::U))) {
        access_points.push_back(std::make_unique<frAccessPoint>(*ap));
      }
    }
  }
  std::vector<std::unique_ptr<frAccessPoint>> access_points;

 private:
  const frLayerNum via_access_layer_;
};

TEST(ViaAccessGeometry, GridDensityThresholdAndNegativeIntervals)
{
  EXPECT_EQ(sampleViaAccessOrigins(0, 150, 10).size(), 16);
  EXPECT_EQ(sampleViaAccessOrigins(0, 160, 10), (std::vector<int>{0, 80, 160}));
  EXPECT_EQ(sampleViaAccessOrigins(0, 170, 10),
            (std::vector<int>{0, 80, 90, 170}));
  EXPECT_EQ(sampleViaAccessOrigins(-17, -1, 5),
            (std::vector<int>{-15, -10, -5}));
  EXPECT_EQ(sampleViaAccessOrigins(-170, 0, 10),
            (std::vector<int>{-170, -90, -80, 0}));
  EXPECT_TRUE(sampleViaAccessOrigins(-4, -1, 5).empty());
  EXPECT_TRUE(sampleViaAccessOrigins(5, 4, 1).empty());
  EXPECT_TRUE(sampleViaAccessOrigins(0, 10, 0).empty());
}

TEST(ViaAccessGeometry, AsymmetricLandingAndBothBoundaryLayers)
{
  const auto window = viaAccessOriginWindow({34, 190, 86, 410},
                                            {-20, -50, 20, 50},
                                            {-30, -10, 10, 20},
                                            {0, 0, 80, 540});
  ASSERT_TRUE(window);
  EXPECT_EQ(*window, odb::Rect(54, 240, 60, 360));
  const auto reflected = viaAccessOriginWindow({-86, -410, -34, -190},
                                               {-20, -50, 20, 50},
                                               {-10, -20, 30, 10},
                                               {-80, -540, 0, 0});
  ASSERT_TRUE(reflected);
  EXPECT_EQ(*reflected, odb::Rect(-60, -360, -54, -240));
}

TEST(ViaAccessGeometry, RejectsSourceOverhangAndBoundaryBypass)
{
  EXPECT_FALSE(viaAccessOriginWindow({43, 90, 84, 333},
                                     {-50, -20, 50, 20},
                                     {-10, -10, 10, 10},
                                     {0, 0, 100, 540}));
  EXPECT_FALSE(viaAccessOriginWindow({43, 90, 84, 333},
                                     {-20, -20, 20, 20},
                                     {-100, -10, 100, 10},
                                     {0, 0, 100, 540}));
}

class ViaAccessTest : public Fixture
{
 protected:
  ViaAccessTest()
  {
    addLayer(design->getTech(), "v1", odb::dbTechLayerType::CUT);
    addLayer(design->getTech(),
             "m2",
             odb::dbTechLayerType::ROUTING,
             odb::dbTechLayerDir::VERTICAL);
    design->getTopBlock()->addTrackPattern(
        std::make_unique<frTrackPattern>(true, 100, 2, 100, 4));
    design->getTopBlock()->addTrackPattern(
        std::make_unique<frTrackPattern>(false, 200, 1, 100, 2));
    auto [master, db_master] = makeMacro("via_access", 0, 0, 600, 600);
    auto* term = static_cast<frMTerm*>(
        makeMacroPin(master, "signal", 0, 0, 300, 400, 2));
    pin_ = term->getPins().front().get();
    inst_ = makeInst("u_via_access", master, db_master);
    inst_term_ = inst_->getInstTerms().front().get();
    pin_->addPinAccess(std::make_unique<frPinAccess>());
    inst_->setPinAccessIdx(0);
    router_cfg->TOP_ROUTING_LAYER = 4;
    router_cfg->VIA_ACCESS_LAYERNUM = 2;
    router_cfg->MINNUMACCESSPOINT_STDCELLPIN = 1;
    initRegionQuery();
  }

  frViaDef* addVia(const std::string& name,
                   const odb::Rect& lower,
                   const bool preferred = false,
                   const frLayerNum lower_layer = 2)
  {
    const auto rectangle = [](const int layer, const odb::Rect& box) {
      auto figure = std::make_unique<frRect>();
      figure->setLayerNum(layer);
      figure->setBBox(box);
      return figure;
    };
    auto via = std::make_unique<frViaDef>(name);
    via->addLayer1Fig(rectangle(lower_layer, lower));
    via->addCutFig(rectangle(lower_layer + 1, {-10, -10, 10, 10}));
    via->addLayer2Fig(rectangle(lower_layer + 2, {-50, -50, 50, 50}));
    via->setDefault(preferred);
    auto* result = design->getTech()->addVia(std::move(via));
    design->getTech()->getLayer(lower_layer + 1)->addViaDef(result);
    return result;
  }

  std::unique_ptr<FlexPA> makePA()
  {
    auto pa = std::make_unique<FlexPA>(
        design.get(), logger.get(), nullptr, router_cfg.get());
    pa->initViaRawPriority();
    pa->initTrackCoords();
    return pa;
  }

  void addViaAccessPoints(FlexPA& pa,
                          std::vector<std::unique_ptr<frAccessPoint>>& aps)
  {
    auto shapes = pa.mergePinShapes(pin_, inst_term_);
    std::vector<std::vector<gtl::polygon_90_data<frCoord>>> polygons(
        shapes.size());
    for (size_t layer = 0; layer < shapes.size(); ++layer) {
      shapes[layer].get_polygons(polygons[layer]);
    }
    pa.genViaAccessPoints(aps, shapes, polygons, pin_, inst_term_);
  }

  int generate(FlexPA& pa) { return pa.genPinAccess(pin_, inst_term_); }

  std::vector<const frViaDef*> priorityVias(const FlexPA& pa,
                                            const int max_vias
                                            = std::numeric_limits<int>::max())
  {
    return pa.getPriorityViaDefs(3, inst_term_, max_vias);
  }

  std::vector<const frViaDef*> viaCatalogue(const FlexPA& pa)
  {
    return pa.getViaCatalogue(3, inst_term_);
  }

  void filterVias(FlexPA& pa, frAccessPoint& ap, const bool try_all_vias)
  {
    auto shapes = pa.mergePinShapes(pin_, inst_term_);
    std::vector<gtl::polygon_90_data<frCoord>> polygons;
    shapes[2].get_polygons(polygons);
    pa.filterViaAccess(
        &ap, polygons, shapes[2], pin_, inst_term_, try_all_vias);
  }

  void filterIOVias(FlexPA& pa,
                    frAccessPoint& ap,
                    frBPin* pin,
                    const bool try_all_vias)
  {
    auto shapes = pa.mergePinShapes(pin, nullptr);
    std::vector<gtl::polygon_90_data<frCoord>> polygons;
    shapes[4].get_polygons(polygons);
    pa.filterViaAccess(&ap, polygons, shapes[4], pin, nullptr, try_all_vias);
  }

  void rebuildViaCatalogue(FlexPA& pa) { pa.initViaRawPriority(); }

  std::map<frCoord, frAccessPointEnum> enclosedBoundaryCoords(FlexPA& pa)
  {
    std::map<frCoord, frAccessPointEnum> coords;
    pa.genAPEnclosedBoundary(coords, {0, 0, 300, 400}, 2, true);
    return coords;
  }

  const auto& tracks(FlexPA& pa, int layer) { return pa.track_coords_[layer]; }
  std::map<frCoord, frAccessPointEnum> nearbyCoords(FlexPA& pa,
                                                    int low,
                                                    int high)
  {
    std::map<frCoord, frAccessPointEnum> coords;
    pa.genAPOnTrack(coords, pa.track_coords_[4], low, high, true);
    return coords;
  }
  void initTracks(FlexPA& pa) { pa.initTrackCoords(); }
  bool revert(FlexPA& pa, frAccessPoint& ap, const odb::Point& offset)
  {
    return pa.revertAccessPoint(&ap, offset);
  }

  void blockCuts(const odb::Rect& box)
  {
    auto* blockage = makeMacroObs(
        inst_->getMaster(), box.xMin(), box.yMin(), box.xMax(), box.yMax(), 3);
    auto instance = std::make_unique<frInstBlockage>(inst_, blockage);
    instance->setId(numBlockages++);
    inst_->addInstBlockage(std::move(instance));
    auto rule = std::make_unique<frCutSpacingConstraint>(
        10, false, false, "", false, -1, -1, false, false, -1);
    design->getTech()->getLayer(3)->addCutSpacingConstraint(rule.get());
    design->getTech()->addUConstraint(std::move(rule));
    initRegionQuery();
  }

  static std::unique_ptr<frAccessPoint> retained(const frViaDef* via)
  {
    auto ap = std::make_unique<frAccessPoint>(odb::Point(100, 200), 2);
    ap->setAccess(frDirEnum::U);
    ap->setType(frAccessPointEnum::OnGrid, true);
    ap->setType(frAccessPointEnum::OnGrid, false);
    ap->addViaDef(via);
    return ap;
  }

  frInst* inst_;
  frInstTerm* inst_term_;
  frMPin* pin_;
};

TEST_F(ViaAccessTest, EqualPriorityViaIdentitiesHaveDeterministicOrder)
{
  auto* last = addVia("fitting_z", {-50, -50, 50, 50});
  auto* first = addVia("fitting_a", {-50, -50, 50, 50});
  auto pa = makePA();
  EXPECT_EQ(priorityVias(*pa), (std::vector<const frViaDef*>{first}));
  EXPECT_EQ(viaCatalogue(*pa), (std::vector<const frViaDef*>{first, last}));
  EXPECT_TRUE(priorityVias(*pa, 0).empty());

  auto* middle = addVia("fitting_m", {-50, -50, 50, 50});
  rebuildViaCatalogue(*pa);
  EXPECT_EQ(priorityVias(*pa), (std::vector<const frViaDef*>{middle}));
  EXPECT_EQ(viaCatalogue(*pa),
            (std::vector<const frViaDef*>{first, middle, last}));
}

TEST_F(ViaAccessTest, EqualPriorityViaAvoidancePreservesRemainingBudget)
{
  auto* last = addVia("fitting_z", {-50, -50, 50, 50});
  auto* first = addVia("fitting_a", {-50, -50, 50, 50});
  auto* second_priority = addVia("second_priority", {-50, -70, 50, 70});
  auto pa = makePA();
  pa->addAvoidViaDef(first);
  EXPECT_EQ(priorityVias(*pa, 1), (std::vector<const frViaDef*>{first}));
  inst_term_->setStubborn(true);
  EXPECT_EQ(priorityVias(*pa, 1),
            (std::vector<const frViaDef*>{second_priority}));
  EXPECT_EQ(priorityVias(*pa), (std::vector<const frViaDef*>{second_priority}));
  EXPECT_EQ(viaCatalogue(*pa),
            (std::vector<const frViaDef*>{last, second_priority}));
}

TEST_F(ViaAccessTest, EqualPriorityViasPreserveNativeEndpointsAndSelections)
{
  auto* supplemental = addVia("fitting_a", {-50, -40, 50, 60});
  auto* representative = addVia("fitting_z", {-50, -50, 50, 50});
  auto* second_priority = addVia("second_priority", {-50, -70, 50, 70});
  auto pa = makePA();
  EXPECT_EQ(priorityVias(*pa, 2),
            (std::vector<const frViaDef*>{representative, second_priority}));
  EXPECT_EQ(priorityVias(*pa),
            (std::vector<const frViaDef*>{representative, second_priority}));
  EXPECT_EQ(viaCatalogue(*pa),
            (std::vector<const frViaDef*>{
                supplemental, representative, second_priority}));
  const auto coords = enclosedBoundaryCoords(*pa);
  EXPECT_EQ(
      coords,
      (std::map<frCoord, frAccessPointEnum>{{50, frAccessPointEnum::EncOpt},
                                            {70, frAccessPointEnum::EncOpt},
                                            {330, frAccessPointEnum::EncOpt},
                                            {350, frAccessPointEnum::EncOpt}}));
  for (const bool try_all_vias : {false, true}) {
    SCOPED_TRACE(try_all_vias);
    frAccessPoint ap(odb::Point(100, 200), 2);
    ap.setAllowVia(true);
    ap.setType(frAccessPointEnum::OnGrid, true);
    ap.setType(frAccessPointEnum::OnGrid, false);
    filterVias(*pa, ap, try_all_vias);
    EXPECT_TRUE(ap.hasAccess(frDirEnum::U));
    EXPECT_EQ(ap.getViaDefs(),
              (std::vector<const frViaDef*>{representative, second_priority}));
  }
}

TEST_F(ViaAccessTest, EqualPriorityUpViasPreserveNativeDownViaSelection)
{
  addLayer(design->getTech(), "v2", odb::dbTechLayerType::CUT);
  addLayer(design->getTech(),
           "m3",
           odb::dbTechLayerType::ROUTING,
           odb::dbTechLayerDir::HORIZONTAL);
  router_cfg->TOP_ROUTING_LAYER = 6;
  auto* down = addVia("down", {-50, -50, 50, 50});
  addVia("up_a", {-50, -40, 50, 60}, false, 4);
  auto* representative = addVia("up_z", {-50, -50, 50, 50}, false, 4);
  auto term = std::make_unique<frBTerm>("io");
  auto pin = std::make_unique<frBPin>();
  auto figure = std::make_unique<frRect>();
  figure->setLayerNum(4);
  figure->setBBox({900, 900, 1200, 1300});
  figure->addToPin(pin.get());
  pin->addPinFig(std::move(figure));
  auto* io_pin = pin.get();
  term->addPin(std::move(pin));
  design->getTopBlock()->addTerm(std::move(term));
  initRegionQuery();
  auto pa = makePA();
  for (const bool try_all_vias : {false, true}) {
    SCOPED_TRACE(try_all_vias);
    frAccessPoint ap(odb::Point(1000, 1000), 4);
    ap.setAllowVia(true);
    ap.setType(frAccessPointEnum::OnGrid, true);
    ap.setType(frAccessPointEnum::OnGrid, false);
    filterIOVias(*pa, ap, io_pin, try_all_vias);
    EXPECT_TRUE(ap.hasAccess(frDirEnum::U));
    EXPECT_TRUE(ap.hasAccess(frDirEnum::D));
    EXPECT_EQ(ap.getViaDefs(),
              (std::vector<const frViaDef*>{representative, down}));
  }
}

TEST_F(ViaAccessTest, LateExactViaSurvivesExistingSuccess)
{
  addVia("default1", {-200, -50, 200, 50}, true);
  addVia("default2", {-250, -50, 250, 50}, true);
  for (int index = 0; index < 158; ++index) {
    addVia("nonfitting_" + std::to_string(index),
           {-400 - 10 * index, -10, 400 + 10 * index, 10});
  }
  auto* ordinary = addVia("fitting_a", {-50, -50, 50, 50});
  auto* late = addVia("fitting_z", {-50, -50, 50, 50});
  auto pa = makePA();
  std::vector<std::unique_ptr<frAccessPoint>> aps;
  auto retained = std::make_unique<frAccessPoint>(odb::Point(100, 200), 2);
  retained->setAccess(frDirEnum::U);
  retained->setType(frAccessPointEnum::OnGrid, true);
  retained->setType(frAccessPointEnum::OnGrid, false);
  retained->addViaDef(ordinary);
  aps.push_back(std::move(retained));
  addViaAccessPoints(*pa, aps);
  bool saw_same_origin = false;
  for (const auto& ap : aps) {
    if (ap->getPoint() == odb::Point(100, 200)) {
      const auto& vias = ap->getViaDefs();
      saw_same_origin = std::ranges::find(vias, late) != vias.end();
    }
  }
  EXPECT_TRUE(saw_same_origin);
  addViaAccessPoints(*pa, aps);
  for (const auto& ap : aps) {
    for (const auto& bucket : ap->getAllViaDefs()) {
      EXPECT_LE(std::ranges::count(bucket, ordinary), 1);
      EXPECT_LE(std::ranges::count(bucket, late), 1);
    }
  }
}

TEST_F(ViaAccessTest, AdmissionsPreserveOriginAndViaOrder)
{
  addVia("wide1", {-200, -50, 200, 50}, true);
  addVia("wide2", {-250, -50, 250, 50}, true);
  auto* ordinary = addVia("fitting_a", {-50, -50, 50, 50});
  auto* alternative = addVia("fitting_z", {-50, -50, 50, 50});
  auto pa = makePA();
  std::vector<std::unique_ptr<frAccessPoint>> aps;
  auto retained = std::make_unique<frAccessPoint>(odb::Point(100, 50), 2);
  retained->setAccess(frDirEnum::U);
  retained->setAccess(frDirEnum::E);
  retained->setType(frAccessPointEnum::OnGrid, true);
  retained->setType(frAccessPointEnum::OnGrid, false);
  retained->addViaDef(ordinary);
  auto* retained_ptr = retained.get();
  aps.push_back(std::move(retained));

  addViaAccessPoints(*pa, aps);

  ASSERT_EQ(aps.size(), 4);
  EXPECT_EQ(aps[0].get(), retained_ptr);
  EXPECT_TRUE(aps[0]->hasAccess(frDirEnum::E));
  EXPECT_EQ(aps[0]->getType(true), frAccessPointEnum::OnGrid);
  EXPECT_EQ(aps[0]->getPoint(), odb::Point(100, 50));
  EXPECT_EQ(aps[1]->getPoint(), odb::Point(200, 50));
  EXPECT_EQ(aps[2]->getPoint(), odb::Point(100, 200));
  EXPECT_EQ(aps[3]->getPoint(), odb::Point(100, 350));
  EXPECT_EQ(aps[0]->getViaDefs(),
            (std::vector<const frViaDef*>{ordinary, alternative}));
  EXPECT_EQ(aps[1]->getViaDefs(), (std::vector<const frViaDef*>{ordinary}));
  EXPECT_EQ(aps[2]->getViaDefs(),
            (std::vector<const frViaDef*>{ordinary, alternative}));
  EXPECT_EQ(aps[3]->getViaDefs(),
            (std::vector<const frViaDef*>{ordinary, alternative}));
}

TEST_F(ViaAccessTest, StubbornAvoidViaIsExcludedFromAdmission)
{
  auto* allowed = addVia("fitting", {-50, -50, 50, 50});
  auto pa = makePA();
  pa->addAvoidViaDef(allowed);
  inst_term_->setStubborn(true);
  std::vector<std::unique_ptr<frAccessPoint>> aps;
  addViaAccessPoints(*pa, aps);
  EXPECT_TRUE(aps.empty());
}

TEST_F(ViaAccessTest, AdditionalPassRunsAfterOrdinaryEarlyStop)
{
  addVia("wide1", {-200, -50, 200, 50}, true);
  addVia("wide2", {-250, -50, 250, 50}, true);
  addVia("late_fit", {-50, -50, 50, 50});
  auto pa = makePA();
  auto observer = std::make_unique<OrdinaryAccessObserver>();
  auto* ordinary = observer.get();
  pa->setDebug(std::move(observer));
  const int final_count = generate(*pa);
  ASSERT_GE(ordinary->access_points.size(),
            router_cfg->MINNUMACCESSPOINT_STDCELLPIN);
  EXPECT_GT(final_count, ordinary->access_points.size());
}

TEST_F(ViaAccessTest, PlanarOnlyChoiceSurvivesAdditionalUpwardAccess)
{
  router_cfg->VIA_ACCESS_LAYERNUM = 0;
  // Leave the source outside the mandatory via-in-pin range so native planar
  // access remains enabled and stops the all-via retry before the late via.
  static_cast<frRect*>(pin_->getFigs().front().get())
      ->setBBox({0, 150, 300, 250});
  addVia("wide1", {-400, -50, 400, 50}, true);
  addVia("wide2", {-500, -50, 500, 50}, true);
  auto* fitting = addVia("late_fit", {-50, -50, 50, 50});
  initRegionQuery();
  auto pa = makePA();
  auto observer = std::make_unique<OrdinaryAccessObserver>(
      router_cfg->VIA_ACCESS_LAYERNUM);
  auto* ordinary = observer.get();
  pa->setDebug(std::move(observer));
  ASSERT_GT(generate(*pa), 0);
  ASSERT_EQ(ordinary->access_points.size(), 2);
  const auto& final_aps = pin_->getPinAccess(0)->getAccessPoints();
  for (const auto& prior : ordinary->access_points) {
    EXPECT_TRUE(prior->hasPlanarAccess());
    EXPECT_FALSE(prior->hasAccess(frDirEnum::U));
    EXPECT_FALSE(prior->hasViaDef());
    EXPECT_EQ(prior->getPoint().y(), 200);
    EXPECT_TRUE(prior->getPoint().x() == 100 || prior->getPoint().x() == 200);
    const auto planar = std::ranges::find_if(final_aps, [&](const auto& ap) {
      return ap->getPoint() == prior->getPoint()
             && ap->getLayerNum() == prior->getLayerNum()
             && !ap->hasAccess(frDirEnum::U);
    });
    ASSERT_NE(planar, final_aps.end());
    for (const auto dir : frDirEnumAll) {
      EXPECT_EQ((*planar)->hasAccess(dir), prior->hasAccess(dir));
    }
    EXPECT_FALSE((*planar)->hasViaDef());
    const auto upward = std::ranges::find_if(final_aps, [&](const auto& ap) {
      return ap->getPoint() == prior->getPoint()
             && ap->getLayerNum() == prior->getLayerNum()
             && ap->hasAccess(frDirEnum::U);
    });
    ASSERT_NE(upward, final_aps.end());
    EXPECT_FALSE((*upward)->hasPlanarAccess());
    EXPECT_FALSE((*upward)->hasAccess(frDirEnum::D));
    EXPECT_EQ((*upward)->getViaDefs(), (std::vector<const frViaDef*>{fitting}));
  }
}

TEST_F(ViaAccessTest, SourceBelowRoutingBottomRetainsNativeAndAddsUpwardAccess)
{
  router_cfg->BOTTOM_ROUTING_LAYER = 4;
  addVia("wide1", {-200, -50, 200, 50}, true);
  addVia("wide2", {-250, -50, 250, 50}, true);
  auto* late = addVia("late_fit", {-50, -50, 50, 50});
  for (const bool explicit_via_in_pin : {false, true}) {
    SCOPED_TRACE(explicit_via_in_pin);
    if (explicit_via_in_pin) {
      router_cfg->VIAINPIN_BOTTOMLAYERNUM = 2;
      router_cfg->VIAINPIN_TOPLAYERNUM = 2;
    }
    auto pa = makePA();
    auto observer = std::make_unique<OrdinaryAccessObserver>();
    auto* observed = observer.get();
    pa->setDebug(std::move(observer));
    ASSERT_GT(generate(*pa), 0);
    const auto& ordinary = observed->access_points;
    ASSERT_FALSE(ordinary.empty());
    const auto& final_aps = pin_->getPinAccess(0)->getAccessPoints();
    for (const auto& prior : ordinary) {
      const auto retained
          = std::ranges::find_if(final_aps, [&](const auto& ap) {
              return ap->getPoint() == prior->getPoint()
                     && ap->getLayerNum() == prior->getLayerNum();
            });
      ASSERT_NE(retained, final_aps.end());
      for (const auto dir : frDirEnumAll) {
        if (prior->hasAccess(dir)) {
          EXPECT_TRUE((*retained)->hasAccess(dir));
        }
      }
      for (const auto* via : prior->getViaDefs()) {
        EXPECT_NE(std::ranges::find((*retained)->getViaDefs(), via),
                  (*retained)->getViaDefs().end());
      }
    }
    const auto window = viaAccessOriginWindow({0, 0, 300, 400},
                                              late->getLayer1ShapeBox(),
                                              late->getLayer2ShapeBox(),
                                              {0, 0, 600, 600});
    ASSERT_TRUE(window);
    bool saw_new_fitting_upward = false;
    for (const auto& ap : final_aps) {
      if (ap->getLayerNum() != 2 || !ap->hasAccess(frDirEnum::U)
          || std::ranges::find(ap->getViaDefs(), late) == ap->getViaDefs().end()
          || std::ranges::any_of(ordinary, [&](const auto& prior) {
               return prior->getPoint() == ap->getPoint()
                      && prior->getLayerNum() == ap->getLayerNum()
                      && std::ranges::find(prior->getViaDefs(), late)
                             != prior->getViaDefs().end();
             })) {
        continue;
      }
      EXPECT_TRUE(window->intersects(ap->getPoint()));
      EXPECT_TRUE(ap->getPoint().x() == 100 || ap->getPoint().x() == 200);
      EXPECT_EQ(ap->getType(false), frAccessPointEnum::OnGrid);
      saw_new_fitting_upward = true;
    }
    EXPECT_TRUE(saw_new_fitting_upward);
  }
}

TEST_F(ViaAccessTest, CanonicalWidthMapOverridesFallbackCatalogue)
{
  auto* mapped = addVia("mapped", {-50, -50, 50, 50});
  auto* excluded = addVia("excluded", {-50, -50, 50, 50});
  auto* rule = odb::dbMetalWidthViaMap::create(db_tech);
  rule->setCutLayer(design->getTech()->getLayer(3)->getDbLayer());
  rule->setAboveLayerWidthLow(100);
  rule->setAboveLayerWidthHigh(100);
  rule->setBelowLayerWidthLow(400);
  rule->setBelowLayerWidthHigh(400);
  rule->setViaName("mapped");
  auto pa = makePA();
  std::vector<std::unique_ptr<frAccessPoint>> aps;
  addViaAccessPoints(*pa, aps);
  ASSERT_FALSE(aps.empty());
  for (const auto& ap : aps) {
    const auto& vias = ap->getViaDefs();
    EXPECT_NE(std::ranges::find(vias, mapped), vias.end());
    EXPECT_EQ(std::ranges::find(vias, excluded), vias.end());
  }
}

TEST_F(ViaAccessTest, SameDirectionLayersUseActualUpperTrackAxis)
{
  design->getTech()->getLayer(4)->getDbLayer()->setDirection(
      odb::dbTechLayerDir::HORIZONTAL);
  design->getTopBlock()->addTrackPattern(
      std::make_unique<frTrackPattern>(false, 150, 2, 100, 4));
  addVia("fitting", {-50, -50, 50, 50});
  auto pa = makePA();
  std::vector<std::unique_ptr<frAccessPoint>> aps;
  addViaAccessPoints(*pa, aps);
  ASSERT_FALSE(aps.empty());
  for (const auto& ap : aps) {
    EXPECT_TRUE(ap->getPoint().y() == 150 || ap->getPoint().y() == 250);
  }
}

TEST_F(ViaAccessTest, InteriorPassesNativeGcWhenEndpointsAreBlocked)
{
  static_cast<frRect*>(pin_->getFigs().front().get())
      ->setBBox({0, 0, 300, 200});
  auto* fitting = addVia("fitting", {-50, -50, 50, 50});
  ASSERT_NE(design->getTech()->getLayer(3)->getShortConstraint(), nullptr);
  // Native cut-short queries are dispatched through cut-spacing rules.
  auto cut_spacing = std::make_unique<frCutSpacingConstraint>(
      10, false, false, "", false, -1, -1, false, false, -1);
  design->getTech()->getLayer(3)->addCutSpacingConstraint(cut_spacing.get());
  design->getTech()->addUConstraint(std::move(cut_spacing));
  ASSERT_TRUE(design->getTech()->getLayer(3)->hasCutSpacing(false));
  initRegionQuery();

  // Without obstructions, the identical endpoint geometry passes native GC.
  auto control = makePA();
  std::vector<std::unique_ptr<frAccessPoint>> control_aps;
  addViaAccessPoints(*control, control_aps);
  ASSERT_FALSE(control_aps.empty());
  for (const auto& ap : control_aps) {
    EXPECT_TRUE(ap->getPoint().y() == 50 || ap->getPoint().y() == 150);
  }

  // Endpoint cuts are blocked while interior witnesses remain legal.
  for (const auto& box :
       {odb::Rect(0, 40, 300, 60), odb::Rect(0, 140, 300, 160)}) {
    auto* blockage = makeMacroObs(
        inst_->getMaster(), box.xMin(), box.yMin(), box.xMax(), box.yMax(), 3);
    auto inst_blockage = std::make_unique<frInstBlockage>(inst_, blockage);
    inst_blockage->setId(numBlockages++);
    inst_->addInstBlockage(std::move(inst_blockage));
  }
  ASSERT_EQ(inst_->getInstBlockages().size(), 2);
  initRegionQuery();

  auto pa = makePA();
  std::vector<std::unique_ptr<frAccessPoint>> aps;
  addViaAccessPoints(*pa, aps);
  ASSERT_FALSE(aps.empty());
  const auto window = viaAccessOriginWindow({0, 0, 300, 200},
                                            fitting->getLayer1ShapeBox(),
                                            fitting->getLayer2ShapeBox(),
                                            {0, 0, 600, 600});
  ASSERT_TRUE(window);
  EXPECT_EQ(window->yMin(), 50);
  EXPECT_EQ(window->yMax(), 150);
  for (const auto& ap : aps) {
    EXPECT_EQ(ap->getLayerNum(), 2);
    EXPECT_TRUE(ap->hasAccess(frDirEnum::U));
    EXPECT_TRUE(ap->getPoint().x() == 100 || ap->getPoint().x() == 200);
    EXPECT_EQ(ap->getType(false), frAccessPointEnum::OnGrid);
    EXPECT_GT(ap->getPoint().y(), window->yMin());
    EXPECT_LT(ap->getPoint().y(), window->yMax());
    EXPECT_EQ(ap->getPoint().x() % 10, 0);
    EXPECT_EQ(ap->getPoint().y() % 10, 0);
    EXPECT_TRUE(window->intersects(ap->getPoint()));
    EXPECT_NE(std::ranges::find(ap->getViaDefs(), fitting),
              ap->getViaDefs().end());
  }
}

TEST_F(ViaAccessTest, NearbyGridUsesFullUpperTracksAfterInsufficientPool)
{
  static_cast<frRect*>(pin_->getFigs().front().get())
      ->setBBox({0, 0, 250, 400});
  design->getTopBlock()->addTrackPattern(
      std::make_unique<frTrackPattern>(true, 300, 1, 100, 4));
  addVia("fitting", {-50, -50, 50, 50});
  auto pa = makePA();
  EXPECT_EQ(nearbyCoords(*pa, 250, 300),
            (std::map<frCoord, frAccessPointEnum>{
                {300, frAccessPointEnum::NearbyGrid}}));
  auto observer = std::make_unique<OrdinaryAccessObserver>();
  auto* ordinary = observer.get();
  pa->setDebug(std::move(observer));
  ASSERT_GT(generate(*pa), 0);
  ASSERT_FALSE(ordinary->access_points.empty());
  const size_t sufficient_count = ordinary->access_points.size();
  for (const auto& ap : ordinary->access_points) {
    EXPECT_NE(ap->getType(false), frAccessPointEnum::NearbyGrid);
  }
  router_cfg->MINNUMACCESSPOINT_STDCELLPIN = 100;
  ordinary->access_points.clear();
  ASSERT_GT(generate(*pa), 0);
  EXPECT_GT(ordinary->access_points.size(), sufficient_count);
  bool nearby = false;
  for (const auto& ap : ordinary->access_points) {
    if (ap->getType(false) == frAccessPointEnum::NearbyGrid) {
      nearby = true;
      EXPECT_EQ(ap->getPoint().x(), 300);
      EXPECT_EQ(tracks(*pa, 4).at(ap->getPoint().x()),
                frAccessPointEnum::OnGrid);
    }
  }
  EXPECT_TRUE(nearby);
}

TEST_F(ViaAccessTest, CoveredAlternativesRetainReservedBudget)
{
  addVia("wide1", {-200, -50, 200, 50}, true);
  addVia("wide2", {-250, -50, 250, 50}, true);
  std::vector<const frViaDef*> fitting;
  for (int i = 0; i < 160; ++i) {
    fitting.push_back(
        addVia("fit_" + std::to_string(1000 + i), {-50, -50, 50, 50}));
  }
  // The x=200 uncovered track fails in both preferred directions. The x=100
  // covered track must still receive its 32 reserved exact-via alternatives.
  blockCuts({190, 0, 210, 400});
  auto pa = makePA();
  std::vector<std::unique_ptr<frAccessPoint>> aps;
  aps.push_back(retained(fitting.front()));
  addViaAccessPoints(*pa, aps);
  ASSERT_EQ(aps.size(), 2);
  EXPECT_EQ(aps[0]->getPoint(), odb::Point(100, 200));
  EXPECT_EQ(aps[1]->getPoint(), odb::Point(100, 50));
  EXPECT_EQ(
      aps[1]->getViaDefs(),
      std::vector<const frViaDef*>(fitting.begin(), fitting.begin() + 32));
  EXPECT_TRUE(aps[1]->hasAccess(frDirEnum::U));
  EXPECT_EQ(aps[1]->getType(false), frAccessPointEnum::OnGrid);
}

TEST_F(ViaAccessTest, TrialBudgetTransfersToCoveredTrackDeterministically)
{
  addVia("wide1", {-200, -50, 200, 50}, true);
  addVia("wide2", {-250, -50, 250, 50}, true);
  std::vector<const frViaDef*> fitting;
  for (int i = 0; i < 160; ++i) {
    fitting.push_back(
        addVia("fit_" + std::to_string(1000 + i), {-50, -50, 50, 50}));
  }
  for (int run = 0; run < 2; ++run) {
    auto pa = makePA();
    std::vector<std::unique_ptr<frAccessPoint>> aps;
    aps.push_back(retained(fitting.front()));
    addViaAccessPoints(*pa, aps);
    ASSERT_EQ(aps.size(), 3);
    EXPECT_EQ(aps[1]->getPoint(), odb::Point(200, 50));
    EXPECT_EQ(aps[1]->getViaDefs(),
              std::vector<const frViaDef*>{fitting.front()});
    EXPECT_EQ(aps[2]->getPoint(), odb::Point(100, 50));
    EXPECT_EQ(
        aps[2]->getViaDefs(),
        std::vector<const frViaDef*>(fitting.begin(), fitting.begin() + 127));
  }
}

TEST_F(ViaAccessTest, FailedTrialsStopAtNativeGcBudget)
{
  const frViaDef* first = nullptr;
  for (int i = 0; i < 160; ++i) {
    auto* via = addVia("fit_" + std::to_string(1000 + i), {-50, -50, 50, 50});
    if (i == 0) {
      first = via;
    }
  }
  blockCuts({0, 0, 300, 400});
  auto pa = makePA();
  auto observer = std::make_unique<ViaCheckObserver>();
  auto* calls = observer.get();
  pa->setDebug(std::move(observer));
  std::vector<std::unique_ptr<frAccessPoint>> aps;
  // Failed coverage discovery consumes two checks before bounded trials.
  // Without the independent GC cap, the 128 trials would make 258 calls.
  aps.push_back(retained(first));
  addViaAccessPoints(*pa, aps);
  ASSERT_EQ(aps.size(), 1);
  EXPECT_EQ(aps.front()->getPoint(), odb::Point(100, 200));
  EXPECT_EQ(aps.front()->getViaDefs(), std::vector<const frViaDef*>{first});
  EXPECT_EQ(calls->via_checks, 256);
}

TEST_F(ViaAccessTest, TrackMidpointAndLastIterationUseWideArithmetic)
{
  constexpr int high = std::numeric_limits<int>::max();
  design->getTopBlock()->addTrackPattern(
      std::make_unique<frTrackPattern>(true, high - 20, 2, 20, 4));
  auto pa = makePA();
  const auto& coords = tracks(*pa, 4);
  EXPECT_EQ(coords.at(high - 20), frAccessPointEnum::OnGrid);
  EXPECT_EQ(coords.at(high), frAccessPointEnum::OnGrid);
  EXPECT_EQ(coords.at((high - 10) / 10 * 10), frAccessPointEnum::HalfGrid);
}

TEST_F(ViaAccessTest, OutOfRangeTrackPatternIsRejectedBeforeIteration)
{
  auto pa = makePA();
  design->getTopBlock()->addTrackPattern(std::make_unique<frTrackPattern>(
      true, std::numeric_limits<int>::max(), 2, 1, 4));
  EXPECT_THROW(initTracks(*pa), std::runtime_error);
}

TEST_F(ViaAccessTest, RevertRejectsOverflowWithoutPartialMutation)
{
  auto pa = makePA();
  frAccessPoint ap(odb::Point(20, 30), 2);
  frPathSeg segment;
  segment.setPoints({10, 30}, {std::numeric_limits<int>::max(), 30});
  ap.addPathSeg(segment);
  EXPECT_FALSE(revert(*pa, ap, {-1, 0}));
  EXPECT_EQ(ap.getPoint(), odb::Point(20, 30));
  EXPECT_EQ(ap.getPathSegs()[0].getBeginPoint(), odb::Point(10, 30));
  EXPECT_EQ(ap.getPathSegs()[0].getEndPoint(),
            odb::Point(std::numeric_limits<int>::max(), 30));
  EXPECT_TRUE(revert(*pa, ap, {10, 20}));
  EXPECT_EQ(ap.getPoint(), odb::Point(10, 10));
  EXPECT_EQ(ap.getPathSegs()[0].getBeginPoint(), odb::Point(0, 10));
}

}  // namespace drt
