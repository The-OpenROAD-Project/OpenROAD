// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "db/infra/frSegStyle.h"
#include "db/obj/frInstTerm.h"
#include "db/obj/frMTerm.h"
#include "db/obj/frNet.h"
#include "db/obj/frShape.h"
#include "fixture.h"
#include "gc/FlexGC.h"
#include "pa/PatternGCContext.h"

namespace drt {
namespace {
using Objects = std::vector<std::pair<frConnFig*, frBlockObject*>>;
using Contributor = std::tuple<std::uintptr_t, frLayerNum, odb::Rect, bool>;
using Marker = std::tuple<odb::Rect,
                          frLayerNum,
                          std::uintptr_t,
                          frConstraintTypeEnum,
                          std::vector<std::uintptr_t>,
                          std::vector<Contributor>,
                          std::vector<Contributor>,
                          bool,
                          bool>;
using Markers = std::vector<Marker>;

template <class T>
std::uintptr_t identity(T* object)
{
  return reinterpret_cast<std::uintptr_t>(object);
}

Markers fingerprint(const FlexGCWorker& worker)
{
  const auto contributors = [](const auto& values) {
    std::vector<Contributor> result;
    for (const auto& [owner, geometry] : values) {
      const auto& [layer, rect, fixed] = geometry;
      result.emplace_back(identity(owner), layer, rect, fixed);
    }
    std::sort(result.begin(), result.end());
    return result;
  };
  Markers result;
  for (const auto& marker : worker.getMarkers()) {
    std::vector<std::uintptr_t> sources;
    for (auto* source : marker->getSrcs()) {
      sources.push_back(identity(source));
    }
    std::sort(sources.begin(), sources.end());
    result.emplace_back(marker->getBBox(),
                        marker->getLayerNum(),
                        identity(marker->getConstraint()),
                        marker->getConstraint()->typeId(),
                        std::move(sources),
                        contributors(marker->getVictims()),
                        contributors(marker->getAggressors()),
                        marker->hasDir(),
                        marker->isH());
  }
  std::sort(result.begin(), result.end());
  return result;
}

bool hasConstraint(const Markers& markers,
                   const frConstraintTypeEnum type,
                   const frLayerNum layer = -1)
{
  return std::any_of(markers.begin(), markers.end(), [&](const auto& marker) {
    return std::get<3>(marker) == type
           && (layer == -1 || std::get<1>(marker) == layer);
  });
}

class PaPatternGCReuseTest : public Fixture
{
 protected:
  struct Settings
  {
    bool ignore_corner{false};
    bool ignore_long_side_eol{false};
    bool ignore_min_area{false};
    bool filter_instance{true};
    bool record{true};
    frBlockObject* target{nullptr};
    RouterConfiguration* configuration{nullptr};
    odb::Rect ext{-100, -100, 2000, 2000};
    odb::Rect drc{ext};
  };

  PaPatternGCReuseTest()
  {
    auto [master, db_master] = makeMacro("pattern_gc", 0, 0, 2000, 2000);
    makeMacroPin(master, "a", 100, 100, 300, 200);
    makeMacroPin(master, "b", 600, 100, 800, 200);
    makeMacroPin(master, "c", 1100, 100, 1300, 200);
    inst_ = makeInst("u1", master, db_master);
    a_ = makeNet("a");
    b_ = makeNet("b");
    c_ = makeNet("c");
    const std::array owners{a_, b_, c_};
    for (std::size_t i = 0; i < owners.size(); ++i) {
      auto* term = inst_->getInstTerms()[i].get();
      term->addToNet(owners[i]);
      owners[i]->addInstTerm(term);
    }
    b_shape_ = static_cast<frRect*>(
        master->getTerms()[1]->getPins()[0]->getFigs()[0].get());
    initRegionQuery();
  }

  static std::unique_ptr<frPathSeg> segment(const odb::Point& begin,
                                            const odb::Point& end)
  {
    auto result = std::make_unique<frPathSeg>();
    result->setPoints(begin, end);
    result->setLayerNum(2);
    frSegStyle style;
    style.setWidth(100);
    style.setBeginStyle(frcTruncateEndStyle, 0);
    style.setEndStyle(frcTruncateEndStyle, 0);
    result->setStyle(style);
    return result;
  }

