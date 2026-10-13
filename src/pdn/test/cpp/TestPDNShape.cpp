// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include <memory>
#include <optional>
#include <set>
#include <vector>

#include "PdnTest.h"
#include "connect.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/lefin.h"
#include "shape.h"
#include "techlayer.h"
#include "tst/db_fixture.h"
#include "tst/physical_builder.h"
#include "via.h"

namespace pdn {
namespace {

// Fixture for Shape tests. Builds a tech with two metal layers and creates
// the power/ground nets used by Shape constructors. Tests that need a
// grid_component_ to back getLogger()/getBlock() use `makeGridComponent()`
// to build a complete VoltageDomain -> Grid -> GridComponent chain.
class TestShape : public PdnTest
{
 protected:
  void SetUp() override
  {
    PdnTest::SetUp();
    metal1_ = tst::addRoutingLayer(
        tech(),
        "m1",
        {.direction = odb::dbTechLayerDir::HORIZONTAL, .width = 70});
    metal2_ = tst::addRoutingLayer(
        tech(),
        "m2",
        {.direction = odb::dbTechLayerDir::VERTICAL, .width = 70});
    metal3_ = tst::addRoutingLayer(
        tech(),
        "m3",
        {.direction = odb::dbTechLayerDir::HORIZONTAL, .width = 70});
    power_ = odb::dbNet::create(block(), "VDD");
    power_->setSigType(odb::dbSigType::POWER);
    power_->setSpecial();
    ground_ = odb::dbNet::create(block(), "VSS");
    ground_->setSigType(odb::dbSigType::GROUND);
    ground_->setSpecial();
    block()->setDieArea(odb::Rect(0, 0, 10000, 10000));
  }

  // Build the chain VoltageDomain -> Grid -> GridComponent. Pointers
  // outlive the test via unique_ptrs owned by the fixture.
  GridComponent* makeGridComponent()
  {
    domain_ = std::make_unique<VoltageDomain>(
        /*pdngen=*/nullptr,
        block(),
        power_,
        ground_,
        std::vector<odb::dbNet*>{},
        getLogger());
    grid_ = std::make_unique<TestGrid>(domain_.get(),
                                       /*name=*/"core",
                                       /*starts_with_power=*/true,
                                       std::vector<odb::dbTechLayer*>{});
    grid_component_ = std::make_unique<TestGridComponent>(grid_.get());
    return grid_component_.get();
  }

  // Build a connect on this fixture's grid going from metal1 to metal2,
  // then a Via on that connect. Used by tests that need real getLowerLayer
  // / getUpperLayer / isFailed accessors.
  ViaPtr makeVia(odb::dbTechLayer* lower,
                 odb::dbTechLayer* upper,
                 const odb::Rect& area = odb::Rect(0, 0, 50, 50))
  {
    if (grid_ == nullptr) {
      makeGridComponent();
    }
    auto connect = std::make_unique<Connect>(grid_.get(), lower, upper);
    Connect* raw = connect.get();
    connects_.push_back(std::move(connect));
    return std::make_shared<Via>(raw,
                                 power_,
                                 area,
                                 /*lower_shape=*/ShapePtr{},
                                 /*upper_shape=*/ShapePtr{});
  }

  odb::dbTechLayer* metal1_ = nullptr;
  odb::dbTechLayer* metal2_ = nullptr;
  odb::dbTechLayer* metal3_ = nullptr;
  odb::dbNet* power_ = nullptr;
  odb::dbNet* ground_ = nullptr;
  std::unique_ptr<VoltageDomain> domain_;
  std::unique_ptr<Grid> grid_;
  std::unique_ptr<GridComponent> grid_component_;
  // Shape::writeToDb no longer looks a BTerm up itself -- PdnGen::writeToDb
  // resolves one per net up front and hands it down (see
  // TestPdnGenBTerms.* for that resolution). Tests that exercise the
  // pin-writing path supply the BTerm the way PdnGen would.
  odb::dbBTerm* makePinBTerm(odb::dbNet* net)
  {
    odb::dbBTerm* bterm = odb::dbBTerm::create(net, net->getConstName());
    bterm->setIoType(odb::dbIoType::INOUT);
    bterm->setSigType(net->getSigType());
    bterm->setSpecial();
    return bterm;
  }

  std::vector<std::unique_ptr<Connect>> connects_;
};

// -------- Construction + simple getters --------

// The 4-arg constructor stores everything verbatim and defaults shape_type
// to kShape, obs == rect, and net stored.
TEST_F(TestShape, NetConstructorStoresValues)
{
  const odb::Rect r(0, 0, 100, 50);
  Shape s(metal1_, power_, r, odb::dbWireShapeType::STRIPE);
  EXPECT_EQ(s.getLayer(), metal1_);
  EXPECT_EQ(s.getNet(), power_);
  EXPECT_EQ(s.getRect(), r);
  EXPECT_EQ(s.getType(), odb::dbWireShapeType::STRIPE);
  EXPECT_EQ(s.shapeType(), Shape::kShape);
  EXPECT_EQ(s.getObstruction(), r);  // obs initialized to rect
  EXPECT_EQ(s.getGridComponent(), nullptr);
}

// The 3-arg constructor (obstruction-style) sets net to nullptr and
// takes an explicit ShapeType.
TEST_F(TestShape, ObsConstructorStoresShapeType)
{
  const odb::Rect r(0, 0, 100, 50);
  Shape s(metal1_, r, Shape::kBlockObs);
  EXPECT_EQ(s.getLayer(), metal1_);
  EXPECT_EQ(s.getNet(), nullptr);
  EXPECT_EQ(s.getRect(), r);
  EXPECT_EQ(s.getType(), odb::dbWireShapeType::NONE);
  EXPECT_EQ(s.shapeType(), Shape::kBlockObs);
}

// Setters and corresponding getters round-trip.
TEST_F(TestShape, SettersRoundTrip)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.setNet(ground_);
  EXPECT_EQ(s.getNet(), ground_);
  s.setRect(odb::Rect(10, 20, 200, 100));
  EXPECT_EQ(s.getRect(), odb::Rect(10, 20, 200, 100));
  s.setShapeType(Shape::kMacroObs);
  EXPECT_EQ(s.shapeType(), Shape::kMacroObs);
  s.setObstruction(odb::Rect(-5, -5, 205, 105));
  EXPECT_EQ(s.getObstruction(), odb::Rect(-5, -5, 205, 105));
}

// Lock state round-trips.
TEST_F(TestShape, LockStateRoundTrip)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 100));
  EXPECT_FALSE(s.isLocked());
  s.setLocked();
  EXPECT_TRUE(s.isLocked());
  s.clearLocked();
  EXPECT_FALSE(s.isLocked());
}

// setGridComponent + getGridComponent round-trip.
TEST_F(TestShape, GridComponentRoundTrip)
{
  GridComponent* gc = makeGridComponent();
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 100));
  EXPECT_EQ(s.getGridComponent(), nullptr);
  s.setGridComponent(gc);
  EXPECT_EQ(s.getGridComponent(), gc);
}

// getLength returns the longest dimension; getWidth the shortest.
TEST_F(TestShape, LengthAndWidthMatchRectExtents)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 200, 50));
  EXPECT_EQ(s.getLength(), 200);
  EXPECT_EQ(s.getWidth(), 50);
}

// -------- Orientation predicates --------

TEST_F(TestShape, OrientationHorizontalVerticalSquare)
{
  Shape h(metal1_, power_, odb::Rect(0, 0, 200, 50));
  EXPECT_TRUE(h.isHorizontal());
  EXPECT_FALSE(h.isVertical());
  EXPECT_FALSE(h.isSquare());

  Shape v(metal1_, power_, odb::Rect(0, 0, 50, 200));
  EXPECT_TRUE(v.isVertical());
  EXPECT_FALSE(v.isHorizontal());
  EXPECT_FALSE(v.isSquare());

  Shape sq(metal1_, power_, odb::Rect(0, 0, 100, 100));
  EXPECT_TRUE(sq.isSquare());
  EXPECT_FALSE(sq.isHorizontal());
  EXPECT_FALSE(sq.isVertical());
}

// isWrongWay: a shape running across its layer's preferred direction is
// "wrong way"; one running along it is not, and neither is a square.
TEST_F(TestShape, IsWrongWayRunsAcrossThePreferredDirection)
{
  // metal1 is HORIZONTAL preferred.
  Shape h_on_h(metal1_, power_, odb::Rect(0, 0, 200, 50));
  EXPECT_FALSE(h_on_h.isWrongWay());
  Shape v_on_h(metal1_, power_, odb::Rect(0, 0, 50, 200));
  EXPECT_TRUE(v_on_h.isWrongWay());
  // metal2 is VERTICAL preferred.
  Shape v_on_v(metal2_, power_, odb::Rect(0, 0, 50, 200));
  EXPECT_FALSE(v_on_v.isWrongWay());
  Shape h_on_v(metal2_, power_, odb::Rect(0, 0, 200, 50));
  EXPECT_TRUE(h_on_v.isWrongWay());
  Shape square(metal1_, power_, odb::Rect(0, 0, 100, 100));
  EXPECT_FALSE(square.isWrongWay());
}

// getLayerDirection forwards to the layer.
TEST_F(TestShape, GetLayerDirectionForwards)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 100));
  EXPECT_EQ(s.getLayerDirection(), odb::dbTechLayerDir::HORIZONTAL);
  Shape s2(metal2_, power_, odb::Rect(0, 0, 100, 100));
  EXPECT_EQ(s2.getLayerDirection(), odb::dbTechLayerDir::VERTICAL);
}

