#define BOOST_TEST_MODULE TestGCellGrid
#include <boost/test/included/unit_test.hpp>
#include "db.h"
#include "helper.cpp"

using namespace odb;
using namespace std;

BOOST_AUTO_TEST_SUITE( test_suite )

BOOST_AUTO_TEST_CASE( test_default )
{
    dbDatabase*  db;
    db        = createSimpleDB();
    auto block     = db->getChip()->getBlock();
    auto l1 = db->getTech()->findLayer("L1");
    dbGCellGrid* grid = dbGCellGrid::create(block);
    grid->addGridPatternX(0, 20, 10);// 0 10 20 30 40 50 ... 190
    grid->addGridPatternY(0, 20, 10);// 0 10 20 30 40 50 ... 190

    BOOST_TEST(grid->getXIdx(-1) == 0);
    BOOST_TEST(grid->getXIdx(0) == 0);
    BOOST_TEST(grid->getXIdx(5) == 0);
    BOOST_TEST(grid->getXIdx(15) == 1);
    BOOST_TEST(grid->getXIdx(210) == 19);
    BOOST_TEST(grid->getHorizontalCapacity(l1, 0, 0) == 0);
    grid->setHorizontalCapacity(l1, 0, 0, 20);
    BOOST_TEST(grid->getHorizontalCapacity(l1, 0, 0) == 20);
    grid->addGridPatternX(30, 20, 10);
    BOOST_TEST(grid->getHorizontalCapacity(l1, 0, 0) == 0);

}

BOOST_AUTO_TEST_SUITE_END()
