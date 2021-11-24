#define BOOST_TEST_MODULE TestGeom
#include <boost/test/included/unit_test.hpp>
#include "db.h"
#include "helper.cpp"
#include <iostream>
using namespace odb;

BOOST_AUTO_TEST_SUITE( test_suite )

BOOST_AUTO_TEST_CASE( test_oct )
{
    Oct* oct = new Oct;
    oct->init(Point(0,0),Point(400,400),40);
    BOOST_TEST((oct->getCenterHigh() == Point(400,400)));
    BOOST_TEST((oct->getCenterLow() == Point(0,0)));
    BOOST_TEST(oct->getWidth()==40);
    BOOST_TEST(oct->xMin()==-20);
    BOOST_TEST(oct->xMax()==420);
    BOOST_TEST(oct->yMin()==-20);
    BOOST_TEST(oct->yMax()==420);
    BOOST_TEST(oct->dx()==440);
    BOOST_TEST(oct->dy()==440);

    BOOST_TEST(oct->getDir() == Oct::OCT_DIR::RIGHT);
    oct->init(Point(0,0),Point(-400,400),40);
    BOOST_TEST(oct->getDir() == Oct::OCT_DIR::LEFT);
    oct->init(Point(0,0),Point(-400,-400),40);
    BOOST_TEST(oct->getDir() == Oct::OCT_DIR::RIGHT);
    oct->init(Point(0,0),Point(400,-400),40);
    BOOST_TEST(oct->getDir() == Oct::OCT_DIR::LEFT);
}
BOOST_AUTO_TEST_CASE( test_geom_shape )
{
    Oct oct(Point(0,0),Point(400,400),40);
    GeomShape* shape = &oct;
    BOOST_TEST(shape->xMin()==-20);
    BOOST_TEST(shape->xMax()==420);
    BOOST_TEST(shape->yMin()==-20);
    BOOST_TEST(shape->yMax()==420);
    BOOST_TEST(shape->dx()==440);
    BOOST_TEST(shape->dy()==440);
    //OCT POINTS
    std::vector<Point> points = shape->getPoints();
    BOOST_TEST(points.size()==9);
    BOOST_TEST((points[0] == Point(-9,-20)));
    BOOST_TEST((points[1] == Point(9,-20)));
    BOOST_TEST((points[2] == Point(420,391)));
    BOOST_TEST((points[3] == Point(420,409)));
    BOOST_TEST((points[4] == Point(409,420)));
    BOOST_TEST((points[5] == Point(391,420)));
    BOOST_TEST((points[6] == Point(-20,9)));
    BOOST_TEST((points[7] == Point(-20,-9)));
    BOOST_TEST((points[8] == Point(-9,-20)));

    //RECT
    Rect rect(Point(0,0),Point(400,400));
    shape = &rect;
    BOOST_TEST(shape->xMin()==0);
    BOOST_TEST(shape->xMax()==400);
    BOOST_TEST(shape->yMin()==0);
    BOOST_TEST(shape->yMax()==400);
    BOOST_TEST(shape->dx()==400);
    BOOST_TEST(shape->dy()==400);
    //RECT POINTS
    points = shape->getPoints();
    BOOST_TEST(points.size()==5);
    BOOST_TEST((points[0] == Point(0,0)));
    BOOST_TEST((points[1] == Point(400,0)));
    BOOST_TEST((points[2] == Point(400,400)));
    BOOST_TEST((points[3] == Point(0,400)));
    BOOST_TEST((points[4] == Point(0,0)));

}
BOOST_AUTO_TEST_CASE( test_rect_merge )
{
    Rect rect(Point(0,0),Point(100,50));
    Oct oct(Point(100,50),Point(200,200),80);
    rect.merge((GeomShape*)&oct);
    BOOST_TEST(rect.xMin()==0);
    BOOST_TEST(rect.xMax()==240);
    BOOST_TEST(rect.yMin()==0);
    BOOST_TEST(rect.yMax()==240);
    BOOST_TEST(rect.dx()==240);
    BOOST_TEST(rect.dy()==240);
}
BOOST_AUTO_TEST_SUITE_END()
