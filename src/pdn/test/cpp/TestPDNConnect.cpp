// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

// Unit tests for the per-layer via-rect generation in Connect -- the stage
// that decides which candidate shapes each layer of a via stack is offered
// before the via generators are asked to fit one.
//
// The focus is the option surface of add_pdn_connect that shapes those rects
// (-min_width_layers in particular), plus the intermediate-layer discovery
// and the complex-stacked-via path they interact with.

#include <functional>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "PdnTest.h"
#include "connect.h"
#include "domain.h"
#include "gmock/gmock.h"
#include "grid.h"
#include "gtest/gtest.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "pdn/PdnGen.hh"
#include "spdlog/sinks/ostream_sink.h"
// Connect's via map holds unique_ptr<DbGenerateStackedVia>, which in turn
// holds a unique_ptr<TechLayer>; both need to be complete here.
#include "techlayer.h"
#include "utl/Logger.h"

namespace pdn {
namespace {

// Connect keeps the rect-generation helpers at protected scope for tests;
// re-publish them here.
class TestableConnect : public Connect
{
 public:
  using Connect::Connect;

  using Connect::ViaLayerRects;

  using Connect::generateComplexStackedViaRects;
  using Connect::generateMinEnclosureViaRects;
  using Connect::generateViaRects;
  using Connect::getMinWidth;
  using Connect::isComplexStackedVia;
};

// Fixture providing the minimum scaffolding a Connect needs: a PdnGen, a
// voltage domain and a grid (Connect reaches the logger and block through
// its grid), plus a routing stack to connect across.
class TestConnect : public PdnTest
{
 protected:
  void SetUp() override
  {
    PdnTest::SetUp();

    power_ = odb::dbNet::create(block(), "VDD");
    power_->setSigType(odb::dbSigType::POWER);
    power_->setSpecial();
    ground_ = odb::dbNet::create(block(), "VSS");
    ground_->setSigType(odb::dbSigType::GROUND);
    ground_->setSpecial();
    block()->setDieArea(odb::Rect(0, 0, 100000, 100000));
    block()->setCoreArea(odb::Rect(1000, 1000, 99000, 99000));

    pdngen_ = std::make_unique<PdnGen>(db_.get(), getLogger());
    domain_ = std::make_unique<VoltageDomain>(pdngen_.get(),
                                              block(),
                                              power_,
                                              ground_,
                                              std::vector<odb::dbNet*>{},
                                              getLogger());
    auto grid = std::make_unique<TestGrid>(domain_.get(),
                                           "grid",
                                           /*starts_with_power=*/true,
                                           std::vector<odb::dbTechLayer*>{});
    grid_ = grid.get();
    domain_->addGrid(std::move(grid));

    // M1(H) V1 M2(V) V2 M3(H) V3 M4(V), every routing layer 100 wide.
    stack_ = makeRoutingStack(/*levels=*/4, kMinWidth);
  }

  odb::dbTechLayer* m(int level) const { return stack_.routing[level - 1]; }

  // The rects a full M1 -> M4 stack offers each layer, after min-enclosure
  // filtering. Index 0 is M1, 1 is M2, 2 is M3, 3 is M4.
  static std::vector<TestableConnect::ViaLayerRects> stackRects(
      const TestableConnect& connect,
      const odb::Rect& lower,
      const odb::Rect& upper)
  {
    auto rects = connect.generateViaRects(lower, upper);
    connect.generateMinEnclosureViaRects(rects);
    return rects;
  }

  // Connect::report() writes through the logger, so tap it with a temporary
  // sink and hand back everything it emitted.
  std::string captureReport(const Connect& connect)
  {
    return captureLog([&] { connect.report(); });
  }

  std::string captureLog(const std::function<void()>& emit)
  {
    std::ostringstream stream;
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(stream);
    sink->set_pattern("%v");
    getLogger()->addSink(sink);
    emit();
    getLogger()->removeSink(sink);
    return stream.str();
  }

  // A VIARULE GENERATE from M<level> up to M<level + 1>: a cut_size square
  // cut on a 100 pitch, enclosed by 10 on both metals.
  odb::dbTechViaGenerateRule* addGenerateRule(int level,
                                              const char* name,
                                              int cut_size = kMinWidth / 2)
  {
    auto* rule = odb::dbTechViaGenerateRule::create(tech(), name, false);
    odb::dbTechViaLayerRule::create(tech(), rule, m(level))
        ->setEnclosure(10, 10);
    auto* cut
        = odb::dbTechViaLayerRule::create(tech(), rule, stack_.cuts[level - 1]);
    cut->setRect(
        odb::Rect(-cut_size / 2, -cut_size / 2, cut_size / 2, cut_size / 2));
    cut->setSpacing(100, 100);
    odb::dbTechViaLayerRule::create(tech(), rule, m(level + 1))
        ->setEnclosure(10, 10);
    return rule;
  }

  // A single cut tech via from M<level> up to M<level + 1>, its 50 square cut
  // enclosed by 10 on both metals.
  odb::dbTechVia* addTechVia(int level, const char* name)
  {
    return tst::addVia(tech(),
                       name,
                       m(level),
                       stack_.cuts[level - 1],
                       m(level + 1),
                       kMinWidth / 2,
                       10);
  }

  ShapePtr makeShape(int level,
                     const odb::Rect& rect,
                     odb::dbNet* net = nullptr,
                     odb::dbWireShapeType type = odb::dbWireShapeType::STRIPE)
  {
    return std::make_shared<Shape>(
        m(level), net == nullptr ? power_ : net, rect, type);
  }

  odb::dbSWire* makeWire(odb::dbNet* net = nullptr)
  {
    return odb::dbSWire::create(net == nullptr ? power_ : net,
                                odb::dbWireType::ROUTED);
  }

  // The vias placed on a wire.
  static std::vector<odb::dbSBox*> placedVias(odb::dbSWire* wire)
  {
    std::vector<odb::dbSBox*> vias;
    for (odb::dbSBox* box : wire->getWires()) {
      if (box->isVia()) {
        vias.push_back(box);
      }
    }
    return vias;
  }

  // The markers recordFailedVias() filed for a failure reason on this
  // fixture's grid, or nullptr when it filed none.
  odb::dbMarkerCategory* failures(const std::string& reason,
                                  const std::string& layers = "M1 -> M2")
  {
    odb::dbMarkerCategory* tool = block()->findMarkerCategory("PDN");
    if (tool == nullptr) {
      return nullptr;
    }
    odb::dbMarkerCategory* via = tool->findMarkerCategory("Via");
    if (via == nullptr) {
      return nullptr;
    }
    return via->findMarkerCategory((reason + " - grid - " + layers).c_str());
  }

  // A horizontal M1 strap and a vertical M2 strap crossing in a 200 x 200
  // overlap centered on (2100, 1100).
  static inline const odb::Rect kM1Strap{0, 1000, 5000, 1200};
  static inline const odb::Rect kM2Strap{2000, 0, 2200, 5000};

  static constexpr int kMinWidth = 100;

