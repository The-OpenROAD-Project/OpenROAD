#include <algorithm>
#include <cstddef>
#include <vector>

#include "boost/polygon/polygon.hpp"
#include "gtest/gtest.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"
#include "odb/isotropy.h"
#include "odb/poly_decomp.h"

namespace odb {
namespace {

// Even-odd point-in-polygon test on the polygon interior (using the sample
// point's center).  Used to validate that the rectangle decomposition covers
// the same area as the source polygon.
bool pointInPolygon(const std::vector<Point>& poly, double x, double y)
{
  bool inside = false;
  const int n = static_cast<int>(poly.size());
  for (int i = 0, j = n - 1; i < n; j = i++) {
    const double xi = poly[i].x(), yi = poly[i].y();
    const double xj = poly[j].x(), yj = poly[j].y();
    if (((yi > y) != (yj > y))
        && (x < (xj - xi) * (y - yi) / (yj - yi) + xi)) {
      inside = !inside;
    }
  }
  return inside;
}

bool pointInRects(const std::vector<Rect>& rects, int x, int y)
{
  for (const Rect& r : rects) {
    if (x >= r.xMin() && x < r.xMax() && y >= r.yMin() && y < r.yMax()) {
      return true;
    }
  }
  return false;
}

// A simple rectangle decomposes to itself.
TEST(geom, decompose_rectangle)
{
  const std::vector<Point> rect = {{0, 0}, {100, 0}, {100, 50}, {0, 50}};
  std::vector<Rect> rects;
  decompose_polygon(rect, rects);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0], Rect(0, 0, 100, 50));
}

// A rectilinear (Manhattan) L-shape still uses the exact polygon_90 path and
// decomposes into two rectangles, unchanged by the non-rectilinear support.
TEST(geom, decompose_rectilinear_l_shape)
{
  const std::vector<Point> l_shape
      = {{0, 0}, {20, 0}, {20, 10}, {10, 10}, {10, 20}, {0, 20}};
  std::vector<Rect> rects;
  decompose_polygon(l_shape, rects);

  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0], Rect(0, 0, 20, 10));
  EXPECT_EQ(rects[1], Rect(0, 10, 10, 20));
}

// Regression for OpenROAD #10256: a 45-degree (octagonal) polygon, like the
// pad geometry in sky130/gscl45 IO cells, must be decomposed into rectangles
// that fully cover the polygon.  The previous polygon_90-only implementation
// corrupted the shape, leaving real metal uncovered (which manifested as
// missing obstructions/pins -> shorts) and filling empty corners.
TEST(geom, decompose_octagon_covers_polygon)
{
  // gscl45nm_polygon.lef metal5 PAD octagon, scaled to integer DBU.
  const std::vector<Point> octagon = {{1450, 542},
                                      {542, 1450},
                                      {-542, 1450},
                                      {-1450, 542},
                                      {-1450, -542},
                                      {-542, -1450},
                                      {542, -1450},
                                      {1450, -542}};

  std::vector<Rect> rects;
  decompose_polygon(octagon, rects);

  // The decomposition must produce geometry (the bug dropped/corrupted it).
  ASSERT_FALSE(rects.empty());

  // Every point inside the octagon must be covered by some rectangle
  // (no under-coverage), and no point outside it should be covered
  // (no over-coverage).
  int undercovered = 0;
  int overcovered = 0;
  for (int y = -1450; y < 1450; y += 11) {
    for (int x = -1450; x < 1450; x += 11) {
      const bool in_poly = pointInPolygon(octagon, x + 0.5, y + 0.5);
      const bool in_rects = pointInRects(rects, x, y);
      if (in_poly && !in_rects) {
        ++undercovered;
      }
      if (!in_poly && in_rects) {
        ++overcovered;
      }
    }
  }

  EXPECT_EQ(undercovered, 0);
  EXPECT_EQ(overcovered, 0);
}