// -------- isValid --------

// Without a min-area rule, any shape is valid.
TEST_F(TestShape, IsValidWithoutMinAreaRule)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 10, 10));
  EXPECT_TRUE(s.isValid());
}

// The layer's AREA is in dbu^2, like the rect's area, so a min area above
// the rect's fails it.
TEST_F(TestShape, IsValidFailsWhenAreaBelowMin)
{
  metal1_->setArea(1e6);  // 1e6 dbu^2, above the rect's 10000
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 100));  // area = 10000
  EXPECT_FALSE(s.isValid());
}

// A shape whose rect-area meets the min-area rule is valid.
TEST_F(TestShape, IsValidPassesWhenAreaMeetsMin)
{
  metal1_->setArea(100);  // 100 dbu^2 -- well below the rect's 10000.
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 100));
  EXPECT_TRUE(s.isValid());
}

// AREA is a minimum, so a shape of exactly that area is valid.
TEST_F(TestShape, IsValidPassesWhenAreaEqualsMin)
{
  metal1_->setArea(10000);
  EXPECT_TRUE(Shape(metal1_, power_, odb::Rect(0, 0, 100, 100)).isValid());
  EXPECT_FALSE(Shape(metal1_, power_, odb::Rect(0, 0, 100, 99)).isValid());
}

// -------- isModifiable / isRemovable --------

// kShape with no connections, not locked -> modifiable and removable.
TEST_F(TestShape, IsRemovableWhenNoConnections)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  EXPECT_TRUE(s.isModifiable());
  // With assume_bterm=false, min_conns=2. 0 connections < 2 -> removable.
  EXPECT_TRUE(s.isRemovable(/*assume_bterm=*/false));
}

// Locked shapes are neither modifiable nor removable.
TEST_F(TestShape, IsRemovableFalseWhenLocked)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.setLocked();
  EXPECT_FALSE(s.isModifiable());
  EXPECT_FALSE(s.isRemovable(false));
}

// Non-kShape (e.g. kBlockObs) shapes are not modifiable.
TEST_F(TestShape, IsModifiableFalseForObstructionTypes)
{
  Shape s(metal1_, odb::Rect(0, 0, 100, 50), Shape::kBlockObs);
  EXPECT_FALSE(s.isModifiable());
}

// With assume_bterm=true, min_conns drops to 1 -> a shape with 1
// connection is no longer removable.
TEST_F(TestShape, IsRemovableHonorsAssumeBTerm)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.addITermConnection(odb::Rect(10, 10, 20, 20));
  // assume_bterm=false -> min_conns=2, count=1 < 2 -> still removable.
  EXPECT_TRUE(s.isRemovable(false));
  // assume_bterm=true  -> min_conns=1, count=1 -> NOT removable.
  EXPECT_FALSE(s.isRemovable(true));
}

// -------- iterm/bterm management --------

TEST_F(TestShape, ITermConnectionAddRemoveClear)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  EXPECT_FALSE(s.hasITermConnections());
  s.addITermConnection(odb::Rect(0, 0, 10, 10));
  s.addITermConnection(odb::Rect(20, 0, 30, 10));
  EXPECT_TRUE(s.hasITermConnections());
  EXPECT_EQ(s.getItermConnections().size(), 2u);
  s.removeITermConnection(odb::Rect(0, 0, 10, 10));
  EXPECT_EQ(s.getItermConnections().size(), 1u);
  s.clearITermConnections();
  EXPECT_FALSE(s.hasITermConnections());
}

TEST_F(TestShape, BTermConnectionAddRemove)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  EXPECT_FALSE(s.hasBTermConnections());
  s.addBTermConnection(odb::Rect(0, 0, 10, 10));
  EXPECT_TRUE(s.hasBTermConnections());
  EXPECT_EQ(s.getBtermConnections().size(), 1u);
  s.removeBTermConnection(odb::Rect(0, 0, 10, 10));
  EXPECT_FALSE(s.hasBTermConnections());
}

TEST_F(TestShape, HasTermConnectionsTrueWhenEither)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  EXPECT_FALSE(s.hasTermConnections());
  s.addITermConnection(odb::Rect(0, 0, 10, 10));
  EXPECT_TRUE(s.hasTermConnections());
}

// updateTermConnections drops iterms/bterms that no longer overlap with
// the shape's rect.
TEST_F(TestShape, UpdateTermConnectionsDropsOutsideTerms)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.addITermConnection(odb::Rect(10, 10, 20, 20));          // inside
  s.addITermConnection(odb::Rect(1000, 1000, 1010, 1010));  // outside
  s.addBTermConnection(odb::Rect(0, 0, 5, 5));              // inside
  s.addBTermConnection(odb::Rect(500, 500, 510, 510));      // outside
  s.updateTermConnections();
  EXPECT_EQ(s.getItermConnections().size(), 1u);
  EXPECT_EQ(s.getBtermConnections().size(), 1u);
}

// -------- vias --------

// Helper to build a real Via* (needs Connect; we only need the Via for the
// stored shared_ptr count + getUpperLayer/getLowerLayer accessors, but
// constructing a real Via requires a Connect chain which is heavy).
// Instead, we exercise add/remove/clear via with nullptr Via shared_ptrs.
TEST_F(TestShape, ViaAddClearAccessors)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  EXPECT_TRUE(s.getVias().empty());
  s.addVia(makeVia(metal1_, metal2_));
  EXPECT_EQ(s.getVias().size(), 1u);
  s.clearVias();
  EXPECT_TRUE(s.getVias().empty());
}

// removeVias drops only the listed vias and keeps the above/below counts in
// step with what is left. A via that spans the shape's layer (neither ends on
// it) is removed without touching either count.
TEST_F(TestShape, RemoveViasUpdatesConnectionCounts)
{
  Shape s(metal2_, power_, odb::Rect(0, 0, 50, 100));
  const ViaPtr below = makeVia(metal1_, metal2_);
  const ViaPtr above = makeVia(metal2_, metal3_);
  const ViaPtr across = makeVia(metal1_, metal3_);
  const ViaPtr kept = makeVia(metal1_, metal2_);
  for (const ViaPtr& via : {below, above, across, kept}) {
    s.addVia(via);
  }
  EXPECT_EQ(s.getNumberOfConnectionsBelow(), 2);
  EXPECT_EQ(s.getNumberOfConnectionsAbove(), 1);

  s.removeVias({below.get(), above.get(), across.get()});

  ASSERT_EQ(s.getVias().size(), 1u);
  EXPECT_EQ(s.getVias().front(), kept);
  EXPECT_EQ(s.getNumberOfConnectionsBelow(), 1);
  EXPECT_EQ(s.getNumberOfConnectionsAbove(), 0);
}

// -------- copy / merge --------

// copy() creates an independent Shape with the same fields.
TEST_F(TestShape, CopyProducesIndependentShape)
{
  Shape s(
      metal1_, power_, odb::Rect(0, 0, 100, 50), odb::dbWireShapeType::STRIPE);
  s.setObstruction(odb::Rect(-5, -5, 105, 55));
  s.addITermConnection(odb::Rect(10, 10, 20, 20));
  s.addBTermConnection(odb::Rect(0, 0, 5, 5));
  s.setAllowsNonPreferredDirectionChange();

  auto c = s.copy();
  EXPECT_EQ(c->getLayer(), metal1_);
  EXPECT_EQ(c->getNet(), power_);
  EXPECT_EQ(c->getRect(), s.getRect());
  EXPECT_EQ(c->getType(), odb::dbWireShapeType::STRIPE);
  EXPECT_EQ(c->getObstruction(), s.getObstruction());
  EXPECT_EQ(c->getItermConnections().size(), 1u);
  EXPECT_EQ(c->getBtermConnections().size(), 1u);
  EXPECT_TRUE(c->allowsNonPreferredDirectionChange());
}

// merge() expands the rect, merges term sets, and regenerates obstruction.
TEST_F(TestShape, MergeExpandsRectAndTermSets)
{
  Shape a(metal1_, power_, odb::Rect(0, 0, 100, 50));
  a.addITermConnection(odb::Rect(10, 10, 20, 20));
  Shape b(metal1_, power_, odb::Rect(150, 0, 250, 50));
  b.addBTermConnection(odb::Rect(160, 10, 170, 20));

  a.merge(&b);
  EXPECT_EQ(a.getRect(), odb::Rect(0, 0, 250, 50));
  EXPECT_EQ(a.getItermConnections().size(), 1u);
  EXPECT_EQ(a.getBtermConnections().size(), 1u);
}

// -------- obstruction geometry --------

// Default obstruction equals the rect, so the halo is zero on every side.
TEST_F(TestShape, ObstructionHaloZeroForDefaultObs)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  auto h = s.getObstructionHalo();
  EXPECT_EQ(h.left, 0);
  EXPECT_EQ(h.right, 0);
  EXPECT_EQ(h.top, 0);
  EXPECT_EQ(h.bottom, 0);
}

// After setObstruction expands the obs, the halo reflects the bloat.
TEST_F(TestShape, ObstructionHaloReflectsExpandedObs)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.setObstruction(odb::Rect(-3, -5, 110, 60));  // l=3, b=5, r=10, t=10
  auto h = s.getObstructionHalo();
  EXPECT_EQ(h.left, 3);
  EXPECT_EQ(h.bottom, 5);
  EXPECT_EQ(h.right, 10);
  EXPECT_EQ(h.top, 10);
}

