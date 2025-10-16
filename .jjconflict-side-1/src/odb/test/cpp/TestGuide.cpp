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
  dbGuide::create(net, layer, layer, {0, 0, 100, 100}, false);
  dbGuide::create(net, layer, layer, {0, 100, 100, 200}, false);
  net->getGuides().reverse();
  EXPECT_EQ(net->getGuides().size(), 2);
  dbGuide* guide = (dbGuide*) *net->getGuides().begin();
  EXPECT_EQ(guide->getNet(), net);
  EXPECT_EQ(guide->getLayer(), layer);
  EXPECT_EQ(guide->getBox(), Rect(0, 0, 100, 100));
  dbGuide::destroy(guide);
  EXPECT_EQ(net->getGuides().size(), 1);
}

TEST_F(SimpleDbFixture, test_clear_guides)
{
  createSimpleDB();
  auto tech = db_->getTech();
  auto block = db_->getChip()->getBlock();
  auto layer = tech->findLayer("L1");
  auto net = dbNet::create(block, "n1");
  dbGuide::create(net, layer, layer, {0, 100, 100, 200}, false);
  dbGuide::create(net, layer, layer, {0, 100, 100, 200}, false);
  dbGuide::create(net, layer, layer, {0, 100, 100, 200}, false);
  EXPECT_EQ(net->getGuides().size(), 3);
  net->clearGuides();
  EXPECT_EQ(net->getGuides().size(), 0);
}

}  // namespace
}  // namespace odb