  std::unique_ptr<FlexGCWorker> seed(const Settings& settings,
                                     const bool record)
  {
    auto worker = std::make_unique<FlexGCWorker>(
        design->getTech(),
        logger.get(),
        settings.configuration ? settings.configuration : router_cfg.get());
    worker->setExtBox(settings.ext);
    worker->setDrcBox(settings.drc);
    worker->setEnableSurgicalFix(false);
    if (settings.filter_instance) {
      worker->setTargetObjs({inst_});
    }
    if (settings.ignore_corner) {
      worker->setIgnoreCornerSpacing();
    }
    if (settings.ignore_long_side_eol) {
      worker->setIgnoreLongSideEOL();
    }
    if (settings.ignore_min_area) {
      worker->setIgnoreMinArea();
    }
    worker->initPA0(design.get(), record);
    if (settings.target) {
      EXPECT_TRUE(worker->setTargetNet(settings.target));
    }
    return worker;
  }

  Markers check(PatternGCContext& context,
                const Objects& objects,
                const Settings& settings,
                const bool expect_reuse)
  {
    auto cold = seed(settings, false);
    for (const auto& [object, owner] : objects) {
      cold->addPAObj(object, owner);
    }
    cold->initPA1();
    cold->main();
    auto fresh = seed(settings, settings.record);
    const auto fresh_identity = identity(fresh.get());
    auto& worker
        = context.check(std::move(fresh), objects, settings.ext, settings.drc);
    EXPECT_EQ(identity(&worker) != fresh_identity, expect_reuse);
    auto markers = fingerprint(worker);
    EXPECT_EQ(markers, fingerprint(*cold));
    return markers;
  }

  void fixedPinRectangle(const std::size_t pin,
                         const frLayerNum layer,
                         const odb::Rect& rect)
  {
    auto figure = std::make_unique<frRect>();
    figure->setBBox(rect);
    figure->setLayerNum(layer);
    inst_->getInstTerms()[pin]->getTerm()->getPins()[0]->addPinFig(
        std::move(figure));
  }

  void addUpperLayers()
  {
    addLayer(design->getTech(), "v1", odb::dbTechLayerType::CUT);
    addLayer(design->getTech(),
             "m2",
             odb::dbTechLayerType::ROUTING,
             odb::dbTechLayerDir::VERTICAL);
  }

