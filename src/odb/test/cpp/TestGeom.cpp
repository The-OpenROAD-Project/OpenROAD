#define BOOST_TEST_MODULE TestGeom
#include <boost/test/included/unit_test.hpp>
#include <iostream>

#include "helper.h"
#include "odb/db.h"

namespace odb {
namespace {

BOOST_AUTO_TEST_SUITE(test_suite)

BOOST_AUTO_TEST_CASE(test_oct)
{
  Oct oct;
  oct.init(Point(0, 0), Point(400, 400), 40);
  BOOST_TEST((oct.getCenterHigh() == Point(400, 400)));
  BOOST_TEST((oct.getCenterLow() == Point(0, 0)));
  BOOST_TEST(oct.getWidth() == 40);
  BOOST_TEST(oct.xMin() == -20);
  BOOST_TEST(oct.xMax() == 420);
  BOOST_TEST(oct.yMin() == -20);
  BOOST_TEST(oct.yMax() == 420);
  BOOST_TEST(oct.dx() == 440);
  BOOST_TEST(oct.dy() == 440);

  BOOST_TEST(oct.getDir() == Oct::OCT_DIR::RIGHT);
  oct.init(Point(0, 0), Point(-400, 400), 40);
  BOOST_TEST(oct.getDir() == Oct::OCT_DIR::LEFT);
  oct.init(Point(0, 0), Point(-400, -400), 40);
  BOOST_TEST(oct.getDir() == Oct::OCT_DIR::RIGHT);
  oct.init(Point(0, 0), Point(400, -400), 40);
  BOOST_TEST(oct.getDir() == Oct::OCT_DIR::LEFT);
}
BOOST_AUTO_TEST_CASE(test_sbox_shapes)
{
  Oct oct(Point(0, 0), Point(400, 400), 40);
  BOOST_TEST(oct.xMin() == -20);
  BOOST_TEST(oct.xMax() == 420);
  BOOST_TEST(oct.yMin() == -20);
  BOOST_TEST(oct.yMax() == 420);
  BOOST_TEST(oct.dx() == 440);
  BOOST_TEST(oct.dy() == 440);
  // OCT POINTS
  std::vector<Point> points = oct.getPoints();
  BOOST_TEST(points.size() == 9);
  BOOST_TEST((points[0] == Point(-9, -20)));
  BOOST_TEST((points[1] == Point(9, -20)));
  BOOST_TEST((points[2] == Point(420, 391)));
  BOOST_TEST((points[3] == Point(420, 409)));
  BOOST_TEST((points[4] == Point(409, 420)));
  BOOST_TEST((points[5] == Point(391, 420)));
  BOOST_TEST((points[6] == Point(-20, 9)));
  BOOST_TEST((points[7] == Point(-20, -9)));
  BOOST_TEST((points[8] == Point(-9, -20)));

  // RECT
  Rect rect(Point(0, 0), Point(400, 400));
  BOOST_TEST(rect.xMin() == 0);
  BOOST_TEST(rect.xMax() == 400);
  BOOST_TEST(rect.yMin() == 0);
  BOOST_TEST(rect.yMax() == 400);
  BOOST_TEST(rect.dx() == 400);
  BOOST_TEST(rect.dy() == 400);
  // RECT POINTS
  points = rect.getPoints();
  BOOST_TEST(points.size() == 5);
  BOOST_TEST((points[0] == Point(0, 0)));
  BOOST_TEST((points[1] == Point(400, 0)));
  BOOST_TEST((points[2] == Point(400, 400)));
  BOOST_TEST((points[3] == Point(0, 400)));
  BOOST_TEST((points[4] == Point(0, 0)));
}
BOOST_AUTO_TEST_CASE(test_rect_merge)
{
  Rect rect(Point(0, 0), Point(100, 50));
  Oct oct(Point(100, 50), Point(200, 200), 80);
  rect.merge(oct);
  BOOST_TEST(rect.xMin() == 0);
  BOOST_TEST(rect.xMax() == 240);
  BOOST_TEST(rect.yMin() == 0);
  BOOST_TEST(rect.yMax() == 240);
  BOOST_TEST(rect.dx() == 240);
  BOOST_TEST(rect.dy() == 240);
}
BOOST_AUTO_TEST_CASE(test_isotropy)
{
  BOOST_CHECK_NE(low, high);
  BOOST_CHECK_EQUAL(low.flipped(), high);
  BOOST_CHECK_EQUAL(high.flipped(), low);

  BOOST_CHECK_NE(horizontal, vertical);
  BOOST_CHECK_NE(proximal, horizontal);
  BOOST_CHECK_EQUAL(horizontal.turn_90(), vertical);
  BOOST_CHECK_EQUAL(vertical.turn_90(), horizontal);
  BOOST_CHECK_EQUAL(horizontal.getDirection(high), east);
  BOOST_CHECK_EQUAL(horizontal.getDirection(low), west);
  BOOST_CHECK_EQUAL(vertical.getDirection(high), north);
  BOOST_CHECK_EQUAL(vertical.getDirection(low), south);

  BOOST_CHECK_NE(west, east);
  BOOST_CHECK_NE(west, south);
  BOOST_CHECK_NE(west, north);
  BOOST_CHECK_NE(east, south);
  BOOST_CHECK_NE(east, north);
  BOOST_CHECK_NE(south, north);

  BOOST_CHECK_EQUAL(north.flipped(), south);
  BOOST_CHECK_EQUAL(north.left(), west);
  BOOST_CHECK_EQUAL(north.right(), east);
  BOOST_TEST(north.is_positive());

  BOOST_CHECK_EQUAL(south.flipped(), north);
  BOOST_CHECK_EQUAL(south.left(), east);
  BOOST_CHECK_EQUAL(south.right(), west);
  BOOST_TEST(south.is_negative());

  BOOST_CHECK_EQUAL(east.flipped(), west);
  BOOST_CHECK_EQUAL(east.left(), north);
  BOOST_CHECK_EQUAL(east.right(), south);
  BOOST_TEST(east.is_positive());

  BOOST_CHECK_EQUAL(west.flipped(), east);
  BOOST_CHECK_EQUAL(west.left(), south);
  BOOST_CHECK_EQUAL(west.right(), north);
  BOOST_TEST(west.is_negative());

  BOOST_CHECK_EQUAL(up.flipped(), down);
  BOOST_CHECK_EQUAL(down.flipped(), up);
  BOOST_TEST(up.is_positive());
  BOOST_TEST(down.is_negative());

  // Make sure they can be used as array indices
  std::vector<int> test(2);
  test[low] = 1;
  test[high] = 2;
  BOOST_CHECK_EQUAL(test[low], 1);
  BOOST_CHECK_EQUAL(test[high], 2);
}
BOOST_AUTO_TEST_SUITE_END()

}  // namespace
}  // namespace odb
