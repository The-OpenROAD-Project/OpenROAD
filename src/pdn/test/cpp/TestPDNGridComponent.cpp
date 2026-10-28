// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Unit tests for GridComponent, the base of every rings / straps / followpin
// component: how it adds, merges, cuts, replaces and writes out its shapes,
// and the layer checks its subclasses rely on.

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "PdnTest.h"
#include "connect.h"
#include "domain.h"
#include "gmock/gmock.h"
#include "grid.h"
#include "grid_component.h"
#include "gtest/gtest.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "shape.h"
// Connect's via map holds stacked vias, which hold TechLayers; both need to
// be complete here.
#include "techlayer.h"
#include "tst/physical_builder.h"
#include "via.h"

namespace pdn {
namespace {

using ::testing::ElementsAre;
using ::testing::UnorderedElementsAre;

// A grid component whose shapes are whatever the test queues for it, with the
// protected helpers re-published.
class QueuedComponent : public GridComponent
{
 public:
  using GridComponent::GridComponent;

  using GridComponent::addShape;
  using GridComponent::checkLayerSpacing;
  using GridComponent::checkLayerWidth;

  void queue(std::unique_ptr<Shape> shape)
  {
    queued_.push_back(std::move(shape));
  }

  void makeShapes(const Shape::ShapeTreeMap& /* other_shapes */) override
  {
    for (auto& shape : queued_) {
      addShape(std::move(shape));
    }
    queued_.clear();
  }
  void report() const override {}
  Type type() const override { return kStrap; }
  void checkLayerSpecifications() const override {}
  bool allowDbPins() const override
  {
    return allow_db_pins && GridComponent::allowDbPins();
  }

  bool allow_intersections = false;
  bool allow_db_pins = true;

 protected:
  bool areIntersectionsAllowed() const override
  {
    return allow_intersections || GridComponent::areIntersectionsAllowed();
  }

 private:
  std::vector<std::unique_ptr<Shape>> queued_;
};

// Fixture with a three-layer stack (M1 horizontal, M2 vertical, M3
// horizontal), each 100 min width, on a 10000 square die, and a core grid to
// hang components on.
class TestPDNGridComponent : public PdnTest
{
 protected:
  void SetUp() override
  {
    PdnTest::SetUp();
    m1_ = addMetal("M1", odb::dbTechLayerDir::HORIZONTAL);
    tst::addCutLayer(tech(), "V1");
    m2_ = addMetal("M2", odb::dbTechLayerDir::VERTICAL);
    tst::addCutLayer(tech(), "V2");
    m3_ = addMetal("M3", odb::dbTechLayerDir::HORIZONTAL);

    power_ = odb::dbNet::create(block(), "VDD");
    power_->setSigType(odb::dbSigType::POWER);
    power_->setSpecial();
    ground_ = odb::dbNet::create(block(), "VSS");
    ground_->setSigType(odb::dbSigType::GROUND);
    ground_->setSpecial();
    block()->setDieArea(odb::Rect(0, 0, 10000, 10000));
    block()->setCoreArea(odb::Rect(1000, 1000, 9000, 9000));

    domain_ = std::make_unique<VoltageDomain>(nullptr,
                                              block(),
                                              power_,
                                              ground_,
                                              std::vector<odb::dbNet*>{},
                                              getLogger());
    grid_ = makeGrid("grid");
  }

  odb::dbTechLayer* addMetal(const char* name, odb::dbTechLayerDir dir)
  {
    odb::dbTechLayer* layer = tst::addRoutingLayer(
        tech(), name, {.direction = dir, .width = kMinWidth});
    layer->setMinWidth(kMinWidth);
    layer->setMaxWidth(kMaxWidth);
    return layer;
  }

  std::unique_ptr<TestGrid> makeGrid(const char* name)
  {
    return std::make_unique<TestGrid>(domain_.get(),
                                      name,
                                      /*starts_with_power=*/true,
                                      std::vector<odb::dbTechLayer*>{});
  }

  static std::unique_ptr<Shape> shape(odb::dbTechLayer* layer,
                                      const odb::Rect& rect,
                                      odb::dbNet* net)
  {
    return std::make_unique<Shape>(
        layer, net, rect, odb::dbWireShapeType::STRIPE);
  }

  std::unique_ptr<Shape> power(odb::dbTechLayer* layer, const odb::Rect& rect)
  {
    return shape(layer, rect, power_);
  }