TEST(geom, test_oct)
{
  Oct oct;
  oct.init(Point(0, 0), Point(400, 400), 40);
  EXPECT_EQ(oct.getCenterHigh(), Point(400, 400));
  EXPECT_EQ(oct.getCenterLow(), Point(0, 0));
  EXPECT_EQ(oct.getWidth(), 40);
  EXPECT_EQ(oct.xMin(), -20);
  EXPECT_EQ(oct.xMax(), 420);
  EXPECT_EQ(oct.yMin(), -20);
  EXPECT_EQ(oct.yMax(), 420);
  EXPECT_EQ(oct.dx(), 440);
  EXPECT_EQ(oct.dy(), 440);

  EXPECT_EQ(oct.getDir(), Oct::OCT_DIR::RIGHT);
  oct.init(Point(0, 0), Point(-400, 400), 40);
  EXPECT_EQ(oct.getDir(), Oct::OCT_DIR::LEFT);
  oct.init(Point(0, 0), Point(-400, -400), 40);
  EXPECT_EQ(oct.getDir(), Oct::OCT_DIR::RIGHT);
  oct.init(Point(0, 0), Point(400, -400), 40);
  EXPECT_EQ(oct.getDir(), Oct::OCT_DIR::LEFT);
}
TEST(geom, test_sbox_shapes)
{
  Oct oct(Point(0, 0), Point(400, 400), 40);
  EXPECT_EQ(oct.xMin(), -20);
  EXPECT_EQ(oct.xMax(), 420);
  EXPECT_EQ(oct.yMin(), -20);
  EXPECT_EQ(oct.yMax(), 420);
  EXPECT_EQ(oct.dx(), 440);
  EXPECT_EQ(oct.dy(), 440);
  // OCT POINTS
  std::vector<Point> points = oct.getPoints();
  EXPECT_EQ(points.size(), 9);
  EXPECT_EQ(points[0], Point(-9, -20));
  EXPECT_EQ(points[1], Point(9, -20));
  EXPECT_EQ(points[2], Point(420, 391));
  EXPECT_EQ(points[3], Point(420, 409));
  EXPECT_EQ(points[4], Point(409, 420));
  EXPECT_EQ(points[5], Point(391, 420));
  EXPECT_EQ(points[6], Point(-20, 9));
  EXPECT_EQ(points[7], Point(-20, -9));
  EXPECT_EQ(points[8], Point(-9, -20));

  // RECT
  Rect rect(Point(0, 0), Point(400, 400));
  EXPECT_EQ(rect.xMin(), 0);
  EXPECT_EQ(rect.xMax(), 400);
  EXPECT_EQ(rect.yMin(), 0);
  EXPECT_EQ(rect.yMax(), 400);
  EXPECT_EQ(rect.dx(), 400);
  EXPECT_EQ(rect.dy(), 400);
  // RECT POINTS
  points = rect.getPoints();
  EXPECT_EQ(points.size(), 5);
  EXPECT_EQ(points[0], Point(0, 0));
  EXPECT_EQ(points[1], Point(400, 0));
  EXPECT_EQ(points[2], Point(400, 400));
  EXPECT_EQ(points[3], Point(0, 400));
  EXPECT_EQ(points[4], Point(0, 0));
}
TEST(geom, test_rect_merge)
{
  Rect rect(Point(0, 0), Point(100, 50));
  Oct oct(Point(100, 50), Point(200, 200), 80);
  rect.merge(oct);
  EXPECT_EQ(rect.xMin(), 0);
  EXPECT_EQ(rect.xMax(), 240);
  EXPECT_EQ(rect.yMin(), 0);
  EXPECT_EQ(rect.yMax(), 240);
  EXPECT_EQ(rect.dx(), 240);
  EXPECT_EQ(rect.dy(), 240);
}
TEST(geom, test_polygon_is_rect)
{
  // the points of a rect can start at any corner
  const std::vector<Point> corners
      = {Point(0, 0), Point(100, 0), Point(100, 50), Point(0, 50)};
  for (std::size_t offset = 0; offset < corners.size(); offset++) {
    std::vector<Point> rotated;
    rotated.reserve(corners.size());
    for (std::size_t i = 0; i < corners.size(); i++) {
      rotated.push_back(corners[(i + offset) % corners.size()]);
    }
    const Polygon polygon(rotated);
    EXPECT_TRUE(polygon.isRect()) << "starting at corner " << offset;
    EXPECT_EQ(polygon.getEnclosingRect(), Rect(0, 0, 100, 50));
  }

  EXPECT_TRUE(Polygon(Rect(0, 0, 100, 50)).isRect());

  // shapes that fill their bounding box, but are not rects
  const Polygon l_shape({Point(0, 0),
                         Point(100, 0),
                         Point(100, 50),
                         Point(50, 50),
                         Point(50, 100),
                         Point(0, 100)});
  EXPECT_FALSE(l_shape.isRect());

  // same point count as a rect, but not a rect
  const Polygon diamond(
      {Point(0, 50), Point(50, 0), Point(100, 50), Point(50, 100)});
  EXPECT_FALSE(diamond.isRect());

  const Polygon trapezoid(
      {Point(0, 0), Point(100, 0), Point(75, 50), Point(25, 50)});
  EXPECT_FALSE(trapezoid.isRect());

  EXPECT_FALSE(Polygon().isRect());

  // extra points along an edge still describe a rect
  const Polygon split_edges({Point(0, 0),
                             Point(50, 0),
                             Point(100, 0),
                             Point(100, 50),
                             Point(50, 50),
                             Point(0, 50)});
  EXPECT_TRUE(split_edges.isRect());
  EXPECT_EQ(split_edges.getEnclosingRect(), Rect(0, 0, 100, 50));

  // a shape with no area is still handled as a rect, an unset die area
  // relies on this
  EXPECT_TRUE(Polygon(Rect(0, 0, 0, 0)).isRect());
  EXPECT_TRUE(Polygon(Rect(0, 0, 100, 0)).isRect());

  // too few points to close a shape
  const std::vector<Point> line_points = {Point(0, 0), Point(100, 0)};
  EXPECT_FALSE(Polygon(line_points).isRect());
}
TEST(geom, test_isotropy)
{
  EXPECT_NE(low, high);
  EXPECT_EQ(low.flipped(), high);
  EXPECT_EQ(high.flipped(), low);

  EXPECT_NE(horizontal, vertical);
  EXPECT_NE(proximal, horizontal);
  EXPECT_EQ(horizontal.turn_90(), vertical);
  EXPECT_EQ(vertical.turn_90(), horizontal);
  EXPECT_EQ(horizontal.getDirection(high), east);
  EXPECT_EQ(horizontal.getDirection(low), west);
  EXPECT_EQ(vertical.getDirection(high), north);
  EXPECT_EQ(vertical.getDirection(low), south);

  EXPECT_NE(west, east);
  EXPECT_NE(west, south);
  EXPECT_NE(west, north);
  EXPECT_NE(east, south);
  EXPECT_NE(east, north);
  EXPECT_NE(south, north);

  EXPECT_EQ(north.flipped(), south);
  EXPECT_EQ(north.left(), west);
  EXPECT_EQ(north.right(), east);
  EXPECT_TRUE(north.is_positive());

  EXPECT_EQ(south.flipped(), north);
  EXPECT_EQ(south.left(), east);
  EXPECT_EQ(south.right(), west);
  EXPECT_TRUE(south.is_negative());

  EXPECT_EQ(east.flipped(), west);
  EXPECT_EQ(east.left(), north);
  EXPECT_EQ(east.right(), south);
  EXPECT_TRUE(east.is_positive());

  EXPECT_EQ(west.flipped(), east);
  EXPECT_EQ(west.left(), south);
  EXPECT_EQ(west.right(), north);
  EXPECT_TRUE(west.is_negative());

  EXPECT_EQ(up.flipped(), down);
  EXPECT_EQ(down.flipped(), up);
  EXPECT_TRUE(up.is_positive());
  EXPECT_TRUE(down.is_negative());

  // Make sure they can be used as array indices
  std::vector<int> test(2);
  test[low] = 1;
  test[high] = 2;
  EXPECT_EQ(test[low], 1);
  EXPECT_EQ(test[high], 2);
}

