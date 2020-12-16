#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include "db.h"
#include <iostream>
using namespace odb;

BOOST_AUTO_TEST_SUITE( test_suite )

BOOST_AUTO_TEST_CASE( test_oct )
{
    Oct* oct = new Oct;
    oct->init(Point(0,0),Point(400,400),40);
    BOOST_ASSERT(oct->getCenterHigh()==Point(400,400));
    BOOST_ASSERT(oct->getCenterLow()==Point(0,0));
    BOOST_ASSERT(oct->getWidth()==40);
    BOOST_ASSERT(oct->xMin()==-20);
    BOOST_ASSERT(oct->xMax()==420);
    BOOST_ASSERT(oct->yMin()==-20);
    BOOST_ASSERT(oct->yMax()==420);
    BOOST_ASSERT(oct->dx()==440);
    BOOST_ASSERT(oct->dy()==440);

    BOOST_ASSERT(oct->getDir()==Oct::OCT_DIR::RIGHT);
    oct->init(Point(0,0),Point(-400,400),40);
    BOOST_ASSERT(oct->getDir()==Oct::OCT_DIR::LEFT);
    oct->init(Point(0,0),Point(-400,-400),40);
    BOOST_ASSERT(oct->getDir()==Oct::OCT_DIR::RIGHT);
    oct->init(Point(0,0),Point(400,-400),40);
    BOOST_ASSERT(oct->getDir()==Oct::OCT_DIR::LEFT);
}
BOOST_AUTO_TEST_CASE( test_geom_shape )
{
    Oct oct(Point(0,0),Point(400,400),40);
    GeomShape* shape = &oct;
    BOOST_ASSERT(shape->xMin()==-20);
    BOOST_ASSERT(shape->xMax()==420);
    BOOST_ASSERT(shape->yMin()==-20);
    BOOST_ASSERT(shape->yMax()==420);
    BOOST_ASSERT(shape->dx()==440);
    BOOST_ASSERT(shape->dy()==440);
    //OCT POINTS
    std::vector<Point> points = shape->getPoints();
    BOOST_ASSERT(points.size()==9);
    BOOST_ASSERT(points[0]==Point(-9,-20));
    BOOST_ASSERT(points[1]==Point(9,-20));
    BOOST_ASSERT(points[2]==Point(420,391));
    BOOST_ASSERT(points[3]==Point(420,409));
    BOOST_ASSERT(points[4]==Point(409,420));
    BOOST_ASSERT(points[5]==Point(391,420));
    BOOST_ASSERT(points[6]==Point(-20,9));
    BOOST_ASSERT(points[7]==Point(-20,-9));
    BOOST_ASSERT(points[8]==Point(-9,-20));

    //RECT
    Rect rect(Point(0,0),Point(400,400));
    shape = &rect;
    BOOST_ASSERT(shape->xMin()==0);
    BOOST_ASSERT(shape->xMax()==400);
    BOOST_ASSERT(shape->yMin()==0);
    BOOST_ASSERT(shape->yMax()==400);
    BOOST_ASSERT(shape->dx()==400);
    BOOST_ASSERT(shape->dy()==400);
    //RECT POINTS
    points = shape->getPoints();
    BOOST_ASSERT(points.size()==5);
    BOOST_ASSERT(points[0]==Point(0,0));
    BOOST_ASSERT(points[1]==Point(400,0));
    BOOST_ASSERT(points[2]==Point(400,400));
    BOOST_ASSERT(points[3]==Point(0,400));
    BOOST_ASSERT(points[4]==Point(0,0));

}
BOOST_AUTO_TEST_CASE( test_rect_merge )
{
    Rect rect(Point(0,0),Point(100,50));
    Oct oct(Point(100,50),Point(200,200),80);
    rect.merge((GeomShape*)&oct);
    BOOST_ASSERT(rect.xMin()==0);
    BOOST_ASSERT(rect.xMax()==240);
    BOOST_ASSERT(rect.yMin()==0);
    BOOST_ASSERT(rect.yMax()==240);
    BOOST_ASSERT(rect.dx()==240);
    BOOST_ASSERT(rect.dy()==240);
}
BOOST_AUTO_TEST_SUITE_END()
