#define BOOST_TEST_MODULE TestGuide
#include <boost/test/included/unit_test.hpp>

#include "db.h"
#include "helper.cpp"

using namespace odb;
using namespace std;

BOOST_AUTO_TEST_SUITE(test_suite)

BOOST_AUTO_TEST_CASE(test_default)
{
  dbDatabase* db;
  db = createSimpleDB();
  auto tech = db->getTech();
  auto block = db->getChip()->getBlock();
  auto layer = tech->findLayer("L1");
  auto net = dbNet::create(block, "n1");
  dbGuide::create(net, layer, {0, 0, 100, 100});
  dbGuide::create(net, layer, {0, 100, 100, 200});
  net->getGuides().reverse();
  BOOST_TEST(net->getGuides().size() == 2);
  dbGuide* guide = (dbGuide*) *net->getGuides().begin();
  BOOST_TEST(guide->getNet() == net);
  BOOST_TEST(guide->getLayer() == layer);
  BOOST_TEST(guide->getBox() == Rect(0, 0, 100, 100));
  dbGuide::destroy(guide);
  BOOST_TEST(net->getGuides().size() == 1);
}

BOOST_AUTO_TEST_CASE(test_clear_guides)
{
  dbDatabase* db;
  db = createSimpleDB();
  auto tech = db->getTech();
  auto block = db->getChip()->getBlock();
  auto layer = tech->findLayer("L1");
  auto net = dbNet::create(block, "n1");
  dbGuide::create(net, layer, {0, 100, 100, 200});
  dbGuide::create(net, layer, {0, 100, 100, 200});
  dbGuide::create(net, layer, {0, 100, 100, 200});
  BOOST_TEST(net->getGuides().size() == 3);
  net->clearGuides();
  BOOST_TEST(net->getGuides().size() == 0);
}

BOOST_AUTO_TEST_SUITE_END()