// The helpers in geom_boost.h come in two flavors: an arbitrary angle set
// that can hold the 45 degree edges of an Oct, and a Manhattan only
// polygon_90 set that cannot.  Both are covered below.

// The distinct corners of a polygon, sorted so that two descriptions of the
// same shape compare equal whichever vertex they start from.
std::vector<Point> corners(const Polygon& polygon)
{
  std::vector<Point> points = polygon.getPoints();
  std::sort(points.begin(), points.end());
  points.erase(std::unique(points.begin(), points.end()), points.end());
  return points;
}

// The bounding boxes of a set of polygons, sorted so that checks do not
// depend on the order boost hands the polygons back in.
std::vector<Rect> enclosingRects(const std::vector<Polygon>& polygons)
{
  std::vector<Rect> rects;
  rects.reserve(polygons.size());
  for (const Polygon& polygon : polygons) {
    rects.push_back(polygon.getEnclosingRect());
  }
  std::sort(rects.begin(), rects.end());

  return rects;
}

TEST(geom, test_boost_to_polygon_set)
{
  const Rect rect(0, 0, 100, 50);
  const Oct oct(Point(0, 0), Point(400, 400), 40);
  const Polygon polygon(
      {Point(0, 0), Point(100, 0), Point(100, 50), Point(0, 50)});

  // the same template accepts every odb shape that encloses an area
  EXPECT_EQ(geom::extractPolygons(geom::toPolygonSet(rect)).size(), 1);
  EXPECT_EQ(geom::extractPolygons(geom::toPolygonSet(oct)).size(), 1);
  EXPECT_EQ(geom::extractPolygons(geom::toPolygonSet(polygon)).size(), 1);

  // a rect survives the round trip as a rect
  const std::vector<Polygon> from_rect
      = geom::extractPolygons(geom::toPolygonSet(rect));
  ASSERT_EQ(from_rect.size(), 1);
  EXPECT_TRUE(from_rect[0].isRect());
  EXPECT_EQ(from_rect[0].getEnclosingRect(), rect);

  // the collection overload unions touching shapes together
  const std::vector<Polygon> merged = geom::extractPolygons(geom::toPolygonSet(
      std::vector<Rect>{Rect(0, 0, 50, 100), Rect(50, 0, 100, 100)}));
  ASSERT_EQ(merged.size(), 1);
  EXPECT_TRUE(merged[0].isRect());
  EXPECT_EQ(merged[0].getEnclosingRect(), Rect(0, 0, 100, 100));

  // and leaves disjoint shapes apart
  EXPECT_EQ(
      geom::extractPolygons(geom::toPolygonSet(std::vector<Rect>{
                                Rect(0, 0, 10, 10), Rect(90, 90, 100, 100)}))
          .size(),
      2);

  // an empty collection gives an empty set, for any shape type
  EXPECT_TRUE(
      geom::extractPolygons(geom::toPolygonSet(std::vector<Rect>{})).empty());
  EXPECT_TRUE(
      geom::extractPolygons(geom::toPolygonSet(std::vector<Oct>{})).empty());
  EXPECT_TRUE(geom::extractPolygons(geom::toPolygonSet(std::vector<Polygon>{}))
                  .empty());
}