// getRectWithLargestObstructionHalo bloats the rect by the max of the
// shape's own halo and the provided halo, on each side independently.
TEST_F(TestShape, RectWithLargestHalo)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.setObstruction(odb::Rect(-3, -2, 105, 52));  // l=3, b=2, r=5, t=2
  Shape::ObstructionHalo halo{/*left=*/10,
                              /*top=*/4,
                              /*right=*/1,
                              /*bottom=*/0};
  const odb::Rect r = s.getRectWithLargestObstructionHalo(halo);
  EXPECT_EQ(r.xMin(), -10);  // max(3, 10) on left
  EXPECT_EQ(r.yMin(), -2);   // max(2, 0) on bottom
  EXPECT_EQ(r.xMax(), 105);  // max(5, 1) on right
  EXPECT_EQ(r.yMax(), 54);   // max(2, 4) on top
}

// An absence of die owes no spacing, so neither halo is applied to it.
TEST_F(TestShape, RectWithLargestHaloIgnoresDieAbsence)
{
  Shape s(metal1_, odb::Rect(0, 0, 100, 50), Shape::kObs);
  s.setObstruction(odb::Rect(-3, -2, 105, 52));
  s.setIsDieAbsence();
  const Shape::ObstructionHalo halo{10, 4, 1, 0};
  EXPECT_EQ(s.getRectWithLargestObstructionHalo(halo), s.getRect());
}

// -------- connection counts --------

// getNumberOfConnections sums vias + iterms + bterms.
TEST_F(TestShape, GetNumberOfConnectionsSumsAll)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.addITermConnection(odb::Rect(10, 10, 20, 20));
  s.addBTermConnection(odb::Rect(40, 10, 50, 20));
  s.addBTermConnection(odb::Rect(60, 10, 70, 20));
  s.addVia(makeVia(metal1_, metal2_));
  EXPECT_EQ(s.getNumberOfConnections(), 4);
}

// Without real Via objects, getNumberOfConnectionsBelow/Above return 0
// because each via's upper/lower layer is null.
TEST_F(TestShape, ConnectionsAboveBelowZeroWithoutVias)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  EXPECT_EQ(s.getNumberOfConnectionsBelow(), 0);
  EXPECT_EQ(s.getNumberOfConnectionsAbove(), 0);
}

// -------- getMinimumRect (base Shape) --------

// With no terms or vias, mergeInit produces an inverted rect.
TEST_F(TestShape, GetMinimumRectEmptyReturnsInverted)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  const odb::Rect r = s.getMinimumRect();
  EXPECT_TRUE(r.isInverted());
}

// With iterm/bterm connections, the rect spans their bounding box.
TEST_F(TestShape, GetMinimumRectMergesTerms)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.addITermConnection(odb::Rect(10, 10, 30, 20));
  s.addBTermConnection(odb::Rect(60, 5, 80, 25));
  EXPECT_EQ(s.getMinimumRect(), odb::Rect(10, 5, 80, 25));
}

// getMinimumRect also merges via areas.
TEST_F(TestShape, GetMinimumRectMergesViaArea)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.addVia(makeVia(metal1_, metal2_, odb::Rect(200, 200, 250, 250)));
  const odb::Rect r = s.getMinimumRect();
  EXPECT_EQ(r.xMin(), 200);
  EXPECT_EQ(r.yMax(), 250);
}

// -------- generateObstruction --------

// Without spacing rules on the layer, obstruction equals rect.
TEST_F(TestShape, GenerateObstructionWithoutRulesEqualsRect)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.generateObstruction();
  EXPECT_EQ(s.getObstruction(), s.getRect());
}

// With a spacing rule on the layer, the obstruction bloats by that
// spacing on all sides.
TEST_F(TestShape, GenerateObstructionUsesLayerSpacing)
{
  metal1_->setSpacing(20);
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.generateObstruction();
  // bloat(20) -> (-20, -20, 120, 70).
  EXPECT_EQ(s.getObstruction(), odb::Rect(-20, -20, 120, 70));
}

// -------- hasInternalConnections / hasDBConnectivity (indirect) --------

// FOLLOWPIN type shapes are treated as having connectivity.
TEST_F(TestShape, FollowPinTypeImpliesInternalConnections)
{
  Shape s(metal1_,
          power_,
          odb::Rect(0, 0, 100, 50),
          odb::dbWireShapeType::FOLLOWPIN);
  EXPECT_TRUE(s.hasInternalConnections());
}

// ITerm connections imply internal connectivity.
TEST_F(TestShape, ITermConnectionImpliesInternalConnections)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  EXPECT_FALSE(s.hasInternalConnections());
  s.addITermConnection(odb::Rect(0, 0, 10, 10));
  EXPECT_TRUE(s.hasInternalConnections());
}

// A shape is floating until something internal connects to it: a bterm alone
// does not count, an iterm does.
TEST_F(TestShape, IsFloatingUntilInternallyConnected)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  EXPECT_TRUE(s.isFloating());
  s.addBTermConnection(odb::Rect(0, 0, 5, 5));
  EXPECT_TRUE(s.isFloating());
  s.addITermConnection(odb::Rect(10, 10, 20, 20));
  EXPECT_FALSE(s.isFloating());
}

// -------- text utilities --------

// getRectText formats a rect to (xMin, yMin) - (xMax, yMax) in microns.
TEST_F(TestShape, GetRectTextFormatsMicrons)
{
  const std::string text
      = Shape::getRectText(odb::Rect(0, 0, 1000, 2000), /*dbu_to_micron=*/1000);
  EXPECT_NE(text.find("0.0000"), std::string::npos);
  EXPECT_NE(text.find("1.0000"), std::string::npos);
  EXPECT_NE(text.find("2.0000"), std::string::npos);
}

// getReportText includes the net name when one is set.
TEST_F(TestShape, GetReportTextIncludesNetAndLayer)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 1000, 1000));
  const std::string text = s.getReportText();
  EXPECT_NE(text.find("VDD"), std::string::npos);
  EXPECT_NE(text.find("m1"), std::string::npos);
}

// getReportText still works for obstruction shapes with no net.
TEST_F(TestShape, GetReportTextWorksWithoutNet)
{
  Shape s(metal1_, odb::Rect(0, 0, 100, 100), Shape::kBlockObs);
  const std::string text = s.getReportText();
  EXPECT_NE(text.find("m1"), std::string::npos);
}

// getDisplayText returns "<net>:<layer>:none" when no grid_component_ set.
TEST_F(TestShape, GetDisplayTextWithoutGridComponent)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  const std::string text = s.getDisplayText();
  EXPECT_NE(text.find("VDD"), std::string::npos);
  EXPECT_NE(text.find("m1"), std::string::npos);
  EXPECT_NE(text.find("none"), std::string::npos);
}

// With a grid_component_ set, getDisplayText includes the component type
// and grid name.
TEST_F(TestShape, GetDisplayTextWithGridComponent)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.setGridComponent(makeGridComponent());
  const std::string text = s.getDisplayText();
  EXPECT_NE(text.find("VDD"), std::string::npos);
  EXPECT_NE(text.find("m1"), std::string::npos);
  EXPECT_NE(text.find("core"), std::string::npos);  // grid name
}

// -------- allowsNonPreferredDirectionChange --------

TEST_F(TestShape, AllowsNonPreferredDirectionChangeFlag)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  EXPECT_FALSE(s.allowsNonPreferredDirectionChange());
  s.setAllowsNonPreferredDirectionChange();
  EXPECT_TRUE(s.allowsNonPreferredDirectionChange());
}

// -------- convertVectorToTree / convertVectorToObstructionTree --------

// Empty input -> empty trees and the vector is cleared.
TEST_F(TestShape, ConvertVectorToTreeEmpty)
{
  ShapeVectorMap vec;
  auto trees = Shape::convertVectorToTree(vec);
  EXPECT_TRUE(trees.empty());
  EXPECT_TRUE(vec.empty());
}

// Populated input -> per-layer trees and the source is swapped to empty.
TEST_F(TestShape, ConvertVectorToTreePopulated)
{
  ShapeVectorMap vec;
  vec[metal1_].push_back(
      std::make_shared<Shape>(metal1_, power_, odb::Rect(0, 0, 10, 10)));
  vec[metal1_].push_back(
      std::make_shared<Shape>(metal1_, power_, odb::Rect(20, 0, 30, 10)));
  auto trees = Shape::convertVectorToTree(vec);
  ASSERT_EQ(trees.count(metal1_), 1u);
  EXPECT_EQ(trees[metal1_].size(), 2u);
  EXPECT_TRUE(vec.empty());  // swap'd out
}

// And the obstruction-tree variant.
TEST_F(TestShape, ConvertVectorToObstructionTree)
{
  ShapeVectorMap vec;
  vec[metal1_].push_back(
      std::make_shared<Shape>(metal1_, power_, odb::Rect(0, 0, 10, 10)));
  auto trees = Shape::convertVectorToObstructionTree(vec);
  EXPECT_EQ(trees.count(metal1_), 1u);
  EXPECT_TRUE(vec.empty());
}

// -------- extendTo --------

