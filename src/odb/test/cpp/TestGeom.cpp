#include <algorithm>
#include <cstddef>
#include <vector>

#include "gtest/gtest.h"
#include "odb/geom.h"
#include "odb/isotropy.h"

namespace odb {
namespace {

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

  // rects recovered from set operations do not start at a predictable corner
  const std::vector<Polygon> cut
      = Polygon(Rect(0, 0, 100, 100)).difference(Rect(50, 0, 100, 100));
  ASSERT_EQ(cut.size(), 1);
  EXPECT_TRUE(cut[0].isRect());
  EXPECT_EQ(cut[0].getEnclosingRect(), Rect(0, 0, 50, 100));

  const std::vector<Polygon> merged = Polygon::merge(
      std::vector<Rect>{Rect(0, 0, 50, 100), Rect(50, 0, 100, 100)});
  ASSERT_EQ(merged.size(), 1);
  EXPECT_TRUE(merged[0].isRect());
  EXPECT_EQ(merged[0].getEnclosingRect(), Rect(0, 0, 100, 100));
}
TEST(geom, test_polygon_difference)
{
  const Polygon rect(Rect(0, 0, 100, 100));

  // subtracting a collection removes the union of its shapes
  std::vector<Polygon> slices = rect.difference(
      std::vector<Rect>{Rect(-10, 20, 110, 40), Rect(-10, 60, 110, 80)});
  ASSERT_EQ(slices.size(), 3);
  std::sort(
      slices.begin(), slices.end(), [](const Polygon& lhs, const Polygon& rhs) {
        return lhs.getEnclosingRect().yMin() < rhs.getEnclosingRect().yMin();
      });
  EXPECT_EQ(slices[0].getEnclosingRect(), Rect(0, 0, 100, 20));
  EXPECT_EQ(slices[1].getEnclosingRect(), Rect(0, 40, 100, 60));
  EXPECT_EQ(slices[2].getEnclosingRect(), Rect(0, 80, 100, 100));

  // overlapping shapes in the collection are handled as one region
  const std::vector<Polygon> overlapping = rect.difference(std::vector<Polygon>{
      Polygon(Rect(-10, 20, 110, 60)), Polygon(Rect(-10, 40, 110, 80))});
  ASSERT_EQ(overlapping.size(), 2);

  // subtracting everything leaves nothing, subtracting nothing changes nothing
  EXPECT_TRUE(rect.difference(std::vector<Rect>{Rect(0, 0, 100, 100)}).empty());
  const std::vector<Polygon> untouched = rect.difference(std::vector<Rect>{});
  ASSERT_EQ(untouched.size(), 1);
  EXPECT_EQ(untouched[0].getEnclosingRect(), Rect(0, 0, 100, 100));

  // a single shape gives the same result as a collection of one
  EXPECT_EQ(rect.difference(Rect(50, 0, 100, 100)),
            rect.difference(std::vector<Rect>{Rect(50, 0, 100, 100)}));
}
TEST(geom, test_polygon_intersection)
{
  const Polygon rect(Rect(0, 0, 100, 100));

  // overlapping shapes produce the shared area
  const std::vector<Polygon> overlap
      = rect.intersection(Rect(50, 50, 150, 150));
  ASSERT_EQ(overlap.size(), 1);
  EXPECT_TRUE(overlap[0].isRect());
  EXPECT_EQ(overlap[0].getEnclosingRect(), Rect(50, 50, 100, 100));

  // abutting and disjoint shapes do not
  EXPECT_TRUE(rect.intersection(Rect(100, 0, 200, 100)).empty());
  EXPECT_TRUE(rect.intersection(Rect(200, 200, 300, 300)).empty());

  // a single intersection can yield more than one polygon
  const Polygon u_shape({Point(0, 0),
                         Point(100, 0),
                         Point(100, 100),
                         Point(70, 100),
                         Point(70, 30),
                         Point(30, 30),
                         Point(30, 100),
                         Point(0, 100)});
  std::vector<Polygon> legs = u_shape.intersection(Rect(0, 50, 100, 100));
  ASSERT_EQ(legs.size(), 2);
  std::sort(
      legs.begin(), legs.end(), [](const Polygon& lhs, const Polygon& rhs) {
        return lhs.getEnclosingRect().xMin() < rhs.getEnclosingRect().xMin();
      });
  EXPECT_EQ(legs[0].getEnclosingRect(), Rect(0, 50, 30, 100));
  EXPECT_EQ(legs[1].getEnclosingRect(), Rect(70, 50, 100, 100));

  // intersecting against a collection uses the union of its shapes
  std::vector<Polygon> pieces = rect.intersection(
      std::vector<Rect>{Rect(-10, 10, 110, 30), Rect(-10, 70, 110, 90)});
  ASSERT_EQ(pieces.size(), 2);
  std::sort(
      pieces.begin(), pieces.end(), [](const Polygon& lhs, const Polygon& rhs) {
        return lhs.getEnclosingRect().yMin() < rhs.getEnclosingRect().yMin();
      });
  EXPECT_EQ(pieces[0].getEnclosingRect(), Rect(0, 10, 100, 30));
  EXPECT_EQ(pieces[1].getEnclosingRect(), Rect(0, 70, 100, 90));

  // overlapping shapes in the collection are handled as one region
  const std::vector<Polygon> unioned = rect.intersection(std::vector<Polygon>{
      Polygon(Rect(-10, 10, 110, 50)), Polygon(Rect(-10, 30, 110, 90))});
  ASSERT_EQ(unioned.size(), 1);
  EXPECT_EQ(unioned[0].getEnclosingRect(), Rect(0, 10, 100, 90));

  // an empty collection has nothing to intersect with
  EXPECT_TRUE(rect.intersection(std::vector<Rect>{}).empty());

  // a single shape gives the same result as a collection of one
  EXPECT_EQ(rect.intersection(Rect(50, 50, 150, 150)),
            rect.intersection(std::vector<Rect>{Rect(50, 50, 150, 150)}));

  // 45 degree edges are kept
  const Polygon diamond(
      {Point(0, 50), Point(50, 0), Point(100, 50), Point(50, 100)});
  const std::vector<Polygon> half = diamond.intersection(Rect(0, 0, 50, 100));
  ASSERT_EQ(half.size(), 1);
  EXPECT_FALSE(half[0].isRect());
  EXPECT_EQ(half[0].getEnclosingRect(), Rect(0, 0, 50, 100));
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