TEST(geom, test_boost_oct_keeps_45_degree_edges)
{
  const Oct oct(Point(0, 0), Point(400, 400), 40);

  const std::vector<Polygon> polys
      = geom::extractPolygons(geom::toPolygonSet(oct));
  ASSERT_EQ(polys.size(), 1);
  EXPECT_FALSE(polys[0].isRect());
  EXPECT_EQ(polys[0].getEnclosingRect(), Rect(-20, -20, 420, 420));

  // all eight corners of the octagon come back, so no 45 degree edge was
  // collapsed on the way through boost
  const Polygon as_polygon(oct);
  EXPECT_EQ(corners(polys[0]).size(), 8);
  EXPECT_EQ(corners(polys[0]), corners(as_polygon));

  // converting the Oct directly matches converting it through a Polygon,
  // which is what Polygon::merge(std::vector<Oct>) does
  EXPECT_EQ(polys, geom::extractPolygons(geom::toPolygonSet(as_polygon)));

  // an Oct can take part in a boolean op without losing its shape.  The
  // upper left edge of this octagon runs (391, 420) -> (-20, 9), so y = x + 29
  // along it.  Clipping everything at or above x = 200 therefore caps the
  // result at y = 229, not at the octagon's yMax of 420 - which is only true
  // if the 45 degree edge survived the trip through boost.
  using boost::polygon::operators::operator-;
  const std::vector<Polygon> clipped = geom::extractPolygons(
      geom::toPolygonSet(oct) - geom::toPolygonSet(Rect(200, -20, 420, 420)));
  ASSERT_EQ(clipped.size(), 1);
  EXPECT_FALSE(clipped[0].isRect());
  EXPECT_EQ(clipped[0].getEnclosingRect(), Rect(-20, -20, 200, 229));
}

