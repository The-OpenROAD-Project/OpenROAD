#define BOOST_TEST_MODULE TestModule
#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <string>

#include "db.h"
#include "helper.h"

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
  }
  ~F_DEFAULT() { dbDatabase::destroy(db); }
  dbDatabase* db;
  dbLib* lib;
  dbBlock* block;
};

BOOST_FIXTURE_TEST_CASE(test_default, F_DEFAULT)
{
  // dbModule::create() Succeed
  BOOST_TEST(dbModule::create(block, "parent_mod") != nullptr);
  dbModule* master_mod = dbModule::create(block, "master_mod");
  // dbModule::create() rejected
  BOOST_TEST(dbModule::create(block, "parent_mod") == nullptr);
  // dbBlock::findModule()
  dbModule* parent_mod = block->findModule("parent_mod");
  BOOST_TEST(parent_mod != nullptr);
  // dbModule::getName()
  BOOST_TEST(std::string(parent_mod->getName()) == "parent_mod");
  // dbModInst::create() Succeed
  BOOST_TEST(dbModInst::create(parent_mod, master_mod, "i1") != nullptr);
  // dbModInst::create() rejected duplicate name
  BOOST_TEST(dbModInst::create(parent_mod, master_mod, "i1") == nullptr);
  // dbModInst::create() rejected master already has a modinst
  BOOST_TEST(dbModInst::create(parent_mod, master_mod, "i2") == nullptr);
  // dbModule::findModInst()1
  dbModInst* modInst = parent_mod->findModInst("i1");
  // dbModule getModInst()
  BOOST_TEST(master_mod->getModInst() == modInst);
  // dbModInst::getName()
  BOOST_TEST(std::string(modInst->getName()) == "i1");
  // dbModule::getChildren()
  BOOST_TEST(parent_mod->getChildren().size() == 1);
  // dbBlock::getModInsts()
  BOOST_TEST(block->getModInsts().size() == 1);
  // dbInst <--> dbModule
  auto inst1 = dbInst::create(block, lib->findMaster("and2"), "inst1");
  auto inst2 = dbInst::create(block, lib->findMaster("and2"), "inst2");
  // dbModule::addInst()
  parent_mod->addInst(inst1);
  parent_mod->addInst(inst2);
  // dbModule::getInsts()
  BOOST_TEST(parent_mod->getInsts().size() == 2);
  // dbInst::getModule()
  BOOST_TEST(std::string(inst1->getModule()->getName()) == "parent_mod");
  // dbModule::addInst() new parent
  block->getTopModule()->addInst(inst2);
  BOOST_TEST(parent_mod->getInsts().size() == 1);
  BOOST_TEST(inst2->getModule() == block->getTopModule());
  // dbInst::destroy -> dbModule insts
  dbInst::destroy(inst1);
  BOOST_TEST(parent_mod->getInsts().size() == 0);
}
BOOST_FIXTURE_TEST_CASE(test_find_modinst, F_DEFAULT)
{
  auto top = block->getTopModule();
  BOOST_TEST(top != nullptr);
  auto master1 = odb::dbModule::create(block, "master1");
  odb::dbModInst::create(top, master1, "minst1");
  auto master2 = odb::dbModule::create(block, "master2");
  auto minst2 = odb::dbModInst::create(master1, master2, "minst2");
  BOOST_TEST(block->findModInst("minst1/minst2") == minst2);
}
struct F_DETAILED
{
  F_DETAILED()
  {
    db = createSimpleDB();
    block = db->getChip()->getBlock();
    lib = db->findLib("lib1");
    master_mod1 = dbModule::create(block, "master_mod1");
    master_mod2 = dbModule::create(block, "master_mod2");
    master_mod3 = dbModule::create(block, "master_mod3");
    parent_mod = dbModule::create(block, "parent_mod");
    i1 = dbModInst::create(parent_mod, master_mod1, "i1");
    i2 = dbModInst::create(parent_mod, master_mod2, "i2");
    i3 = dbModInst::create(parent_mod, master_mod3, "i3");
    inst1 = dbInst::create(block, lib->findMaster("and2"), "inst1");
    inst2 = dbInst::create(block, lib->findMaster("and2"), "inst2");
    inst3 = dbInst::create(block, lib->findMaster("and2"), "inst3");
    parent_mod->addInst(inst1);
    parent_mod->addInst(inst2);
    parent_mod->addInst(inst3);
  }
  ~F_DETAILED() { dbDatabase::destroy(db); }
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
};
BOOST_FIXTURE_TEST_CASE(test_destroy, F_DETAILED)
{
  BOOST_TEST(block->getModInsts().size() == 3);
  // dbModInst::destroy()
  dbModInst::destroy(i1);
  BOOST_TEST(parent_mod->findModInst("i1") == nullptr);
  BOOST_TEST(block->getModInsts().size() == 2);
  // dbModule::destroy() master
  dbModule::destroy(master_mod2);
  BOOST_TEST(parent_mod->findModInst("i2") == nullptr);
  BOOST_TEST(block->getModInsts().size() == 1);
  // dbModule::destroy() parent
  dbModule::destroy(parent_mod);
  BOOST_TEST(block->findModule("parent_mod") == nullptr);
  BOOST_TEST(block->getModInsts().size() == 0);
}
BOOST_FIXTURE_TEST_CASE(test_iterators, F_DETAILED)
{
  int i;
  dbSet<dbModInst> children = parent_mod->getChildren();
  dbSet<dbModInst>::iterator modinst_itr;
  BOOST_TEST(children.size() == 3);
  BOOST_TEST(children.reversible());
  for (int j = 0; j < 2; j++) {
    if (j == 1) {
      children.reverse();
    }
    for (modinst_itr = children.begin(), i = j ? 1 : 3;
         modinst_itr != children.end();
         ++modinst_itr, i = i + (j ? 1 : -1)) {
      switch (i) {
        case 1:
          BOOST_TEST(*modinst_itr == i1);
          break;
        case 2:
          BOOST_TEST(*modinst_itr == i2);
          break;
        case 3:
          BOOST_TEST(*modinst_itr == i3);
          break;
        default:
          BOOST_TEST(false);
          break;
      }
    }
  }

  dbSet<dbInst> insts = parent_mod->getInsts();
  dbSet<dbInst>::iterator inst_itr;
  BOOST_TEST(insts.size() == 3);
  BOOST_TEST(insts.reversible());
  for (int j = 0; j < 2; j++) {
    if (j == 1) {
      insts.reverse();
    }
    for (inst_itr = insts.begin(), i = j ? 1 : 3; inst_itr != insts.end();
         ++inst_itr, i = i + (j ? 1 : -1)) {
      switch (i) {
        case 1:
          BOOST_TEST(*inst_itr == inst1);
          break;
        case 2:
          BOOST_TEST(*inst_itr == inst2);
          break;
        case 3:
          BOOST_TEST(*inst_itr == inst3);
          break;
        default:
          BOOST_TEST(false);
          break;
      }
    }
  }
}

