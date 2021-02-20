#define BOOST_TEST_MODULE TestLef58Properties
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
    grid->addGridPatternX(0, 20, 10);
    grid->addGridPatternY(0, 20, 10);

    uint xIdx = grid->getXIdx(15);
    uint yIdx = grid->getYIdx(25);
    BOOST_TEST(xIdx == 1);
    BOOST_TEST(yIdx == 2);
    BOOST_TEST(grid->getHorizontalCapacity(l1,xIdx,yIdx) == 0);
    grid->setHorizontalCapacity(l1, xIdx,yIdx, 20);
    BOOST_TEST(grid->getHorizontalCapacity(l1,xIdx,yIdx) == 20);
    grid->addGridPatternX(30, 20, 10);
    BOOST_TEST(grid->getHorizontalCapacity(l1,xIdx,yIdx) == 0);

}

BOOST_AUTO_TEST_SUITE_END()