// extendTo on a horizontal shape extends the x range; the obstruction tree
// has no intersections, so we get a new shape back.
TEST_F(TestShape, ExtendToHorizontalProducesExtendedShape)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  Shape::ShapeTree shapes;
  Shape::ObstructionTree obstructions;
  const odb::Rect target(-50, 0, 150, 50);
  auto extended = s.extendTo(target, shapes, obstructions, nullptr);
  ASSERT_NE(extended, nullptr);
  EXPECT_EQ(extended->getRect(), odb::Rect(-50, 0, 150, 50));
}

// extendTo on a square shape returns nullptr (no clear extend direction).
TEST_F(TestShape, ExtendToSquareReturnsNullptr)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 100));
  Shape::ShapeTree shapes;
  Shape::ObstructionTree obstructions;
  EXPECT_EQ(
      s.extendTo(odb::Rect(-50, -50, 150, 150), shapes, obstructions, nullptr),
      nullptr);
}

// extendTo on a vertical shape extends the y range only.
TEST_F(TestShape, ExtendToVerticalProducesExtendedShape)
{
  Shape s(metal2_, power_, odb::Rect(0, 0, 50, 100));
  Shape::ShapeTree shapes;
  Shape::ObstructionTree obstructions;
  auto extended
      = s.extendTo(odb::Rect(-20, -50, 70, 150), shapes, obstructions, nullptr);
  ASSERT_NE(extended, nullptr);
  EXPECT_EQ(extended->getRect(), odb::Rect(0, -50, 50, 150));
}

// extendTo where the target doesn't actually extend the rect returns
// nullptr.
TEST_F(TestShape, ExtendToNoExtensionReturnsNullptr)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  Shape::ShapeTree shapes;
  Shape::ObstructionTree obstructions;
  // Target is entirely inside the existing rect -> nothing to extend.
  EXPECT_EQ(
      s.extendTo(odb::Rect(10, 10, 50, 40), shapes, obstructions, nullptr),
      nullptr);
}

// extendTo aborts when an obstruction intersects the proposed extension.
TEST_F(TestShape, ExtendToBlockedByObstructionReturnsNullptr)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  Shape::ShapeTree shapes;
  Shape::ObstructionTree obstructions;
  // Insert an obstruction that overlaps the extension area.
  auto obs = std::make_shared<Shape>(
      metal1_, odb::Rect(200, 0, 300, 50), Shape::kObs);
  obstructions.insert(obs);
  EXPECT_EQ(s.extendTo(odb::Rect(0, 0, 500, 50), shapes, obstructions, nullptr),
            nullptr);
}

// extendTo aborts when an existing shape intersects the extended
// obstruction.
TEST_F(TestShape, ExtendToBlockedByExistingShapeReturnsNullptr)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  Shape::ShapeTree shapes;
  Shape::ObstructionTree obstructions;
  // Existing shape near the extension target.
  auto other
      = std::make_shared<Shape>(metal1_, power_, odb::Rect(120, 0, 200, 50));
  shapes.insert(other);
  EXPECT_EQ(s.extendTo(odb::Rect(0, 0, 250, 50), shapes, obstructions, nullptr),
            nullptr);
}

// -------- cut() --------

// cut() with no overlapping obstructions returns false and produces no
// replacements.
TEST_F(TestShape, CutWithoutObstructionsReturnsFalse)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 200, 50));
  Shape::ObstructionTree obstructions;
  std::vector<std::unique_ptr<Shape>> replacements;
  EXPECT_FALSE(s.cut(obstructions, /*ignore_grid=*/nullptr, replacements));
  EXPECT_TRUE(replacements.empty());
}

// cut() with a horizontal shape and a vertical obstruction in the middle
// produces two replacement shapes on either side, each ending the layer's
// spacing short of the obstruction.
TEST_F(TestShape, CutWithVerticalObstructionProducesReplacements)
{
  metal1_->setSpacing(20);
  Shape s(metal1_, power_, odb::Rect(0, 0, 300, 50));
  s.generateObstruction();
  Shape::ObstructionTree obstructions;
  auto obs = std::make_shared<Shape>(
      metal1_, odb::Rect(100, 0, 200, 50), Shape::kObs);
  obstructions.insert(obs);
  std::vector<std::unique_ptr<Shape>> replacements;
  EXPECT_TRUE(s.cut(obstructions, nullptr, replacements));
  std::set<odb::Rect> rects;
  for (const auto& r : replacements) {
    rects.insert(r->getRect());
  }
  EXPECT_EQ(rects,
            (std::set<odb::Rect>{odb::Rect(0, 0, 80, 50),
                                 odb::Rect(220, 0, 300, 50)}));
}

// -------- populateMapFromDb --------

// Empty net (no SWires, no BTerms) populates nothing.
TEST_F(TestShape, PopulateMapFromDbEmptyNet)
{
  ShapeVectorMap map;
  odb::dbNet* net = odb::dbNet::create(block(), "X");
  Shape::populateMapFromDb(net, map);
  EXPECT_TRUE(map.empty());
}

// A net with an SWire wire shape adds a Shape per box.
TEST_F(TestShape, PopulateMapFromDbReadsSWire)
{
  odb::dbNet* net = odb::dbNet::create(block(), "WSW");
  odb::dbSWire* swire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);
  odb::dbSBox::create(
      swire, metal1_, 0, 0, 100, 50, odb::dbWireShapeType::STRIPE);
  ShapeVectorMap map;
  Shape::populateMapFromDb(net, map);
  ASSERT_EQ(map.count(metal1_), 1u);
  EXPECT_EQ(map[metal1_].size(), 1u);
  EXPECT_EQ(map[metal1_].front()->getRect(), odb::Rect(0, 0, 100, 50));
}

// A BPin box on a CUT layer (routing level 0) is skipped by
// populateMapFromDb.
TEST_F(TestShape, PopulateMapFromDbSkipsNonRoutingLayerBoxesOnBPin)
{
  odb::dbTechLayer* cut
      = odb::dbTechLayer::create(tech(), "v_cut", odb::dbTechLayerType::CUT);
  cut->setWidth(50);
  odb::dbNet* net = odb::dbNet::create(block(), "BTCUT");
  odb::dbBTerm* bterm = odb::dbBTerm::create(net, "BTCUT");
  odb::dbBPin* bpin = odb::dbBPin::create(bterm);
  bpin->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  odb::dbBox::create(bpin, cut, 0, 0, 100, 50);
  ShapeVectorMap map;
  Shape::populateMapFromDb(net, map);
  EXPECT_TRUE(map.empty());
}

// An OCTILINEAR SBox is treated as an obstruction (the Shape's net is
// cleared and the shape type is set to kObs).
TEST_F(TestShape, PopulateMapFromDbConvertsOctilinearToObstruction)
{
  odb::dbNet* net = odb::dbNet::create(block(), "OCT");
  odb::dbSWire* swire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);
  // OCTILINEAR boxes are 45-degree paths described by two endpoints and a
  // width; pass non-zero width so the path is well-formed.
  odb::dbSBox::create(swire,
                      metal1_,
                      /*x1=*/0,
                      /*y1=*/0,
                      /*x2=*/100,
                      /*y2=*/100,
                      odb::dbWireShapeType::STRIPE,
                      odb::dbSBox::OCTILINEAR,
                      /*width=*/10);
  ShapeVectorMap map;
  Shape::populateMapFromDb(net, map);
  ASSERT_EQ(map.count(metal1_), 1u);
  EXPECT_EQ(map[metal1_].front()->shapeType(), Shape::kObs);
  EXPECT_EQ(map[metal1_].front()->getNet(), nullptr);
}

// A block-level VIA in an SWire drives the `getBlockVia()` branch of
// populateMapFromDb's via box-collection path.
TEST_F(TestShape, PopulateMapFromDbReadsBlockViaSWire)
{
  // Block-level via with metal/cut/metal sub-boxes.
  odb::dbTechLayer* cut
      = odb::dbTechLayer::create(tech(), "bvcut", odb::dbTechLayerType::CUT);
  cut->setWidth(50);
  odb::dbVia* bv = odb::dbVia::create(block(), "BV1");
  odb::dbBox::create(bv, metal1_, -35, -35, 35, 35);
  odb::dbBox::create(bv, cut, -25, -25, 25, 25);
  odb::dbBox::create(bv, metal2_, -35, -35, 35, 35);

  odb::dbNet* net = odb::dbNet::create(block(), "BVNET");
  odb::dbSWire* swire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);
  odb::dbSBox::create(swire, bv, 200, 200, odb::dbWireShapeType::STRIPE);

  ShapeVectorMap map;
  Shape::populateMapFromDb(net, map);
  EXPECT_EQ(map[metal1_].size(), 1u);
  EXPECT_EQ(map[cut].size(), 1u);
  EXPECT_EQ(map[metal2_].size(), 1u);
}

// SWire VIA boxes contribute one Shape per via sub-box, drawn from the
// via's tech-via boxes (cut + metal enclosures).
TEST_F(TestShape, PopulateMapFromDbReadsViaSWire)
{
  // Build a minimal tech via (metal1 enc + cut + metal2 enc) referenced
  // from the swire.
  odb::dbTechLayer* cut = tst::addCutLayer(tech(), "v1cut");
  cut->setWidth(50);
  odb::dbTechVia* tv = tst::addVia(tech(), "V1", metal1_, cut, metal2_, 50, 10);

  odb::dbNet* net = odb::dbNet::create(block(), "WVIA");
  odb::dbSWire* swire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);
  odb::dbSBox::create(
      swire, tv, /*x=*/200, /*y=*/200, odb::dbWireShapeType::STRIPE);

  ShapeVectorMap map;
  Shape::populateMapFromDb(net, map);
  // Three layers should each end up with a single fixed shape.
  EXPECT_EQ(map[metal1_].size(), 1u);
  EXPECT_EQ(map[cut].size(), 1u);
  EXPECT_EQ(map[metal2_].size(), 1u);
}