TEST(geom, test_boost_to_polygon_90)
{
  const Rect rect(0, 0, 100, 50);

  // a single rect converts to one polygon and back unchanged
  EXPECT_EQ(geom::extractRectangles(geom::toPolygonSet90(rect)),
            std::vector<Rect>{rect});

  // toPolygon90 and toPolygonSet90 describe the same shape
  using boost::polygon::operators::operator+=;
  geom::BoostPolygon90Set from_polygon;
  from_polygon += geom::toPolygon90(rect);
  EXPECT_EQ(geom::extractRectangles(from_polygon),
            geom::extractRectangles(geom::toPolygonSet90(rect)));

  // the 90 set also hands back odb polygons
  const std::vector<Polygon> polys
      = geom::extractPolygons(geom::toPolygonSet90(rect));
  ASSERT_EQ(polys.size(), 1);
  EXPECT_TRUE(polys[0].isRect());
  EXPECT_EQ(polys[0].getEnclosingRect(), rect);

  // the collection overload unions touching shapes together
  EXPECT_EQ(geom::extractRectangles(geom::toPolygonSet90(
                std::vector<Rect>{Rect(0, 0, 50, 100), Rect(50, 0, 100, 100)})),
            std::vector<Rect>{Rect(0, 0, 100, 100)});

  // an empty collection gives an empty set
  EXPECT_TRUE(geom::extractRectangles(geom::toPolygonSet90(std::vector<Rect>{}))
                  .empty());

  // a Manhattan Polygon is accepted and decomposed back to rectangles
  const Polygon l_shape({Point(0, 0),
                         Point(100, 0),
                         Point(100, 50),
                         Point(50, 50),
                         Point(50, 100),
                         Point(0, 100)});
  std::vector<Rect> l_rects
      = geom::extractRectangles(geom::toPolygonSet90(l_shape));
  std::sort(l_rects.begin(), l_rects.end());
  EXPECT_EQ(l_rects,
            (std::vector<Rect>{Rect(0, 0, 100, 50), Rect(0, 50, 50, 100)}));

  // for a Manhattan shape the two flavors agree
  const std::vector<Polygon> arbitrary_angle
      = geom::extractPolygons(geom::toPolygonSet(l_shape));
  const std::vector<Polygon> manhattan
      = geom::extractPolygons(geom::toPolygonSet90(l_shape));
  ASSERT_EQ(arbitrary_angle.size(), 1);
  ASSERT_EQ(manhattan.size(), 1);
  EXPECT_EQ(arbitrary_angle[0].getEnclosingRect(),
            manhattan[0].getEnclosingRect());
}

TEST(geom, test_boost_extract_rectangles)
{
  // cutting a notch out of a shape splits it, the pattern pdn uses to
  // trim straps around obstructions
  using boost::polygon::operators::operator-;
  std::vector<Rect> cut
      = geom::extractRectangles(geom::toPolygonSet90(Rect(0, 0, 300, 100))
                                - geom::toPolygonSet90(Rect(100, 0, 200, 100)));
  std::sort(cut.begin(), cut.end());
  EXPECT_EQ(cut,
            (std::vector<Rect>{Rect(0, 0, 100, 100), Rect(200, 0, 300, 100)}));

  // the slicing orientation decides which way a shape is carved up
  const geom::BoostPolygon90Set plus = geom::toPolygonSet90(
      std::vector<Rect>{Rect(0, 40, 120, 80), Rect(40, 0, 80, 120)});

  std::vector<Rect> horizontal_slices
      = geom::extractRectangles(plus, boost::polygon::HORIZONTAL);
  std::sort(horizontal_slices.begin(), horizontal_slices.end());
  EXPECT_EQ(
      horizontal_slices,
      (std::vector<Rect>{
          Rect(0, 40, 120, 80), Rect(40, 0, 80, 40), Rect(40, 80, 80, 120)}));

  std::vector<Rect> vertical_slices
      = geom::extractRectangles(plus, boost::polygon::VERTICAL);
  std::sort(vertical_slices.begin(), vertical_slices.end());
  EXPECT_EQ(
      vertical_slices,
      (std::vector<Rect>{
          Rect(0, 40, 40, 80), Rect(40, 0, 80, 120), Rect(80, 40, 120, 80)}));

  EXPECT_NE(horizontal_slices, vertical_slices);
}

TEST(geom, test_boost_to_rect)
{
  EXPECT_EQ(geom::toRect(geom::BoostRectangle(5, 6, 7, 8)), Rect(5, 6, 7, 8));
  EXPECT_EQ(geom::toRect(geom::BoostRectangle(-10, -20, 0, 0)),
            Rect(-10, -20, 0, 0));

  // a zero area rect is preserved rather than normalized away
  EXPECT_EQ(geom::toRect(geom::BoostRectangle(5, 5, 5, 5)), Rect(5, 5, 5, 5));
}