struct F_HIERARCHY
{
  F_HIERARCHY()
  {
    db = createSimpleDB();
    block = db->getChip()->getBlock();
    lib = db->findLib("lib1");

    // Top level object
    parent_mod = dbModule::create(block, "parent_mod");
    // Top level ports
    parent_modbterm_ip1 = dbModBTerm::create(parent_mod, "ip1");
    parent_modbterm_ip2 = dbModBTerm::create(parent_mod, "ip2");
    parent_modbterm_ip3 = dbModBTerm::create(parent_mod, "ip3");
    parent_modbterm_ip4 = dbModBTerm::create(parent_mod, "ip4");
    parent_modbterm_op1 = dbModBTerm::create(parent_mod, "op1");
    // Top level nets
    mod_net_ip1 = dbModNet::create(parent_mod, "ip1");
    mod_net_ip2 = dbModNet::create(parent_mod, "ip2");
    mod_net_ip3 = dbModNet::create(parent_mod, "ip3");
    mod_net_ip4 = dbModNet::create(parent_mod, "ip4");
    mod_net_int1 = dbModNet::create(parent_mod, "int1");
    mod_net_int2 = dbModNet::create(parent_mod, "int2");
    mod_net_op1 = dbModNet::create(parent_mod, "op1");

    parent_modbterm_ip1->connect(mod_net_ip1);
    parent_modbterm_ip2->connect(mod_net_ip2);
    parent_modbterm_ip3->connect(mod_net_ip3);
    parent_modbterm_ip4->connect(mod_net_ip4);
    parent_modbterm_op1->connect(mod_net_op1);

    // child instances
    master_mod1 = dbModule::create(block, "master_mod1");
    master_mod2 = dbModule::create(block, "master_mod2");
    master_mod3 = dbModule::create(block, "master_mod3");

    i1 = dbModInst::create(parent_mod, master_mod1, "i1");
    mod_i1_a1 = dbModITerm::create(i1, "a1");
    mod_i1_a2 = dbModITerm::create(i1, "a2");
    mod_i1_z = dbModITerm::create(i1, "z");

    // hook up i1
    mod_i1_a1->connect(mod_net_ip1);
    mod_i1_a2->connect(mod_net_ip2);
    mod_i1_z->connect(mod_net_int1);

    i2 = dbModInst::create(parent_mod, master_mod2, "i2");
    mod_i2_a1 = dbModITerm::create(i2, "a1");
    mod_i2_a2 = dbModITerm::create(i2, "a2");
    mod_i2_z = dbModITerm::create(i2, "z");

    // hook up i2
    mod_i2_a1->connect(mod_net_ip3);
    mod_i2_a2->connect(mod_net_ip4);
    mod_i2_z->connect(mod_net_int2);

    i3 = dbModInst::create(parent_mod, master_mod3, "i3");
    mod_i3_a1 = dbModITerm::create(i3, "a1");
    mod_i3_a2 = dbModITerm::create(i3, "a2");
    mod_i3_z = dbModITerm::create(i3, "z");

    // hook up i3
    mod_i3_a1->connect(mod_net_int1);
    mod_i3_a2->connect(mod_net_int2);
    mod_i3_z->connect(mod_net_op1);

    inst1 = dbInst::create(block, lib->findMaster("and2"), "inst1");
    inst2 = dbInst::create(block, lib->findMaster("and2"), "inst2");
    inst3 = dbInst::create(block, lib->findMaster("and2"), "inst3");

    parent_mod->addInst(inst1);
    parent_mod->addInst(inst2);
    parent_mod->addInst(inst3);
  }