// -------- FollowPinShape --------

// FollowPinShape inherits from Shape with the FOLLOWPIN type. It is never
// removable.
TEST_F(TestShape, FollowPinShapeIsNotRemovable)
{
  FollowPinShape fp(metal1_, power_, odb::Rect(0, 0, 100, 50));
  EXPECT_FALSE(fp.isRemovable(false));
  EXPECT_FALSE(fp.isRemovable(true));
}

// setAllowsNonPreferredDirectionChange is a no-op for FollowPinShape.
TEST_F(TestShape, FollowPinSetAllowsNonPreferredIsNoOp)
{
  FollowPinShape fp(metal1_, power_, odb::Rect(0, 0, 100, 50));
  fp.setAllowsNonPreferredDirectionChange();
  EXPECT_FALSE(fp.allowsNonPreferredDirectionChange());
}

// FollowPin getLayerDirection: square -> layer's preferred direction.
TEST_F(TestShape, FollowPinDirectionSquareUsesLayerDirection)
{
  FollowPinShape fp(metal1_, power_, odb::Rect(0, 0, 100, 100));
  EXPECT_EQ(fp.getLayerDirection(), odb::dbTechLayerDir::HORIZONTAL);
}

// FollowPin getLayerDirection: horizontal shape -> HORIZONTAL.
TEST_F(TestShape, FollowPinDirectionHorizontalShape)
{
  FollowPinShape fp(metal1_, power_, odb::Rect(0, 0, 200, 50));
  EXPECT_EQ(fp.getLayerDirection(), odb::dbTechLayerDir::HORIZONTAL);
}

// FollowPin getLayerDirection: vertical shape -> VERTICAL.
TEST_F(TestShape, FollowPinDirectionVerticalShape)
{
  FollowPinShape fp(metal1_, power_, odb::Rect(0, 0, 50, 200));
  EXPECT_EQ(fp.getLayerDirection(), odb::dbTechLayerDir::VERTICAL);
}

// FollowPin copy preserves the FOLLOWPIN type.
TEST_F(TestShape, FollowPinCopyPreservesType)
{
  FollowPinShape fp(metal1_, power_, odb::Rect(0, 0, 200, 50));
  auto c = fp.copy();
  EXPECT_EQ(c->getType(), odb::dbWireShapeType::FOLLOWPIN);
}

// FollowPin merge expands the rect.
TEST_F(TestShape, FollowPinMergeExpandsRect)
{
  FollowPinShape a(metal1_, power_, odb::Rect(0, 0, 100, 50));
  FollowPinShape b(metal1_, power_, odb::Rect(150, 0, 250, 50));
  a.merge(&b);
  EXPECT_EQ(a.getRect(), odb::Rect(0, 0, 250, 50));
}

// FollowPin merge with a plain Shape still merges the rects via the base.
TEST_F(TestShape, FollowPinMergeWithBaseShape)
{
  FollowPinShape a(metal1_, power_, odb::Rect(0, 0, 100, 50));
  Shape b(metal1_, power_, odb::Rect(150, 0, 250, 50));
  a.merge(&b);  // dynamic_cast fails, but rect still merged via Shape::merge
  EXPECT_EQ(a.getRect(), odb::Rect(0, 0, 250, 50));
}

// FollowPin updateTermConnections also drops outside terms.
TEST_F(TestShape, FollowPinUpdateTermConnectionsDropsOutside)
{
  FollowPinShape fp(metal1_, power_, odb::Rect(0, 0, 100, 50));
  fp.addITermConnection(odb::Rect(10, 10, 20, 20));
  fp.addITermConnection(odb::Rect(500, 500, 510, 510));
  fp.updateTermConnections();
  EXPECT_EQ(fp.getItermConnections().size(), 1u);
}

// FollowPin getMinimumRect keeps the original width AND the original height.
TEST_F(TestShape, FollowPinGetMinimumRectKeepsHeight)
{
  FollowPinShape fp(metal1_, power_, odb::Rect(0, 0, 200, 50));
  fp.addITermConnection(odb::Rect(50, 10, 60, 40));
  const odb::Rect r = fp.getMinimumRect();
  // For a horizontal followpin, the y range is the rect's y range.
  EXPECT_EQ(r.yMin(), 0);
  EXPECT_EQ(r.yMax(), 50);
}

// FollowPin cut() ignores grid-level obstructions.
TEST_F(TestShape, FollowPinCutIgnoresGridObstructions)
{
  FollowPinShape fp(metal1_, power_, odb::Rect(0, 0, 200, 50));
  Shape::ObstructionTree obs;
  auto gobs = std::make_shared<GridObsShape>(
      metal1_, odb::Rect(50, 0, 150, 50), /*grid=*/nullptr);
  obs.insert(gobs);
  std::vector<std::unique_ptr<Shape>> replacements;
  EXPECT_FALSE(fp.cut(obs, /*ignore_grid=*/nullptr, replacements));
}

// -------- GridObsShape --------

// GridObsShape stores the originating grid pointer.
TEST_F(TestShape, GridObsShapeBelongsTo)
{
  // Use a sentinel Grid pointer (a valid Grid would need full setup; the
  // GridObsShape only stores and compares the pointer).
  const Grid* fake_grid_a = reinterpret_cast<const Grid*>(0x1);
  const Grid* fake_grid_b = reinterpret_cast<const Grid*>(0x2);
  GridObsShape g(metal1_, odb::Rect(0, 0, 100, 50), fake_grid_a);
  EXPECT_EQ(g.getGrid(), fake_grid_a);
  EXPECT_TRUE(g.belongsTo(fake_grid_a));
  EXPECT_FALSE(g.belongsTo(fake_grid_b));
}

// GridObsShape sets its obstruction equal to its rect.
TEST_F(TestShape, GridObsShapeObstructionEqualsRect)
{
  GridObsShape g(metal1_,
                 odb::Rect(0, 0, 100, 50),
                 /*grid=*/nullptr);
  EXPECT_EQ(g.getObstruction(), g.getRect());
  EXPECT_EQ(g.shapeType(), Shape::kGridObs);
}

// -------- Via-backed connection counting --------

// A shape sitting on m2 with one m1->m2 via (m2 is its UPPER layer) counts
// as a "below" connection, and an m2->m3 via (m2 is its LOWER layer)
// counts as an "above" connection.
TEST_F(TestShape, GetNumberOfConnectionsAboveAndBelow)
{
  Shape s(metal2_, power_, odb::Rect(0, 0, 100, 100));
  s.addVia(makeVia(metal1_, metal2_));  // m2 is upper -> below
  s.addVia(makeVia(metal2_, metal3_));  // m2 is lower -> above
  s.addVia(makeVia(metal2_, metal3_));  // m2 is lower -> above
  EXPECT_EQ(s.getNumberOfConnectionsBelow(), 1);
  EXPECT_EQ(s.getNumberOfConnectionsAbove(), 2);
  EXPECT_EQ(s.getNumberOfConnections(), 3);
}

// hasInternalConnections / hasDBConnectivity via the via path: a shape
// with a single non-failed via has both kinds of connectivity.
TEST_F(TestShape, HasInternalConnectionsThroughVia)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.addVia(makeVia(metal1_, metal2_));
  EXPECT_TRUE(s.hasInternalConnections());
}

// -------- writeToDb / addBPinToDb --------

// writeToDb on a "floating" shape (no connections, no FOLLOWPIN, no
// non-failed vias) emits a warning and returns no boxes.
TEST_F(TestShape, WriteToDbFloatingShapeWarnsAndReturnsEmpty)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.setGridComponent(makeGridComponent());
  odb::dbSWire* swire = odb::dbSWire::create(power_, odb::dbWireType::ROUTED);
  const auto boxes = s.writeToDb(swire,
                                 /*bterm=*/nullptr,
                                 /*make_rect_as_pin=*/false);
  EXPECT_TRUE(boxes.empty());
}

// A locked shape is never floating: writeToDb emits an SBox.
TEST_F(TestShape, WriteToDbLockedEmitsBox)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.setLocked();
  s.setGridComponent(makeGridComponent());
  odb::dbSWire* swire = odb::dbSWire::create(power_, odb::dbWireType::ROUTED);
  const auto boxes = s.writeToDb(swire, nullptr, false);
  EXPECT_EQ(boxes.size(), 1u);
}

// A FOLLOWPIN shape is treated as connected; writeToDb emits an SBox.
TEST_F(TestShape, WriteToDbFollowPinTypeEmitsBox)
{
  Shape s(metal1_,
          power_,
          odb::Rect(0, 0, 100, 50),
          odb::dbWireShapeType::FOLLOWPIN);
  s.setGridComponent(makeGridComponent());
  odb::dbSWire* swire = odb::dbSWire::create(power_, odb::dbWireType::ROUTED);
  const auto boxes = s.writeToDb(swire, nullptr, false);
  EXPECT_EQ(boxes.size(), 1u);
}