  static constexpr int kMinWidth = 100;
  static constexpr int kMaxWidth = 2000;

  odb::dbTechLayer* m1_ = nullptr;
  odb::dbTechLayer* m2_ = nullptr;
  odb::dbTechLayer* m3_ = nullptr;
  odb::dbNet* power_ = nullptr;
  odb::dbNet* ground_ = nullptr;
  std::unique_ptr<VoltageDomain> domain_;
  std::unique_ptr<TestGrid> grid_;
};

// -------- Accessors --------

// A component reaches the block, logger and domain through its grid, and
// starts with power when its grid does.
TEST_F(TestPDNGridComponent, AccessorsComeFromTheGrid)
{
  QueuedComponent component(grid_.get());

  EXPECT_EQ(component.getGrid(), grid_.get());
  EXPECT_EQ(component.getBlock(), block());
  EXPECT_EQ(component.getLogger(), getLogger());
  EXPECT_EQ(component.getDomain(), domain_.get());
  EXPECT_TRUE(component.getStartsWithPower());
  component.setStartWithPower(false);
  EXPECT_TRUE(component.getStartsWithGround());

  const std::unique_ptr<TestGrid> other = makeGrid("other");
  component.setGrid(other.get());
  EXPECT_EQ(component.getGrid(), other.get());
}

// The base class answers the per-component questions with the defaults a
// plain user-requested component wants.
TEST_F(TestPDNGridComponent, DefaultsSuitAUserRequestedComponent)
{
  QueuedComponent component(grid_.get());
  const GridComponent& base = component;

  EXPECT_FALSE(base.isAutoInserted());
  EXPECT_TRUE(base.checkForRepairChannels());
  Shape::ShapeTreeMap shapes;
  Shape::ObstructionTreeMap obstructions;
  EXPECT_FALSE(component.refineShapes(shapes, obstructions));
  component.getConnectableShapes(shapes);
  EXPECT_TRUE(shapes.empty());
}

TEST_F(TestPDNGridComponent, TypeToStringNamesEveryType)
{
  EXPECT_EQ(GridComponent::typeToString(GridComponent::kRing), "Ring");
  EXPECT_EQ(GridComponent::typeToString(GridComponent::kStrap), "Strap");
  EXPECT_EQ(GridComponent::typeToString(GridComponent::kFollowpin),
            "Followpin");
  EXPECT_EQ(GridComponent::typeToString(GridComponent::kPadConnect),
            "Pad connect");
  EXPECT_EQ(GridComponent::typeToString(GridComponent::kMacroEdgeConnect),
            "Macro edge connect");
  EXPECT_EQ(GridComponent::typeToString(GridComponent::kRepairChannel),
            "Repair channel");
  EXPECT_EQ(GridComponent::typeToString(static_cast<GridComponent::Type>(99)),
            "Unknown");
}

// -------- Nets --------

// Without nets of its own a component lays out the grid's nets, starting with
// whichever it starts with; once given nets it uses those.
TEST_F(TestPDNGridComponent, NetsDefaultToTheGridsInLayoutOrder)
{
  QueuedComponent component(grid_.get());
  EXPECT_THAT(component.getNets(), ElementsAre(power_, ground_));
  component.setStartWithPower(false);
  EXPECT_THAT(component.getNets(), ElementsAre(ground_, power_));
  EXPECT_EQ(component.getNetCount(), 2);

  component.setNets({ground_});
  EXPECT_THAT(component.getNets(), ElementsAre(ground_));
  EXPECT_EQ(component.getNetCount(), 1);
}

// A component can only be given nets its grid has.
TEST_F(TestPDNGridComponent, SetNetsRejectsANetOutsideTheGrid)
{
  QueuedComponent component(grid_.get());
  odb::dbNet* other = odb::dbNet::create(block(), "OTHER");
  EXPECT_THROW(component.setNets({power_, other}), std::runtime_error);
}

// -------- addShape --------

// An added shape belongs to the component and carries an obstruction grown
// by the layer spacing.
TEST_F(TestPDNGridComponent, AddShapeOwnsTheShapeAndBuildsItsObstruction)
{
  m1_->setSpacing(50);
  QueuedComponent component(grid_.get());
  const odb::Rect rect(1000, 1000, 5000, 1100);

  const ShapePtr added = component.addShape(power(m1_, rect));
  ASSERT_NE(added, nullptr);
  EXPECT_EQ(added->getGridComponent(), &component);
  EXPECT_EQ(added->getObstruction(), odb::Rect(950, 950, 5050, 1150));
  EXPECT_EQ(component.getShapeCount(), 1);
  EXPECT_THAT(component.getShapeRects(), ElementsAre(rect));
  EXPECT_EQ(component.getShapes().at(m1_).size(), 1u);
  EXPECT_FALSE(added->hasBTermConnections());

  component.clearShapes();
  EXPECT_EQ(component.getShapeCount(), 0);
}

// A shape that breaks the layer's minimum area is not added.
TEST_F(TestPDNGridComponent, AddShapeRejectsAShapeUnderTheMinimumArea)
{
  m1_->setArea(1e6);  // compared against the rect's area in dbu^2
  QueuedComponent component(grid_.get());

  EXPECT_EQ(component.addShape(power(m1_, odb::Rect(1000, 1000, 2000, 1100))),
            nullptr);
  EXPECT_EQ(component.getShapeCount(), 0);
}

// Overlapping a shape of another net would be a short.
TEST_F(TestPDNGridComponent, AddShapeRejectsAShortToAnotherNet)
{
  QueuedComponent component(grid_.get());
  ASSERT_NE(component.addShape(power(m1_, odb::Rect(1000, 1000, 5000, 1100))),
            nullptr);

  EXPECT_EQ(component.addShape(
                shape(m1_, odb::Rect(2000, 1000, 2100, 1100), ground_)),
            nullptr);
  EXPECT_EQ(component.getShapeCount(), 1);
}

// Shapes of one net that only touch are left as they are.
TEST_F(TestPDNGridComponent, AddShapeKeepsShapesOfOneNetThatOnlyTouch)
{
  QueuedComponent component(grid_.get());
  ASSERT_NE(component.addShape(power(m1_, odb::Rect(1000, 1000, 2000, 1100))),
            nullptr);
  ASSERT_NE(component.addShape(power(m1_, odb::Rect(2000, 1000, 3000, 1100))),
            nullptr);
  EXPECT_EQ(component.getShapeCount(), 2);
}

// Touching metal is connected metal, so shapes of two nets that only touch,
// even at a corner, are a short.
TEST_F(TestPDNGridComponent, AddShapeRejectsShapesOfTwoNetsThatOnlyTouch)
{
  QueuedComponent component(grid_.get());
  ASSERT_NE(component.addShape(power(m1_, odb::Rect(1000, 1000, 2000, 1100))),
            nullptr);
  EXPECT_EQ(component.addShape(
                shape(m1_, odb::Rect(2000, 1000, 3000, 1100), ground_)),
            nullptr);
  EXPECT_EQ(component.addShape(
                shape(m1_, odb::Rect(2000, 1100, 3000, 1200), ground_)),
            nullptr);
  EXPECT_EQ(component.getShapeCount(), 1);
}

// Overlapping shapes of the same net that line up across their width merge
// into one, whichever axis they run along.
TEST_F(TestPDNGridComponent, AddShapeMergesAlignedOverlapsOfTheSameNet)
{
  QueuedComponent component(grid_.get());
  component.addShape(power(m1_, odb::Rect(1000, 1000, 3000, 1100)));
  component.addShape(power(m1_, odb::Rect(2000, 1000, 4000, 1100)));
  component.addShape(power(m2_, odb::Rect(5000, 1000, 5100, 3000)));
  component.addShape(power(m2_, odb::Rect(5000, 2000, 5100, 4000)));

  EXPECT_THAT(component.getShapeRects(),
              UnorderedElementsAre(odb::Rect(1000, 1000, 4000, 1100),
                                   odb::Rect(5000, 1000, 5100, 4000)));
}

// Same-net shapes that overlap without lining up cannot be merged into one
// rect, so the new one is dropped -- unless the component allows overlaps.
TEST_F(TestPDNGridComponent, AddShapeHandlesAMisalignedOverlapOfTheSameNet)
{
  const odb::Rect first(1000, 1000, 3000, 1100);
  const odb::Rect offset(2000, 1050, 4000, 1150);

  QueuedComponent strict(grid_.get());
  strict.addShape(power(m1_, first));
  EXPECT_EQ(strict.addShape(power(m1_, offset)), nullptr);
  EXPECT_EQ(strict.getShapeCount(), 1);

  QueuedComponent overlapping(grid_.get());
  overlapping.allow_intersections = true;
  overlapping.addShape(power(m1_, first));
  EXPECT_NE(overlapping.addShape(power(m1_, offset)), nullptr);
  EXPECT_EQ(overlapping.getShapeCount(), 2);
}

// A shape reaching the edge of the die gets a pin there, one min width deep.
TEST_F(TestPDNGridComponent, AddShapeAddsAPinWhereAShapeReachesTheDieEdge)
{
  QueuedComponent component(grid_.get());

  const ShapePtr across
      = component.addShape(power(m1_, odb::Rect(0, 1000, 10000, 1100)));
  EXPECT_THAT(across->getBtermConnections(),
              UnorderedElementsAre(odb::Rect(0, 1000, 100, 1100),
                                   odb::Rect(9900, 1000, 10000, 1100)));

  const ShapePtr up
      = component.addShape(power(m2_, odb::Rect(2000, 0, 2100, 10000)));
  EXPECT_THAT(up->getBtermConnections(),
              UnorderedElementsAre(odb::Rect(2000, 0, 2100, 100),
                                   odb::Rect(2000, 9900, 2100, 10000)));

  const ShapePtr inside
      = component.addShape(power(m1_, odb::Rect(1000, 3000, 9000, 3100)));
  EXPECT_FALSE(inside->hasBTermConnections());
}

// On a polygon die the walls of a notch are die edges too. The die here is
// a 10000 square with its top-right 5000 square cut away.
TEST_F(TestPDNGridComponent, AddShapeAddsAPinAtTheWallOfANotch)
{
  block()->setDieArea(odb::Polygon(std::vector<odb::Point>{{0, 0},
                                                           {10000, 0},
                                                           {10000, 5000},
                                                           {5000, 5000},
                                                           {5000, 10000},
                                                           {0, 10000}}));
  QueuedComponent component(grid_.get());

  const ShapePtr to_wall
      = component.addShape(power(m1_, odb::Rect(0, 7000, 5000, 7100)));
  EXPECT_THAT(to_wall->getBtermConnections(),
              UnorderedElementsAre(odb::Rect(0, 7000, 100, 7100),
                                   odb::Rect(4900, 7000, 5000, 7100)));

  const ShapePtr to_floor
      = component.addShape(power(m2_, odb::Rect(7000, 1000, 7100, 5000)));
  EXPECT_THAT(to_floor->getBtermConnections(),
              ElementsAre(odb::Rect(7000, 4900, 7100, 5000)));

  const ShapePtr short_of_wall
      = component.addShape(power(m1_, odb::Rect(1000, 8000, 4000, 8100)));
  EXPECT_FALSE(short_of_wall->hasBTermConnections());
}

// -------- removeShape / replaceShape --------

// Removing a shape takes it out of the component and out of the vias that
// land on it, and leaves the other shapes alone.
TEST_F(TestPDNGridComponent, RemoveShapeDetachesItFromItsVias)
{
  QueuedComponent component(grid_.get());
  const ShapePtr kept
      = component.addShape(power(m1_, odb::Rect(1000, 1000, 5000, 1100)));
  const ShapePtr removed
      = component.addShape(power(m1_, odb::Rect(1000, 3000, 5000, 3100)));
  Connect connect(grid_.get(), m1_, m2_);
  const auto via = std::make_shared<Via>(
      &connect, power_, odb::Rect(2000, 3000, 2100, 3100), removed, nullptr);
  removed->addVia(via);

  component.removeShape(removed.get());
  EXPECT_THAT(component.getShapeRects(), ElementsAre(kept->getRect()));
  EXPECT_EQ(via->getLowerShape(), nullptr);

  // A shape the component does not hold is ignored.
  const std::unique_ptr<Shape> stranger
      = power(m1_, odb::Rect(1000, 5000, 5000, 5100));
  component.removeShape(stranger.get());
  EXPECT_EQ(component.getShapeCount(), 1);
}

// The replacements of a shape take over the vias that land on them, as the
// lower or upper shape according to the via's layers. A replacement that
// cannot be added is skipped.
TEST_F(TestPDNGridComponent, ReplaceShapeHandsItsViasToTheReplacements)
{
  QueuedComponent component(grid_.get());
  const ShapePtr strap
      = component.addShape(power(m2_, odb::Rect(2000, 1000, 2100, 9000)));
  component.addShape(shape(m2_, odb::Rect(3000, 7000, 3100, 9000), ground_));

  Connect m1_m2(grid_.get(), m1_, m2_);
  Connect m2_m3(grid_.get(), m2_, m3_);
  Connect m1_m3(grid_.get(), m1_, m3_);
  const odb::Rect area(2000, 2000, 2100, 2100);
  const ShapePtr m1 = std::make_shared<Shape>(m1_, power_, area);
  const ShapePtr m3 = std::make_shared<Shape>(m3_, power_, area);
  const auto below = std::make_shared<Via>(&m1_m2, power_, area, m1, strap);
  const auto above = std::make_shared<Via>(&m2_m3, power_, area, strap, m3);
  const auto through = std::make_shared<Via>(&m1_m3, power_, area, m1, m3);
  for (const ViaPtr& via : {below, above, through}) {
    strap->addVia(via);
  }

  std::vector<std::unique_ptr<Shape>> replacements;
  replacements.push_back(power(m2_, odb::Rect(2000, 1000, 2100, 5000)));
  // Shorts to the ground strap, so it is not added.
  replacements.push_back(power(m2_, odb::Rect(2000, 6000, 3100, 9000)));
  component.replaceShape(strap.get(), replacements);
  EXPECT_TRUE(replacements.empty());

  ASSERT_EQ(component.getShapeCount(), 2);
  const ShapePtr piece = below->getUpperShape();
  ASSERT_NE(piece, nullptr);
  EXPECT_EQ(piece->getRect(), odb::Rect(2000, 1000, 2100, 5000));
  EXPECT_EQ(above->getLowerShape(), piece);
  EXPECT_EQ(through->getLowerShape(), m1);
  EXPECT_EQ(through->getUpperShape(), m3);
}

// A shape can also be replaced by a single shape.
TEST_F(TestPDNGridComponent, ReplaceShapeWithASingleShape)
{
  QueuedComponent component(grid_.get());
  const ShapePtr strap
      = component.addShape(power(m2_, odb::Rect(2000, 1000, 2100, 9000)));
  Connect connect(grid_.get(), m1_, m2_);
  const odb::Rect area(2000, 2000, 2100, 2100);
  const auto via
      = std::make_shared<Via>(&connect,
                              power_,
                              area,
                              std::make_shared<Shape>(m1_, power_, area),
                              strap);
  strap->addVia(via);

  component.replaceShape(strap.get(),
                         power(m2_, odb::Rect(2000, 1500, 2100, 5000)));
  EXPECT_THAT(component.getShapeRects(),
              ElementsAre(odb::Rect(2000, 1500, 2100, 5000)));
  EXPECT_EQ(via->getUpperShape()->getRect(), odb::Rect(2000, 1500, 2100, 5000));
}

// -------- Sharing shapes and obstructions --------

// A component adds its shapes and obstructions to shared maps, and takes
// them back out again without touching anyone else's.
TEST_F(TestPDNGridComponent, ShapesAndObstructionsGoInAndOutOfSharedMaps)
{
  QueuedComponent component(grid_.get());
  component.addShape(power(m1_, odb::Rect(1000, 1000, 5000, 1100)));
  component.addShape(power(m1_, odb::Rect(1000, 3000, 5000, 3100)));
  component.addShape(power(m2_, odb::Rect(2000, 1000, 2100, 5000)));
  const ShapePtr foreign
      = std::make_shared<Shape>(m1_, power_, odb::Rect(1000, 6000, 5000, 6100));

  Shape::ShapeTreeMap shapes;
  shapes[m1_].insert(foreign);
  component.getShapes(shapes);
  EXPECT_EQ(shapes[m1_].size(), 3u);
  EXPECT_EQ(shapes[m2_].size(), 1u);
  component.removeShapes(shapes);
  EXPECT_EQ(shapes[m1_].size(), 1u);
  EXPECT_EQ(shapes[m2_].size(), 0u);

  Shape::ObstructionTreeMap obstructions;
  obstructions[m1_].insert(foreign);
  component.getObstructions(obstructions);
  EXPECT_EQ(obstructions[m1_].size(), 3u);
  EXPECT_EQ(obstructions[m2_].size(), 1u);
  component.removeObstructions(obstructions);
  EXPECT_EQ(obstructions[m1_].size(), 1u);
  EXPECT_EQ(obstructions[m2_].size(), 0u);
}

// -------- cutShapes / make --------

// Shapes are cut around the obstructions on their own layer; a layer with no
// obstructions, or a shape clear of them, is left alone.
TEST_F(TestPDNGridComponent, CutShapesCutsAroundObstructionsOnTheSameLayer)
{
  QueuedComponent component(grid_.get());
  component.addShape(power(m1_, odb::Rect(1000, 1000, 9000, 1100)));
  component.addShape(power(m1_, odb::Rect(1000, 3000, 9000, 3100)));
  component.addShape(power(m2_, odb::Rect(5000, 1000, 5100, 9000)));

  Shape::ObstructionTreeMap obstructions;
  obstructions[m1_].insert(std::make_shared<Shape>(
      m1_, odb::Rect(4000, 900, 5000, 1200), Shape::kObs));
  component.cutShapes(obstructions);

  EXPECT_THAT(component.getShapeRects(),
              UnorderedElementsAre(odb::Rect(1000, 1000, 4000, 1100),
                                   odb::Rect(5000, 1000, 9000, 1100),
                                   odb::Rect(1000, 3000, 9000, 3100),
                                   odb::Rect(5000, 1000, 5100, 9000)));
}

// make() builds the component's shapes, cuts them around what is already
// there, then publishes them for the components after it. It reports whether
// anything was added.
TEST_F(TestPDNGridComponent, MakeBuildsCutsAndPublishesTheShapes)
{
  QueuedComponent component(grid_.get());
  component.queue(power(m1_, odb::Rect(1000, 1000, 9000, 1100)));

  Shape::ShapeTreeMap shapes;
  Shape::ObstructionTreeMap obstructions;
  obstructions[m1_].insert(std::make_shared<Shape>(
      m1_, odb::Rect(4000, 900, 5000, 1200), Shape::kObs));
  EXPECT_TRUE(component.make(shapes, obstructions));

  EXPECT_THAT(component.getShapeRects(),
              UnorderedElementsAre(odb::Rect(1000, 1000, 4000, 1100),
                                   odb::Rect(5000, 1000, 9000, 1100)));
  EXPECT_EQ(shapes[m1_].size(), 2u);
  EXPECT_EQ(obstructions[m1_].size(), 3u);

  Shape::ShapeTreeMap no_shapes;
  Shape::ObstructionTreeMap no_obstructions;
  EXPECT_FALSE(component.make(no_shapes, no_obstructions));
}

// -------- writeToDb --------

// Shapes are written to the wire of their net, in layer order; shapes of a
// net without a wire are left out. On a pin layer the shape becomes a pin of
// its net's bterm too -- unless the component does not make pins.
TEST_F(TestPDNGridComponent, WriteToDbWritesEachNetsShapesToItsWire)
{
  QueuedComponent component(grid_.get());
  auto locked = [](std::unique_ptr<Shape> shape) {
    shape->setLocked();
    return shape;
  };
  const ShapePtr m2_power = component.addShape(
      locked(power(m2_, odb::Rect(2000, 1000, 2100, 9000))));
  const ShapePtr m1_power = component.addShape(
      locked(power(m1_, odb::Rect(1000, 1000, 9000, 1100))));
  component.addShape(
      locked(shape(m1_, odb::Rect(1000, 3000, 9000, 3100), ground_)));

  odb::dbSWire* wire = odb::dbSWire::create(power_, odb::dbWireType::ROUTED);
  odb::dbBTerm* bterm = odb::dbBTerm::create(power_, "VDD");
  bterm->setSigType(odb::dbSigType::POWER);
  const odb::PtrMap<odb::dbNet, odb::dbSWire*> wires{{power_, wire}};
  const odb::PtrMap<odb::dbNet, odb::dbBTerm*> bterms{{power_, bterm}};

  component.allow_db_pins = false;
  EXPECT_EQ(component.writeToDb(wires, bterms, {m2_}).size(), 2u);
  EXPECT_TRUE(bterm->getBPins().empty());

  component.allow_db_pins = true;
  const auto written = component.writeToDb(wires, bterms, {m2_});
  ASSERT_EQ(written.size(), 2u);
  ASSERT_EQ(written.at(m1_power.get()).size(), 1u);
  ASSERT_EQ(written.at(m2_power.get()).size(), 2u);
  EXPECT_EQ(bterm->getBPins().size(), 1u);
  // The M1 shape was added second but, on the lower layer, is written first.
  EXPECT_LT(written.at(m1_power.get())[0]->getId(),
            written.at(m2_power.get())[0]->getId());
}

// -------- Layer checks --------

// A width must lie between the layer's minimum and maximum, and on twice the
// manufacturing grid.
TEST_F(TestPDNGridComponent, CheckLayerWidthEnforcesTheLayerLimits)
{
  QueuedComponent component(grid_.get());
  const auto dir = odb::dbTechLayerDir::HORIZONTAL;

  EXPECT_NO_THROW(component.checkLayerWidth(m1_, 200, dir));
  EXPECT_THROW(component.checkLayerWidth(m1_, 50, dir), std::runtime_error);
  EXPECT_THROW(component.checkLayerWidth(m1_, 3000, dir), std::runtime_error);
  // both limits are inclusive
  EXPECT_NO_THROW(component.checkLayerWidth(m1_, 100, dir));
  EXPECT_NO_THROW(component.checkLayerWidth(m1_, 2000, dir));
  EXPECT_THROW(component.checkLayerWidth(m1_, 90, dir), std::runtime_error);
  EXPECT_THROW(component.checkLayerWidth(m1_, 2010, dir), std::runtime_error);
  EXPECT_THROW(component.checkLayerWidth(m1_, 205, dir), std::runtime_error);

  tech()->setManufacturingGrid(0);
  EXPECT_NO_THROW(component.checkLayerWidth(m1_, 205, dir));
}

// A width table limits the widths in the direction it applies to -- the
// layer's own, or the other one for a wrong-direction table -- up to its
// largest entry.
TEST_F(TestPDNGridComponent, CheckLayerWidthHonorsTheWidthTables)
{
  QueuedComponent component(grid_.get());
  const auto horizontal = odb::dbTechLayerDir::HORIZONTAL;
  const auto vertical = odb::dbTechLayerDir::VERTICAL;

  // M1 is horizontal, so its table limits horizontal straps.
  odb::dbTechLayerWidthTableRule* table
      = odb::dbTechLayerWidthTableRule::create(m1_);
  table->addWidth(200);
  table->addWidth(400);
  EXPECT_NO_THROW(component.checkLayerWidth(m1_, 200, horizontal));
  EXPECT_THROW(component.checkLayerWidth(m1_, 300, horizontal),
               std::runtime_error);
  EXPECT_NO_THROW(component.checkLayerWidth(m1_, 400, horizontal));
  EXPECT_THROW(component.checkLayerWidth(m1_, 390, horizontal),
               std::runtime_error);
  EXPECT_NO_THROW(component.checkLayerWidth(m1_, 600, horizontal));
  EXPECT_NO_THROW(component.checkLayerWidth(m1_, 300, vertical));

  // M2 is vertical, so its wrong-direction table limits horizontal straps.
  odb::dbTechLayerWidthTableRule* wrong_dir
      = odb::dbTechLayerWidthTableRule::create(m2_);
  wrong_dir->setWrongDirection(true);
  wrong_dir->addWidth(200);
  wrong_dir->addWidth(400);
  EXPECT_THROW(component.checkLayerWidth(m2_, 300, horizontal),
               std::runtime_error);
  EXPECT_NO_THROW(component.checkLayerWidth(m2_, 300, vertical));

  // A table with no widths limits nothing.
  odb::dbTechLayerWidthTableRule::create(m3_);
  EXPECT_NO_THROW(component.checkLayerWidth(m3_, 300, horizontal));
}

// A spacing must be at least the layer's minimum and on the manufacturing
// grid.
TEST_F(TestPDNGridComponent, CheckLayerSpacingEnforcesTheLayerLimits)
{
  m1_->setSpacing(100);
  QueuedComponent component(grid_.get());
  const auto dir = odb::dbTechLayerDir::HORIZONTAL;

  EXPECT_NO_THROW(component.checkLayerSpacing(m1_, 200, 100, dir));
  EXPECT_THROW(component.checkLayerSpacing(m1_, 200, 50, dir),
               std::runtime_error);
  EXPECT_THROW(component.checkLayerSpacing(m1_, 200, 102, dir),
               std::runtime_error);

  tech()->setManufacturingGrid(0);
  EXPECT_NO_THROW(component.checkLayerSpacing(m1_, 200, 102, dir));
}

}  // namespace
}  // namespace pdn
