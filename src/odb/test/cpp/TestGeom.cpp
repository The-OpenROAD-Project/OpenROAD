#include <vector>

#include "gtest/gtest.h"
#include "odb/geom.h"
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

}  // namespace
}  // namespace odb