// writeToDb with a BTerm + make_rect_as_pin promotes the rect to a
// b-pin: returns the SBox + the pin box.
TEST_F(TestShape, WriteToDbMakeRectAsPinAddsBPinBox)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.setLocked();
  s.setGridComponent(makeGridComponent());
  odb::dbSWire* swire = odb::dbSWire::create(power_, odb::dbWireType::ROUTED);
  const auto boxes = s.writeToDb(swire,
                                 makePinBTerm(power_),
                                 /*make_rect_as_pin=*/true);
  EXPECT_GE(boxes.size(), 2u);  // SBox + at least one pin box
}

// writeToDb with add_pins + a BTerm connection on an X-axis die edge
// promotes that bterm to a pin and stretches it along Y.
TEST_F(TestShape, WriteToDbBTermOnXEdgeStretchesAlongY)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.setLocked();
  s.addBTermConnection(odb::Rect(0, 10, 5, 20));  // touches block left edge
  s.setGridComponent(makeGridComponent());
  odb::dbSWire* swire = odb::dbSWire::create(power_, odb::dbWireType::ROUTED);
  const auto boxes = s.writeToDb(swire,
                                 makePinBTerm(power_),
                                 /*make_rect_as_pin=*/false);
  EXPECT_GE(boxes.size(), 1u);  // at least the SBox; bterm box added too
}

// And the symmetric right-edge case: bterm whose xMax touches the die area
// right edge.
TEST_F(TestShape, WriteToDbBTermOnRightEdgeStretchesAlongY)
{
  Shape s(metal1_, power_, odb::Rect(9900, 0, 10000, 50));
  s.setLocked();
  // BTerm touching the right edge of the 10000x10000 die area.
  s.addBTermConnection(odb::Rect(9995, 10, 10000, 20));
  s.setGridComponent(makeGridComponent());
  odb::dbSWire* swire = odb::dbSWire::create(power_, odb::dbWireType::ROUTED);
  const auto boxes = s.writeToDb(swire, makePinBTerm(power_), false);
  EXPECT_GE(boxes.size(), 1u);
}

// writeToDb on a non-locked shape connected only through a non-failed via
// drives hasDBConnectivity through its via-loop branch.
TEST_F(TestShape, WriteToDbConnectedThroughViaEmitsBox)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.addVia(makeVia(metal1_, metal2_));
  s.setGridComponent(makeGridComponent());
  odb::dbSWire* swire = odb::dbSWire::create(power_, odb::dbWireType::ROUTED);
  const auto boxes = s.writeToDb(swire, nullptr, false);
  EXPECT_EQ(boxes.size(), 1u);
}

// writeToDb with a BTerm + a bterm connection on a Y-axis die edge stretches
// along X.
TEST_F(TestShape, WriteToDbBTermOnYEdgeStretchesAlongX)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.setLocked();
  s.addBTermConnection(odb::Rect(10, 0, 20, 5));  // touches block bottom edge
  s.setGridComponent(makeGridComponent());
  odb::dbSWire* swire = odb::dbSWire::create(power_, odb::dbWireType::ROUTED);
  const auto boxes = s.writeToDb(swire, makePinBTerm(power_), false);
  EXPECT_GE(boxes.size(), 1u);
}

// addBPinToDb attaches the pin to exactly the BTerm it was handed, rather
// than looking one up itself.
TEST_F(TestShape, WriteToDbAttachesPinToTheSuppliedBTerm)
{
  odb::dbBTerm* supplied = makePinBTerm(power_);
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.setLocked();
  s.addBTermConnection(odb::Rect(0, 10, 5, 20));
  s.setGridComponent(makeGridComponent());
  odb::dbSWire* swire = odb::dbSWire::create(power_, odb::dbWireType::ROUTED);
  s.writeToDb(swire, supplied, false);
  // The supplied BTerm should now have at least one BPin attached.
  EXPECT_GE(supplied->getBPins().size(), 1u);
}

// A null BTerm means "no pins": the SBox is still emitted, but nothing is
// promoted to a pin even when the shape carries bterm connections and
// make_rect_as_pin is set.
TEST_F(TestShape, WriteToDbWithNullBTermWritesNoPins)
{
  odb::dbBTerm* unused = makePinBTerm(power_);
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.setLocked();
  s.addBTermConnection(odb::Rect(0, 10, 5, 20));
  s.setGridComponent(makeGridComponent());
  odb::dbSWire* swire = odb::dbSWire::create(power_, odb::dbWireType::ROUTED);
  const auto boxes = s.writeToDb(swire,
                                 /*bterm=*/nullptr,
                                 /*make_rect_as_pin=*/true);
  EXPECT_EQ(boxes.size(), 1u);  // the SBox only
  EXPECT_TRUE(unused->getBPins().empty());
}

// When the supplied BTerm already carries a BPin without a matching box,
// addBPinToDb reuses that first BPin (rather than creating a new one) and
// adds a fresh box.
TEST_F(TestShape, WriteToDbReusesExistingBPinForNewBox)
{
  odb::dbBTerm* bterm = makePinBTerm(power_);
  odb::dbBPin* existing_pin = odb::dbBPin::create(bterm);
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.setLocked();
  // Request a pin box at a rect that's NOT already present on the BPin.
  s.addBTermConnection(odb::Rect(0, 10, 5, 20));
  s.setGridComponent(makeGridComponent());
  odb::dbSWire* swire = odb::dbSWire::create(power_, odb::dbWireType::ROUTED);
  s.writeToDb(swire, bterm, /*make_rect_as_pin=*/false);
  // The first existing pin should still be present (and now have a box).
  ASSERT_GE(existing_pin->getBoxes().size(), 1u);
  EXPECT_EQ(bterm->getBPins().size(), 1u);  // no second pin created
}

// addBPinToDb finds an existing BPin whose box already matches the
// requested rect -- the pin is reused and NO new box is added.
TEST_F(TestShape, WriteToDbReusesMatchingExistingPinBox)
{
  odb::dbBTerm* bterm = makePinBTerm(power_);
  odb::dbBPin* pin = odb::dbBPin::create(bterm);
  // Use a bterm rect interior to the block so the die-edge adjustment in
  // writeToDb leaves it unchanged, then pre-place the matching box.
  const odb::Rect existing_rect(50, 10, 60, 20);
  odb::dbBox::create(pin,
                     metal1_,
                     existing_rect.xMin(),
                     existing_rect.yMin(),
                     existing_rect.xMax(),
                     existing_rect.yMax());
  // Add another box on a different layer so the layer-mismatch continue
  // branch fires while scanning.
  odb::dbBox::create(pin, metal2_, 0, 0, 50, 50);

  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.setLocked();
  s.addBTermConnection(existing_rect);
  s.setGridComponent(makeGridComponent());
  odb::dbSWire* swire = odb::dbSWire::create(power_, odb::dbWireType::ROUTED);
  const size_t before = pin->getBoxes().size();
  s.writeToDb(swire, bterm, false);
  EXPECT_EQ(pin->getBoxes().size(), before);  // no new box added
}

// -------- populateMapFromDb additional paths --------

// A net with a fixed BPin on a routing layer contributes a kFixed Shape.
TEST_F(TestShape, PopulateMapFromDbReadsFixedBPins)
{
  odb::dbNet* net = odb::dbNet::create(block(), "BTNET");
  odb::dbBTerm* bterm = odb::dbBTerm::create(net, "BTNET");
  odb::dbBPin* bpin = odb::dbBPin::create(bterm);
  bpin->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  odb::dbBox::create(bpin, metal1_, 0, 0, 100, 50);
  ShapeVectorMap map;
  Shape::populateMapFromDb(net, map);
  ASSERT_EQ(map.count(metal1_), 1u);
  EXPECT_EQ(map[metal1_].size(), 1u);
  EXPECT_EQ(map[metal1_].front()->shapeType(), Shape::kFixed);
}

// A non-FIXED bpin is skipped.
TEST_F(TestShape, PopulateMapFromDbSkipsUnfixedBPins)
{
  odb::dbNet* net = odb::dbNet::create(block(), "BTNET2");
  odb::dbBTerm* bterm = odb::dbBTerm::create(net, "BTNET2");
  odb::dbBPin* bpin = odb::dbBPin::create(bterm);
  // No setPlacementStatus(FIRM) -> default is UNPLACED -> isFixed() is false.
  odb::dbBox::create(bpin, metal1_, 0, 0, 100, 50);
  ShapeVectorMap map;
  Shape::populateMapFromDb(net, map);
  EXPECT_TRUE(map.empty());
}

// -------- generateObstruction with spacing rules --------

// A layer spacing on the layer drives the dbspacing branch.
TEST_F(TestShape, GenerateObstructionUsesLayerSpacingBranch)
{
  metal1_->setSpacing(20);
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.generateObstruction();
  EXPECT_EQ(s.getObstruction(), odb::Rect(-20, -20, 120, 70));
}

// An EOL spacing rule whose eolWidth is wider than the shape applies the
// extra EOL spacing along the longitudinal direction of the shape. For a
// horizontal shape (width=50 < eolWidth=100), spacing extends in X.
TEST_F(TestShape, GenerateObstructionHonorsEolRule)
{
  odb::dbTechLayerSpacingEolRule* eol
      = odb::dbTechLayerSpacingEolRule::create(metal1_);
  eol->setEolWidth(100);
  eol->setEolSpace(30);
  Shape s(metal1_, power_, odb::Rect(0, 0, 200, 50));  // width=50
  s.generateObstruction();
  // EOL extends in X for a horizontal shape: -30, +30 on x.
  const odb::Rect obs = s.getObstruction();
  EXPECT_LE(obs.xMin(), -30);
  EXPECT_GE(obs.xMax(), 230);
}