  frInst* inst_{nullptr};
  frNet* a_{nullptr};
  frNet* b_{nullptr};
  frNet* c_{nullptr};
  frRect* b_shape_{nullptr};
};

TEST_F(PaPatternGCReuseTest, CandidatesOwnersAndFallback)
{
  PatternGCContext context;
  auto route = segment({100, 150}, {700, 150});
  Objects objects{{route.get(), a_}};
  const auto failing = check(context, objects, {}, false);
  ASSERT_TRUE(hasConstraint(failing, frConstraintTypeEnum::frcShortConstraint));
  ASSERT_FALSE(std::get<4>(failing.front()).empty());
  ASSERT_FALSE(std::get<5>(failing.front()).empty());
  ASSERT_FALSE(std::get<6>(failing.front()).empty());
  route->setPoints({100, 150}, {400, 150});
  EXPECT_TRUE(check(context, objects, {}, true).empty());
  route->setPoints({100, 150}, {700, 150});
  EXPECT_EQ(check(context, objects, {}, true), failing);
  route->setPoints({200, 150}, {700, 150});
  EXPECT_FALSE(check(context, {{route.get(), b_}}, {}, true).empty());
  EXPECT_TRUE(check(context, {}, {}, true).empty());
  for (auto* owner : {makeNet("unknown_a"), makeNet("unknown_b")}) {
    EXPECT_FALSE(check(context, {{route.get(), owner}}, {}, false).empty());
  }
  route->setPoints({100, 150}, {700, 150});
  EXPECT_EQ(check(context, objects, {}, true), failing);
}

class FixedInputTest : public PaPatternGCReuseTest,
                       public testing::WithParamInterface<bool>
{
};

TEST_P(FixedInputTest, InvalidationAndRestoration)
{
  PatternGCContext context;
  auto route = segment({100, 150}, {700, 150});
  Objects objects{{route.get(), a_}};
  const auto failing = check(context, objects, {}, false);
  ASSERT_FALSE(failing.empty());
  auto* term = inst_->getInstTerms()[1].get();
  if (GetParam()) {
    term->addToNet(a_);
  } else {
    b_shape_->setBBox({1600, 100, 1800, 200});
  }
  initRegionQuery();
  EXPECT_TRUE(check(context, objects, {}, false).empty());
  term->addToNet(b_);
  b_shape_->setBBox({600, 100, 800, 200});
  initRegionQuery();
  EXPECT_EQ(check(context, objects, {}, true), failing);
}
INSTANTIATE_TEST_SUITE_P(GeometryAndOwner, FixedInputTest, testing::Bool());

TEST_F(PaPatternGCReuseTest, ImportWindowInvalidates)
{
  PatternGCContext context;
  auto route = segment({100, 150}, {700, 150});
  Objects objects{{route.get(), a_}};
  const auto failing = check(context, objects, {}, false);
  ASSERT_FALSE(failing.empty());
  Settings settings;
  settings.ext = {-100, -100, 500, 500};
  EXPECT_TRUE(check(context, objects, settings, false).empty());
  EXPECT_EQ(check(context, objects, {}, true), failing);
}

TEST_F(PaPatternGCReuseTest, CurrentWindowsApplyOnReuse)
{
  auto area = std::make_unique<frAreaConstraint>(20000);
  design->getTech()->getLayer(2)->setAreaConstraint(area.get());
  design->getTech()->addUConstraint(std::move(area));
  PatternGCContext context;
  auto route = segment({100, 1000}, {200, 1000});
  Objects objects{{route.get(), a_}};
  const auto failing = check(context, objects, {}, false);
  ASSERT_TRUE(hasConstraint(failing, frConstraintTypeEnum::frcAreaConstraint));
  Settings settings;
  settings.ext = {-200, -200, 2200, 2200};
  settings.drc = {-100, -100, 2000, 800};
  EXPECT_TRUE(check(context, objects, settings, true).empty());
  EXPECT_EQ(check(context, objects, {}, true), failing);
}

TEST_F(PaPatternGCReuseTest, SettingsSeparateContexts)
{
  auto configuration = *router_cfg;
  PatternGCContext context;
  auto route = segment({100, 150}, {700, 150});
  Objects objects{{route.get(), a_}};
  ASSERT_FALSE(check(context, objects, {}, false).empty());
  std::array<Settings, 7> variants;
  variants[0].target = a_;
  variants[1].target = b_;
  variants[2].filter_instance = false;
  variants[3].ignore_corner = true;
  variants[4].ignore_long_side_eol = true;
  variants[5].ignore_min_area = true;
  variants[6].configuration = &configuration;
  for (const auto& settings : variants) {
    check(context, objects, settings, false);
    check(context, objects, settings, true);
  }
}

TEST_F(PaPatternGCReuseTest, UnrecordedAndDatabaseRouteFallback)
{
  PatternGCContext context;
  auto route = segment({1600, 600}, {1800, 600});
  Objects objects{{route.get(), a_}};
  Settings settings;
  settings.record = false;
  check(context, objects, settings, false);
  check(context, objects, settings, false);
  settings.record = true;
  check(context, objects, settings, false);
  check(context, objects, settings, true);
  makePathseg(c_, 2, {1600, 600}, {1800, 600});
  initRegionQuery();
  settings.filter_instance = false;
  for (int i = 0; i < 2; ++i) {
    ASSERT_TRUE(hasConstraint(check(context, objects, settings, false),
                              frConstraintTypeEnum::frcShortConstraint));
  }
}

TEST_F(PaPatternGCReuseTest, CutStackReplacement)
{
  addUpperLayers();
  addLayer(design->getTech(), "v2", odb::dbTechLayerType::CUT);
  addLayer(design->getTech(),
           "m3",
           odb::dbTechLayerType::ROUTING,
           odb::dbTechLayerDir::HORIZONTAL);
  auto* via12 = makeViaDef("via12", 3, {0, 0}, {100, 100});
  auto* via23 = makeViaDef("via23", 5, {0, 0}, {100, 100});
  for (const auto layer : {3, 5}) {
    auto spacing = std::make_unique<frCutSpacingConstraint>(
        100, false, false, "", false, -1, -1, false, false, -1);
    design->getTech()->getLayer(layer)->addCutSpacingConstraint(spacing.get());
    design->getTech()->addUConstraint(std::move(spacing));
    fixedPinRectangle(layer == 3 ? 1 : 2, layer, {300, 650, 400, 750});
  }
  initRegionQuery();
  frVia bottom(via12, {300, 650});
  frVia top(via23, {300, 650});
  PatternGCContext context;
  const auto bottom_only = check(context, {{&bottom, a_}}, {}, false);
  ASSERT_TRUE(
      hasConstraint(bottom_only, frConstraintTypeEnum::frcShortConstraint, 3));
  Objects stack{{&bottom, a_}, {&top, a_}};
  const auto failing = check(context, stack, {}, true);
  ASSERT_TRUE(
      hasConstraint(failing, frConstraintTypeEnum::frcShortConstraint, 3));
  ASSERT_TRUE(
      hasConstraint(failing, frConstraintTypeEnum::frcShortConstraint, 5));
  bottom.setOrigin({1600, 1500});
  top.setOrigin({1600, 1500});
  EXPECT_TRUE(check(context, stack, {}, true).empty());
  bottom.setOrigin({300, 650});
  top.setOrigin({300, 650});
  EXPECT_EQ(check(context, {{&bottom, a_}}, {}, true), bottom_only);
  EXPECT_TRUE(check(context, {}, {}, true).empty());
  EXPECT_EQ(check(context, stack, {}, true), failing);
}

TEST_F(PaPatternGCReuseTest, MetalWidthViaMapQueryOrder)
{
  addUpperLayers();
  auto* small = makeViaDef("V1_BAR", 3, {0, 0}, {100, 100});
  makeViaDef("V1_LARGE", 3, {0, 0}, {200, 200});
  auto* rule = odb::dbMetalWidthViaMap::create(db_tech);
  rule->setAboveLayerWidthLow(300);
  rule->setAboveLayerWidthHigh(300);
  rule->setBelowLayerWidthLow(300);
  rule->setBelowLayerWidthHigh(300);
  rule->setCutLayer(db_tech->findLayer("v1"));
  rule->setViaName("V1_LARGE");
  makeMetalWidthViaMap(3, rule);
  for (const auto layer : {2, 4}) {
    fixedPinRectangle(1, layer, {0, 600, 1000, 900});
    fixedPinRectangle(2, layer, {0, 550, 1000, 850});
  }
  initRegionQuery();
  frVia candidate(small, {200, 650});
  Objects objects{{&candidate, a_}};
  PatternGCContext context;
  const auto failing = check(context, objects, {}, false);
  ASSERT_TRUE(
      hasConstraint(failing, frConstraintTypeEnum::frcMetalWidthViaConstraint));
  EXPECT_EQ(check(context, objects, {}, true), failing);
  candidate.setOrigin({1600, 1500});
  EXPECT_TRUE(check(context, objects, {}, true).empty());
  candidate.setOrigin({200, 650});
  EXPECT_EQ(check(context, objects, {}, true), failing);
}

TEST_F(PaPatternGCReuseTest, ContextLifetimeIsolation)
{
  auto first = std::make_unique<PatternGCContext>();
  PatternGCContext second;
  auto route = segment({100, 150}, {700, 150});
  Objects objects{{route.get(), a_}};
  const auto failing = check(*first, objects, {}, false);
  ASSERT_FALSE(failing.empty());
  EXPECT_EQ(check(second, objects, {}, false), failing);
  EXPECT_EQ(check(*first, objects, {}, true), failing);
  EXPECT_EQ(check(second, objects, {}, true), failing);
  first = std::make_unique<PatternGCContext>();
  EXPECT_EQ(check(*first, objects, {}, false), failing);
  EXPECT_EQ(check(*first, objects, {}, true), failing);
}
}  // namespace
}  // namespace drt
