#define BOOST_TEST_MODULE TestGroup
#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include "db.h"
#include "helper.cpp"
#include <string>

using namespace odb;
using namespace std;
struct F_DETAILED {
  F_DETAILED()
  {
    db        = create2LevetDbNoBTerms();
    block     = db->getChip()->getBlock();
    lib       = db->findLib("lib1");
    master_mod1 = dbModule::create(block,"master_mod1");
    master_mod2 = dbModule::create(block,"master_mod2");
    master_mod3 = dbModule::create(block,"master_mod3");
    parent_mod = dbModule::create(block,"parent_mod");
    i1 = dbModInst::create(parent_mod,master_mod1,"i1");
    i2 = dbModInst::create(parent_mod,master_mod2,"i2");
    i3 = dbModInst::create(parent_mod,master_mod3,"i3");
    inst1 = block->findInst("i1");
    inst2 = block->findInst("i2");
    inst3 = block->findInst("i3");
    n1 = block->findNet("n1");
    n2 = block->findNet("n2");
    n3 = block->findNet("n3");
    group  = dbGroup::create(block,"group");
    domain = dbGroup::create(block,"domain",0,0,100,100);
    child1 = dbGroup::create(block,"child1");
    child2 = dbGroup::create(block,"child2");
    child3 = dbGroup::create(block,"child3");
    
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
  dbNet*      n1;
  dbNet*      n2;
  dbNet*      n3;
  dbGroup*    group;
  dbGroup*    domain;
  dbGroup*    child1;
  dbGroup*    child2;
  dbGroup*    child3;
};
BOOST_FIXTURE_TEST_SUITE(test_suite,F_DETAILED)
BOOST_AUTO_TEST_CASE(test_group_default)
{
  BOOST_TEST (group!=nullptr);
  BOOST_TEST (dbGroup::create(block,"group")==nullptr);
  BOOST_TEST (block->getGroups().size()==5);
  dbGroup* new_group = block->findGroup("group");
  BOOST_TEST (new_group!=nullptr);
  BOOST_TEST (std::string(new_group->getName())=="group");
  dbGroup::destroy(new_group);
  BOOST_TEST (block->getGroups().size()==4);
  BOOST_TEST (block->findGroup("group")==nullptr);

  BOOST_TEST((domain->getBox() == Rect(0,0,100,100)));
  BOOST_TEST(domain->hasBox());
  BOOST_TEST(!child1->hasBox());
  domain->setBox(Rect(2,2,50,50));
  BOOST_TEST((domain->getBox() == Rect(2,2,50,50)));
  
  BOOST_TEST(child1->getType() == dbGroup::PHYSICAL_CLUSTER);
  BOOST_TEST(domain->getType() == dbGroup::VOLTAGE_DOMAIN);
  domain->setType(dbGroup::PHYSICAL_CLUSTER);
  BOOST_TEST(domain->getType() == dbGroup::PHYSICAL_CLUSTER);

}
BOOST_AUTO_TEST_CASE(test_group_modinst)
{
  auto insts = group->getModInsts();
  BOOST_TEST(insts.size()==3);
  BOOST_TEST(*insts.begin()==i3);
  BOOST_TEST(i3->getGroup()==group);
  group->removeModInst(i3);
  BOOST_TEST(insts.size()==2);
  BOOST_TEST(i3->getGroup()==nullptr);
  BOOST_TEST(*insts.begin()==i2); 
  dbModInst::destroy(i2);
  BOOST_TEST(insts.size()==1);
  BOOST_TEST(*insts.begin()==i1); 
  dbGroup::destroy(group);
  BOOST_TEST(i1->getGroup()==nullptr); 
}
BOOST_AUTO_TEST_CASE(test_group_inst)
{  
  auto insts = group->getInsts();
  BOOST_TEST(insts.size()==3);
  BOOST_TEST(*insts.begin()==inst3);
  BOOST_TEST(inst3->getGroup()==group);
  group->removeInst(inst3);
  BOOST_TEST(insts.size()==2);
  BOOST_TEST(inst3->getGroup()==nullptr);
  BOOST_TEST(*insts.begin()==inst2); 
  dbInst::destroy(inst2);
  BOOST_TEST(insts.size()==1);
  BOOST_TEST(*insts.begin()==inst1); 
  dbGroup::destroy(group);
  BOOST_TEST(inst1->getGroup()==nullptr); 
}
BOOST_AUTO_TEST_CASE(test_group_net)
{
  auto power_nets = domain->getPowerNets();
  auto ground_nets = domain->getGroundNets();
  BOOST_TEST(power_nets.size()==3);
  BOOST_TEST(ground_nets.size()==0);
  BOOST_TEST(*power_nets.begin()==n1);
  domain->removeNet(n1);
  BOOST_TEST(power_nets.size()==2);
  BOOST_TEST(*power_nets.begin()==n2);
  dbNet::destroy(n2);
  BOOST_TEST(power_nets.size()==1);
  BOOST_TEST(*power_nets.begin()==n3);
  domain->addGroundNet(n3);
  BOOST_TEST(power_nets.size()==0);
  BOOST_TEST(ground_nets.size()==1);
  BOOST_TEST(*ground_nets.begin()==n3);
}
BOOST_AUTO_TEST_CASE(test_group_group)
{
  auto groups = group->getGroups();
  BOOST_TEST(groups.size()==3);
  BOOST_TEST(*groups.begin()==child3);
  BOOST_TEST(child3->getParentGroup()==group);
  group->removeGroup(child3);
  BOOST_TEST(groups.size()==2);
  BOOST_TEST(child3->getParentGroup()==nullptr);
  BOOST_TEST(*groups.begin()==child2); 
  dbGroup::destroy(child2);
  BOOST_TEST(groups.size()==1);
  BOOST_TEST(*groups.begin()==child1); 
  dbGroup::destroy(group);
  BOOST_TEST(child1->getParentGroup()==nullptr); 
}
BOOST_AUTO_TEST_CASE(test_group_modinst_iterator)
{
  dbSet<dbModInst> modinsts = group->getModInsts();
  dbSet<dbModInst>::iterator modinst_itr;
  int i;
  BOOST_TEST(modinsts.reversible());
  for(int j = 0; j<2 ; j++)
  {
    if(j==1)
      modinsts.reverse();
    for(modinst_itr = modinsts.begin() , i=j?1:3 ; modinst_itr != modinsts.end() ; ++modinst_itr, i=i+(j?1:-1))
      BOOST_TEST(std::string(((dbModInst*)*modinst_itr)->getName())=="i"+to_string(i));
  }
  
}
BOOST_AUTO_TEST_CASE(test_group_inst_iterator)
{
  dbSet<dbInst> insts = group->getInsts();
  dbSet<dbInst>::iterator inst_itr;
  int i;
  BOOST_TEST(insts.reversible());
  for(int j = 0; j<2 ; j++)
  {
    if(j==1)
      insts.reverse();
    for(inst_itr = insts.begin() , i=j?1:3 ; inst_itr != insts.end() ; ++inst_itr, i=i+(j?1:-1))
      BOOST_TEST(std::string(((dbInst*)*inst_itr)->getName())=="i"+to_string(i));
  }
}
BOOST_AUTO_TEST_CASE(test_group_net_iterators)
{
  dbSet<dbNet> nets = group->getPowerNets();
  dbSet<dbNet>::iterator net_itr;
  int i;
  BOOST_TEST(nets.reversible());
  for(int j = 0; j<2 ; j++)
  {
    if(j==1)
      nets.reverse();
    for(net_itr = nets.begin() , i=j?3:1 ; net_itr != nets.end() ; ++net_itr, i=i+(j?-1:1))
      BOOST_TEST(std::string(((dbNet*)*net_itr)->getName())=="n"+to_string(i));
  }
  group->addGroundNet(n1);
  group->addGroundNet(n2);
  group->addGroundNet(n3);

  nets = group->getGroundNets();
  BOOST_TEST(nets.reversible());
  for(int j = 0; j<2 ; j++)
  {
    if(j==1)
      nets.reverse();
    for(net_itr = nets.begin() , i=j?3:1 ; net_itr != nets.end() ; ++net_itr, i=i+(j?-1:1))
      BOOST_TEST(std::string(((dbNet*)*net_itr)->getName())=="n"+to_string(i));
  }
}
BOOST_AUTO_TEST_CASE(test_group_group_iterator)
{
  dbSet<dbGroup> children = group->getGroups();
  dbSet<dbGroup>::iterator group_itr;
  int i;
  BOOST_TEST(children.reversible());
  for(int j = 0; j<2 ; j++)
  {
    if(j==1)
      children.reverse();
    for(group_itr = children.begin() , i=j?1:3 ; group_itr != children.end() ; ++group_itr, i=i+(j?1:-1))
      BOOST_TEST(std::string(((dbGroup*)*group_itr)->getName())=="child"+to_string(i));
  }
}
BOOST_AUTO_TEST_SUITE_END()