TEST(geom, test_boost_enclosing_rect)
{
  // works on either flavor of set
  EXPECT_EQ(geom::getEnclosingRect(geom::toPolygonSet90(
                std::vector<Rect>{Rect(0, 0, 10, 10), Rect(90, 90, 100, 100)})),
            Rect(0, 0, 100, 100));
  EXPECT_EQ(geom::getEnclosingRect(
                geom::toPolygonSet(Oct(Point(0, 0), Point(400, 400), 40))),
            Rect(-20, -20, 420, 420));

  // and on a bare polygon
  EXPECT_EQ(geom::getEnclosingRect(geom::toPolygon90(Rect(0, 0, 100, 50))),
            Rect(0, 0, 100, 50));

  // an empty set has no extents, and leaves the Rect default constructed
  EXPECT_EQ(geom::getEnclosingRect(geom::BoostPolygon90Set()),
            Rect(0, 0, 0, 0));
  EXPECT_EQ(geom::getEnclosingRect(geom::BoostPolygonSet()), Rect(0, 0, 0, 0));
}

// mergePolygons is the one entry point that takes any of the three odb area
// shapes, so each of Rect, Oct and Polygon is exercised through it below.

TEST(geom, test_boost_merge_polygons_rects)
{
  // abutting rects come back as a single shape
  const std::vector<Polygon> abutting = geom::mergePolygons(
      std::vector<Rect>{Rect(0, 0, 50, 100), Rect(50, 0, 100, 100)});
  ASSERT_EQ(abutting.size(), 1);
  EXPECT_TRUE(abutting[0].isRect());
  EXPECT_EQ(abutting[0].getEnclosingRect(), Rect(0, 0, 100, 100));

  // as do overlapping ones, with the overlap counted once
  const std::vector<Polygon> overlapping = geom::mergePolygons(
      std::vector<Rect>{Rect(0, 0, 60, 100), Rect(40, 0, 100, 100)});
  ASSERT_EQ(overlapping.size(), 1);
  EXPECT_TRUE(overlapping[0].isRect());
  EXPECT_EQ(overlapping[0].getEnclosingRect(), Rect(0, 0, 100, 100));
  // the seams where the inputs met survive as collinear vertices, so a
  // merged rect carries more than four points
  EXPECT_EQ(corners(overlapping[0]),
            (std::vector<Point>{Point(0, 0),
                                Point(0, 100),
                                Point(40, 0),
                                Point(40, 100),
                                Point(60, 0),
                                Point(60, 100),
                                Point(100, 0),
                                Point(100, 100)}));

  // a union that is not itself a rect keeps its outline rather than
  // collapsing to the bounding box
  const std::vector<Polygon> l_shape = geom::mergePolygons(
      std::vector<Rect>{Rect(0, 0, 100, 50), Rect(0, 50, 50, 100)});
  ASSERT_EQ(l_shape.size(), 1);
  EXPECT_FALSE(l_shape[0].isRect());
  EXPECT_EQ(l_shape[0].getEnclosingRect(), Rect(0, 0, 100, 100));
  EXPECT_EQ(corners(l_shape[0]),
            (std::vector<Point>{Point(0, 0),
                                Point(0, 50),
                                Point(0, 100),
                                Point(50, 50),
                                Point(50, 100),
                                Point(100, 0),
                                Point(100, 50)}));

  // disjoint rects stay apart
  EXPECT_EQ(enclosingRects(geom::mergePolygons(
                std::vector<Rect>{Rect(90, 90, 100, 100), Rect(0, 0, 10, 10)})),
            (std::vector<Rect>{Rect(0, 0, 10, 10), Rect(90, 90, 100, 100)}));

  // a lone rect survives the round trip as a rect
  const std::vector<Polygon> single
      = geom::mergePolygons(std::vector<Rect>{Rect(0, 0, 100, 50)});
  ASSERT_EQ(single.size(), 1);
  EXPECT_TRUE(single[0].isRect());
  EXPECT_EQ(single[0].getEnclosingRect(), Rect(0, 0, 100, 50));

  // and a repeated rect does not grow the result
  EXPECT_EQ(geom::mergePolygons(
                std::vector<Rect>{Rect(0, 0, 100, 50), Rect(0, 0, 100, 50)}),
            single);

  // an empty collection merges to nothing
  EXPECT_TRUE(geom::mergePolygons(std::vector<Rect>{}).empty());
}