  std::unique_ptr<PdnGen> pdngen_;
  std::unique_ptr<VoltageDomain> domain_;
  Grid* grid_ = nullptr;
  odb::dbNet* power_ = nullptr;
  odb::dbNet* ground_ = nullptr;
  RoutingStack stack_;
};

// -------- Layer discovery --------

// An M1 -> M4 connect picks up M2 and M3 as intermediate routing layers, and
// the cut layers between them as (non-routing) intermediate layers.
TEST_F(TestConnect, IntermediateRoutingLayersAreDiscoveredInOrder)
{
  const TestableConnect connect(grid_, m(1), m(4));

  EXPECT_EQ(connect.getLowerLayer(), m(1));
  EXPECT_EQ(connect.getUpperLayer(), m(4));
  EXPECT_TRUE(connect.isMultiLayerVia());
  EXPECT_THAT(connect.getIntermediteRoutingLayers(),
              ::testing::ElementsAre(m(2), m(3)));
  EXPECT_THAT(connect.getIntermediteLayers(),
              ::testing::ElementsAre(
                  stack_.cuts[0], m(2), stack_.cuts[1], m(3), stack_.cuts[2]));
}

// Adjacent layers give a single-layer via with no intermediate routing layers.
TEST_F(TestConnect, AdjacentLayersFormASingleLayerVia)
{
  const TestableConnect connect(grid_, m(1), m(2));

  EXPECT_TRUE(connect.isSingleLayerVia());
  EXPECT_TRUE(connect.getIntermediteRoutingLayers().empty());
}

// The constructor normalizes the layer order, so the caller may pass the
// endpoints either way round.
TEST_F(TestConnect, LayerOrderIsNormalized)
{
  const TestableConnect connect(grid_, m(4), m(1));

  EXPECT_EQ(connect.getLowerLayer(), m(1));
  EXPECT_EQ(connect.getUpperLayer(), m(4));
}

// A layer with a LEF58 TYPE (a MIM cap, say) is not part of the stack.
TEST_F(TestConnect, LayersWithALef58TypeAreNotIntermediate)
{
  stack_.cuts[1]->setLef58Type(odb::dbTechLayer::MIMCAP);
  const TestableConnect connect(grid_, m(1), m(4));

  EXPECT_THAT(
      connect.getIntermediteLayers(),
      ::testing::ElementsAre(stack_.cuts[0], m(2), m(3), stack_.cuts[2]));
}

// -------- generateViaRects --------

// The plain path offers the lower shape, the upper shape, and the overlap of
// the two to every intermediate layer.
TEST_F(TestConnect, GenerateViaRectsOffersTheOverlapToEachIntermediateLayer)
{
  const TestableConnect connect(grid_, m(1), m(4));

  const odb::Rect lower(0, 0, 1000, 400);
  const odb::Rect upper(300, 0, 700, 1000);
  const odb::Rect overlap(300, 0, 700, 400);

  const auto rects = connect.generateViaRects(lower, upper);
  ASSERT_EQ(rects.size(), 4u);  // M1, M2, M3, M4
  EXPECT_THAT(rects[0], ::testing::ElementsAre(lower));
  EXPECT_THAT(rects[1], ::testing::ElementsAre(overlap));
  EXPECT_THAT(rects[2], ::testing::ElementsAre(overlap));
  EXPECT_THAT(rects[3], ::testing::ElementsAre(upper));
}

// -------- generateMinEnclosureViaRects: the default (no -min_width_layers) --

// By default each intermediate layer is offered both the full overlap and a
// min-width variant of it, so the generator can pick whichever fits. The
// min-width variant is clamped along the layer's *width* direction only.
TEST_F(TestConnect, MinEnclosureRectsAddAMinWidthVariantPerIntermediateLayer)
{
  const TestableConnect connect(grid_, m(1), m(4));

  const odb::Rect lower(0, 0, 1000, 400);
  const odb::Rect upper(300, 0, 700, 1000);
  const odb::Rect overlap(300, 0, 700, 400);
  const auto rects = stackRects(connect, lower, upper);

  // M2 is VERTICAL: x (its width axis) is clamped to 100 about the center at
  // x = 500; y is untouched.
  EXPECT_THAT(
      rects[1],
      ::testing::UnorderedElementsAre(overlap, odb::Rect(450, 0, 550, 400)));
  // M3 is HORIZONTAL: y is clamped to 100 about the center at y = 200.
  EXPECT_THAT(
      rects[2],
      ::testing::UnorderedElementsAre(overlap, odb::Rect(300, 150, 700, 250)));
  // The endpoint layers are never touched.
  EXPECT_THAT(rects[0], ::testing::ElementsAre(lower));
  EXPECT_THAT(rects[3], ::testing::ElementsAre(upper));
}

// An overlap already at min width in the layer's width direction produces a
// min-width variant identical to the overlap, so nothing is added.
TEST_F(TestConnect, MinEnclosureRectsAddNothingWhenAlreadyAtMinWidth)
{
  const TestableConnect connect(grid_, m(1), m(4));

  const odb::Rect lower(0, 0, 1000, kMinWidth);
  const odb::Rect upper(450, 0, 450 + kMinWidth, 1000);
  const odb::Rect overlap(450, 0, 550, 100);
  const auto rects = stackRects(connect, lower, upper);

  EXPECT_THAT(rects[1], ::testing::ElementsAre(overlap));
  EXPECT_THAT(rects[2], ::testing::ElementsAre(overlap));
}

// The variant is forced to *exactly* min width, so a sub-min-width overlap is
// grown rather than left alone.
TEST_F(TestConnect, MinEnclosureRectsGrowAnUndersizedOverlap)
{
  const TestableConnect connect(grid_, m(1), m(4));

  const odb::Rect lower(0, 0, 1000, 400);
  const odb::Rect upper(480, 0, 520, 1000);  // 40 wide, under the 100 min
  const odb::Rect overlap(480, 0, 520, 400);
  const auto rects = stackRects(connect, lower, upper);

  EXPECT_THAT(
      rects[1],
      ::testing::UnorderedElementsAre(overlap, odb::Rect(450, 0, 550, 400)));
}

// A single-layer via has no intermediate layers, so min-enclosure filtering
// is a no-op on both endpoints.
TEST_F(TestConnect, MinEnclosureRectsLeaveASingleLayerViaAlone)
{
  const TestableConnect connect(grid_, m(1), m(2));

  const odb::Rect lower(0, 0, 1000, 400);
  const odb::Rect upper(300, 0, 700, 1000);
  const auto rects = stackRects(connect, lower, upper);

  ASSERT_EQ(rects.size(), 2u);
  EXPECT_THAT(rects[0], ::testing::ElementsAre(lower));
  EXPECT_THAT(rects[1], ::testing::ElementsAre(upper));
}

// -------- -min_width_layers --------

// A layer named in -min_width_layers is offered *only* the min-width rect:
// the full-overlap candidate is dropped so the via array cannot fill the
// stripe with extra rows across the layer's width. Layers not named keep both.
TEST_F(TestConnect, MinWidthLayerDropsTheFullOverlapCandidate)
{
  TestableConnect connect(grid_, m(1), m(4));
  connect.setMinWidthLayers({m(2)});

  const odb::Rect lower(0, 0, 1000, 400);
  const odb::Rect upper(300, 0, 700, 1000);
  const odb::Rect overlap(300, 0, 700, 400);
  const auto rects = stackRects(connect, lower, upper);

  EXPECT_THAT(rects[1], ::testing::ElementsAre(odb::Rect(450, 0, 550, 400)));
  EXPECT_THAT(
      rects[2],
      ::testing::UnorderedElementsAre(overlap, odb::Rect(300, 150, 700, 250)));
}

// The clamp follows each layer's own routing direction: the VERTICAL M2 keeps
// its full y extent while the HORIZONTAL M3 keeps its full x extent. Neither
// is squared off.
TEST_F(TestConnect, MinWidthLayersClampOnlyTheirOwnWidthDirection)
{
  TestableConnect connect(grid_, m(1), m(4));
  connect.setMinWidthLayers({m(2), m(3)});

  const odb::Rect lower(0, 0, 1000, 400);
  const odb::Rect upper(300, 0, 700, 1000);
  const auto rects = stackRects(connect, lower, upper);

  ASSERT_EQ(rects[1].size(), 1u);
  ASSERT_EQ(rects[2].size(), 1u);
  const odb::Rect& m2_rect = *rects[1].begin();
  const odb::Rect& m3_rect = *rects[2].begin();

  EXPECT_EQ(m2_rect, odb::Rect(450, 0, 550, 400));
  EXPECT_EQ(m2_rect.dx(), kMinWidth);
  EXPECT_EQ(m2_rect.dy(), 400);  // preferred direction untouched

  EXPECT_EQ(m3_rect, odb::Rect(300, 150, 700, 250));
  EXPECT_EQ(m3_rect.dy(), kMinWidth);
  EXPECT_EQ(m3_rect.dx(), 400);  // preferred direction untouched
}

// Naming a layer whose overlap is already min width changes nothing -- the
// candidate set was already a single rect.
TEST_F(TestConnect, MinWidthLayerIsANoOpWhenAlreadyAtMinWidth)
{
  TestableConnect connect(grid_, m(1), m(4));
  connect.setMinWidthLayers({m(2), m(3)});

  const odb::Rect lower(0, 0, 1000, kMinWidth);
  const odb::Rect upper(450, 0, 450 + kMinWidth, 1000);
  const odb::Rect overlap(450, 0, 550, 100);
  const auto rects = stackRects(connect, lower, upper);

  EXPECT_THAT(rects[1], ::testing::ElementsAre(overlap));
  EXPECT_THAT(rects[2], ::testing::ElementsAre(overlap));
}

// A sub-min-width overlap is still grown to min width; -min_width_layers only
// removes the full-overlap alternative, it never shrinks below min width.
TEST_F(TestConnect, MinWidthLayerStillGrowsAnUndersizedOverlap)
{
  TestableConnect connect(grid_, m(1), m(4));
  connect.setMinWidthLayers({m(2)});

  const odb::Rect lower(0, 0, 1000, 400);
  const odb::Rect upper(480, 0, 520, 1000);
  const auto rects = stackRects(connect, lower, upper);

  EXPECT_THAT(rects[1], ::testing::ElementsAre(odb::Rect(450, 0, 550, 400)));
}

// Naming the stack's own endpoints (or any layer that is not an intermediate
// routing layer) is inert -- those layers land on real straps and are never
// min-width filtered.
TEST_F(TestConnect, MinWidthLayersIgnoreNonIntermediateLayers)
{
  TestableConnect connect(grid_, m(1), m(4));
  connect.setMinWidthLayers({m(1), m(4), stack_.cuts[0]});

  const odb::Rect lower(0, 0, 1000, 400);
  const odb::Rect upper(300, 0, 700, 1000);
  const odb::Rect overlap(300, 0, 700, 400);
  const auto rects = stackRects(connect, lower, upper);

  EXPECT_THAT(rects[0], ::testing::ElementsAre(lower));
  EXPECT_THAT(rects[3], ::testing::ElementsAre(upper));
  // The intermediate layers still get both candidates.
  EXPECT_EQ(rects[1].size(), 2u);
  EXPECT_THAT(
      rects[2],
      ::testing::UnorderedElementsAre(overlap, odb::Rect(300, 150, 700, 250)));
}

// setMinWidthLayers accumulates rather than replacing, matching setOnGrid.
TEST_F(TestConnect, SetMinWidthLayersAccumulatesAcrossCalls)
{
  TestableConnect connect(grid_, m(1), m(4));
  connect.setMinWidthLayers({m(2)});
  connect.setMinWidthLayers({m(3)});

  const odb::Rect lower(0, 0, 1000, 400);
  const odb::Rect upper(300, 0, 700, 1000);
  const auto rects = stackRects(connect, lower, upper);

  EXPECT_THAT(rects[1], ::testing::ElementsAre(odb::Rect(450, 0, 550, 400)));
  EXPECT_THAT(rects[2], ::testing::ElementsAre(odb::Rect(300, 150, 700, 250)));
}

// An empty list leaves the default behavior in place.
TEST_F(TestConnect, SetMinWidthLayersWithNoLayersIsANoOp)
{
  TestableConnect connect(grid_, m(1), m(4));
  connect.setMinWidthLayers({});

  const odb::Rect lower(0, 0, 1000, 400);
  const odb::Rect upper(300, 0, 700, 1000);
  const auto rects = stackRects(connect, lower, upper);

  EXPECT_EQ(rects[1].size(), 2u);
  EXPECT_EQ(rects[2].size(), 2u);
}

// The min-width rect is exactly min width even when that is odd, with the
// integer-division rounding landing on the low side of the center.
TEST_F(TestConnect, MinWidthLayerHandlesAnOddMinWidth)
{
  m(2)->setWidth(101);
  TestableConnect connect(grid_, m(1), m(4));
  connect.setMinWidthLayers({m(2)});

  const odb::Rect lower(0, 0, 1000, 400);
  const odb::Rect upper(300, 0, 700, 1000);
  const auto rects = stackRects(connect, lower, upper);

  ASSERT_EQ(rects[1].size(), 1u);
  const odb::Rect& m2_rect = *rects[1].begin();
  EXPECT_EQ(m2_rect.dx(), 101);
  EXPECT_EQ(m2_rect, odb::Rect(450, 0, 551, 400));
}

// -------- Interaction with the complex-stacked-via path --------

// An overlap narrower than an intermediate layer's min width makes the stack
// "complex": the intermediate rect is pre-grown to fit a via before the
// min-enclosure pass runs.
TEST_F(TestConnect, ComplexStackedViaIsDetectedFromAnUndersizedOverlap)
{
  const TestableConnect connect(grid_, m(1), m(4));

  // 40-wide overlap, under M2/M3's 100 min width.
  EXPECT_TRUE(connect.isComplexStackedVia(odb::Rect(0, 0, 1000, 400),
                                          odb::Rect(480, 0, 520, 1000)));
  // A comfortably sized overlap is not complex.
  EXPECT_FALSE(connect.isComplexStackedVia(odb::Rect(0, 0, 1000, 400),
                                           odb::Rect(300, 0, 700, 1000)));
}

// A single-layer via is never complex -- it has no intermediate layer that
// could be too narrow.
TEST_F(TestConnect, SingleLayerViaIsNeverComplex)
{
  const TestableConnect connect(grid_, m(1), m(2));

  EXPECT_FALSE(connect.isComplexStackedVia(odb::Rect(0, 0, 1000, 400),
                                           odb::Rect(480, 0, 520, 1000)));
}

// With no enclosure rules in the tech, the width a complex stack targets is
// just the layer min width.
TEST_F(TestConnect, GetMinWidthIsTheLayerMinWidthWithoutEnclosureRules)
{
  const TestableConnect connect(grid_, m(1), m(4));

  EXPECT_EQ(connect.getMinWidth(m(2)), kMinWidth);
}

// A cut enclosure rule on a neighboring cut layer widens the target so a via
// is guaranteed to fit: min width + 2 * the worst-case overhang.
TEST_F(TestConnect, GetMinWidthAddsTheWorstCaseCutEnclosure)
{
  addCutEnclosureRule(stack_.cuts[1],
                      /*first_overhang=*/30,
                      /*second_overhang=*/45);
  const TestableConnect connect(grid_, m(1), m(4));

  EXPECT_EQ(connect.getMinWidth(m(2)), kMinWidth + 2 * 45);
}

// At the bottom and top of the layer stack only the one neighboring cut layer
// has enclosures to account for.
TEST_F(TestConnect, GetMinWidthAtTheEndsOfTheStackLooksOneWay)
{
  addCutEnclosureRule(stack_.cuts[0], 30, 30);
  addCutEnclosureRule(stack_.cuts[2], 40, 40);
  const TestableConnect connect(grid_, m(1), m(4));

  EXPECT_EQ(connect.getMinWidth(m(1)), kMinWidth + 2 * 30);
  EXPECT_EQ(connect.getMinWidth(m(4)), kMinWidth + 2 * 40);
}

// A single-cut tech via on a neighboring cut layer widens the target by the
// overhang its widest metal implies: (300 - 50 cut - 100 min width) / 2. A
// tech via with more than one cut on that layer does not count.
TEST_F(TestConnect, GetMinWidthAddsTheOverhangOfASingleCutTechVia)
{
  odb::dbTechVia* single = odb::dbTechVia::create(tech(), "SINGLE");
  odb::dbBox::create(single, m(1), -35, -35, 35, 35);
  odb::dbBox::create(single, stack_.cuts[0], -25, -25, 25, 25);
  odb::dbBox::create(single, m(2), -150, -150, 150, 150);
  odb::dbTechVia* multi = odb::dbTechVia::create(tech(), "MULTI");
  odb::dbBox::create(multi, m(1), -500, -500, 500, 500);
  odb::dbBox::create(multi, stack_.cuts[0], -75, -25, -25, 25);
  odb::dbBox::create(multi, stack_.cuts[0], 25, -25, 75, 25);
  odb::dbBox::create(multi, m(2), -500, -500, 500, 500);
  const TestableConnect connect(grid_, m(1), m(4));

  EXPECT_EQ(connect.getMinWidth(m(2)), kMinWidth + 2 * 75);
}

// The tapered path grows an overlap that is too short as well as one that is
// too narrow, about its center.
TEST_F(TestConnect, ComplexStackedViaRectsGrowAnOverlapThatIsTooShort)
{
  const TestableConnect connect(grid_, m(1), m(4));

  const odb::Rect lower(0, 480, 1000, 520);  // 40 tall, under the 100 min
  const odb::Rect upper(300, 0, 700, 1000);
  ASSERT_TRUE(connect.isComplexStackedVia(lower, upper));

  const auto rects = connect.generateComplexStackedViaRects(lower, upper);
  EXPECT_THAT(rects[1], ::testing::ElementsAre(odb::Rect(300, 450, 700, 550)));
  EXPECT_THAT(rects[2], ::testing::ElementsAre(odb::Rect(300, 450, 700, 550)));
}

// The complex path grows the intermediate rect to min width, and the
// min-enclosure pass then clamps that grown rect back in the width direction.
// With -min_width_layers only the clamped rect survives.
TEST_F(TestConnect, MinWidthLayersComposeWithTheComplexStackedPath)
{
  TestableConnect connect(grid_, m(1), m(4));
  connect.setMinWidthLayers({m(3)});

  const odb::Rect lower(0, 0, 1000, 400);
  const odb::Rect upper(480, 0, 520, 1000);  // 40-wide overlap -> complex
  ASSERT_TRUE(connect.isComplexStackedVia(lower, upper));

  auto rects = connect.generateComplexStackedViaRects(lower, upper);
  // Both intermediate layers were grown to 100 in x by the complex path.
  EXPECT_THAT(rects[1], ::testing::ElementsAre(odb::Rect(450, 0, 550, 400)));
  EXPECT_THAT(rects[2], ::testing::ElementsAre(odb::Rect(450, 0, 550, 400)));

  connect.generateMinEnclosureViaRects(rects);
  // M2 (vertical) is already 100 wide in x, so its candidate set collapses to
  // the one rect; M3 (horizontal) is clamped in y and, being min-width, keeps
  // only the clamped rect.
  EXPECT_THAT(rects[1], ::testing::ElementsAre(odb::Rect(450, 0, 550, 400)));
  EXPECT_THAT(rects[2], ::testing::ElementsAre(odb::Rect(450, 150, 550, 250)));
}

// -------- Other option accessors --------

// The cut pitch and max rows/columns options round-trip.
TEST_F(TestConnect, CutPitchAndMaxRowsColumnsRoundTrip)
{
  TestableConnect connect(grid_, m(1), m(4));

  EXPECT_FALSE(connect.hasCutPitch());
  connect.setCutPitch(120, 240);
  EXPECT_TRUE(connect.hasCutPitch());
  EXPECT_EQ(connect.getCutPitchX(), 120);
  EXPECT_EQ(connect.getCutPitchY(), 240);

  connect.setMaxRows(3);
  connect.setMaxColumns(5);
  EXPECT_EQ(connect.getMaxRows(), 3);
  EXPECT_EQ(connect.getMaxColumns(), 5);
}

// setSplitCuts drops the stack's endpoints -- only intermediate layers can be
// split -- and reports pitch/stagger per remaining layer.
TEST_F(TestConnect, SplitCutsAreKeptOnlyForIntermediateLayers)
{
  TestableConnect connect(grid_, m(1), m(4));

  odb::PtrMap<odb::dbTechLayer, Connect::SplitCut> splits;
  splits[m(1)] = {/*pitch=*/200, /*stagger=*/false};
  splits[m(2)] = {/*pitch=*/300, /*stagger=*/true};
  splits[m(4)] = {/*pitch=*/400, /*stagger=*/false};
  connect.setSplitCuts(splits);

  EXPECT_EQ(connect.getSplitCutPitch(m(1)), 0);
  EXPECT_FALSE(connect.getSplitCutStagger(m(1)));
  EXPECT_EQ(connect.getSplitCutPitch(m(2)), 300);
  EXPECT_TRUE(connect.getSplitCutStagger(m(2)));
  EXPECT_EQ(connect.getSplitCutPitch(m(4)), 0);
  // A layer with no entry reports no split.
  EXPECT_EQ(connect.getSplitCutPitch(m(3)), 0);
  EXPECT_FALSE(connect.getSplitCutStagger(m(3)));
}

// -------- report --------

// report() names the min-width layers, the same way it names the ongrid
// layers -- the option is invisible in `pdngen -report_only` otherwise.
TEST_F(TestConnect, ReportNamesTheMinWidthLayers)
{
  TestableConnect connect(grid_, m(1), m(4));
  connect.setOnGrid({m(2)});
  connect.setMinWidthLayers({m(2), m(3)});

  const std::string report = captureReport(connect);
  EXPECT_THAT(report, ::testing::HasSubstr("Connect layers M1 -> M4"));
  EXPECT_THAT(report, ::testing::HasSubstr("Ongrid layers: M2"));
  EXPECT_THAT(report, ::testing::HasSubstr("Minimum width layers: M2 M3"));
}

// Without the option the line is omitted entirely, so untouched connect
// rules keep their existing report output.
TEST_F(TestConnect, ReportOmitsMinWidthLayersWhenUnset)
{
  const TestableConnect connect(grid_, m(1), m(4));

  EXPECT_THAT(captureReport(connect),
              ::testing::Not(::testing::HasSubstr("Minimum width layers")));
}

// containsIntermediateLayer covers cut layers as well as routing layers, and
// excludes the endpoints.
TEST_F(TestConnect, ContainsIntermediateLayerExcludesTheEndpoints)
{
  const TestableConnect connect(grid_, m(1), m(4));

  EXPECT_FALSE(connect.containsIntermediateLayer(m(1)));
  EXPECT_TRUE(connect.containsIntermediateLayer(m(2)));
  EXPECT_TRUE(connect.containsIntermediateLayer(stack_.cuts[0]));
  EXPECT_FALSE(connect.containsIntermediateLayer(m(4)));
}

// getAllLayers walks the whole stack; getAllRoutingLayers skips the cuts.
TEST_F(TestConnect, GetAllLayersAndGetAllRoutingLayers)
{
  const TestableConnect connect(grid_, m(1), m(4));

  EXPECT_THAT(connect.getAllLayers(),
              ::testing::ElementsAre(m(1),
                                     stack_.cuts[0],
                                     m(2),
                                     stack_.cuts[1],
                                     m(3),
                                     stack_.cuts[2],
                                     m(4)));
  EXPECT_THAT(connect.getAllRoutingLayers(),
              ::testing::ElementsAre(m(1), m(2), m(3), m(4)));
}

// overlaps / startsBelow compare the layer ranges of two connect rules.
TEST_F(TestConnect, OverlapsAndStartsBelowCompareLayerRanges)
{
  const TestableConnect low(grid_, m(1), m(2));
  const TestableConnect high(grid_, m(3), m(4));
  const TestableConnect spanning(grid_, m(2), m(3));

  EXPECT_TRUE(low.startsBelow(&high));
  EXPECT_FALSE(high.startsBelow(&low));

  EXPECT_FALSE(low.overlaps(&high));
  EXPECT_FALSE(high.overlaps(&low));
  EXPECT_TRUE(low.overlaps(&spanning));
  EXPECT_TRUE(spanning.overlaps(&high));
}

// -------- Construction errors --------

// A connect rule needs two different layers.
TEST_F(TestConnect, ConstructorRejectsTheSameLayerTwice)
{
  EXPECT_THROW(Connect(grid_, m(1), m(1)), std::runtime_error);
}

// Both ends of a connect rule must be routing layers.
TEST_F(TestConnect, ConstructorRejectsNonRoutingLayers)
{
  EXPECT_THROW(Connect(grid_, stack_.cuts[0], m(2)), std::runtime_error);
  EXPECT_THROW(Connect(grid_, m(1), stack_.cuts[0]), std::runtime_error);
}

// No via can cross between the front side and the backside of the die.
TEST_F(TestConnect, ConstructorRejectsCrossingToTheBackside)
{
  m(4)->setBackside(true);
  EXPECT_THROW(Connect(grid_, m(1), m(4)), std::runtime_error);
}

// -------- Accessors --------

// A connect rule applies to a via only when both of its layers match.
TEST_F(TestConnect, AppliesToViaMatchesTheExactLayerPair)
{
  Connect m1_m2(grid_, m(1), m(2));
  Connect m1_m3(grid_, m(1), m(3));
  Connect m2_m3(grid_, m(2), m(3));
  auto via_on = [&](Connect& connect) {
    return std::make_shared<Via>(
        &connect, power_, odb::Rect(0, 0, 100, 100), nullptr, nullptr);
  };

  EXPECT_TRUE(m1_m2.appliesToVia(via_on(m1_m2)));
  EXPECT_FALSE(m1_m2.appliesToVia(via_on(m1_m3)));
  EXPECT_FALSE(m1_m2.appliesToVia(via_on(m2_m3)));
}

TEST_F(TestConnect, GridRoundTrips)
{
  Connect connect(grid_, m(1), m(2));
  EXPECT_EQ(connect.getGrid(), grid_);
  connect.setGrid(nullptr);
  EXPECT_EQ(connect.getGrid(), nullptr);
}

// report() lists every option that has been set.
TEST_F(TestConnect, ReportListsEveryOption)
{
  Connect connect(grid_, m(1), m(3));
  connect.addFixedVia(addGenerateRule(1, "RULE12"));
  connect.addFixedVia(addTechVia(2, "VIA23"));
  connect.setCutPitch(120, 240);
  connect.setMaxRows(3);
  connect.setMaxColumns(5);
  odb::PtrMap<odb::dbTechLayer, Connect::SplitCut> splits;
  splits[m(2)] = {/*pitch=*/300, /*stagger=*/true};
  connect.setSplitCuts(splits);

  const std::string report = captureReport(connect);
  EXPECT_THAT(report, ::testing::HasSubstr("Fixed vias: RULE12 VIA23"));
  EXPECT_THAT(report, ::testing::HasSubstr("Cut pitch X-direction: 0.1200"));
  EXPECT_THAT(report, ::testing::HasSubstr("Cut pitch Y-direction: 0.2400"));
  EXPECT_THAT(report, ::testing::HasSubstr("Maximum rows: 3"));
  EXPECT_THAT(report, ::testing::HasSubstr("Maximum columns: 5"));
  EXPECT_THAT(report,
              ::testing::HasSubstr("Layer: M2 with pitch 0.3000 staggered"));
}

// -------- makeVia --------

// A generate rule builds a via at the center of the overlap: here a 2 x 2
// array, as two 50 cuts on a 100 pitch with 10 of enclosure fit in 200.
// Another overlap of the same size reuses the same via definition.
TEST_F(TestConnect, MakeViaPlacesAGeneratedViaAtTheOverlap)
{
  odb::dbTechViaGenerateRule* rule = addGenerateRule(1, "RULE12");
  Connect connect(grid_, m(1), m(2));
  odb::dbSWire* wire = makeWire();

  DbVia::ViaLayerShape shapes;
  connect.makeVia(wire,
                  makeShape(1, kM1Strap),
                  makeShape(2, kM2Strap),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  EXPECT_FALSE(shapes.bottom.empty());
  EXPECT_FALSE(shapes.top.empty());

  connect.makeVia(wire,
                  makeShape(1, kM1Strap),
                  makeShape(2, odb::Rect(4000, 0, 4200, 5000)),
                  odb::dbWireShapeType::STRIPE,
                  shapes);

  std::set<odb::Point> centers;
  for (odb::dbSBox* via : placedVias(wire)) {
    centers.insert(via->getViaXY());
  }
  EXPECT_THAT(
      centers,
      ::testing::ElementsAre(odb::Point(2100, 1100), odb::Point(4100, 1100)));
  ASSERT_EQ(block()->getVias().size(), 1u);
  odb::dbVia* via = *block()->getVias().begin();
  EXPECT_EQ(via->getViaGenerateRule(), rule);
  EXPECT_EQ(via->getViaParams().getNumCutRows(), 2);
  EXPECT_EQ(via->getViaParams().getNumCutCols(), 2);
}

// The connect rule's cut pitch overrides the generate rule's.
TEST_F(TestConnect, MakeViaUsesTheConnectCutPitch)
{
  addGenerateRule(1, "RULE12");
  Connect connect(grid_, m(1), m(2));
  connect.setCutPitch(150, 150);

  DbVia::ViaLayerShape shapes;
  connect.makeVia(makeWire(),
                  makeShape(1, kM1Strap),
                  makeShape(2, kM2Strap),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  ASSERT_EQ(block()->getVias().size(), 1u);
  const odb::dbViaParams params = (*block()->getVias().begin())->getViaParams();
  EXPECT_EQ(params.getXCutSpacing(), 150 - 50);
  EXPECT_EQ(params.getYCutSpacing(), 150 - 50);
}

// The via goes at the center of the overlap after its edges are snapped to
// twice the manufacturing grid, which keeps that center on grid. The M1 strap
// runs from 1005 to 1200, whose midpoint of 1102.5 is off the 5 grid; snapped
// to 1010 and 1200 the midpoint is 1105.
TEST_F(TestConnect, MakeViaCentersTheViaOnTheManufacturingGrid)
{
  addGenerateRule(1, "RULE12");
  Connect connect(grid_, m(1), m(2));
  odb::dbSWire* wire = makeWire();
  ShapePtr lower = makeShape(1, odb::Rect(0, 1005, 5000, 1200));

  DbVia::ViaLayerShape shapes;
  connect.makeVia(wire,
                  lower,
                  makeShape(2, kM2Strap),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  const std::vector<odb::dbSBox*> vias = placedVias(wire);
  ASSERT_EQ(vias.size(), 1u);
  EXPECT_EQ(vias[0]->getViaXY(), odb::Point(2100, 1105));
  ASSERT_FALSE(shapes.bottom.empty());
  for (const auto& [rect, box] : shapes.bottom) {
    EXPECT_TRUE(lower->getRect().contains(rect));
  }
}

// With no generate rule for the layers, a tech via is used instead.
TEST_F(TestConnect, MakeViaFallsBackToATechVia)
{
  odb::dbTechVia* tech_via = addTechVia(1, "VIA12");
  Connect connect(grid_, m(1), m(2));
  odb::dbSWire* wire = makeWire();

  DbVia::ViaLayerShape shapes;
  connect.makeVia(wire,
                  makeShape(1, kM1Strap),
                  makeShape(2, kM2Strap),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  const std::vector<odb::dbSBox*> vias = placedVias(wire);
  ASSERT_EQ(vias.size(), 1u);
  EXPECT_EQ(vias[0]->getTechVia(), tech_via);
  EXPECT_EQ(vias[0]->getViaXY(), odb::Point(2100, 1100));
}

// A generate rule restricted to wider straps is passed over for these 200
// wide ones, and the tech via used instead.
TEST_F(TestConnect, MakeViaSkipsAGenerateRuleMeantForOtherWidths)
{
  odb::dbTechViaGenerateRule* rule = addGenerateRule(1, "RULE12");
  for (uint32_t i = 0; i < rule->getViaLayerRuleCount(); i++) {
    odb::dbTechViaLayerRule* layer_rule = rule->getViaLayerRule(i);
    if (layer_rule->getLayer()->getType() == odb::dbTechLayerType::ROUTING) {
      layer_rule->setWidth(1000, 2000);
    }
  }
  odb::dbTechVia* tech_via = addTechVia(1, "VIA12");
  Connect connect(grid_, m(1), m(2));
  odb::dbSWire* wire = makeWire();

  DbVia::ViaLayerShape shapes;
  connect.makeVia(wire,
                  makeShape(1, kM1Strap),
                  makeShape(2, kM2Strap),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  const std::vector<odb::dbSBox*> vias = placedVias(wire);
  ASSERT_EQ(vias.size(), 1u);
  EXPECT_EQ(vias[0]->getTechVia(), tech_via);
}

// A tech via whose metal would spill out of a shape that cannot grow is
// passed over too, and with nothing left no via is placed. The failure is
// only a candidate for a recheck, which is not reported.
TEST_F(TestConnect, MakeViaPlacesNothingWhenNoViaFits)
{
  addTechVia(1, "VIA12");
  Connect connect(grid_, m(1), m(2));
  odb::dbSWire* wire = makeWire();
  ShapePtr pad = makeShape(1, odb::Rect(2050, 1050, 2150, 1150));
  pad->setLocked();

  DbVia::ViaLayerShape shapes;
  connect.makeVia(wire,
                  pad,
                  makeShape(2, odb::Rect(2000, 0, 2100, 5000)),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  EXPECT_TRUE(shapes.bottom.empty());
  EXPECT_TRUE(shapes.top.empty());
  EXPECT_TRUE(wire->getWires().empty());

  connect.recordFailedVias();
  EXPECT_EQ(failures("Recheck"), nullptr);
}

// A rule that cannot meet the layer's minimum cut count is not built from:
// nine cuts are required, and only four fit.
TEST_F(TestConnect, MakeViaPlacesNothingWhenNoRuleMeetsTheMinimumCuts)
{
  addGenerateRule(1, "RULE12");
  addMinCutRuleV54(m(1), /*num_cuts=*/9, /*width=*/100);
  Connect connect(grid_, m(1), m(2));
  odb::dbSWire* wire = makeWire();

  DbVia::ViaLayerShape shapes;
  connect.makeVia(wire,
                  makeShape(1, kM1Strap),
                  makeShape(2, kM2Strap),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  EXPECT_TRUE(wire->getWires().empty());
}

// Of two rules that can both build a via, the one with more cut area wins.
TEST_F(TestConnect, MakeViaPrefersTheRuleWithMoreCutArea)
{
  addGenerateRule(1, "SMALL", /*cut_size=*/30);
  odb::dbTechViaGenerateRule* big = addGenerateRule(1, "BIG");
  Connect connect(grid_, m(1), m(2));

  DbVia::ViaLayerShape shapes;
  connect.makeVia(makeWire(),
                  makeShape(1, kM1Strap),
                  makeShape(2, kM2Strap),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  ASSERT_EQ(block()->getVias().size(), 1u);
  EXPECT_EQ((*block()->getVias().begin())->getViaGenerateRule(), big);
}

// A shape that cannot grow -- one that is locked, or that an instance pin
// lands on -- must hold the whole via, so the via is narrowed to fit rather
// than allowed to spill past the shape's end.
//
// Narrowed to fit, the via has no enclosure past the shape's end, where the
// rule asks for 10, so it does not meet the rule either. Only that it stays
// inside the shape is checked.
TEST_F(TestConnect, MakeViaKeepsTheViaInsideAShapeThatCannotGrow)
{
  addGenerateRule(1, "RULE12");
  Connect connect(grid_, m(1), m(2));

  // The M1 strap ends 50 into the M2 strap, so a via's 10 of enclosure past
  // a 50 cut would reach past its end.
  ShapePtr locked = makeShape(1, odb::Rect(0, 1000, 2050, 1200));
  locked->setLocked();
  DbVia::ViaLayerShape shapes;
  connect.makeVia(makeWire(),
                  locked,
                  makeShape(2, kM2Strap),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  ASSERT_FALSE(shapes.bottom.empty());
  for (const auto& [rect, box] : shapes.bottom) {
    EXPECT_TRUE(locked->getRect().contains(rect));
  }

  // The same for an M2 strap that ends 50 into the M1 strap and carries an
  // instance pin.
  ShapePtr pinned = makeShape(2, odb::Rect(2000, 0, 2200, 1050));
  pinned->addITermConnection(odb::Rect(2000, 0, 2200, 100));
  connect.makeVia(makeWire(),
                  makeShape(1, kM1Strap),
                  pinned,
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  ASSERT_FALSE(shapes.top.empty());
  for (const auto& [rect, box] : shapes.top) {
    EXPECT_TRUE(pinned->getRect().contains(rect));
  }
}

// Vias are cached by the size of the overlap, but one that took enclosure
// from the strap past the overlap depends on that strap too, so it is not
// reused. The cut layer requires 30 of enclosure below the cut, and an 80
// wide overlap leaves 15: the first strap runs past the overlap and lends the
// rest, the second, the same 80 wide as the overlap, has none to lend, so no
// legal via fits it.
TEST_F(TestConnect, MakeViaDoesNotReuseAViaThatBorrowedEnclosure)
{
  addGenerateRule(1, "RULE12");
  addAboveBelowCutEnclosureRule(
      stack_.cuts[0], 30, 30, /*above=*/false, /*below=*/true);
  Connect connect(grid_, m(1), m(2));

  DbVia::ViaLayerShape shapes;
  odb::dbSWire* wide = makeWire();
  connect.makeVia(wide,
                  makeShape(1, odb::Rect(0, 0, 400, 2000)),
                  makeShape(2, odb::Rect(160, 900, 240, 1100)),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  EXPECT_EQ(placedVias(wide).size(), 1u);

  odb::dbSWire* exact = makeWire();
  connect.makeVia(exact,
                  makeShape(1, odb::Rect(1160, 0, 1240, 2000)),
                  makeShape(2, odb::Rect(1160, 900, 1240, 1100)),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  EXPECT_TRUE(placedVias(exact).empty());
}

// A connect rule spanning several layers stacks a via on every cut layer
// in between.
TEST_F(TestConnect, MakeViaStacksAViaOnEachCutLayer)
{
  addGenerateRule(1, "RULE12");
  addGenerateRule(2, "RULE23");
  Connect connect(grid_, m(1), m(3));
  odb::dbSWire* wire = makeWire();

  DbVia::ViaLayerShape shapes;
  connect.makeVia(wire,
                  makeShape(1, kM1Strap),
                  makeShape(3, odb::Rect(2000, 900, 2200, 1300)),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  odb::PtrSet<odb::dbTechLayer> cut_layers;
  for (odb::dbSBox* via : placedVias(wire)) {
    cut_layers.insert(via->getBlockVia()->getViaParams().getCutLayer());
  }
  EXPECT_THAT(cut_layers,
              ::testing::UnorderedElementsAre(stack_.cuts[0], stack_.cuts[1]));
}

// An overlap narrower than the intermediate layer's minimum width takes the
// tapered path, which offers the intermediate layer a rect grown to its
// minimum width, and the stack is still built.
//
// The M3 strap is itself under M3's minimum width, so no via here meets every
// rule: the M3 enclosure across the strap is 5 against the rule's 10, and the
// M2 metal between the two vias is 70 wide against M2's minimum of 100. Only
// that the stack is built is checked.
TEST_F(TestConnect, MakeViaStacksThroughAnOverlapUnderTheMinimumWidth)
{
  addGenerateRule(1, "RULE12");
  addGenerateRule(2, "RULE23");
  Connect connect(grid_, m(1), m(3));
  odb::dbSWire* wire = makeWire();

  DbVia::ViaLayerShape shapes;
  connect.makeVia(wire,
                  makeShape(1, kM1Strap),
                  makeShape(3, odb::Rect(2000, 1000, 2060, 1200)),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  ASSERT_TRUE(connect.isMultiLayerVia());
  EXPECT_EQ(placedVias(wire).size(), 2u);
  EXPECT_FALSE(shapes.bottom.empty());
  EXPECT_FALSE(shapes.top.empty());
}

// Split cuts on an intermediate layer place one single-cut via per pitch, and
// a staggered split offsets the ground net's cuts by half a pitch so they
// interleave with the power net's.
TEST_F(TestConnect, MakeViaStaggersSplitCutsOnTheGroundNet)
{
  addGenerateRule(1, "RULE12");
  addGenerateRule(2, "RULE23");
  Connect connect(grid_, m(1), m(3));
  odb::PtrMap<odb::dbTechLayer, Connect::SplitCut> splits;
  splits[m(2)] = {/*pitch=*/200, /*stagger=*/true};
  connect.setSplitCuts(splits);

  auto cut_positions = [&](odb::dbNet* net) {
    odb::dbSWire* wire = makeWire(net);
    DbVia::ViaLayerShape shapes;
    connect.makeVia(wire,
                    makeShape(1, odb::Rect(0, 1000, 5000, 1600), net),
                    makeShape(3, odb::Rect(2000, 1000, 2600, 1600), net),
                    odb::dbWireShapeType::STRIPE,
                    shapes);
    std::set<int> xs;
    for (odb::dbSBox* via : placedVias(wire)) {
      if (via->getBlockVia()->getViaParams().getCutLayer() == stack_.cuts[0]) {
        xs.insert(via->getViaXY().x());
      }
    }
    return xs;
  };

  const std::set<int> power = cut_positions(power_);
  const std::set<int> ground = cut_positions(ground_);
  ASSERT_GT(power.size(), 1u);
  ASSERT_EQ(ground.size(), power.size());
  EXPECT_EQ(*std::next(power.begin()) - *power.begin(), 200);
  EXPECT_EQ(*ground.begin() - *power.begin(), 100);
}

// -------- Fixed vias and filtering --------

// A fixed generate rule is the only rule used, even when a better one, or a
// tech via, is available.
TEST_F(TestConnect, AFixedGenerateRuleIsTheOnlyRuleUsed)
{
  odb::dbTechViaGenerateRule* small = addGenerateRule(1, "SMALL", 30);
  addGenerateRule(1, "BIG");
  addTechVia(1, "VIA12");
  Connect connect(grid_, m(1), m(2));
  connect.addFixedVia(small);
  odb::dbSWire* wire = makeWire();

  DbVia::ViaLayerShape shapes;
  connect.makeVia(wire,
                  makeShape(1, kM1Strap),
                  makeShape(2, kM2Strap),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  const std::vector<odb::dbSBox*> vias = placedVias(wire);
  ASSERT_EQ(vias.size(), 1u);
  ASSERT_NE(vias[0]->getBlockVia(), nullptr);
  EXPECT_EQ(vias[0]->getBlockVia()->getViaGenerateRule(), small);
}

// A fixed tech via is the only via used, even when a generate rule is
// available.
TEST_F(TestConnect, AFixedTechViaIsTheOnlyViaUsed)
{
  addGenerateRule(1, "RULE12");
  addTechVia(1, "VIA12_A");
  odb::dbTechVia* fixed = addTechVia(1, "VIA12_B");
  Connect connect(grid_, m(1), m(2));
  connect.addFixedVia(fixed);
  odb::dbSWire* wire = makeWire();

  DbVia::ViaLayerShape shapes;
  connect.makeVia(wire,
                  makeShape(1, kM1Strap),
                  makeShape(2, kM2Strap),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  const std::vector<odb::dbSBox*> vias = placedVias(wire);
  ASSERT_EQ(vias.size(), 1u);
  EXPECT_EQ(vias[0]->getTechVia(), fixed);
}

// A generate rule has to describe exactly a bottom metal, a cut and a top
// metal to be used; one missing its cut layer is ignored.
TEST_F(TestConnect, AGenerateRuleWithoutThreeLayersIsIgnored)
{
  auto* partial = odb::dbTechViaGenerateRule::create(tech(), "PARTIAL", false);
  odb::dbTechViaLayerRule::create(tech(), partial, m(1));
  odb::dbTechViaLayerRule::create(tech(), partial, m(2));
  odb::dbTechVia* tech_via = addTechVia(1, "VIA12");
  Connect connect(grid_, m(1), m(2));
  odb::dbSWire* wire = makeWire();

  DbVia::ViaLayerShape shapes;
  connect.makeVia(wire,
                  makeShape(1, kM1Strap),
                  makeShape(2, kM2Strap),
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  const std::vector<odb::dbSBox*> vias = placedVias(wire);
  ASSERT_EQ(vias.size(), 1u);
  EXPECT_EQ(vias[0]->getTechVia(), tech_via);
}

// filterVias drops every generate rule and tech via whose name matches.
TEST_F(TestConnect, FilterViasDropsMatchingRulesAndVias)
{
  odb::dbTechViaGenerateRule* small = addGenerateRule(1, "SMALL", 30);
  addGenerateRule(1, "BIG");
  Connect generate(grid_, m(1), m(2));
  generate.filterVias("BIG");

  DbVia::ViaLayerShape shapes;
  generate.makeVia(makeWire(),
                   makeShape(1, kM1Strap),
                   makeShape(2, kM2Strap),
                   odb::dbWireShapeType::STRIPE,
                   shapes);
  ASSERT_EQ(block()->getVias().size(), 1u);
  EXPECT_EQ((*block()->getVias().begin())->getViaGenerateRule(), small);

  addTechVia(2, "VIA23_A");
  odb::dbTechVia* kept = addTechVia(2, "VIA23_B");
  Connect tech_vias(grid_, m(2), m(3));
  tech_vias.filterVias("_A$");
  odb::dbSWire* wire = makeWire();
  tech_vias.makeVia(wire,
                    makeShape(2, kM2Strap),
                    makeShape(3, kM1Strap),
                    odb::dbWireShapeType::STRIPE,
                    shapes);
  const std::vector<odb::dbSBox*> vias = placedVias(wire);
  ASSERT_EQ(vias.size(), 1u);
  EXPECT_EQ(vias[0]->getTechVia(), kept);
}

// -------- Failure reporting --------

// recordFailedVias files a marker per failed via under a category per
// reason, except for vias only waiting on a recheck. Recording again reuses
// the categories.
TEST_F(TestConnect, RecordFailedViasFilesAMarkerPerReason)
{
  Connect connect(grid_, m(1), m(2));
  const odb::Rect area(0, 0, 100, 100);
  for (const FailedViaReason reason : {FailedViaReason::kObstructed,
                                       FailedViaReason::kOverlapping,
                                       FailedViaReason::kBuild,
                                       FailedViaReason::kRipup,
                                       FailedViaReason::kRecheck,
                                       FailedViaReason::kOther}) {
    connect.addFailedVia(reason, area, power_);
  }
  connect.recordFailedVias();
  connect.recordFailedVias();

  for (const char* reason :
       {"Obstructed", "Overlapping", "Build", "Ripup", "Other"}) {
    odb::dbMarkerCategory* category = failures(reason);
    ASSERT_NE(category, nullptr) << reason;
    EXPECT_EQ(category->getMarkerCount(), 2) << reason;
  }
  EXPECT_EQ(failures("Recheck"), nullptr);
}

// Markers stop at the category's limit.
TEST_F(TestConnect, RecordFailedViasStopsAtTheCategoryLimit)
{
  Connect connect(grid_, m(1), m(2));
  connect.addFailedVia(
      FailedViaReason::kBuild, odb::Rect(0, 0, 10, 10), power_);
  connect.addFailedVia(
      FailedViaReason::kBuild, odb::Rect(20, 0, 30, 10), power_);
  odb::dbMarkerCategory* tool = odb::dbMarkerCategory::create(block(), "PDN");
  odb::dbMarkerCategory* via = odb::dbMarkerCategory::createOrGet(tool, "Via");
  odb::dbMarkerCategory::createOrGet(via, "Build - grid - M1 -> M2")
      ->setMaxMarkers(1);

  connect.recordFailedVias();
  EXPECT_EQ(failures("Build")->getMarkerCount(), 1);
}

// clearShapes forgets the failures recorded so far.
TEST_F(TestConnect, ClearShapesForgetsTheFailures)
{
  Connect connect(grid_, m(1), m(2));
  connect.addFailedVia(
      FailedViaReason::kBuild, odb::Rect(0, 0, 10, 10), power_);
  connect.clearShapes();
  connect.recordFailedVias();
  EXPECT_EQ(block()->findMarkerCategory("PDN"), nullptr);
}

// A dummy via standing in for one that could not be built reports it as a
// build failure when asked to.
TEST_F(TestConnect, ADummyViaRecordsABuildFailure)
{
  Connect connect(grid_, m(1), m(2));
  DbGenerateDummyVia dummy(&connect,
                           odb::Rect(-50, -50, 50, 50),
                           m(1),
                           m(2),
                           /*add_report=*/true,
                           "");
  dummy.generate(block(),
                 makeWire(),
                 odb::dbWireShapeType::STRIPE,
                 1000,
                 1000,
                 {},
                 getLogger());
  connect.recordFailedVias();

  odb::dbMarkerCategory* category = failures("Build");
  ASSERT_NE(category, nullptr);
  EXPECT_EQ(category->getMarkerCount(), 1);
}

// With the Write debug group on, printViaReport counts the vias placed from
// each definition. A via that could not be cached is not in the count.
TEST_F(TestConnect, PrintViaReportCountsThePlacedVias)
{
  addGenerateRule(1, "RULE12");
  Connect connect(grid_, m(1), m(2));
  DbVia::ViaLayerShape shapes;
  odb::dbSWire* wire = makeWire();
  for (const int x : {2000, 3000}) {
    connect.makeVia(wire,
                    makeShape(1, kM1Strap),
                    makeShape(2, odb::Rect(x, 0, x + 200, 5000)),
                    odb::dbWireShapeType::STRIPE,
                    shapes);
  }
  ShapePtr locked = makeShape(2, odb::Rect(4000, 0, 4300, 5000));
  locked->setLocked();
  connect.makeVia(wire,
                  makeShape(1, kM1Strap),
                  locked,
                  odb::dbWireShapeType::STRIPE,
                  shapes);
  ASSERT_EQ(placedVias(wire).size(), 3u);

  EXPECT_TRUE(captureLog([&] { connect.printViaReport(); }).empty());

  getLogger()->setDebugLevel(utl::PDN, "Write", 2);
  const std::string report = captureLog([&] { connect.printViaReport(); });
  EXPECT_THAT(report,
              ::testing::HasSubstr("Vias (grid) from M1 -> M2: 1 types"));
  EXPECT_THAT(report, ::testing::HasSubstr("Via \"RULE12\": 2"));
}

// -------- Via --------

// A via knows its net, area, shapes and connect rule, and takes its layers
// from the connect rule.
TEST_F(TestConnect, ViaAccessorsReflectItsConnectAndShapes)
{
  Connect connect(grid_, m(1), m(3));
  ShapePtr lower = makeShape(1, kM1Strap);
  ShapePtr upper = makeShape(3, kM1Strap);
  const odb::Rect area(2000, 1000, 2200, 1200);
  Via via(&connect, power_, area, lower, upper);

  EXPECT_EQ(via.getNet(), power_);
  EXPECT_EQ(via.getArea(), area);
  EXPECT_EQ(via.getConnect(), &connect);
  EXPECT_EQ(via.getGrid(), grid_);
  EXPECT_EQ(via.getLowerLayer(), m(1));
  EXPECT_EQ(via.getUpperLayer(), m(3));
  EXPECT_EQ(via.getLowerShape(), lower);
  EXPECT_EQ(via.getUpperShape(), upper);
  EXPECT_TRUE(via.containsIntermediateLayer(m(2)));
  EXPECT_FALSE(via.containsIntermediateLayer(m(1)));
  EXPECT_FALSE(via.isFailed());

  ShapePtr other = makeShape(1, kM1Strap);
  via.setLowerShape(other);
  via.setUpperShape(other);
  EXPECT_EQ(via.getLowerShape(), other);
  EXPECT_EQ(via.getUpperShape(), other);
}

// The display text names the net, the layers and the grid.
TEST_F(TestConnect, ViaDisplayTextNamesTheNetLayersAndGrid)
{
  Connect connect(grid_, m(1), m(2));
  Via via(&connect, power_, odb::Rect(0, 0, 100, 100), nullptr, nullptr);
  EXPECT_EQ(via.getDisplayText(), "VDD:M1:M2:grid");

  connect.setGrid(nullptr);
  EXPECT_EQ(via.getDisplayText(), "VDD:M1:M2:");
}

// A via is valid only while it has both of its shapes; removing either one,
// but not an unrelated shape, invalidates it.
TEST_F(TestConnect, ViaRemoveShapeInvalidatesIt)
{
  Connect connect(grid_, m(1), m(2));
  ShapePtr lower = makeShape(1, kM1Strap);
  ShapePtr upper = makeShape(2, kM2Strap);
  ShapePtr unrelated = makeShape(1, kM1Strap);
  const odb::Rect area(0, 0, 100, 100);

  Via lower_removed(&connect, power_, area, lower, upper);
  EXPECT_TRUE(lower_removed.isValid());
  lower_removed.removeShape(unrelated.get());
  EXPECT_TRUE(lower_removed.isValid());
  lower_removed.removeShape(lower.get());
  EXPECT_FALSE(lower_removed.isValid());
  EXPECT_EQ(lower_removed.getLowerShape(), nullptr);

  Via upper_removed(&connect, power_, area, lower, upper);
  upper_removed.removeShape(upper.get());
  EXPECT_FALSE(upper_removed.isValid());
  EXPECT_EQ(upper_removed.getUpperShape(), nullptr);
}

// copy() makes an independent via with the same connect, net, area and
// shapes.
TEST_F(TestConnect, ViaCopyKeepsEverything)
{
  Connect connect(grid_, m(1), m(2));
  ShapePtr lower = makeShape(1, kM1Strap);
  ShapePtr upper = makeShape(2, kM2Strap);
  const odb::Rect area(2000, 1000, 2200, 1200);
  const Via via(&connect, power_, area, lower, upper);

  const std::unique_ptr<Via> copy(via.copy());
  EXPECT_EQ(copy->getConnect(), &connect);
  EXPECT_EQ(copy->getNet(), power_);
  EXPECT_EQ(copy->getArea(), area);
  EXPECT_EQ(copy->getLowerShape(), lower);
  EXPECT_EQ(copy->getUpperShape(), upper);
}

// Vias compare their layer ranges through their connect rules.
TEST_F(TestConnect, ViaOverlapsAndStartsBelowFollowTheirConnects)
{
  Connect low(grid_, m(1), m(2));
  Connect high(grid_, m(3), m(4));
  Connect spanning(grid_, m(2), m(3));
  const odb::Rect area(0, 0, 100, 100);
  auto via_on = [&](Connect& connect) {
    return std::make_shared<Via>(&connect, power_, area, nullptr, nullptr);
  };

  EXPECT_TRUE(via_on(low)->startsBelow(via_on(high)));
  EXPECT_FALSE(via_on(high)->startsBelow(via_on(low)));
  EXPECT_FALSE(via_on(low)->overlaps(via_on(high)));
  EXPECT_TRUE(via_on(low)->overlaps(via_on(spanning)));
}

// convertVectorToTree moves the vias into a tree indexed by their areas.
TEST_F(TestConnect, ViaConvertVectorToTreeIndexesByArea)
{
  Connect connect(grid_, m(1), m(2));
  std::vector<ViaPtr> vias;
  for (const int x : {0, 1000}) {
    vias.push_back(std::make_shared<Via>(
        &connect, power_, odb::Rect(x, 0, x + 100, 100), nullptr, nullptr));
  }
  const ViaPtr second = vias[1];

  const Via::ViaTree tree = Via::convertVectorToTree(vias);
  EXPECT_TRUE(vias.empty());
  EXPECT_EQ(tree.size(), 2u);
  std::vector<ViaPtr> found;
  tree.query(bgi::intersects(odb::Rect(950, 0, 1050, 50)),
             std::back_inserter(found));
  EXPECT_THAT(found, ::testing::ElementsAre(second));
}

// -------- Via::writeToDb --------

// A via whose metal reaches past the end of a strap extends the strap along
// its layer's routing direction to cover it. Shapes of different wire types
// are connected with a STRIPE via.
TEST_F(TestConnect, WriteToDbExtendsAShapeAlongItsLayer)
{
  addGenerateRule(1, "RULE12");
  Connect connect(grid_, m(1), m(2));
  odb::dbSWire* wire = makeWire();
  // The M1 strap ends 50 into the M2 strap; the via's 10 of enclosure past
  // its 50 cut reaches 10 beyond that.
  ShapePtr lower = makeShape(1,
                             odb::Rect(0, 1000, 2050, 1200),
                             power_,
                             odb::dbWireShapeType::FOLLOWPIN);
  ShapePtr upper = makeShape(2, kM2Strap);
  Via via(&connect,
          power_,
          lower->getRect().intersect(upper->getRect()),
          lower,
          upper);

  via.writeToDb(wire, block(), {});
  EXPECT_FALSE(via.isFailed());
  EXPECT_EQ(lower->getRect(), odb::Rect(0, 1000, 2060, 1200));
  EXPECT_EQ(upper->getRect(), kM2Strap);
  const std::vector<odb::dbSBox*> vias = placedVias(wire);
  ASSERT_EQ(vias.size(), 1u);
  EXPECT_EQ(vias[0]->getWireShapeType(), odb::dbWireShapeType::STRIPE);
}

// A via whose metal would grow a shape into an obstruction is removed, and
// recorded as ripped up.
TEST_F(TestConnect, WriteToDbRipsUpAViaThatReachesAnObstruction)
{
  addGenerateRule(1, "RULE12");
  Connect connect(grid_, m(1), m(2));
  odb::dbSWire* wire = makeWire();
  ShapePtr lower = makeShape(1, odb::Rect(0, 1000, 2050, 1200));
  ShapePtr upper = makeShape(2, kM2Strap);
  Via via(&connect,
          power_,
          lower->getRect().intersect(upper->getRect()),
          lower,
          upper);
  Shape::ObstructionTreeMap obstructions;
  obstructions[m(1)].insert(std::make_shared<Shape>(
      m(1), odb::Rect(2055, 1000, 2100, 1200), Shape::kObs));

  via.writeToDb(wire, block(), obstructions);
  EXPECT_TRUE(via.isFailed());
  EXPECT_TRUE(wire->getWires().empty());
  EXPECT_EQ(lower->getRect(), odb::Rect(0, 1000, 2050, 1200));
  connect.recordFailedVias();
  odb::dbMarkerCategory* category = failures("Ripup");
  ASSERT_NE(category, nullptr);
  EXPECT_EQ(category->getMarkerCount(), 1);
}

// The same for a tech via.
TEST_F(TestConnect, WriteToDbRipsUpATechViaThatReachesAnObstruction)
{
  // Unlike tst::addVia, the top metal is no wider than the cut, so the via
  // fits the M2 strap and only its bottom metal reaches past the M1 strap.
  odb::dbTechVia* tech_via = odb::dbTechVia::create(tech(), "VIA12");
  odb::dbBox::create(tech_via, m(1), -35, -35, 35, 35);
  odb::dbBox::create(tech_via, stack_.cuts[0], -25, -25, 25, 25);
  odb::dbBox::create(tech_via, m(2), -25, -25, 25, 25);
  Connect connect(grid_, m(1), m(2));
  odb::dbSWire* wire = makeWire();
  ShapePtr lower = makeShape(1, odb::Rect(0, 1000, 2050, 1200));
  ShapePtr upper = makeShape(2, kM2Strap);
  Via via(&connect,
          power_,
          lower->getRect().intersect(upper->getRect()),
          lower,
          upper);
  Shape::ObstructionTreeMap obstructions;
  obstructions[m(1)].insert(std::make_shared<Shape>(
      m(1), odb::Rect(2055, 1000, 2100, 1200), Shape::kObs));

  via.writeToDb(wire, block(), obstructions);
  EXPECT_TRUE(via.isFailed());
  EXPECT_TRUE(wire->getWires().empty());
}

// Growing a shape across its layer's routing direction would widen it, so a
// via that needs that is removed instead -- unless the shape allows it. The
// 40 x 60 M1 shape and the 60 x 40 M2 shape both run against their layers.
TEST_F(TestConnect, WriteToDbRipsUpAViaThatWidensAShapeAcrossItsLayer)
{
  addGenerateRule(1, "RULE12");
  Connect connect(grid_, m(1), m(2));
  auto write = [&](const ShapePtr& lower, const ShapePtr& upper) {
    Via via(&connect,
            power_,
            lower->getRect().intersect(upper->getRect()),
            lower,
            upper);
    via.writeToDb(makeWire(), block(), {});
    return !via.isFailed();
  };

  const odb::Rect m1_stub(2000, 1000, 2040, 1060);
  EXPECT_FALSE(write(makeShape(1, m1_stub), makeShape(2, kM2Strap)));
  ShapePtr m1_widening = makeShape(1, m1_stub);
  m1_widening->setAllowsNonPreferredDirectionChange();
  EXPECT_TRUE(write(m1_widening, makeShape(2, kM2Strap)));
  EXPECT_GT(m1_widening->getRect().dy(), m1_stub.dy());

  const odb::Rect m2_stub(2000, 1000, 2060, 1040);
  EXPECT_FALSE(write(makeShape(1, kM1Strap), makeShape(2, m2_stub)));
}

// When no via can be built the via is marked as failed to build.
TEST_F(TestConnect, WriteToDbMarksAViaThatCannotBeBuilt)
{
  Connect connect(grid_, m(1), m(2));
  ShapePtr lower = makeShape(1, kM1Strap);
  ShapePtr upper = makeShape(2, kM2Strap);
  Via via(&connect,
          power_,
          lower->getRect().intersect(upper->getRect()),
          lower,
          upper);

  via.writeToDb(makeWire(), block(), {});
  EXPECT_TRUE(via.isFailed());
  connect.recordFailedVias();
  odb::dbMarkerCategory* category = failures("Build");
  ASSERT_NE(category, nullptr);
  EXPECT_EQ(category->getMarkerCount(), 1);
}

// Removing the bottom via of a stack would leave the rest of it hanging, so
// the whole stack goes.
TEST_F(TestConnect, WriteToDbRipsUpTheWholeStack)
{
  addGenerateRule(1, "RULE12");
  addGenerateRule(2, "RULE23");
  Connect connect(grid_, m(1), m(3));
  odb::dbSWire* wire = makeWire();
  ShapePtr lower = makeShape(1, odb::Rect(0, 1000, 2050, 1200));
  ShapePtr upper = makeShape(3, odb::Rect(2000, 1000, 5000, 1200));
  Via via(&connect,
          power_,
          lower->getRect().intersect(upper->getRect()),
          lower,
          upper);
  Shape::ObstructionTreeMap obstructions;
  obstructions[m(1)].insert(std::make_shared<Shape>(
      m(1), odb::Rect(2055, 1000, 2100, 1200), Shape::kObs));

  via.writeToDb(wire, block(), obstructions);
  EXPECT_TRUE(via.isFailed());
  EXPECT_TRUE(wire->getWires().empty());
}

// The same when the rest of the stack is a tech via.
TEST_F(TestConnect, WriteToDbRipsUpAStackEndingInATechVia)
{
  addGenerateRule(1, "RULE12");
  addTechVia(2, "VIA23");
  Connect connect(grid_, m(1), m(3));
  odb::dbSWire* wire = makeWire();
  ShapePtr lower = makeShape(1, odb::Rect(0, 1000, 2050, 1200));
  ShapePtr upper = makeShape(3, odb::Rect(2000, 1000, 5000, 1200));
  Via via(&connect,
          power_,
          lower->getRect().intersect(upper->getRect()),
          lower,
          upper);
  Shape::ObstructionTreeMap obstructions;
  obstructions[m(1)].insert(std::make_shared<Shape>(
      m(1), odb::Rect(2055, 1000, 2100, 1200), Shape::kObs));

  via.writeToDb(wire, block(), obstructions);
  EXPECT_TRUE(via.isFailed());
  EXPECT_TRUE(wire->getWires().empty());
}

}  // namespace
}  // namespace pdn
