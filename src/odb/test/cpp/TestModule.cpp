#include <cstdio>
#include <sstream>
#include <string>

#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"
#include "odb/dbSet.h"

namespace odb {
namespace {

/*
  Extract the hierarchical information in human readable format.
  Shows the dbNet and dbModNet view of the database.
*/

void DbStrDebugHierarchy(dbBlock* block, std::stringstream& str_db)
{
  char tmp_str[10000];
  sprintf(tmp_str,
          "Debug: Data base tables for block at %s:\n",
          block->getName().c_str());
  str_db << tmp_str;

  sprintf(tmp_str, "Db nets (The Flat db view)\n");
  str_db << tmp_str;

  for (auto dbnet : block->getNets()) {
    sprintf(tmp_str,
            "dbnet %s (id %u)\n",
            dbnet->getName().c_str(),
            dbnet->getId());
    str_db << tmp_str;

    for (auto db_iterm : dbnet->getITerms()) {
      sprintf(tmp_str,
              "\t-> Db Iterm %u (%s)\n",
              db_iterm->getId(),
              db_iterm->getName().c_str());
      str_db << tmp_str;
    }
    for (auto db_bterm : dbnet->getBTerms()) {
      sprintf(tmp_str, "\t-> Db Bterm %u\n", db_bterm->getId());
      str_db << tmp_str;
    }
  }

  sprintf(tmp_str, "Block ports\n");
  // got through the ports and their owner
  sprintf(tmp_str, "\t\tBTerm Ports +++\n");
  str_db << tmp_str;
  dbSet<dbBTerm> block_bterms = block->getBTerms();
  for (auto bt : block_bterms) {
    sprintf(tmp_str,
            "\t\tBterm (%u) %s Net %s (%u)  Mod Net %s (%u) \n",
            bt->getId(),
            bt->getName().c_str(),
            bt->getNet() ? bt->getNet()->getName().c_str() : "",
            bt->getNet() ? bt->getNet()->getId() : 0,
            bt->getModNet() ? bt->getModNet()->getName().c_str() : "",
            bt->getModNet() ? bt->getModNet()->getId() : 0);
    str_db << tmp_str;
  }
  sprintf(tmp_str, "\t\tBTerm Ports ---\n");
  str_db << tmp_str;

  sprintf(tmp_str, "The hierarchical db view:\n");
  str_db << tmp_str;
  dbSet<dbModule> block_modules = block->getModules();
  sprintf(tmp_str, "Content size %u modules\n", block_modules.size());
  str_db << tmp_str;
  for (auto mi : block_modules) {
    dbModule* cur_obj = mi;
    if (cur_obj == block->getTopModule()) {
      sprintf(tmp_str, "Top Module\n");
      str_db << tmp_str;
    }
    sprintf(tmp_str,
            "\tModule %s %s\n",
            (cur_obj == block->getTopModule()) ? "(Top Module)" : "",
            (cur_obj)->getName());
    str_db << tmp_str;
    // in case of top level, care as the bterms double up as pins
    if (cur_obj == block->getTopModule()) {
      for (auto bterm : block->getBTerms()) {
        sprintf(tmp_str,
                "Top B-term %s dbNet %s (%d) modNet %s (%d)\n",
                bterm->getName().c_str(),
                bterm->getNet() ? bterm->getNet()->getName().c_str() : "",
                bterm->getNet() ? bterm->getNet()->getId() : -1,
                bterm->getModNet() ? bterm->getModNet()->getName().c_str() : "",
                bterm->getModNet() ? bterm->getModNet()->getId() : -1);
        str_db << tmp_str;
      }
    }
    // got through the module ports and their owner
    sprintf(tmp_str, "\t\tModBTerm Ports +++\n");
    str_db << tmp_str;

    dbSet<dbModBTerm> module_ports = cur_obj->getModBTerms();
    for (auto module_port : module_ports) {
      sprintf(
          tmp_str,
          "\t\tPort %s Net %s (%d)\n",
          module_port->getName(),
          (module_port->getModNet())
              ? (module_port->getModNet()->getName().c_str())
              : "No-modnet",
          (module_port->getModNet()) ? module_port->getModNet()->getId() : -1);
      str_db << tmp_str;
      sprintf(tmp_str,
              "\t\tPort parent %s\n\n",
              module_port->getParent()->getName());
      str_db << tmp_str;
    }
    sprintf(tmp_str, "\t\tModBTermPorts ---\n");
    str_db << tmp_str;

    sprintf(tmp_str, "\t\tModule instances +++\n");
    str_db << tmp_str;
    dbSet<dbModInst> module_instances = mi->getModInsts();
    for (auto module_inst : module_instances) {
      sprintf(tmp_str, "\t\tMod inst %s ", module_inst->getName());
      str_db << tmp_str;
      dbModule* master = module_inst->getMaster();
      sprintf(
          tmp_str, "\t\tMaster %s\n\n", module_inst->getMaster()->getName());
      str_db << tmp_str;
      dbBlock* owner = master->getOwner();
      if (owner != block) {
        sprintf(tmp_str, "\t\t\tMaster owner in wrong block\n");
        str_db << tmp_str;
      }
      sprintf(tmp_str, "\t\tConnections\n");
      str_db << tmp_str;
      for (dbModITerm* miterm_pin : module_inst->getModITerms()) {
        sprintf(tmp_str,
                "\t\t\tModIterm : %s (%u) Mod Net %s (%u) \n",
                miterm_pin->getName(),
                miterm_pin->getId(),
                miterm_pin->getModNet()
                    ? (miterm_pin->getModNet()->getName().c_str())
                    : "No-net",
                miterm_pin->getModNet() ? miterm_pin->getModNet()->getId() : 0);
        str_db << tmp_str;
      }
    }
    sprintf(tmp_str, "\t\tModule instances ---\n");
    str_db << tmp_str;
    sprintf(tmp_str, "\t\tDb instances +++\n");
    str_db << tmp_str;
    for (dbInst* db_inst : cur_obj->getInsts()) {
      sprintf(tmp_str, "\t\tdb inst %s\n", db_inst->getName().c_str());
      str_db << tmp_str;
      sprintf(tmp_str, "\t\tdb iterms:\n");
      str_db << tmp_str;
      dbSet<dbITerm> iterms = db_inst->getITerms();
      dbSet<dbITerm>::iterator iterm_itr;
      for (iterm_itr = iterms.begin(); iterm_itr != iterms.end(); ++iterm_itr) {
        dbITerm* iterm = *iterm_itr;
        dbMTerm* mterm = iterm->getMTerm();
        sprintf(
            tmp_str,
            "\t\t\t\t iterm: %s (%u) Net: %s Mod net : %s (%d)\n",
            mterm->getName().c_str(),
            iterm->getId(),
            iterm->getNet() ? iterm->getNet()->getName().c_str() : "unk-dbnet",
            iterm->getModNet() ? iterm->getModNet()->getName().c_str()
                               : "unk-modnet",
            iterm->getModNet() ? iterm->getModNet()->getId() : -1);
        str_db << tmp_str;
      }
    }
    sprintf(tmp_str, "\t\tDb instances ---\n");
    str_db << tmp_str;
    sprintf(tmp_str, "\tModule nets (modnets) +++ \n");
    str_db << tmp_str;
    sprintf(tmp_str,
            "\t# mod nets %u in %s \n",
            cur_obj->getModNets().size(),
            cur_obj->getName());

    str_db << tmp_str;
    dbSet<dbModNet> mod_nets = cur_obj->getModNets();
    for (auto mod_net : mod_nets) {
      sprintf(tmp_str,
              "\t\tNet: %s (%u)\n",
              mod_net->getName().c_str(),
              mod_net->getId());
      str_db << tmp_str;
      sprintf(tmp_str,
              "\t\tConnections -> modIterms/modbterms/bterms/iterms:\n");
      str_db << tmp_str;
      sprintf(
          tmp_str, "\t\t -> %u moditerms\n", mod_net->getModITerms().size());
      str_db << tmp_str;
      for (dbModITerm* modi_term : mod_net->getModITerms()) {
        sprintf(tmp_str, "\t\t\t%s\n", modi_term->getName());
        str_db << tmp_str;
      }
      sprintf(
          tmp_str, "\t\t -> %u modbterms\n", mod_net->getModBTerms().size());
      str_db << tmp_str;
      for (dbModBTerm* modb_term : mod_net->getModBTerms()) {
        sprintf(tmp_str, "\t\t\t%s\n", modb_term->getName());
        str_db << tmp_str;
      }
      sprintf(tmp_str, "\t\t -> %u iterms\n", mod_net->getITerms().size());
      str_db << tmp_str;
      for (dbITerm* db_iterm : mod_net->getITerms()) {
        sprintf(tmp_str, "\t\t\t%s\n", db_iterm->getName().c_str());
        str_db << tmp_str;
      }
      sprintf(tmp_str, "\t\t -> %u bterms\n", mod_net->getBTerms().size());
      str_db << tmp_str;
      for (dbBTerm* db_bterm : mod_net->getBTerms()) {
        sprintf(tmp_str, "\t\t\t%s\n", db_bterm->getName().c_str());
        str_db << tmp_str;
      }
    }
  }
}

class ModuleFixture : public SimpleDbFixture
{
 protected:
  ModuleFixture()
  {
    createSimpleDB();
    block_ = db_->getChip()->getBlock();
    lib_ = db_->findLib("lib1");
  }
  dbLib* lib_;
  dbBlock* block_;
};

TEST_F(ModuleFixture, test_default)
{
  // dbModule::create() Succeed
  EXPECT_NE(dbModule::create(block_, "parent_mod"), nullptr);
  dbModule* master_mod = dbModule::create(block_, "master_mod");
  // dbModule::create() rejected
  EXPECT_EQ(dbModule::create(block_, "parent_mod"), nullptr);
  // dbBlock::findModule()
  dbModule* parent_mod = block_->findModule("parent_mod");
  ASSERT_NE(parent_mod, nullptr);
  // dbModule::getName()
  EXPECT_EQ(std::string(parent_mod->getName()), "parent_mod");
  // dbModInst::create() Succeed
  EXPECT_NE(dbModInst::create(parent_mod, master_mod, "i1"), nullptr);
  // dbModInst::create() rejected duplicate name
  EXPECT_EQ(dbModInst::create(parent_mod, master_mod, "i1"), nullptr);
  // dbModInst::create() rejected master already has a modinst
  EXPECT_EQ(dbModInst::create(parent_mod, master_mod, "i2"), nullptr);
  // dbModule::findModInst()1
  dbModInst* modInst = parent_mod->findModInst("i1");
  // dbModule getModInst()
  EXPECT_EQ(master_mod->getModInst(), modInst);
  // dbModInst::getName()
  EXPECT_EQ(std::string(modInst->getName()), "i1");
  // dbModule::getChildren()
  EXPECT_EQ(parent_mod->getChildren().size(), 1);
  // dbBlock::getModInsts()
  EXPECT_EQ(block_->getModInsts().size(), 1);
  // dbInst <--> dbModule
  auto inst1 = dbInst::create(block_, lib_->findMaster("and2"), "inst1");
  auto inst2 = dbInst::create(block_, lib_->findMaster("and2"), "inst2");
  // dbModule::addInst()
  parent_mod->addInst(inst1);
  parent_mod->addInst(inst2);
  // dbModule::getInsts()
  EXPECT_EQ(parent_mod->getInsts().size(), 2);
  // dbInst::getModule()
  EXPECT_EQ(std::string(inst1->getModule()->getName()), "parent_mod");
  // dbModule::addInst() new parent
  block_->getTopModule()->addInst(inst2);
  EXPECT_EQ(parent_mod->getInsts().size(), 1);
  EXPECT_EQ(inst2->getModule(), block_->getTopModule());
  // dbInst::destroy -> dbModule insts
  dbInst::destroy(inst1);
  EXPECT_EQ(parent_mod->getInsts().size(), 0);
}
TEST_F(ModuleFixture, test_find_modinst)
{
  auto top = block_->getTopModule();
  ASSERT_NE(top, nullptr);
  auto master1 = odb::dbModule::create(block_, "master1");
  odb::dbModInst::create(top, master1, "minst1");
  auto master2 = odb::dbModule::create(block_, "master2");
  auto minst2 = odb::dbModInst::create(master1, master2, "minst2");
  EXPECT_EQ(block_->findModInst("minst1/minst2"), minst2);
}
class DetailedFixture : public SimpleDbFixture
{
 protected:
  DetailedFixture()
  {
    createSimpleDB();
    block = db_->getChip()->getBlock();
    lib = db_->findLib("lib1");
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
TEST_F(DetailedFixture, test_destroy)
{
  EXPECT_EQ(block->getModInsts().size(), 3);

  // We now require that the module instance be deleted
  dbModInst::destroy(i1);
  dbModule::destroy(master_mod1);
  EXPECT_EQ(parent_mod->findModInst("i1"), nullptr);

  EXPECT_EQ(block->getModInsts().size(), 2);
  dbModInst::destroy(i2);
  dbModule::destroy(master_mod2);

  EXPECT_EQ(parent_mod->findModInst("i2"), nullptr);
  EXPECT_EQ(block->getModInsts().size(), 1);

  dbModule::destroy(parent_mod);
  EXPECT_EQ(block->findModule("parent_mod"), nullptr);
  EXPECT_EQ(block->getModInsts().size(), 0);
}

TEST_F(DetailedFixture, test_iterators)
{
  int i;
  dbSet<dbModInst> children = parent_mod->getChildren();
  dbSet<dbModInst>::iterator modinst_itr;
  EXPECT_EQ(children.size(), 3);
  EXPECT_TRUE(children.reversible());
  for (int j = 0; j < 2; j++) {
    if (j == 1) {
      children.reverse();
    }
    for (modinst_itr = children.begin(), i = j ? 1 : 3;
         modinst_itr != children.end();
         ++modinst_itr, i = i + (j ? 1 : -1)) {
      switch (i) {
        case 1:
          EXPECT_EQ(*modinst_itr, i1);
          break;
        case 2:
          EXPECT_EQ(*modinst_itr, i2);
          break;
        case 3:
          EXPECT_EQ(*modinst_itr, i3);
          break;
        default:
          FAIL();
          break;
      }
    }
  }

  dbSet<dbInst> insts = parent_mod->getInsts();
  dbSet<dbInst>::iterator inst_itr;
  EXPECT_EQ(insts.size(), 3);
  EXPECT_TRUE(insts.reversible());
  for (int j = 0; j < 2; j++) {
    if (j == 1) {
      insts.reverse();
    }
    for (inst_itr = insts.begin(), i = j ? 1 : 3; inst_itr != insts.end();
         ++inst_itr, i = i + (j ? 1 : -1)) {
      switch (i) {
        case 1:
          EXPECT_EQ(*inst_itr, inst1);
          break;
        case 2:
          EXPECT_EQ(*inst_itr, inst2);
          break;
        case 3:
          EXPECT_EQ(*inst_itr, inst3);
          break;
        default:
          FAIL();
          break;
      }
    }
  }
}

struct HierarchyFixture : public SimpleDbFixture
{
 protected:
  HierarchyFixture()
  {
    createSimpleDB();
    block = db_->getChip()->getBlock();
    lib = db_->findLib("lib1");

    odb::dbDatabase::beginEco(block);

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
    inst3 = dbInst::create(block, lib->findMaster("and2"), "inst4");

    parent_mod->addInst(inst1);
    parent_mod->addInst(inst2);
    parent_mod->addInst(inst3);
    std::stringstream str_str;
    DbStrDebugHierarchy(block, str_str);
    unsigned signature = 0;
    const std::string str = str_str.str();
    for (const char* p = str.c_str(); p && *p != '\0'; p++) {
      signature += ((*p) * 5);
    }
    (void) (signature);
  }

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

TEST_F(HierarchyFixture, test_hierarchy)
{
  dbSet<dbModNet> parent_modnets = parent_mod->getModNets();
  EXPECT_EQ(parent_modnets.size(), 7);
  dbSet<dbModBTerm> parent_modbterms = parent_mod->getModBTerms();
  EXPECT_EQ(parent_modbterms.size(), 5);
  dbSet<dbModITerm> i1_moditerms = i1->getModITerms();
  EXPECT_EQ(i1_moditerms.size(), 3);
  dbSet<dbModITerm> i2_moditerms = i2->getModITerms();
  EXPECT_EQ(i2_moditerms.size(), 3);
  dbSet<dbModITerm> i3_moditerms = i3->getModITerms();
  EXPECT_EQ(i3_moditerms.size(), 3);

  int i;
  dbSet<dbModInst> children = parent_mod->getChildren();
  dbSet<dbModInst>::iterator modinst_itr;
  EXPECT_EQ(children.size(), 3);
  EXPECT_TRUE(children.reversible());
  for (int j = 0; j < 2; j++) {
    if (j == 1) {
      children.reverse();
    }
    for (modinst_itr = children.begin(), i = j ? 1 : 3;
         modinst_itr != children.end();
         ++modinst_itr, i = i + (j ? 1 : -1)) {
      switch (i) {
        case 1:
          EXPECT_EQ(*modinst_itr, i1);
          break;
        case 2:
          EXPECT_EQ(*modinst_itr, i2);
          break;
        case 3:
          EXPECT_EQ(*modinst_itr, i3);
          break;
        default:
          FAIL();
          break;
      }
    }
  }

  dbSet<dbInst> insts = parent_mod->getInsts();
  dbSet<dbInst>::iterator inst_itr;
  EXPECT_EQ(insts.size(), 3);
  EXPECT_TRUE(insts.reversible());
  for (int j = 0; j < 2; j++) {
    if (j == 1) {
      insts.reverse();
    }
    for (inst_itr = insts.begin(), i = j ? 1 : 3; inst_itr != insts.end();
         ++inst_itr, i = i + (j ? 1 : -1)) {
      switch (i) {
        case 1:
          EXPECT_EQ(*inst_itr, inst1);
          break;
        case 2:
          EXPECT_EQ(*inst_itr, inst2);
          break;
        case 3:
          EXPECT_EQ(*inst_itr, inst3);
          break;
        default:
          FAIL();
          break;
      }
    }
  }
}

/*
Test the hierarchical journalling
*/

TEST_F(HierarchyFixture, test_hierarchy_journalling)
{
  // undo the construction
  odb::dbDatabase::undoEco(block);
  // sanity check nothing left.
  dbSet<dbModNet> parent_modnets = parent_mod->getModNets();
  EXPECT_EQ(parent_modnets.size(), 0);
  dbSet<dbModBTerm> parent_modbterms = parent_mod->getModBTerms();
  EXPECT_EQ(parent_modbterms.size(), 0);
  dbSet<dbModITerm> i1_moditerms = i1->getModITerms();
  EXPECT_EQ(i1_moditerms.size(), 0);
  dbSet<dbModITerm> i2_moditerms = i2->getModITerms();
  EXPECT_EQ(i2_moditerms.size(), 0);
  dbSet<dbModITerm> i3_moditerms = i3->getModITerms();
  EXPECT_EQ(i3_moditerms.size(), 0);
}

}  // namespace
}  // namespace odb
