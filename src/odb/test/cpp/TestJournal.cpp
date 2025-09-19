#define BOOST_TEST_MODULE TestJournal
#include <functional>

#include "boost/test/included/unit_test.hpp"
#include "env.h"
#include "helper.h"
#include "odb/db.h"
#include "odb/geom.h"

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

BOOST_FIXTURE_TEST_CASE(test_undo_mod_inst_destroy, F_DEFAULT)
{
  auto flat_net = dbNet::create(block, "n");
  // Make module and contents
  auto module = dbModule::create(block, "test");
  auto mod_net = dbModNet::create(module, "lower_net");
  auto mod_bterm = dbModBTerm::create(module, "bterm");
  mod_bterm->connect(mod_net);
  auto leaf_inst = dbInst::create(block, and2, "i1", false, module);
  auto iterm = leaf_inst->findITerm("b");
  iterm->connect(flat_net, mod_net);

  // Instantiate the module and connect it
  auto top = block->getTopModule();
  dbModNet* top_net = dbModNet::create(top, "top_net");
  auto inst = dbModInst::create(top, module, "mod_inst");
  auto mod_iterm = dbModITerm::create(inst, "b", mod_bterm);
  mod_iterm->connect(top_net);

  auto group = dbGroup::create(block, "g");
  group->addModInst(inst);

  in_eco([&]() {
    dbModInst::destroy(inst);
    BOOST_TEST(block->getModInsts().size() == 0);
  });

  BOOST_TEST(block->findModInst("mod_inst") == inst);
  BOOST_TEST(inst->getGroup() == group);
  BOOST_TEST(inst->getParent() == top);
  BOOST_TEST(inst->getMaster() == module);
  BOOST_TEST(inst->getModITerms().size() == 1);
  BOOST_TEST(*inst->getModITerms().begin() == mod_iterm);
  BOOST_TEST(mod_iterm->getModNet() == top_net);
  BOOST_TEST(block->getInsts().size() == 1);
  BOOST_TEST(*block->getInsts().begin() == leaf_inst);
  BOOST_TEST(block->getModInsts().size() == 1);
  BOOST_TEST(*block->getModInsts().begin() == inst);
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

BOOST_FIXTURE_TEST_CASE(RenameNet, F_DEFAULT)
{
  dbNet* net = dbNet::create(block, "original_net_name");
  BOOST_TEST(net != nullptr);
  BOOST_TEST(net->getName() == "original_net_name");

  in_eco([&]() {
    net->rename("new_net_name");
    BOOST_TEST(net->getName() == "new_net_name");
  });

  BOOST_TEST(net->getName() == "original_net_name");

  dbNet::destroy(net);
}

BOOST_FIXTURE_TEST_CASE(RenameInst, F_DEFAULT)
{
  dbMaster* master = lib->findMaster("and2");
  BOOST_TEST(master != nullptr);
  dbInst* inst = dbInst::create(block, master, "original_inst_name");
  BOOST_TEST(inst != nullptr);
  BOOST_TEST(inst->getName() == "original_inst_name");

  in_eco([&]() {
    inst->rename("new_inst_name");
    BOOST_TEST(inst->getName() == "new_inst_name");
  });

  BOOST_TEST(inst->getName() == "original_inst_name");

  dbInst::destroy(inst);
}

BOOST_FIXTURE_TEST_CASE(RenameModNet, F_DEFAULT)
{
  dbModule* module = dbModule::create(block, "original_module_name");
  BOOST_TEST(module != nullptr);

  dbModNet* mod_net = dbModNet::create(module, "original_mod_net_name");
  BOOST_TEST(mod_net != nullptr);
  BOOST_TEST(mod_net->getName() == "original_mod_net_name");

  in_eco([&]() {
    mod_net->rename("new_mod_net_name");
    BOOST_TEST(mod_net->getName() == "new_mod_net_name");
  });

  BOOST_TEST(mod_net->getName() == "original_mod_net_name");

  dbModNet::destroy(mod_net);
  dbModule::destroy(module);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace
}  // namespace odb