// EOL rule with eolWidth narrower than the shape doesn't apply.
TEST_F(TestShape, GenerateObstructionSkipsEolForWideShapes)
{
  odb::dbTechLayerSpacingEolRule* eol
      = odb::dbTechLayerSpacingEolRule::create(metal1_);
  eol->setEolWidth(10);  // narrower than shape width=50
  eol->setEolSpace(30);
  Shape s(metal1_, power_, odb::Rect(0, 0, 200, 50));
  s.generateObstruction();
  // EOL didn't apply, so the obstruction equals the rect.
  EXPECT_EQ(s.getObstruction(), s.getRect());
}

// Vertical shape with an applicable EOL rule extends in Y instead of X.
TEST_F(TestShape, GenerateObstructionHonorsEolForVerticalShape)
{
  odb::dbTechLayerSpacingEolRule* eol
      = odb::dbTechLayerSpacingEolRule::create(metal2_);
  eol->setEolWidth(100);
  eol->setEolSpace(30);
  Shape s(metal2_, power_, odb::Rect(0, 0, 50, 200));
  s.generateObstruction();
  const odb::Rect obs = s.getObstruction();
  EXPECT_LE(obs.yMin(), -30);
  EXPECT_GE(obs.yMax(), 230);
}

// A LEF58 SPACING TABLE PRL rule on the layer drives the spacing-table
// branch of generateObstruction.
TEST_F(TestShape, GenerateObstructionHonorsSpacingTablePrlRule)
{
  odb::dbTechLayerSpacingTablePrlRule* rule
      = odb::dbTechLayerSpacingTablePrlRule::create(metal1_);
  rule->setEolWidth(0);
  // Single (width=0, length=0) entry with spacing 40 -- always applies.
  rule->setTable(/*width_tbl=*/{0},
                 /*length_tbl=*/{0},
                 /*spacing_tbl=*/{{40}},
                 /*excluded_map=*/{});
  Shape s(metal1_, power_, odb::Rect(0, 0, 100, 50));
  s.generateObstruction();
  // The PRL spacing of 40 should bloat the obstruction.
  EXPECT_LE(s.getObstruction().xMin(), -40);
}

// SpacingTablePrl rules marked WrongDirection are skipped when the shape
// is NOT wrong-way.
TEST_F(TestShape, GenerateObstructionSkipsWrongDirectionRuleForRightWayShape)
{
  odb::dbTechLayerSpacingTablePrlRule* rule
      = odb::dbTechLayerSpacingTablePrlRule::create(metal1_);
  rule->setEolWidth(0);
  rule->setWrongDirection(true);
  rule->setTable({0}, {0}, {{999}}, {});
  // Horizontal shape on a horizontal layer is right-way -- rule is skipped.
  Shape s(metal1_, power_, odb::Rect(0, 0, 200, 50));
  s.generateObstruction();
  // Obstruction equals the rect since the rule didn't apply (and no other
  // spacing rule is configured).
  EXPECT_EQ(s.getObstruction(), s.getRect());
}

// ...and applied when it is: a vertical shape on a horizontal layer is
// bloated by the wrong-direction spacing.
TEST_F(TestShape, GenerateObstructionAppliesWrongDirectionRuleToWrongWayShape)
{
  odb::dbTechLayerSpacingTablePrlRule* rule
      = odb::dbTechLayerSpacingTablePrlRule::create(metal1_);
  rule->setEolWidth(0);
  rule->setWrongDirection(true);
  rule->setTable({0}, {0}, {{999}}, {});
  Shape s(metal1_, power_, odb::Rect(0, 0, 50, 200));
  s.generateObstruction();
  EXPECT_EQ(s.getObstruction(), odb::Rect(-999, -999, 1049, 1199));
}

// -------- cut() additional branches --------

// cut with a same-net non-kShape obstruction fully spanning the shape is
// skipped (the obstruction is "inside" the new strap so no violation).
TEST_F(TestShape, CutSkipsSameNetObstructionFullyOverlappingShape)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 300, 50));
  Shape::ObstructionTree obstructions;
  auto same_net_obs = std::make_shared<Shape>(
      metal1_, odb::Rect(100, 0, 200, 50), Shape::kObs);
  same_net_obs->setNet(power_);  // same net as the shape
  obstructions.insert(same_net_obs);
  std::vector<std::unique_ptr<Shape>> replacements;
  EXPECT_FALSE(s.cut(obstructions, nullptr, replacements));
}

// Vertical shape with a horizontal obstruction in the middle is cut into
// two replacements.
TEST_F(TestShape, CutVerticalShapeWithHorizontalObstructionProducesReplacements)
{
  metal2_->setSpacing(20);
  Shape s(metal2_, power_, odb::Rect(0, 0, 50, 300));
  s.generateObstruction();
  Shape::ObstructionTree obstructions;
  auto obs = std::make_shared<Shape>(
      metal2_, odb::Rect(0, 100, 50, 200), Shape::kObs);
  obstructions.insert(obs);
  std::vector<std::unique_ptr<Shape>> replacements;
  EXPECT_TRUE(s.cut(obstructions, nullptr, replacements));
  std::set<odb::Rect> rects;
  for (const auto& r : replacements) {
    rects.insert(r->getRect());
  }
  EXPECT_EQ(rects,
            (std::set<odb::Rect>{odb::Rect(0, 0, 50, 80),
                                 odb::Rect(0, 220, 50, 300)}));
}

// Vertical shape's same-net obstruction that spans the shape's x range is
// skipped (drives the vertical branch of the same-net-skip logic in cut).
TEST_F(TestShape, CutVerticalSkipsSameNetObstructionSpanningShape)
{
  Shape s(metal2_, power_, odb::Rect(0, 0, 50, 300));
  Shape::ObstructionTree obstructions;
  auto same_net_obs = std::make_shared<Shape>(
      metal2_, odb::Rect(0, 100, 50, 200), Shape::kObs);
  same_net_obs->setNet(power_);
  obstructions.insert(same_net_obs);
  std::vector<std::unique_ptr<Shape>> replacements;
  EXPECT_FALSE(s.cut(obstructions, nullptr, replacements));
}

// The Grid* overload of cut() filters out kGridObs obstructions belonging
// to `ignore_grid`. With one such obstruction the cut becomes a no-op.
TEST_F(TestShape, CutWithIgnoreGridSkipsMatchingGridObs)
{
  // Build a real Grid to use as the ignore-grid sentinel.
  makeGridComponent();
  Shape s(metal1_, power_, odb::Rect(0, 0, 300, 50));
  Shape::ObstructionTree obstructions;
  auto grid_obs = std::make_shared<GridObsShape>(
      metal1_, odb::Rect(100, 0, 200, 50), grid_.get());
  obstructions.insert(grid_obs);
  std::vector<std::unique_ptr<Shape>> replacements;
  EXPECT_FALSE(s.cut(obstructions, /*ignore_grid=*/grid_.get(), replacements));
}

// And the symmetric case where the GridObs belongs to a DIFFERENT grid
// than ignore_grid: the obstruction is kept and the cut runs.
TEST_F(TestShape, CutWithIgnoreGridKeepsMismatchedGridObs)
{
  makeGridComponent();
  Shape s(metal1_, power_, odb::Rect(0, 0, 300, 50));
  Shape::ObstructionTree obstructions;
  auto grid_obs = std::make_shared<GridObsShape>(
      metal1_, odb::Rect(100, 0, 200, 50), grid_.get());
  obstructions.insert(grid_obs);
  std::vector<std::unique_ptr<Shape>> replacements;
  // Pass nullptr ignore_grid so belongsTo(nullptr) is false -> obs kept.
  EXPECT_TRUE(s.cut(obstructions, /*ignore_grid=*/nullptr, replacements));
}

// cut() with an obstruction on a NULL layer also matches (the layer filter
// allows both `layer == self.layer` and `other.layer == nullptr`).
TEST_F(TestShape, CutMatchesObstructionWithNullLayer)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 300, 50));
  Shape::ObstructionTree obstructions;
  // The obstruction's layer must be null to drive the `layer == nullptr`
  // branch in cut's bgi::satisfies filter.
  auto obs = std::make_shared<Shape>(
      /*layer=*/nullptr, odb::Rect(100, 0, 200, 50), Shape::kObs);
  obstructions.insert(obs);
  std::vector<std::unique_ptr<Shape>> replacements;
  EXPECT_TRUE(s.cut(obstructions, nullptr, replacements));
}

// Pad metal on the shape's own net only cuts the shape when it sits inside
// the pad's spacing without touching the shape: touching merges, and a pad a
// full spacing away is legal even though the two obstruction boxes meet.
TEST_F(TestShape, CutAgainstSameNetPadObsOnlyCutsInsideItsSpacing)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 300, 50));
  s.setObstruction(odb::Rect(-20, -20, 320, 70));

  auto cut_by_pad = [&](const odb::Rect& pad_rect) {
    auto pad = std::make_shared<Shape>(metal1_, pad_rect, Shape::kPadObs);
    pad->setNet(power_);
    pad->setObstruction(pad_rect.bloat(20, odb::vertical));
    Shape::ObstructionTree obstructions;
    obstructions.insert(pad);
    std::vector<std::unique_ptr<Shape>> replacements;
    return s.cut(obstructions, nullptr, replacements);
  };

  // Touching the shape: merges.
  EXPECT_FALSE(cut_by_pad(odb::Rect(100, 40, 200, 90)));
  // Only the obstruction boxes meet: the pad is a full spacing away.
  EXPECT_FALSE(cut_by_pad(odb::Rect(100, 80, 200, 130)));
  // The shape reaches into the pad's spacing: a violation.
  EXPECT_TRUE(cut_by_pad(odb::Rect(100, 60, 200, 110)));
}

