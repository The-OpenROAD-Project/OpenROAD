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
