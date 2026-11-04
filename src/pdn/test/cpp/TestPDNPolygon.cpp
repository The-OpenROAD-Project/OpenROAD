// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Unit tests for Region, the polygon-90 set PDN uses for outlines that need
// not be rectangles (a polygon die, a core cut around macros), and for
// getInstanceOutline.

#include <tuple>
#include <utility>
#include <vector>

#include "PdnTest.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "polygon.h"
#include "tst/physical_builder.h"

namespace pdn {
namespace {

using ::testing::ElementsAre;
using ::testing::UnorderedElementsAreArray;

using TestRegion = PdnTest;

// A 4000 square with its top-right 2000 square cut away, leaving a notch whose
// walls are the vertical edge at x = 2000 and the horizontal one at y = 2000.
odb::Polygon lShape()
{
  return odb::Polygon(std::vector<odb::Point>{
      {0, 0}, {4000, 0}, {4000, 2000}, {2000, 2000}, {2000, 4000}, {0, 4000}});
}

using EdgeTuple = std::tuple<odb::Point, odb::Point, odb::Point, bool, bool>;

std::vector<EdgeTuple> edgeTuples(const Region& region)
{
  std::vector<EdgeTuple> edges;
  for (const Region::Edge& edge : region.getEdges()) {
    edges.emplace_back(edge.start,
                       edge.end,
                       edge.normal,
                       edge.convex_at_start,
                       edge.convex_at_end);
  }
  return edges;
}

// -------- Construction --------

// A default region, one built from a rect with no area, and one built from a
// polygon with no points are all empty, and every query on an empty region
// comes back empty.
TEST_F(TestRegion, AnEmptyRegionHasNothingInIt)
{
  for (const Region& empty : {Region(),
                              Region(odb::Rect(0, 0, 0, 100)),
                              Region(odb::Rect(0, 0, 100, 0)),
                              Region(odb::Polygon())}) {
    EXPECT_TRUE(empty.isEmpty());
    EXPECT_EQ(empty.getEnclosingRect(), odb::Rect());
    EXPECT_TRUE(empty.getRects().empty());
    EXPECT_TRUE(empty.getPolygons().empty());
    EXPECT_TRUE(empty.getEdges().empty());
    EXPECT_TRUE(empty.bloat({10, 10, 10, 10}).isEmpty());
    EXPECT_TRUE(empty.fillHoles().isEmpty());
    EXPECT_EQ(empty.getMarginBeyond(odb::Rect(0, 0, 10, 10), odb::Point(1, 0)),
              0);
    EXPECT_FALSE(
        empty.isOnInteriorWall(odb::Rect(0, 0, 10, 10), odb::Point(1, 0)));
  }
}

// The same L can be built from its outline, from a list of outlines, or from
// the overlapping rects that make it up.
TEST_F(TestRegion, ARegionCanBeBuiltFromRectsOrPolygons)
{
  const Region from_rect(odb::Rect(0, 0, 4000, 2000));
  EXPECT_THAT(from_rect.getRects(), ElementsAre(odb::Rect(0, 0, 4000, 2000)));

  const Region from_polygon(lShape());
  EXPECT_EQ(from_polygon.getEnclosingRect(), odb::Rect(0, 0, 4000, 4000));
  EXPECT_EQ(from_polygon.getPolygons().size(), 1u);

  const Region from_rects(std::vector<odb::Rect>{odb::Rect(0, 0, 4000, 2000),
                                                 odb::Rect(0, 0, 2000, 4000)});
  const Region from_polygons(std::vector<odb::Polygon>{lShape()});
  for (const Region& l : {from_rects, from_polygons}) {
    EXPECT_TRUE(l.contains(from_polygon));
    EXPECT_TRUE(from_polygon.contains(l));
  }
}

// -------- Boolean operations --------

TEST_F(TestRegion, BooleanOperationsCombineTheAreas)
{
  const Region a(odb::Rect(0, 0, 2000, 1000));
  const Region b(odb::Rect(1000, 0, 3000, 1000));

  EXPECT_THAT(a.intersect(b).getRects(),
              ElementsAre(odb::Rect(1000, 0, 2000, 1000)));
  EXPECT_THAT(a.intersect(odb::Rect(1500, 0, 5000, 500)).getRects(),
              ElementsAre(odb::Rect(1500, 0, 2000, 500)));
  EXPECT_TRUE(a.intersect(Region(odb::Rect(5000, 0, 6000, 1000))).isEmpty());
  EXPECT_THAT(a.subtract(b).getRects(),
              ElementsAre(odb::Rect(0, 0, 1000, 1000)));
  EXPECT_THAT(a.unite(b).getRects(), ElementsAre(odb::Rect(0, 0, 3000, 1000)));

  EXPECT_TRUE(a.unite(b).contains(a));
  EXPECT_FALSE(a.contains(b));
}

// -------- bloat / fillHoles --------

// Each side moves by its own margin; a negative margin pulls it in.
TEST_F(TestRegion, BloatMovesEachSideByItsOwnMargin)
{
  const Region region(odb::Rect(1000, 1000, 3000, 2000));

  EXPECT_EQ(region.bloat({.left = 100, .bottom = 200, .right = 300, .top = 400})
                .getEnclosingRect(),
            odb::Rect(900, 800, 3300, 2400));
  EXPECT_EQ(
      region.bloat({.left = -100, .bottom = -200, .right = -300, .top = -400})
          .getEnclosingRect(),
      odb::Rect(1100, 1200, 2700, 1600));
  EXPECT_EQ(
      region.bloat({.left = 100, .bottom = -200, .right = -300, .top = 400})
          .getEnclosingRect(),
      odb::Rect(900, 1200, 2700, 2400));
}

// fillHoles closes the hole in a ring and keeps separate pieces separate.
TEST_F(TestRegion, FillHolesKeepsOnlyTheOuterBoundaries)
{
  const odb::Rect hole(1000, 1000, 2000, 2000);
  const Region ring
      = Region(odb::Rect(0, 0, 3000, 3000)).subtract(Region(hole));
  const Region other(odb::Rect(5000, 0, 6000, 1000));
  ASSERT_FALSE(ring.contains(Region(hole)));

  const Region filled = ring.unite(other).fillHoles();
  EXPECT_EQ(filled.getPolygons().size(), 2u);
  EXPECT_TRUE(filled.contains(Region(odb::Rect(0, 0, 3000, 3000))));
  EXPECT_TRUE(filled.contains(other));
}

// -------- getEdges --------

// A rect has four edges, walked counter-clockwise with the outward normal on
// each, and every corner convex.
TEST_F(TestRegion, GetEdgesOfARectWalkItsSides)
{
  const Region region(odb::Rect(0, 0, 4000, 2000));

  const std::vector<EdgeTuple> expected{
      {{0, 0}, {4000, 0}, {0, -1}, true, true},
      {{4000, 0}, {4000, 2000}, {1, 0}, true, true},
      {{4000, 2000}, {0, 2000}, {0, 1}, true, true},
      {{0, 2000}, {0, 0}, {-1, 0}, true, true},
  };
  EXPECT_THAT(edgeTuples(region), UnorderedElementsAreArray(expected));
}

// The L has one concave corner, at the inside of the notch, where the two
// notch walls meet.
TEST_F(TestRegion, GetEdgesMarksTheConcaveCornerOfAnL)
{
  const Region region(lShape());

  const std::vector<EdgeTuple> expected{
      {{0, 0}, {4000, 0}, {0, -1}, true, true},
      {{4000, 0}, {4000, 2000}, {1, 0}, true, true},
      {{4000, 2000}, {2000, 2000}, {0, 1}, true, false},
      {{2000, 2000}, {2000, 4000}, {1, 0}, false, true},
      {{2000, 4000}, {0, 4000}, {0, 1}, true, true},
      {{0, 4000}, {0, 0}, {-1, 0}, true, true},
  };
  EXPECT_THAT(edgeTuples(region), UnorderedElementsAreArray(expected));

  for (const Region::Edge& edge : region.getEdges()) {
    EXPECT_EQ(edge.isHorizontal(), edge.normal.x() == 0);
  }
}

// Vertices in the middle of a straight run are merged away, so an edge always
// runs corner to corner -- whether they come from the outline or from rows
// stacked to make the region.
TEST_F(TestRegion, GetEdgesMergesCollinearVertices)
{
  const Region outline(odb::Polygon(std::vector<odb::Point>{
      {0, 0}, {2000, 0}, {4000, 0}, {4000, 1000}, {4000, 2000}, {0, 2000}}));
  const Region rows(std::vector<odb::Rect>{odb::Rect(0, 0, 4000, 1000),
                                           odb::Rect(0, 1000, 4000, 2000)});
  const Region rect(odb::Rect(0, 0, 4000, 2000));

  EXPECT_THAT(edgeTuples(outline), UnorderedElementsAreArray(edgeTuples(rect)));
  EXPECT_THAT(edgeTuples(rows), UnorderedElementsAreArray(edgeTuples(rect)));
}

// -------- getMarginBeyond --------

// The margin is how far the whole face can move outward before any of it
// leaves the region.
TEST_F(TestRegion, GetMarginBeyondMeasuresHowFarTheFaceCanMove)
{
  const Region region(lShape());
  const odb::Rect inside(500, 500, 1000, 1000);

  EXPECT_EQ(region.getMarginBeyond(inside, odb::Point(0, 1)), 3000);
  EXPECT_EQ(region.getMarginBeyond(inside, odb::Point(1, 0)), 3000);
  EXPECT_EQ(region.getMarginBeyond(inside, odb::Point(0, -1)), 500);
  EXPECT_EQ(region.getMarginBeyond(inside, odb::Point(-1, 0)), 500);

  // Below the notch the region stops at the notch floor.
  EXPECT_EQ(region.getMarginBeyond(odb::Rect(2500, 500, 3500, 1500),
                                   odb::Point(0, 1)),
            500);
  // A face that straddles the notch wall is held back by the part of it
  // under the notch.
  EXPECT_EQ(region.getMarginBeyond(odb::Rect(1500, 500, 2500, 1000),
                                   odb::Point(0, 1)),
            1000);
}

// A face with no extent across its normal has nothing to move.
TEST_F(TestRegion, GetMarginBeyondADegenerateFaceIsZero)
{
  const Region region(lShape());
  EXPECT_EQ(
      region.getMarginBeyond(odb::Rect(500, 500, 500, 1000), odb::Point(0, 1)),
      0);
}

// The edge form measures from the edge's span along its own normal,
// whichever way round the edge runs.
TEST_F(TestRegion, GetMarginBeyondAnEdgeUsesItsSpanAndNormal)
{
  const Region region(lShape());

  Region::Edge edge;
  edge.start = odb::Point(500, 1000);
  edge.end = odb::Point(1000, 1000);
  edge.normal = odb::Point(0, 1);
  EXPECT_EQ(region.getMarginBeyond(edge), 3000);

  std::swap(edge.start, edge.end);
  EXPECT_EQ(region.getMarginBeyond(edge), 3000);
}

// -------- isOnInteriorWall --------

// The walls of the notch are interior walls: a face on one has nowhere to go.
TEST_F(TestRegion, AFaceOnTheNotchIsOnAnInteriorWall)
{
  const Region region(lShape());

  EXPECT_TRUE(region.isOnInteriorWall(odb::Rect(1500, 3000, 2000, 3500),
                                      odb::Point(1, 0)));
  EXPECT_TRUE(region.isOnInteriorWall(odb::Rect(2500, 1500, 3500, 2000),
                                      odb::Point(0, 1)));
}

// Only faces on the notch count: not one on the bounding box, not one with
// room to move, not one on a shape outside the region, and not one that
// overhangs the wall rather than stopping at it.
TEST_F(TestRegion, OtherFacesAreNotOnAnInteriorWall)
{
  const Region region(lShape());

  EXPECT_FALSE(region.isOnInteriorWall(odb::Rect(0, 1000, 500, 1500),
                                       odb::Point(-1, 0)));
  EXPECT_FALSE(region.isOnInteriorWall(odb::Rect(500, 500, 1000, 1000),
                                       odb::Point(1, 0)));
  EXPECT_FALSE(region.isOnInteriorWall(odb::Rect(3000, 3000, 3500, 3500),
                                       odb::Point(-1, 0)));
  EXPECT_FALSE(region.isOnInteriorWall(odb::Rect(1500, 3000, 2500, 3500),
                                       odb::Point(1, 0)));
}

// -------- getInstanceOutline --------

// Fixture with an OVERLAP layer for outlines, a routing layer and a tech via
// for obstructions that are not outlines, and a 1000 x 2000 block master.
class TestInstanceOutline : public PdnTest
{
 protected:
  void SetUp() override
  {
    PdnTest::SetUp();
    overlap_ = odb::dbTechLayer::create(
        tech(), "OVERLAP", odb::dbTechLayerType::OVERLAP);
    metal_ = tst::addRoutingLayer(tech(), "M1");
    odb::dbTechLayer* cut = tst::addCutLayer(tech(), "V1");
    odb::dbTechLayer* upper = tst::addRoutingLayer(tech(), "M2");
    via_ = tst::addVia(tech(), "VIA12", metal_, cut, upper);
  }

