#define BOOST_TEST_MODULE TestModule
#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include "db.h"
#include "helper.cpp"
#include <string>

using namespace odb;
using namespace std;
BOOST_AUTO_TEST_SUITE(test_suite)

struct F_DEFAULT {
  F_DEFAULT()
  {
    db        = createSimpleDB();
    block     = db->getChip()->getBlock();
    lib       = db->findLib("lib1");
  }
  ~F_DEFAULT()
  {
    dbDatabase::destroy(db);
  }
  dbDatabase* db;
  dbLib*      lib;
  dbBlock*    block;
};
BOOST_FIXTURE_TEST_CASE(test_default,F_DEFAULT)
{
  //dbModule::create() Succeed
  BOOST_TEST(dbModule::create(block,"parent_mod")!=nullptr);
  dbModule* master_mod = dbModule::create(block,"master_mod");
  //dbModule::create() rejected
  BOOST_TEST(dbModule::create(block,"parent_mod")==nullptr);
  //dbBlock::findModule()
  dbModule* parent_mod = block->findModule("parent_mod");
  BOOST_TEST(parent_mod!=nullptr);
  //dbModule::getName()
  BOOST_TEST(string(parent_mod->getName())=="parent_mod");
  //dbModInst::create() Succeed
  BOOST_TEST(dbModInst::create(parent_mod,master_mod,"i1")!=nullptr);
  //dbModInst::create() rejected duplicate name
  BOOST_TEST(dbModInst::create(parent_mod,master_mod,"i1")==nullptr);
  //dbModInst::create() rejected master already has a modinst
  BOOST_TEST(dbModInst::create(parent_mod,master_mod,"i2")==nullptr);
  //dbModule::findModInst()1
  dbModInst* modInst = parent_mod->findModInst("i1");
  //dbModule getModInst()
  BOOST_TEST(master_mod->getModInst()==modInst);
  //dbModInst::getName()
  BOOST_TEST(string(modInst->getName())=="i1");
  //dbModule::getChildren()
  BOOST_TEST(parent_mod->getChildren().size()==1);
  //dbBlock::getModInsts()
  BOOST_TEST(block->getModInsts().size()==1);
  //dbInst <--> dbModule
  auto inst1 = dbInst::create(block,lib->findMaster("and2"),"inst1");
  auto inst2 = dbInst::create(block,lib->findMaster("and2"),"inst2");
  //dbModule::addInst()
  parent_mod->addInst(inst1);
  parent_mod->addInst(inst2);
  //dbModule::getInsts()
  BOOST_TEST(parent_mod->getInsts().size()==2);
  //dbInst::getModule()
  BOOST_TEST(std::string(inst1->getModule()->getName())=="parent_mod");
  //dbModule::removeInst()
  parent_mod->removeInst(inst2);
  BOOST_TEST(parent_mod->getInsts().size()==1);
  BOOST_TEST(inst2->getModule()==nullptr);
  //dbInst::destroy -> dbModule insts
  dbInst::destroy(inst1);
  BOOST_TEST(parent_mod->getInsts().size()==0);
}
struct F_DETAILED {
  F_DETAILED()
  {
    db        = createSimpleDB();
    block     = db->getChip()->getBlock();
    lib       = db->findLib("lib1");
    master_mod1 = dbModule::create(block,"master_mod1");
    master_mod2 = dbModule::create(block,"master_mod2");
    master_mod3 = dbModule::create(block,"master_mod3");
    parent_mod = dbModule::create(block,"parent_mod");
    i1 = dbModInst::create(parent_mod,master_mod1,"i1");
    i2 = dbModInst::create(parent_mod,master_mod2,"i2");
    i3 = dbModInst::create(parent_mod,master_mod3,"i3");
    inst1 = dbInst::create(block,lib->findMaster("and2"),"inst1");
    inst2 = dbInst::create(block,lib->findMaster("and2"),"inst2");
    inst3 = dbInst::create(block,lib->findMaster("and2"),"inst3");
    parent_mod->addInst(inst1);
    parent_mod->addInst(inst2);
    parent_mod->addInst(inst3);
  }
  ~F_DETAILED()
  {
    dbDatabase::destroy(db);
  }
  dbDatabase* db;
  dbLib*      lib;
  dbBlock*    block;
  dbModule*   master_mod1;
  dbModule*   master_mod2;
  dbModule*   master_mod3;
  dbModule*   parent_mod;
  dbModInst*  i1;
  dbModInst*  i2;
  dbModInst*  i3;
  dbInst*     inst1;
  dbInst*     inst2;
  dbInst*     inst3;
};
BOOST_FIXTURE_TEST_CASE(test_destroy,F_DETAILED)
{
  BOOST_TEST(block->getModInsts().size()==3);
  //dbModInst::destroy()
  dbModInst::destroy(i1);
  BOOST_TEST(parent_mod->findModInst("i1")==nullptr);
  BOOST_TEST(block->getModInsts().size()==2);
  //dbModule::destroy() master
  dbModule::destroy(master_mod2);
  BOOST_TEST(parent_mod->findModInst("i2")==nullptr);
  BOOST_TEST(block->getModInsts().size()==1);
  //dbModule::destroy() parent
  dbModule::destroy(parent_mod);
  BOOST_TEST(block->findModule("parent_mod")==nullptr);
  BOOST_TEST(block->getModInsts().size()==0);
}
BOOST_FIXTURE_TEST_CASE(test_iterators,F_DETAILED)
{
  int i;
  dbSet<dbModInst> children = parent_mod->getChildren();
  dbSet<dbModInst>::iterator modinst_itr;
  BOOST_TEST(children.size()==3);
  BOOST_TEST(children.reversible());
  for(int j = 0; j<2 ; j++)
  {
    if(j==1)
      children.reverse();
    for(modinst_itr = children.begin() , i=j?1:3 ; modinst_itr != children.end() ; ++modinst_itr, i=i+(j?1:-1))
      switch (i)
      {
      case 1:
        BOOST_TEST(*modinst_itr==i1);
        break;
      case 2:
        BOOST_TEST(*modinst_itr==i2);
        break;
      case 3:
        BOOST_TEST(*modinst_itr==i3);
        break;
      default:
        BOOST_TEST(false);
        break;
      }
  }
  
  dbSet<dbInst> insts = parent_mod->getInsts();
  dbSet<dbInst>::iterator inst_itr;
  BOOST_TEST(insts.size()==3);
  BOOST_TEST(insts.reversible());
  for(int j = 0; j<2 ; j++)
  {
    if(j==1)
      insts.reverse();
    for(inst_itr = insts.begin() , i=j?1:3 ; inst_itr != insts.end() ; ++inst_itr, i=i+(j?1:-1))
      switch (i)
      {
      case 1:
        BOOST_TEST(*inst_itr==inst1);
        break;
      case 2:
        BOOST_TEST(*inst_itr==inst2);
        break;
      case 3:
        BOOST_TEST(*inst_itr==inst3);
        break;
      default:
        BOOST_TEST(false);
        break;
      }
  }
}

BOOST_AUTO_TEST_SUITE_END()