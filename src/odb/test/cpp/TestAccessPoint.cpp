#define BOOST_TEST_MODULE TestAccessPoint
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
    auto and2 = db->findMaster("and2");
    auto term = and2->findMTerm("a");
    auto layer = db->getTech()->findLayer("L1");
    auto pin = dbMPin::create(term);
    auto ap = dbAccessPoint::create(pin);
    auto inst = dbInst::create(db->getChip()->getBlock(), and2, "i1");
    auto iterm = inst->getITerm(term);
    ap->setPoint(Point(10,250));
    ap->setLayer(layer);
    ap->setHighType(dbAccessPoint::HalfGrid);
    ap->setAccess(true, dbDirection::DOWN);
    iterm->addAccessPoint(ap);

    utl::Logger* logger = new utl::Logger();
    dbDatabase* db2 = dbDatabase::create();
    db2->setLogger(logger);
    FILE *write, *read;
    std::string path = std::string(std::getenv("BASE_DIR"))
            + "/results/TestAccessPointDbRW";
    write = fopen(path.c_str(), "w");
    db->write(write);
    read = fopen(path.c_str(), "r");
    db2->read(read);
    auto aps = db2->getChip()->getBlock()->findInst("i1")->findITerm("a")->getAccessPoints();
    BOOST_TEST(aps.size() == 1);
    ap = aps[0];
    BOOST_TEST(ap->getPoint().x() == 10);
    BOOST_TEST(ap->getPoint().y() == 250);
    BOOST_TEST(ap->getMPin()->getMTerm()->getName() == "a");
    BOOST_TEST(ap->getLayer()->getName() == "L1");
    BOOST_TEST(ap->hasAccess());
    BOOST_TEST(ap->getHighType() == dbAccessPoint::HalfGrid);
    BOOST_TEST(ap->hasAccess(dbDirection::DOWN));
    BOOST_TEST(!ap->hasAccess(dbDirection::UP));    
}

BOOST_AUTO_TEST_SUITE_END()
