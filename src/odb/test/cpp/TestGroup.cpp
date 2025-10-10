#include <array>
#include <string>
#include <utility>

#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"
#include "odb/dbSet.h"

namespace odb {
namespace {

class GroupFixture : public SimpleDbFixture
{
 protected:
  GroupFixture()
  {
    create2LevetDbNoBTerms();
    block = db_->getChip()->getBlock();
    lib = db_->findLib("lib1");
    master_mod1 = dbModule::create(block, "master_mod1");
    master_mod2 = dbModule::create(block, "master_mod2");
    master_mod3 = dbModule::create(block, "master_mod3");
    parent_mod = dbModule::create(block, "parent_mod");
    i1 = dbModInst::create(parent_mod, master_mod1, "i1");
    i2 = dbModInst::create(parent_mod, master_mod2, "i2");
    i3 = dbModInst::create(parent_mod, master_mod3, "i3");
    inst1 = block->findInst("i1");
    inst2 = block->findInst("i2");
    inst3 = block->findInst("i3");
    n1 = block->findNet("n1");
    n2 = block->findNet("n2");
    n3 = block->findNet("n3");
    group = dbGroup::create(block, "group");
    region = dbRegion::create(block, "domain");
    domain = dbGroup::create(region, "domain");
    domain->setType(dbGroupType::VOLTAGE_DOMAIN);
    child1 = dbGroup::create(block, "child1");
    child2 = dbGroup::create(block, "child2");
    child3 = dbGroup::create(block, "child3");

    group->addModInst(i1);
    group->addModInst(i2);
    group->addModInst(i3);

    group->addInst(inst1);
    group->addInst(inst2);
    group->addInst(inst3);

    group->addGroup(child1);
    group->addGroup(child2);
    group->addGroup(child3);

    domain->addPowerNet(n1);
    domain->addPowerNet(n2);
    domain->addPowerNet(n3);
  }
  dbDatabase* db;
  dbLib* lib;
  dbBlock* block;
  dbModule* master_mod1;
  dbModule* master_mod2;
  dbModule* master_mod3;
  dbModule* parent_mod;
  dbModInst* i1;
  dbModInst* i2;
  dbModInst* i3;
  dbInst* inst1;
  dbInst* inst2;
  dbInst* inst3;
  dbNet* n1;
  dbNet* n2;
  dbNet* n3;
  dbGroup* group;
  dbGroup* domain;
  dbGroup* child1;
  dbGroup* child2;
  dbGroup* child3;
  dbRegion* region;
};

TEST_F(GroupFixture, test_group_default)
{
  ASSERT_NE(group, nullptr);
  EXPECT_EQ(dbGroup::create(block, "group"), nullptr);
  EXPECT_EQ(block->getGroups().size(), 5);
  dbGroup* new_group = block->findGroup("group");
  ASSERT_NE(new_group, nullptr);
  EXPECT_STREQ(new_group->getName(), "group");
  dbGroup::destroy(new_group);
  EXPECT_EQ(block->getGroups().size(), 4);
  EXPECT_EQ(block->findGroup("group"), nullptr);
  EXPECT_EQ(region->getGroups().size(), 1);
  EXPECT_EQ(*region->getGroups().begin(), domain);
  EXPECT_EQ(domain->getRegion(), region);
  EXPECT_EQ(child1->getType(), dbGroupType::PHYSICAL_CLUSTER);
  EXPECT_EQ(domain->getType(), dbGroupType::VOLTAGE_DOMAIN);
  domain->setType(dbGroupType::PHYSICAL_CLUSTER);
  EXPECT_EQ(domain->getType(), dbGroupType::PHYSICAL_CLUSTER);
}
TEST_F(GroupFixture, test_group_modinst)
{
  auto insts = group->getModInsts();
  EXPECT_EQ(insts.size(), 3);
  EXPECT_EQ(*insts.begin(), i3);
  EXPECT_EQ(i3->getGroup(), group);
  group->removeModInst(i3);
  EXPECT_EQ(insts.size(), 2);
  EXPECT_EQ(i3->getGroup(), nullptr);
  EXPECT_EQ(*insts.begin(), i2);
  dbModInst::destroy(i2);
  EXPECT_EQ(insts.size(), 1);
  EXPECT_EQ(*insts.begin(), i1);
  dbGroup::destroy(group);
  EXPECT_EQ(i1->getGroup(), nullptr);
}
TEST_F(GroupFixture, test_group_inst)
{
  auto insts = group->getInsts();
  EXPECT_EQ(insts.size(), 3);
  EXPECT_EQ(*insts.begin(), inst3);
  EXPECT_EQ(inst3->getGroup(), group);
  group->removeInst(inst3);
  EXPECT_EQ(insts.size(), 2);
  EXPECT_EQ(inst3->getGroup(), nullptr);
  EXPECT_EQ(*insts.begin(), inst2);
  dbInst::destroy(inst2);
  EXPECT_EQ(insts.size(), 1);
  EXPECT_EQ(*insts.begin(), inst1);
  dbGroup::destroy(group);
  EXPECT_EQ(inst1->getGroup(), nullptr);
}
TEST_F(GroupFixture, test_group_net)
{
  auto power_nets = domain->getPowerNets();
  auto ground_nets = domain->getGroundNets();
  EXPECT_EQ(power_nets.size(), 3);
  EXPECT_EQ(ground_nets.size(), 0);
  EXPECT_EQ(*power_nets.begin(), n1);
  domain->removeNet(n1);
  EXPECT_EQ(power_nets.size(), 2);
  EXPECT_EQ(*power_nets.begin(), n2);
  dbNet::destroy(n2);
  EXPECT_EQ(power_nets.size(), 1);
  EXPECT_EQ(*power_nets.begin(), n3);
  domain->addGroundNet(n3);
  EXPECT_EQ(power_nets.size(), 0);
  EXPECT_EQ(ground_nets.size(), 1);
  EXPECT_EQ(*ground_nets.begin(), n3);
}
TEST_F(GroupFixture, test_group_group)
{
  auto groups = group->getGroups();
  EXPECT_EQ(groups.size(), 3);
  EXPECT_EQ(*groups.begin(), child3);
  EXPECT_EQ(child3->getParentGroup(), group);
  group->removeGroup(child3);
  EXPECT_EQ(groups.size(), 2);
  EXPECT_EQ(child3->getParentGroup(), nullptr);
  EXPECT_EQ(*groups.begin(), child2);
  dbGroup::destroy(child2);
  EXPECT_EQ(groups.size(), 1);
  EXPECT_EQ(*groups.begin(), child1);
  dbGroup::destroy(group);
  EXPECT_EQ(child1->getParentGroup(), nullptr);
}

template <typename T>
void expect_str_names(dbSet<T> objects, const std::array<const char*, 3>& names)
{
  for (auto [itr, i] = std::pair{objects.begin(), 0}; itr != objects.end();
       ++itr, ++i) {
    EXPECT_STREQ((*itr)->getName(), names[i]);
  }
}

template <typename T>
void expect_names(dbSet<T> objects, const std::array<const char*, 3>& names)
{
  for (auto [itr, i] = std::pair{objects.begin(), 0}; itr != objects.end();
       ++itr, ++i) {
    EXPECT_EQ((*itr)->getName(), names[i]);
  }
}

TEST_F(GroupFixture, test_group_modinst_iterator)
{
  dbSet<dbModInst> modinsts = group->getModInsts();
  expect_str_names(modinsts, {"i3", "i2", "i1"});

  EXPECT_TRUE(modinsts.reversible());
  modinsts.reverse();

  expect_str_names(modinsts, {"i1", "i2", "i3"});
}
TEST_F(GroupFixture, test_group_inst_iterator)
{
  dbSet<dbInst> insts = group->getInsts();
  expect_names(insts, {"i3", "i2", "i1"});

  EXPECT_TRUE(insts.reversible());
  insts.reverse();

  expect_names(insts, {"i1", "i2", "i3"});
}
TEST_F(GroupFixture, test_group_net_iterators)
{
  dbSet<dbNet> nets = group->getPowerNets();

  expect_names(nets, {"n3", "n2", "n1"});
  nets.reverse();
  expect_names(nets, {"n1", "n2", "n3"});

  group->addGroundNet(n1);
  group->addGroundNet(n2);
  group->addGroundNet(n3);

  nets = group->getGroundNets();
  EXPECT_TRUE(nets.reversible());

  expect_names(nets, {"n1", "n2", "n3"});
  nets.reverse();
  expect_names(nets, {"n3", "n2", "n1"});
}
TEST_F(GroupFixture, test_group_group_iterator)
{
  dbSet<dbGroup> children = group->getGroups();

  expect_str_names(children, {"child3", "child2", "child1"});

  EXPECT_TRUE(children.reversible());
  children.reverse();

  expect_str_names(children, {"child1", "child2", "child3"});
}

}  // namespace
}  // namespace odb