  ~F_HIERARCHY() { dbDatabase::destroy(db); }
  dbDatabase* db;
  dbLib* lib;
  dbBlock* block;

  // 2 and gates -> and gate -> op1
  dbModBTerm* parent_modbterm_ip1;
  dbModBTerm* parent_modbterm_ip2;
  dbModBTerm* parent_modbterm_ip3;
  dbModBTerm* parent_modbterm_ip4;
  dbModBTerm* parent_modbterm_op1;

  dbModITerm* mod_i1_a1;
  dbModITerm* mod_i1_a2;
  dbModITerm* mod_i1_z;

  dbModITerm* mod_i2_a1;
  dbModITerm* mod_i2_a2;
  dbModITerm* mod_i2_z;

  dbModITerm* mod_i3_a1;
  dbModITerm* mod_i3_a2;
  dbModITerm* mod_i3_z;

  dbModNet* mod_net_ip1;
  dbModNet* mod_net_ip2;
  dbModNet* mod_net_ip3;
  dbModNet* mod_net_ip4;
  dbModNet* mod_net_int1;
  dbModNet* mod_net_int2;
  dbModNet* mod_net_op1;

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
};

BOOST_FIXTURE_TEST_CASE(test_hierarchy, F_HIERARCHY)
{
  dbSet<dbModNet> parent_modnets = parent_mod->getModNets();
  BOOST_TEST(parent_modnets.size() == 7);
  dbSet<dbModBTerm> parent_modbterms = parent_mod->getModBTerms();
  BOOST_TEST(parent_modbterms.size() == 5);
  dbSet<dbModITerm> i1_moditerms = i1->getModITerms();
  BOOST_TEST(i1_moditerms.size() == 3);
  dbSet<dbModITerm> i2_moditerms = i2->getModITerms();
  BOOST_TEST(i2_moditerms.size() == 3);
  dbSet<dbModITerm> i3_moditerms = i3->getModITerms();
  BOOST_TEST(i3_moditerms.size() == 3);

  int i;
  dbSet<dbModInst> children = parent_mod->getChildren();
  dbSet<dbModInst>::iterator modinst_itr;
  BOOST_TEST(children.size() == 3);
  BOOST_TEST(children.reversible());
  for (int j = 0; j < 2; j++) {
    if (j == 1) {
      children.reverse();
    }
    for (modinst_itr = children.begin(), i = j ? 1 : 3;
         modinst_itr != children.end();
         ++modinst_itr, i = i + (j ? 1 : -1)) {
      switch (i) {
        case 1:
          BOOST_TEST(*modinst_itr == i1);
          break;
        case 2:
          BOOST_TEST(*modinst_itr == i2);
          break;
        case 3:
          BOOST_TEST(*modinst_itr == i3);
          break;
        default:
          BOOST_TEST(false);
          break;
      }
    }
  }

  dbSet<dbInst> insts = parent_mod->getInsts();
  dbSet<dbInst>::iterator inst_itr;
  BOOST_TEST(insts.size() == 3);
  BOOST_TEST(insts.reversible());
  for (int j = 0; j < 2; j++) {
    if (j == 1) {
      insts.reverse();
    }
    for (inst_itr = insts.begin(), i = j ? 1 : 3; inst_itr != insts.end();
         ++inst_itr, i = i + (j ? 1 : -1)) {
      switch (i) {
        case 1:
          BOOST_TEST(*inst_itr == inst1);
          break;
        case 2:
          BOOST_TEST(*inst_itr == inst2);
          break;
        case 3:
          BOOST_TEST(*inst_itr == inst3);
          break;
        default:
          BOOST_TEST(false);
          break;
      }
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace
}  // namespace odb
