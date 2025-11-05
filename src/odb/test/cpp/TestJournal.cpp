#include <functional>

#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace odb {
namespace {

class JournalFixture : public SimpleDbFixture
{
 protected:
  JournalFixture()
  {
    createSimpleDB();
    block = db_->getChip()->getBlock();
    lib = db_->findLib("lib1");
    and2 = lib->findMaster("and2");
    or2 = lib->findMaster("or2");
  }
  void in_eco(const std::function<void(void)>& func)
  {
    dbDatabase::beginEco(block);
    func();
    dbDatabase::undoEco(block);
  }
  dbLib* lib;
  dbBlock* block;
  dbMaster* and2;
  dbMaster* or2;
};

TEST_F(JournalFixture, test_undo_inst_create)
{
  in_eco([&]() { dbInst::create(block, and2, "a"); });
  EXPECT_EQ(block->findInst("a"), nullptr);
}

TEST_F(JournalFixture, test_undo_inst_destroy)
{
  auto module = dbModule::create(block, "m");
  auto inst = dbInst::create(block, and2, "a", false, module);
  inst->setOrigin(100, 200);
  // Ensure non-default values are restored
  inst->setPlacementStatus(dbPlacementStatus::PLACED);
  auto group = dbGroup::create(block, "g");
  group->addInst(inst);
  auto region = dbRegion::create(block, "r");
  region->addInst(inst);

  in_eco([&]() { dbInst::destroy(inst); });

  EXPECT_EQ(block->findInst("a"), inst);
  EXPECT_EQ(inst->getPlacementStatus(), dbPlacementStatus::PLACED);
  EXPECT_EQ(inst->getGroup(), group);
  EXPECT_EQ(inst->getModule(), module);
  EXPECT_EQ(inst->getRegion(), region);
  EXPECT_EQ(inst->getLocation(), Point(100, 200));
}

TEST_F(JournalFixture, test_undo_mod_inst_destroy)
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
    EXPECT_EQ(block->getModInsts().size(), 0);
  });

  EXPECT_EQ(block->findModInst("mod_inst"), inst);
  EXPECT_EQ(inst->getGroup(), group);
  EXPECT_EQ(inst->getParent(), top);
  EXPECT_EQ(inst->getMaster(), module);
  EXPECT_EQ(inst->getModITerms().size(), 1);
  EXPECT_EQ(*inst->getModITerms().begin(), mod_iterm);
  EXPECT_EQ(mod_iterm->getModNet(), top_net);
  EXPECT_EQ(block->getInsts().size(), 1);
  EXPECT_EQ(*block->getInsts().begin(), leaf_inst);
  EXPECT_EQ(block->getModInsts().size(), 1);
  EXPECT_EQ(*block->getModInsts().begin(), inst);
}

TEST_F(JournalFixture, test_undo_net_create)
{
  in_eco([&]() {
    auto net = dbNet::create(block, "n");
    net->setSigType(dbSigType::SIGNAL);
  });
  EXPECT_EQ(block->findNet("n"), nullptr);
}

TEST_F(JournalFixture, test_undo_net_destroy)
{
  auto net = dbNet::create(block, "n");
  // Ensure non-default values are restored
  net->setSourceType(dbSourceType::TIMING);
  auto ndr = dbTechNonDefaultRule::create(block, "ndr");
  net->setNonDefaultRule(ndr);

  in_eco([&]() { dbNet::destroy(net); });

  EXPECT_EQ(block->findNet("n"), net);
  EXPECT_EQ(net->getSourceType(), dbSourceType::TIMING);
  EXPECT_EQ(net->getNonDefaultRule(), ndr);
}

TEST_F(JournalFixture, test_undo_net_destroy_guides)
{
  // Ensure guides are restored on undo of dbNet destroy
  auto net = dbNet::create(block, "n");
  auto l1 = db_->getTech()->findLayer("L1");
  const Rect box{0, 0, 100, 100};
  dbGuide::create(net, l1, l1, box, false);

  in_eco([&]() { dbNet::destroy(net); });

  EXPECT_EQ(net->getGuides().size(), 1);
  const auto guide = *net->getGuides().begin();
  EXPECT_EQ(guide->getLayer(), l1);
  EXPECT_EQ(guide->getBox(), box);
  EXPECT_EQ(guide->getViaLayer(), l1);
}

TEST_F(JournalFixture, test_undo_guide_create)
{
  auto net = dbNet::create(block, "n");
  auto l1 = db_->getTech()->findLayer("L1");
  const Rect box{0, 0, 100, 100};

  in_eco([&]() { dbGuide::create(net, l1, l1, box, false); });

  EXPECT_EQ(net->getGuides().size(), 0);
}