TEST(geom, test_boost_merge_polygons_octs)
{
  const Oct oct(Point(0, 0), Point(400, 400), 40);

  // a lone oct passes through with all eight corners intact, so merging
  // never quietly squares off a 45 degree edge
  const std::vector<Polygon> single
      = geom::mergePolygons(std::vector<Oct>{oct});
  ASSERT_EQ(single.size(), 1);
  EXPECT_FALSE(single[0].isRect());
  EXPECT_EQ(single[0].getEnclosingRect(), Rect(-20, -20, 420, 420));
  EXPECT_EQ(corners(single[0]).size(), 8);
  EXPECT_EQ(corners(single[0]), corners(Polygon(oct)));

  // octs overlapping along their diagonal union into one shape that still
  // reaches past the corners of either
  const std::vector<Polygon> merged = geom::mergePolygons(
      std::vector<Oct>{oct, Oct(Point(200, 200), Point(600, 600), 40)});
  ASSERT_EQ(merged.size(), 1);
  EXPECT_FALSE(merged[0].isRect());
  EXPECT_EQ(merged[0].getEnclosingRect(), Rect(-20, -20, 620, 620));

  // disjoint octs stay apart, each keeping its own eight corners
  const std::vector<Polygon> apart = geom::mergePolygons(
      std::vector<Oct>{oct, Oct(Point(1000, 1000), Point(1400, 1400), 40)});
  ASSERT_EQ(apart.size(), 2);
  EXPECT_EQ(enclosingRects(apart),
            (std::vector<Rect>{Rect(-20, -20, 420, 420),
                               Rect(980, 980, 1420, 1420)}));
  for (const Polygon& polygon : apart) {
    EXPECT_FALSE(polygon.isRect());
    EXPECT_EQ(corners(polygon).size(), 8);
  }

  EXPECT_TRUE(geom::mergePolygons(std::vector<Oct>{}).empty());
}

TEST(geom, test_boost_merge_polygons_polygons)
{
  const Polygon l_shape({Point(0, 0),
                         Point(100, 0),
                         Point(100, 50),
                         Point(50, 50),
                         Point(50, 100),
                         Point(0, 100)});

  // an L and the rect that fills its notch merge back into a full square
  const std::vector<Polygon> filled = geom::mergePolygons(
      std::vector<Polygon>{l_shape, Polygon(Rect(50, 50, 100, 100))});
  ASSERT_EQ(filled.size(), 1);
  EXPECT_TRUE(filled[0].isRect());
  EXPECT_EQ(filled[0].getEnclosingRect(), Rect(0, 0, 100, 100));

  // 45 degree edges survive a merge of arbitrary angle polygons
  const Polygon diamond(
      {Point(0, 50), Point(50, 0), Point(100, 50), Point(50, 100)});
  const std::vector<Polygon> diamonds
      = geom::mergePolygons(std::vector<Polygon>{diamond,
                                                 Polygon({Point(50, 50),
                                                          Point(100, 0),
                                                          Point(150, 50),
                                                          Point(100, 100)})});
  ASSERT_EQ(diamonds.size(), 1);
  EXPECT_FALSE(diamonds[0].isRect());
  EXPECT_EQ(diamonds[0].getEnclosingRect(), Rect(0, 0, 150, 100));

  // a repeated polygon comes back as the one shape it describes
  const std::vector<Polygon> duplicate
      = geom::mergePolygons(std::vector<Polygon>{diamond, diamond});
  ASSERT_EQ(duplicate.size(), 1);
  EXPECT_EQ(corners(duplicate[0]), corners(diamond));

  // disjoint polygons stay apart
  EXPECT_EQ(enclosingRects(geom::mergePolygons(std::vector<Polygon>{
                Polygon(Rect(90, 90, 100, 100)), l_shape})),
            (std::vector<Rect>{Rect(0, 0, 100, 100), Rect(90, 90, 100, 100)}));

  // the same shapes merge the same way whether they arrive as Rects or as
  // Polygons
  const std::vector<Rect> rects{Rect(0, 0, 100, 50), Rect(0, 50, 50, 100)};
  EXPECT_EQ(geom::mergePolygons(rects),
            geom::mergePolygons(
                std::vector<Polygon>{Polygon(rects[0]), Polygon(rects[1])}));

  EXPECT_TRUE(geom::mergePolygons(std::vector<Polygon>{}).empty());
}

}  // namespace
}  // namespace odb