  odb::dbMaster* makeMacro(const char* name)
  {
    return tst::makeMaster(
        db_->findLib("lib"), name, 1000, 2000, {}, odb::dbMasterType::BLOCK);
  }

  odb::dbInst* place(odb::dbMaster* master,
                     int x,
                     int y,
                     odb::dbOrientType orient = odb::dbOrientType::R0)
  {
    odb::dbInst* inst = odb::dbInst::create(block(), master, "inst");
    inst->setOrient(orient);
    inst->setLocation(x, y);
    inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
    return inst;
  }

  odb::dbTechLayer* overlap_ = nullptr;
  odb::dbTechLayer* metal_ = nullptr;
  odb::dbTechVia* via_ = nullptr;
};

// A macro that declares no outline is its placed bounding box.
TEST_F(TestInstanceOutline, AMacroWithoutAnOutlineIsItsBoundingBox)
{
  odb::dbMaster* master = makeMacro("plain");
  odb::dbBox::create(master, metal_, 0, 0, 1000, 2000);

  EXPECT_THAT(getInstanceOutline(place(master, 100, 200)).getRects(),
              ElementsAre(odb::Rect(100, 200, 1100, 2200)));
}

// An OVERLAP polygon is the macro's outline, placed with the instance.
// Obstructions on other layers, and via obstructions, are not part of it.
TEST_F(TestInstanceOutline, AnOverlapPolygonIsTheOutline)
{
  odb::dbMaster* master = makeMacro("lshaped");
  odb::dbPolygon::create(
      master,
      overlap_,
      {{0, 0}, {1000, 0}, {1000, 1000}, {500, 1000}, {500, 2000}, {0, 2000}});
  odb::dbBox::create(master, metal_, 0, 0, 1000, 2000);
  odb::dbPolygon::create(
      master, metal_, {{0, 0}, {1000, 0}, {1000, 2000}, {0, 2000}});
  odb::dbBox::create(master, via_, 500, 500);

  const Region outline = getInstanceOutline(place(master, 1000, 0));
  EXPECT_EQ(outline.getEnclosingRect(), odb::Rect(1000, 0, 2000, 2000));
  EXPECT_TRUE(outline.contains(Region(odb::Rect(1000, 1000, 1500, 2000))));
  EXPECT_FALSE(outline.contains(Region(odb::Rect(1500, 1000, 2000, 2000))));
}

// OVERLAP rects make up the outline too, and follow the instance's
// orientation: mirrored about y, the notch moves to the other side.
TEST_F(TestInstanceOutline, OverlapRectsAreTheOutlineInPlacedOrientation)
{
  odb::dbMaster* master = makeMacro("rects");
  odb::dbBox::create(master, overlap_, 0, 0, 1000, 1000);
  odb::dbBox::create(master, overlap_, 0, 1000, 500, 2000);

  const Region outline
      = getInstanceOutline(place(master, 0, 0, odb::dbOrientType::MY));
  EXPECT_EQ(outline.getEnclosingRect(), odb::Rect(0, 0, 1000, 2000));
  EXPECT_TRUE(outline.contains(Region(odb::Rect(500, 1000, 1000, 2000))));
  EXPECT_FALSE(outline.contains(Region(odb::Rect(0, 1000, 500, 2000))));
}

}  // namespace
}  // namespace pdn
