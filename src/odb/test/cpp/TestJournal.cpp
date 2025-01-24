#define BOOST_TEST_MODULE TestJournal
#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <string>

#include "env.h"
#include "helper.h"
#include "odb/db.h"

namespace odb {
namespace {

BOOST_AUTO_TEST_SUITE(test_suite)

struct F_DEFAULT
{
  F_DEFAULT()
  {
    db = createSimpleDB();
    block = db->getChip()->getBlock();
    lib = db->findLib("lib1");
    and2 = lib->findMaster("and2");
    or2 = lib->findMaster("or2");
  }
  ~F_DEFAULT() { dbDatabase::destroy(db); }
  void in_eco(const std::function<void(void)>& func)
  {
    dbDatabase::beginEco(block);
    func();
    dbDatabase::endEco(block);
    dbDatabase::undoEco(block);
  }
  dbDatabase* db;
  dbLib* lib;
  dbBlock* block;
  dbMaster* and2;
  dbMaster* or2;
};

BOOST_FIXTURE_TEST_CASE(test_undo_inst_create, F_DEFAULT)
{
  in_eco([&]() { dbInst::create(block, and2, "a"); });
  BOOST_TEST(block->findInst("a") == nullptr);
}

BOOST_FIXTURE_TEST_CASE(test_undo_inst_destroy, F_DEFAULT)
{
  auto module = dbModule::create(block, "m");
  auto inst = dbInst::create(block, and2, "a", false, module);
  // Ensure non-default values are restored
  inst->setPlacementStatus(dbPlacementStatus::PLACED);
  auto group = dbGroup::create(block, "g");
  group->addInst(inst);
  auto region = dbRegion::create(block, "r");
  region->addInst(inst);

  in_eco([&]() { dbInst::destroy(inst); });

  BOOST_TEST(block->findInst("a") == inst);
  BOOST_TEST(inst->getPlacementStatus() == dbPlacementStatus::PLACED);
  BOOST_TEST(inst->getGroup() == group);
  BOOST_TEST(inst->getModule() == module);
  BOOST_TEST(inst->getRegion() == region);
}

BOOST_FIXTURE_TEST_CASE(test_undo_net_create, F_DEFAULT)
{
  in_eco([&]() {
    auto net = dbNet::create(block, "n");
    net->setSigType(dbSigType::SIGNAL);
  });
  BOOST_TEST(block->findNet("n") == nullptr);
}

BOOST_FIXTURE_TEST_CASE(test_undo_net_destroy, F_DEFAULT)
{
  auto net = dbNet::create(block, "n");
  // Ensure non-default values are restored
  net->setSourceType(dbSourceType::TIMING);
  auto ndr = dbTechNonDefaultRule::create(block, "ndr");
  net->setNonDefaultRule(ndr);

  in_eco([&]() { dbNet::destroy(net); });

  BOOST_TEST(block->findNet("n") == net);
  BOOST_TEST(net->getSourceType() == dbSourceType::TIMING);
  BOOST_TEST(net->getNonDefaultRule() == ndr);
}

BOOST_FIXTURE_TEST_CASE(test_undo_net_destroy_guides, F_DEFAULT)
{
  // Ensure guides are restored on undo of dbNet destroy
  auto net = dbNet::create(block, "n");
  auto l1 = db->getTech()->findLayer("L1");
  const Rect box{0, 0, 100, 100};
  dbGuide::create(net, l1, l1, box, false);

  in_eco([&]() { dbNet::destroy(net); });

  BOOST_TEST(net->getGuides().size() == 1);
  const auto guide = *net->getGuides().begin();
  BOOST_TEST(guide->getLayer() == l1);
  BOOST_TEST(guide->getBox() == box);
  BOOST_TEST(guide->getViaLayer() == l1);
}

BOOST_FIXTURE_TEST_CASE(test_undo_guide_create, F_DEFAULT)
{
  auto net = dbNet::create(block, "n");
  auto l1 = db->getTech()->findLayer("L1");
  const Rect box{0, 0, 100, 100};

  in_eco([&]() { dbGuide::create(net, l1, l1, box, false); });

  BOOST_TEST(net->getGuides().size() == 0);
}

BOOST_FIXTURE_TEST_CASE(test_undo_inst_swap, F_DEFAULT)
{
  auto inst = dbInst::create(block, and2, "a");
  in_eco([&]() { inst->swapMaster(or2); });
  BOOST_TEST(inst->getMaster() == and2);
}

BOOST_FIXTURE_TEST_CASE(test_undo_inst_place, F_DEFAULT)
{
  auto inst = dbInst::create(block, and2, "a");
  in_eco([&]() { inst->setPlacementStatus(dbPlacementStatus::PLACED); });
  BOOST_TEST(inst->getPlacementStatus() == dbPlacementStatus::NONE);
}

BOOST_FIXTURE_TEST_CASE(test_undo_inst_move, F_DEFAULT)
{
  auto inst = dbInst::create(block, and2, "a");
  inst->setPlacementStatus(dbPlacementStatus::PLACED);
  const Point point(1000, 2000);
  inst->setLocation(point.x(), point.y());
  auto bbox = inst->getBBox()->getBox();

  in_eco([&]() { inst->setLocation(3000, 4000); });

  BOOST_TEST(inst->getLocation() == point);
  BOOST_TEST(inst->getBBox()->getBox() == bbox);
}

BOOST_FIXTURE_TEST_CASE(test_undo_inst_flip, F_DEFAULT)
{
  auto inst = dbInst::create(block, and2, "a");
  inst->setPlacementStatus(dbPlacementStatus::PLACED);
  const Point point(1000, 2000);
  inst->setLocation(point.x(), point.y());
  const auto bbox = inst->getBBox()->getBox();

  in_eco([&]() { inst->setOrient(dbOrientType::MX); });

  BOOST_TEST(inst->getLocation() == point);
  BOOST_TEST(inst->getBBox()->getBox() == bbox);
}

BOOST_FIXTURE_TEST_CASE(test_undo_bterm_connect, F_DEFAULT)
{
  auto net1 = dbNet::create(block, "n1");
  auto net2 = dbNet::create(block, "n2");
  auto bterm = dbBTerm::create(net1, "b");
  in_eco([&]() { bterm->connect(net2); });
  BOOST_TEST(bterm->getNet() == net1);
}

BOOST_FIXTURE_TEST_CASE(test_undo_bterm_disconnect, F_DEFAULT)
{
  auto net = dbNet::create(block, "n");
  auto bterm = dbBTerm::create(net, "b");
  in_eco([&]() { bterm->disconnect(); });
  BOOST_TEST(bterm->getNet() == net);
}

BOOST_FIXTURE_TEST_CASE(test_undo_iterm_connect, F_DEFAULT)
{
  auto inst = dbInst::create(block, and2, "a");
  auto net1 = dbNet::create(block, "n1");
  auto net2 = dbNet::create(block, "n2");
  auto iterm = inst->findITerm("b");
  iterm->connect(net1);
  in_eco([&]() { iterm->connect(net2); });
  BOOST_TEST(iterm->getNet() == net1);
}

BOOST_FIXTURE_TEST_CASE(test_undo_iterm_disconnect, F_DEFAULT)
{
  auto inst = dbInst::create(block, and2, "a");
  auto net = dbNet::create(block, "n");
  auto iterm = inst->findITerm("b");
  iterm->connect(net);
  in_eco([&]() { iterm->disconnect(); });
  BOOST_TEST(iterm->getNet() == net);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace
}  // namespace odb
