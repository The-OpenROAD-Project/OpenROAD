#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"

namespace odb {
namespace {

TEST_F(SimpleDbFixture, test_default)
{
  createSimpleDB();
  auto tech = db_->getTech();
  auto block = db_->getChip()->getBlock();
  auto layer = tech->findLayer("L1");
  auto net = dbNet::create(block, "n1");
  dbNetTrack::create(net, layer, {0, 0, 100, 100});
  dbNetTrack::create(net, layer, {0, 100, 100, 200});
  net->getTracks().reverse();
  EXPECT_EQ(net->getTracks().size(), 2);
  dbNetTrack* track = (dbNetTrack*) *net->getTracks().begin();
  EXPECT_EQ(track->getNet(), net);
  EXPECT_EQ(track->getLayer(), layer);
  EXPECT_EQ(track->getBox(), Rect(0, 0, 100, 100));
  dbNetTrack::destroy(track);
  EXPECT_EQ(net->getTracks().size(), 1);
}

TEST_F(SimpleDbFixture, test_clear_tracks)
{
  createSimpleDB();
  auto tech = db_->getTech();
  auto block = db_->getChip()->getBlock();
  auto layer = tech->findLayer("L1");
  auto net = dbNet::create(block, "n1");
  dbNetTrack::create(net, layer, {0, 100, 100, 200});
  dbNetTrack::create(net, layer, {0, 100, 100, 200});
  dbNetTrack::create(net, layer, {0, 100, 100, 200});
  EXPECT_EQ(net->getTracks().size(), 3);
  net->clearTracks();
  EXPECT_EQ(net->getTracks().size(), 0);
}

}  // namespace
}  // namespace odb
