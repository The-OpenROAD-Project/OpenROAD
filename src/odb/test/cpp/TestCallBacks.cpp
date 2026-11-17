#include <algorithm>

#include "CallBack.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "src/odb/test/cpp/helper/helper.h"
#include "tst/db_fixture.h"

namespace odb {
namespace {

class CallbackFixture : public SimpleDbFixture
{
 protected:
  CallBack cb_;
};

TEST_F(CallbackFixture, test_inst_and_iterm)
{
  createSimpleDB();
  dbBlock* block = db_->getChip()->getBlock();
  dbNet* n1 = dbNet::create(block, "n1");
  cb_.addOwner(block);
  dbInst* i1 = dbInst::create(block, db_->findMaster("and2"), "i1");
  EXPECT_EQ(cb_.events.size(), 4);
  EXPECT_EQ(cb_.events[0], "Create inst i1");
  EXPECT_EQ(cb_.events[1], "Create iterm a of inst i1");
  EXPECT_EQ(cb_.events[2], "Create iterm b of inst i1");
  EXPECT_EQ(cb_.events[3], "Create iterm o of inst i1");
  cb_.clearEvents();
  cb_.pause();
  dbRegion* parent_region = dbRegion::create(block, "parent");
  cb_.unpause();
  dbInst::create(block, db_->findMaster("and2"), "i2", parent_region);
  EXPECT_EQ(cb_.events.size(), 4);
  EXPECT_EQ(cb_.events[0], "Create inst i2 in region parent");
  EXPECT_EQ(cb_.events[1], "Create iterm a of inst i2");
  EXPECT_EQ(cb_.events[2], "Create iterm b of inst i2");
  EXPECT_EQ(cb_.events[3], "Create iterm o of inst i2");
  cb_.clearEvents();
  i1->swapMaster(db_->findMaster("or2"));
  EXPECT_EQ(cb_.events.size(), 2);
  EXPECT_EQ(cb_.events[0], "PreSwap inst i1 from master and2 to master or2");
  EXPECT_EQ(cb_.events[1], "PostSwap inst i1 to master or2");
  cb_.clearEvents();
  i1->setOrigin(100, 100);
  EXPECT_EQ(cb_.events.size(), 2);
  EXPECT_EQ(cb_.events[0], "PreMove inst i1");
  EXPECT_EQ(cb_.events[1], "PostMove inst i1");
  cb_.clearEvents();
  i1->setOrigin(100, 100);
  EXPECT_EQ(cb_.events.size(), 2);
  EXPECT_EQ(cb_.events[0], "PreMove inst i1");
  EXPECT_EQ(cb_.events[1], "PostMove inst i1");
  cb_.clearEvents();
  i1->setPlacementStatus(dbPlacementStatus::FIRM);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Change inst status: i1 -> FIRM");
  cb_.clearEvents();
  i1->setPlacementStatus(dbPlacementStatus::FIRM);
  EXPECT_EQ(cb_.events.size(), 0);
  i1->setPlacementStatus(dbPlacementStatus::NONE);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Change inst status: i1 -> NONE");
  cb_.clearEvents();
  i1->findITerm("a")->connect(n1);
  EXPECT_EQ(cb_.events.size(), 2);
  EXPECT_EQ(cb_.events[0], "PreConnect iterm to net n1");
  EXPECT_EQ(cb_.events[1], "PostConnect iterm to net n1");
  cb_.clearEvents();
  i1->findITerm("a")->connect(n1);
  EXPECT_EQ(cb_.events.size(), 0);
  i1->findITerm("a")->disconnect();
  EXPECT_EQ(cb_.events.size(), 2);
  EXPECT_EQ(cb_.events[0], "PreDisconnect iterm from net n1");
  EXPECT_EQ(cb_.events[1], "PostDisconnect iterm from net n1");
  cb_.clearEvents();
  i1->findITerm("a")->disconnect();
  EXPECT_EQ(cb_.events.size(), 0);

  dbInst::destroy(i1);

  EXPECT_EQ(cb_.events.size(), 4);
  EXPECT_EQ(cb_.events[0], "Destroy iterm o of inst i1");
  EXPECT_EQ(cb_.events[1], "Destroy iterm b of inst i1");
  EXPECT_EQ(cb_.events[2], "Destroy iterm a of inst i1");
  EXPECT_EQ(cb_.events[3], "Destroy inst i1");
}

TEST_F(CallbackFixture, test_net)
{
  createSimpleDB();
  dbBlock* block = db_->getChip()->getBlock();
  cb_.addOwner(block);
  dbNet* n1 = dbNet::create(block, "n1");
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Create net n1");
  cb_.clearEvents();
  dbNet::destroy(n1);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Destroy net n1");
}

TEST_F(CallbackFixture, test_bterm)
{
  create2LevetDbNoBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  cb_.addOwner(block);
  auto n1 = block->findNet("n1");
  dbBTerm* IN1 = dbBTerm::create(n1, "IN1");
  IN1->setIoType(dbIoType::INPUT);
  EXPECT_EQ(cb_.events.size(), 3);
  EXPECT_EQ(cb_.events[0], "Create IN1");
  EXPECT_EQ(cb_.events[1], "Preconnect IN1 to n1");
  EXPECT_EQ(cb_.events[2], "Postconnect IN1");
  cb_.clearEvents();
  IN1->disconnect();
  EXPECT_EQ(cb_.events.size(), 2);
  EXPECT_EQ(cb_.events[0], "Predisconnect IN1");
  EXPECT_EQ(cb_.events[1], "Postdisconnect IN1 from n1");
  cb_.clearEvents();
  dbBTerm::destroy(IN1);

  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Destroy IN1");
}

TEST_F(CallbackFixture, test_bpin)
{
  create2LevetDbWithBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  cb_.addOwner(block);
  dbBPin* pin = dbBPin::create(block->findBTerm("IN1"));
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Create BPin for IN1");
  cb_.clearEvents();
  pin->setPlacementStatus(dbPlacementStatus::FIRM);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Change BPin status: <dbBPin:1> -> FIRM");
  cb_.clearEvents();
  pin->setPlacementStatus(dbPlacementStatus::FIRM);
  EXPECT_EQ(cb_.events.size(), 0);
  cb_.clearEvents();
  pin->setPlacementStatus(dbPlacementStatus::NONE);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Change BPin status: <dbBPin:1> -> NONE");
  cb_.clearEvents();
  dbBox* box
      = dbBox::create(pin, db_->getTech()->findLayer("L1"), 0, 0, 100, 100);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Create BPin box (0,0) (100,100)");
  cb_.clearEvents();
  dbBox::destroy(box);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Destroy bpin box");
  cb_.clearEvents();
  dbBPin::destroy(pin);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Destroy BPin");
}

TEST_F(CallbackFixture, test_blockage)
{
  create2LevetDbWithBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  cb_.addOwner(block);
  dbBlockage* blk = dbBlockage::create(block, 0, 0, 100, 100);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Create blockage (0,0) (100,100)");
  cb_.clearEvents();
  dbBlockage::destroy(blk);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Destroy blockage");
}

TEST_F(CallbackFixture, test_obstruction)
{
  create2LevetDbWithBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  cb_.addOwner(block);
  dbObstruction* obs = dbObstruction::create(
      block, db_->getTech()->findLayer("L1"), 0, 0, 100, 100);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Create obstruction (0,0) (100,100)");
  cb_.clearEvents();
  dbObstruction::destroy(obs);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Destroy obstruction");
}

TEST_F(CallbackFixture, test_region)
{
  create2LevetDbWithBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  cb_.addOwner(block);
  dbRegion* parent_region = dbRegion::create(block, "parent");
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Create region parent");
  cb_.clearEvents();
  dbBox::create(parent_region, 0, 0, 1, 1);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Add box (0, 0) (1, 1) to region parent");
  cb_.clearEvents();
  dbRegion::destroy(parent_region);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Destroy region parent");
}

TEST_F(CallbackFixture, test_group)
{
  create2LevetDbWithBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  dbInst* inst = block->findInst("i1");
  ASSERT_NE(inst, nullptr);
  cb_.addOwner(block);

  dbGroup* parent = dbGroup::create(block, "parent");
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Create group parent");
  cb_.clearEvents();

  parent->addInst(inst);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Add inst i1 to group parent");
  cb_.clearEvents();

  // Creating under a parent is a create plus the parenting it performs.
  dbGroup* child = dbGroup::create(parent, "child");
  EXPECT_EQ(cb_.events.size(), 2);
  EXPECT_EQ(cb_.events[0], "Create group child");
  EXPECT_EQ(cb_.events[1], "Add group child to group parent");
  cb_.clearEvents();

  parent->removeGroup(child);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Remove group child from group parent");
  cb_.clearEvents();

  parent->removeInst(inst);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Remove inst i1 from group parent");
  cb_.clearEvents();

  // destroy detaches the members first, so a receiver never hears about a
  // group it was told is gone; the group is still readable when Destroy fires.
  parent->addInst(inst);
  cb_.clearEvents();
  dbGroup::destroy(parent);
  EXPECT_EQ(cb_.events.size(), 2);
  EXPECT_EQ(cb_.events[0], "Remove inst i1 from group parent");
  EXPECT_EQ(cb_.events[1], "Destroy group parent");
}

TEST_F(CallbackFixture, test_group_modinst)
{
  create2LevetDbWithBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  dbModule* parent_mod = dbModule::create(block, "parent_mod");
  dbModule* master_mod = dbModule::create(block, "master_mod");
  dbModInst* modinst = dbModInst::create(parent_mod, master_mod, "mi");
  ASSERT_NE(modinst, nullptr);
  dbGroup* group = dbGroup::create(block, "g");
  cb_.addOwner(block);

  group->addModInst(modinst);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Add modinst mi to group g");
  cb_.clearEvents();

  group->removeModInst(modinst);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Remove modinst mi from group g");
}

TEST_F(CallbackFixture, test_group_in_region)
{
  create2LevetDbWithBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  dbRegion* region = dbRegion::create(block, "r");
  cb_.addOwner(block);

  // Joining the region has no callback of its own.
  dbGroup* group = dbGroup::create(region, "g");
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Create group g");
  EXPECT_EQ(group->getRegion(), region);
}

TEST_F(CallbackFixture, test_group_move_inst)
{
  create2LevetDbWithBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  dbInst* inst = block->findInst("i1");
  ASSERT_NE(inst, nullptr);
  dbGroup* from = dbGroup::create(block, "a");
  dbGroup* to = dbGroup::create(block, "b");
  from->addInst(inst);
  cb_.addOwner(block);

  to->addInst(inst);
  EXPECT_EQ(cb_.events.size(), 2);
  EXPECT_EQ(cb_.events[0], "Remove inst i1 from group a");
  EXPECT_EQ(cb_.events[1], "Add inst i1 to group b");
  EXPECT_EQ(inst->getGroup(), to);
}

TEST_F(CallbackFixture, test_group_destroy_tree)
{
  create2LevetDbWithBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  dbInst* inst = block->findInst("i1");
  ASSERT_NE(inst, nullptr);
  dbGroup* parent = dbGroup::create(block, "parent");
  dbGroup* mid = dbGroup::create(parent, "mid");
  dbGroup* leaf = dbGroup::create(mid, "leaf");
  mid->addInst(inst);
  cb_.addOwner(block);

  dbGroup::destroy(mid);
  EXPECT_EQ(cb_.events.size(), 4);
  EXPECT_EQ(cb_.events[0], "Remove inst i1 from group mid");
  EXPECT_EQ(cb_.events[1], "Remove group leaf from group mid");
  EXPECT_EQ(cb_.events[2], "Remove group mid from group parent");
  EXPECT_EQ(cb_.events[3], "Destroy group mid");
  EXPECT_EQ(leaf->getParentGroup(), nullptr);
  EXPECT_TRUE(parent->getGroups().empty());
}

TEST_F(CallbackFixture, test_group_destroy_grouped_inst)
{
  create2LevetDbWithBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  dbInst* inst = block->findInst("i1");
  ASSERT_NE(inst, nullptr);
  dbGroup* group = dbGroup::create(block, "g");
  group->addInst(inst);
  cb_.addOwner(block);

  // The inst's iterms report themselves too, so check the order only.
  dbInst::destroy(inst);
  const auto removed
      = std::ranges::find(cb_.events, "Remove inst i1 from group g");
  const auto destroyed = std::ranges::find(cb_.events, "Destroy inst i1");
  ASSERT_NE(removed, cb_.events.end());
  ASSERT_NE(destroyed, cb_.events.end());
  EXPECT_LT(removed, destroyed);
  EXPECT_TRUE(group->getInsts().empty());
}

TEST_F(CallbackFixture, test_group_destroy_grouped_modinst)
{
  create2LevetDbWithBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  dbModule* parent_mod = dbModule::create(block, "parent_mod");
  dbModule* master_mod = dbModule::create(block, "master_mod");
  dbModInst* modinst = dbModInst::create(parent_mod, master_mod, "mi");
  ASSERT_NE(modinst, nullptr);
  dbGroup* group = dbGroup::create(block, "g");
  group->addModInst(modinst);
  cb_.addOwner(block);

  dbModInst::destroy(modinst);
  EXPECT_EQ(cb_.events.size(), 2);
  EXPECT_EQ(cb_.events[0], "Remove modinst mi from group g");
  EXPECT_EQ(cb_.events[1], "Destroy modinst mi");
  EXPECT_TRUE(group->getModInsts().empty());
}

TEST_F(CallbackFixture, test_row)
{
  create2LevetDbWithBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  cb_.addOwner(block);
  dbSite* site = dbSite::create(db_->findLib("lib1"), "site1");
  dbRow* row = dbRow::create(
      block, "row1", site, 0, 0, dbOrientType::MX, dbRowDir::HORIZONTAL, 1, 20);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Create row row1");
  cb_.clearEvents();
  dbRow::destroy(row);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Destroy row row1");
}

TEST_F(CallbackFixture, test_wire)
{
  create2LevetDbWithBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  cb_.addOwner(block);
  dbWire* wire1 = dbWire::create(block->findNet("n1"));
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Create wire");
  cb_.clearEvents();
  wire1->attach(block->findNet("n1"));
  EXPECT_EQ(cb_.events.size(), 0);
  wire1->detach();
  EXPECT_EQ(cb_.events.size(), 2);
  EXPECT_EQ(cb_.events[0], "PreDetach wire");
  EXPECT_EQ(cb_.events[1], "PostDetach wire from n1");
  cb_.clearEvents();
  wire1->attach(block->findNet("n1"));
  EXPECT_EQ(cb_.events.size(), 2);
  EXPECT_EQ(cb_.events[0], "PreAttach wire to n1");
  EXPECT_EQ(cb_.events[1], "PostAttach wire");
  cb_.clearEvents();

  cb_.pause();
  dbWire* wire2 = dbWire::create(block);
  cb_.unpause();
  wire2->append(wire1);
  EXPECT_EQ(cb_.events.size(), 2);
  EXPECT_EQ(cb_.events[0], "PreAppend wire");
  EXPECT_EQ(cb_.events[1], "PostAppend wire");
  cb_.clearEvents();

  dbWire::destroy(wire2);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Destroy wire");
}

TEST_F(CallbackFixture, test_swire)
{
  create2LevetDbWithBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  cb_.addOwner(block);
  dbSWire* wire = dbSWire::create(block->findNet("n1"), dbWireType::NOSHIELD);
  EXPECT_EQ(cb_.events.size(), 1);
  EXPECT_EQ(cb_.events[0], "Create swire");
  cb_.clearEvents();
  dbSBox::create(wire,
                 db_->getTech()->findLayer("L1"),
                 0,
                 100,
                 100,
                 100,
                 dbWireShapeType::IOWIRE,
                 dbSBox::Direction::HORIZONTAL);

  dbSWire::destroy(wire);
  EXPECT_EQ(cb_.events.size(), 3);
  EXPECT_EQ(cb_.events[0], "PreDestroySBoxes");
  EXPECT_EQ(cb_.events[1], "PostDestroySBoxes");
  EXPECT_EQ(cb_.events[2], "Destroy swire");
}

TEST_F(CallbackFixture, test_findInst_in_callback)
{
  class FindCallback : public dbBlockCallBackObj
  {
   public:
    void inDbITermDestroy(dbITerm* iterm) override
    {
      dbInst* inst = iterm->getInst();
      if (auto it = inst->findITerm("a")) {
        EXPECT_NE(it->getId(), 0);
      }
      if (auto it = inst->findITerm("b")) {
        EXPECT_NE(it->getId(), 0);
      }
      if (auto it = inst->findITerm("o")) {
        EXPECT_NE(it->getId(), 0);
      }
    }
  };

  createSimpleDB();
  dbBlock* block = db_->getChip()->getBlock();
  FindCallback cb;
  cb.addOwner(block);
  dbLib* lib = db_->findLib("lib1");
  auto and2 = lib->findMaster("and2");
  dbInst* i1 = dbInst::create(block, and2, "i1");
  dbInst::destroy(i1);
}

}  // namespace
}  // namespace odb
