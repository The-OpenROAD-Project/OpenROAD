#define BOOST_TEST_MODULE TestGuide
#include <boost/test/included/unit_test.hpp>

#include "helper.h"
#include "odb/db.h"

namespace odb {
namespace {

BOOST_AUTO_TEST_SUITE(test_suite)

BOOST_AUTO_TEST_CASE(test_default)
{
  dbDatabase* db;
  db = createSimpleDB();
  auto tech = db->getTech();
  auto block = db->getChip()->getBlock();
  auto layer = tech->findLayer("L1");
  auto net = dbNet::create(block, "n1");
  dbNetTrack::create(net, layer, {0, 0, 100, 100});
  dbNetTrack::create(net, layer, {0, 100, 100, 200});
  net->getTracks().reverse();
  BOOST_TEST(net->getTracks().size() == 2);
  dbNetTrack* track = (dbNetTrack*) *net->getTracks().begin();
  BOOST_TEST(track->getNet() == net);
  BOOST_TEST(track->getLayer() == layer);
  BOOST_TEST(track->getBox() == Rect(0, 0, 100, 100));
  dbNetTrack::destroy(track);
  BOOST_TEST(net->getTracks().size() == 1);
}

BOOST_AUTO_TEST_CASE(test_clear_tracks)
{
  dbDatabase* db;
  db = createSimpleDB();
  auto tech = db->getTech();
  auto block = db->getChip()->getBlock();
  auto layer = tech->findLayer("L1");
  auto net = dbNet::create(block, "n1");
  dbNetTrack::create(net, layer, {0, 100, 100, 200});
  dbNetTrack::create(net, layer, {0, 100, 100, 200});
  dbNetTrack::create(net, layer, {0, 100, 100, 200});
  BOOST_TEST(net->getTracks().size() == 3);
  net->clearTracks();
  BOOST_TEST(net->getTracks().size() == 0);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace
}  // namespace odb