TEST_F(JournalFixture, test_undo_inst_swap)
{
  auto inst = dbInst::create(block, and2, "a");
  in_eco([&]() { inst->swapMaster(or2); });
  EXPECT_EQ(inst->getMaster(), and2);
}

TEST_F(JournalFixture, test_undo_inst_place)
{
  auto inst = dbInst::create(block, and2, "a");
  in_eco([&]() { inst->setPlacementStatus(dbPlacementStatus::PLACED); });
  EXPECT_EQ(inst->getPlacementStatus(), dbPlacementStatus::NONE);
}

TEST_F(JournalFixture, test_undo_inst_move)
{
  auto inst = dbInst::create(block, and2, "a");
  inst->setPlacementStatus(dbPlacementStatus::PLACED);
  const Point point(1000, 2000);
  inst->setLocation(point.x(), point.y());
  auto bbox = inst->getBBox()->getBox();

  in_eco([&]() { inst->setLocation(3000, 4000); });

  EXPECT_EQ(inst->getLocation(), point);
  EXPECT_EQ(inst->getBBox()->getBox(), bbox);
}

TEST_F(JournalFixture, test_undo_inst_flip)
{
  auto inst = dbInst::create(block, and2, "a");
  inst->setPlacementStatus(dbPlacementStatus::PLACED);
  const Point point(1000, 2000);
  inst->setLocation(point.x(), point.y());
  const auto bbox = inst->getBBox()->getBox();

  in_eco([&]() { inst->setOrient(dbOrientType::MX); });

  EXPECT_EQ(inst->getLocation(), point);
  EXPECT_EQ(inst->getBBox()->getBox(), bbox);
}

TEST_F(JournalFixture, test_undo_bterm_connect)
{
  auto net1 = dbNet::create(block, "n1");
  auto net2 = dbNet::create(block, "n2");
  auto bterm = dbBTerm::create(net1, "b");
  in_eco([&]() { bterm->connect(net2); });
  EXPECT_EQ(bterm->getNet(), net1);
}

TEST_F(JournalFixture, test_undo_bterm_disconnect)
{
  auto net = dbNet::create(block, "n");
  auto bterm = dbBTerm::create(net, "b");
  in_eco([&]() { bterm->disconnect(); });
  EXPECT_EQ(bterm->getNet(), net);
}

TEST_F(JournalFixture, test_undo_iterm_connect)
{
  auto inst = dbInst::create(block, and2, "a");
  auto net1 = dbNet::create(block, "n1");
  auto net2 = dbNet::create(block, "n2");
  auto iterm = inst->findITerm("b");
  iterm->connect(net1);
  in_eco([&]() { iterm->connect(net2); });
  EXPECT_EQ(iterm->getNet(), net1);
}

TEST_F(JournalFixture, test_undo_iterm_disconnect)
{
  auto inst = dbInst::create(block, and2, "a");
  auto net = dbNet::create(block, "n");
  auto iterm = inst->findITerm("b");
  iterm->connect(net);
  in_eco([&]() { iterm->disconnect(); });
  EXPECT_EQ(iterm->getNet(), net);
}

TEST_F(JournalFixture, RenameNet)
{
  dbNet* net = dbNet::create(block, "original_net_name");
  ASSERT_NE(net, nullptr);
  EXPECT_EQ(net->getName(), "original_net_name");

  in_eco([&]() {
    net->rename("new_net_name");
    EXPECT_EQ(net->getName(), "new_net_name");
  });

  EXPECT_EQ(net->getName(), "original_net_name");

  dbNet::destroy(net);
}

TEST_F(JournalFixture, RenameInst)
{
  dbMaster* master = lib->findMaster("and2");
  ASSERT_NE(master, nullptr);
  dbInst* inst = dbInst::create(block, master, "original_inst_name");
  ASSERT_NE(inst, nullptr);
  EXPECT_EQ(inst->getName(), "original_inst_name");

  in_eco([&]() {
    inst->rename("new_inst_name");
    EXPECT_EQ(inst->getName(), "new_inst_name");
  });

  EXPECT_EQ(inst->getName(), "original_inst_name");

  dbInst::destroy(inst);
}

TEST_F(JournalFixture, RenameModNet)
{
  dbModule* module = dbModule::create(block, "original_module_name");
  ASSERT_NE(module, nullptr);

  dbModNet* mod_net = dbModNet::create(module, "original_mod_net_name");
  ASSERT_NE(mod_net, nullptr);
  EXPECT_EQ(mod_net->getName(), "original_mod_net_name");

  in_eco([&]() {
    mod_net->rename("new_mod_net_name");
    EXPECT_EQ(mod_net->getName(), "new_mod_net_name");
  });

  EXPECT_EQ(mod_net->getName(), "original_mod_net_name");

  dbModNet::destroy(mod_net);
  dbModule::destroy(module);
}

}  // namespace
}  // namespace odb