// A violation that spans the whole shape leaves only the obstruction's halo
// on either end, which lies outside the shape and so yields no replacement.
TEST_F(TestShape, CutDropsPiecesOutsideTheShape)
{
  Shape s(metal1_, power_, odb::Rect(0, 0, 300, 50));
  s.setObstruction(odb::Rect(-20, -20, 320, 70));
  Shape::ObstructionTree obstructions;
  obstructions.insert(
      std::make_shared<Shape>(metal1_, odb::Rect(20, 0, 280, 50), Shape::kObs));
  std::vector<std::unique_ptr<Shape>> replacements;
  EXPECT_TRUE(s.cut(obstructions, nullptr, replacements));
  EXPECT_TRUE(replacements.empty());
}

// -------- FollowPinShape with rows --------

// FollowPinShape::getMinimumRect merges in the rows' bounding boxes.
TEST_F(TestShape, FollowPinGetMinimumRectMergesRows)
{
  odb::dbSite* site = odb::dbSite::create(db_->findLib("lib"), "core");
  site->setWidth(200);
  site->setHeight(50);

  odb::dbRow* r1 = odb::dbRow::create(block(),
                                      "r1",
                                      site,
                                      /*origin_x=*/-100,
                                      /*origin_y=*/0,
                                      odb::dbOrientType::R0,
                                      odb::dbRowDir::HORIZONTAL,
                                      /*num_sites=*/1,
                                      /*spacing=*/200);
  odb::dbRow* r2 = odb::dbRow::create(block(),
                                      "r2",
                                      site,
                                      /*origin_x=*/300,
                                      /*origin_y=*/0,
                                      odb::dbOrientType::R0,
                                      odb::dbRowDir::HORIZONTAL,
                                      1,
                                      200);

  FollowPinShape fp(metal1_, power_, odb::Rect(0, 0, 200, 50));
  fp.addRow(r1);
  fp.addRow(r2);
  const odb::Rect r = fp.getMinimumRect();
  // The min rect should span from r1's xMin (-100) to r2's xMax (500).
  EXPECT_LE(r.xMin(), -100);
  EXPECT_GE(r.xMax(), 500);
}

// A vertical followpin keeps its own x extent and stretches along y to cover
// its rows.
TEST_F(TestShape, FollowPinVerticalGetMinimumRectMergesRows)
{
  odb::dbSite* site = odb::dbSite::create(db_->findLib("lib"), "core");
  site->setWidth(200);
  site->setHeight(50);
  odb::dbRow* row = odb::dbRow::create(block(),
                                       "r1",
                                       site,
                                       /*origin_x=*/0,
                                       /*origin_y=*/-100,
                                       odb::dbOrientType::R0,
                                       odb::dbRowDir::HORIZONTAL,
                                       /*num_sites=*/1,
                                       /*spacing=*/200);

  FollowPinShape fp(metal2_, power_, odb::Rect(0, 0, 50, 200));
  fp.addITermConnection(odb::Rect(10, 100, 40, 120));
  fp.addRow(row);
  EXPECT_EQ(fp.getMinimumRect(), odb::Rect(0, -100, 50, 120));
  EXPECT_FALSE(fp.isFloating());
}

// FollowPinShape::updateTermConnections drops rows that no longer overlap
// the shape after a rect change.
TEST_F(TestShape, FollowPinUpdateTermConnectionsDropsNonOverlappingRows)
{
  odb::dbSite* site = odb::dbSite::create(db_->findLib("lib"), "core2");
  site->setWidth(100);
  site->setHeight(50);

  odb::dbRow* inside = odb::dbRow::create(block(),
                                          "in",
                                          site,
                                          0,
                                          0,
                                          odb::dbOrientType::R0,
                                          odb::dbRowDir::HORIZONTAL,
                                          1,
                                          100);
  odb::dbRow* outside = odb::dbRow::create(block(),
                                           "out",
                                           site,
                                           1000,
                                           1000,
                                           odb::dbOrientType::R0,
                                           odb::dbRowDir::HORIZONTAL,
                                           1,
                                           100);

  FollowPinShape fp(metal1_, power_, odb::Rect(0, 0, 100, 50));
  fp.addRow(inside);
  fp.addRow(outside);
  fp.updateTermConnections();
  // After the update, the outside row should be gone and the inside row
  // remains; getMinimumRect should still cover at least the rect.
  const odb::Rect r = fp.getMinimumRect();
  EXPECT_EQ(r.dx(), 100);
}

// -------- LEF58 WRONGDIRECTION spacing, read from a tech LEF --------

// Reads a tech LEF whose two routing layers, M1 horizontal and M2 vertical,
// each have a plain spacing of 100 and a LEF58 WRONGDIRECTION spacing table
// of 500. The WRONGDIRECTION table applies only to shapes running across
// their layer's routing direction.
class TestShapeWrongDirection : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    odb::lefin reader(db_.get(), getLogger(), false);
    odb::dbTech* tech = reader.createTech(
        "tech",
        getFilePath("_main/src/pdn/test/cpp/wrong_direction_spacing.lef")
            .c_str());
    tst::makeBlock(db_.get(), tech);
    net_ = odb::dbNet::create(db_->getChip()->getBlock(), "VDD");
  }

  odb::dbTechLayer* layer(const char* name) const
  {
    return db_->getTech()->findLayer(name);
  }

  // The rects a strap is cut into to stay clear of an obstruction, or
  // nothing if it does not need cutting.
  std::optional<std::set<odb::Rect>> cut(odb::dbTechLayer* layer,
                                         const odb::Rect& strap,
                                         const odb::Rect& obstruction) const
  {
    Shape shape(layer, net_, strap);
    shape.generateObstruction();
    Shape::ObstructionTree obstructions;
    obstructions.insert(
        std::make_shared<Shape>(layer, obstruction, Shape::kObs));
    std::vector<std::unique_ptr<Shape>> replacements;
    if (!shape.cut(obstructions, nullptr, replacements)) {
      return std::nullopt;
    }
    std::set<odb::Rect> rects;
    for (const auto& replacement : replacements) {
      rects.insert(replacement->getRect());
    }
    return rects;
  }

  odb::dbNet* net_ = nullptr;
};

// The LEF's WRONGDIRECTION table reaches odb with its flag intact.
TEST_F(TestShapeWrongDirection, TheLefRuleIsReadAsWrongDirection)
{
  for (const char* name : {"M1", "M2"}) {
    const auto rules = layer(name)->getTechLayerSpacingTablePrlRules();
    ASSERT_EQ(rules.size(), 1u) << name;
    EXPECT_TRUE((*rules.begin())->isWrongDirection()) << name;
  }
}

// A shape along its layer keeps the plain spacing; a shape across it gets
// the WRONGDIRECTION spacing, on either layer direction.
TEST_F(TestShapeWrongDirection, OnlyWrongWayShapesGetTheWrongDirectionSpacing)
{
  const odb::Rect horizontal(0, 0, 2000, 100);
  const odb::Rect vertical(0, 0, 100, 2000);

  auto obstruction = [&](const char* name, const odb::Rect& rect) {
    Shape shape(layer(name), net_, rect);
    shape.generateObstruction();
    return shape.getObstruction();
  };

  EXPECT_EQ(obstruction("M1", horizontal), odb::Rect(-100, -100, 2100, 200));
  EXPECT_EQ(obstruction("M1", vertical), odb::Rect(-500, -500, 600, 2500));
  EXPECT_EQ(obstruction("M2", vertical), odb::Rect(-100, -100, 200, 2100));
  EXPECT_EQ(obstruction("M2", horizontal), odb::Rect(-500, -500, 2500, 600));
}

// So a strap across its layer is cut back from an obstruction 300 away, out
// to the WRONGDIRECTION spacing on either side of it, while the same strap
// along the layer is far enough from it already.
TEST_F(TestShapeWrongDirection, AWrongWayStrapIsCutAtTheWrongDirectionSpacing)
{
  const odb::Rect horizontal(0, 0, 2000, 100);
  const odb::Rect above(800, 400, 1200, 500);
  const odb::Rect vertical(0, 0, 100, 2000);
  const odb::Rect beside(400, 800, 500, 1200);

  EXPECT_EQ(cut(layer("M1"), vertical, beside),
            (std::set<odb::Rect>{odb::Rect(0, 0, 100, 300),
                                 odb::Rect(0, 1700, 100, 2000)}));
  EXPECT_EQ(cut(layer("M1"), horizontal, above), std::nullopt);

  EXPECT_EQ(cut(layer("M2"), horizontal, above),
            (std::set<odb::Rect>{odb::Rect(0, 0, 300, 100),
                                 odb::Rect(1700, 0, 2000, 100)}));
  EXPECT_EQ(cut(layer("M2"), vertical, beside), std::nullopt);
}

}  // namespace
}  // namespace pdn
