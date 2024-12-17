#define BOOST_TEST_MODULE TestModule
#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <string>

#include "helper.h"
#include "odb/db.h"

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
            bt->getModNet() ? bt->getModNet()->getName() : "",
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
            ((dbModule*) cur_obj)->getName());
    str_db << tmp_str;
    // in case of top level, care as the bterms double up as pins
    if (cur_obj == block->getTopModule()) {
      for (auto bterm : block->getBTerms()) {
        sprintf(tmp_str,
                "Top B-term %s dbNet %s (%d) modNet %s (%d)\n",
                bterm->getName().c_str(),
                bterm->getNet() ? bterm->getNet()->getName().c_str() : "",
                bterm->getNet() ? bterm->getNet()->getId() : -1,
                bterm->getModNet() ? bterm->getModNet()->getName() : "",
                bterm->getModNet() ? bterm->getModNet()->getId() : -1);
        str_db << tmp_str;
      }
    }
    // got through the module ports and their owner
    sprintf(tmp_str, "\t\tModBTerm Ports +++\n");
    str_db << tmp_str;

    dbSet<dbModBTerm> module_ports = cur_obj->getModBTerms();
    for (dbSet<dbModBTerm>::iterator mod_bterm_iter = module_ports.begin();
         mod_bterm_iter != module_ports.end();
         mod_bterm_iter++) {
      dbModBTerm* module_port = *mod_bterm_iter;
      sprintf(
          tmp_str,
          "\t\tPort %s Net %s (%d)\n",
          module_port->getName(),
          (module_port->getModNet()) ? (module_port->getModNet()->getName())
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
    for (dbSet<dbModInst>::iterator mod_inst_iter = module_instances.begin();
         mod_inst_iter != module_instances.end();
         mod_inst_iter++) {
      dbModInst* module_inst = *mod_inst_iter;
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
                miterm_pin->getModNet() ? (miterm_pin->getModNet()->getName())
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
            iterm->getModNet() ? iterm->getModNet()->getName() : "unk-modnet",
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
    for (dbSet<dbModNet>::iterator mod_net_iter = mod_nets.begin();
         mod_net_iter != mod_nets.end();
         mod_net_iter++) {
      dbModNet* mod_net = *mod_net_iter;
      sprintf(
          tmp_str, "\t\tNet: %s (%u)\n", mod_net->getName(), mod_net->getId());
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


/* 
   Test fixture for hierarchical connect test
   ------------------------------------------

   Two module instances, 1 with 1 inverter, second with 4 inverters.
   Modules have different hierarchical depth and shared terms
   to stress test the hierarchical connection code.
   
   So any reconnection has to go up and down the hierarchy.

   top level module <-- {inv_module, 4inv_module}
   inv_module <-- 1 inverter
   
   
   Test:
   Add a buffer in module 1 and make sure connectivity ok
   (both hierarchical and flat).
 */

dbDatabase* createSimpleDBDelimiter()
{
  utl::Logger* logger = new utl::Logger();
  dbDatabase* db = dbDatabase::create();
  db->setLogger(logger);
  dbTech* tech = dbTech::create(db, "tech");
  dbTechLayer::create(tech, "L1", dbTechLayerType::MASTERSLICE);
  dbLib* lib = dbLib::create(db, "lib1", tech, ',');
  dbChip* chip = dbChip::create(db);
  //set up the delimiter
  dbBlock::create(chip, "simple_block",nullptr,'/');
  createMaster2X1(lib, "and2", 1000, 1000, "a", "b", "o");
  createMaster2X1(lib, "or2", 500, 500, "a", "b", "o");
  createMaster1X1(lib, "inv1", 500, 500, "ip0", "op0");  
  return db;
}


struct F_HCONNECT
{
  F_HCONNECT()
  {
    db = createSimpleDBDelimiter();
    block = db->getChip()->getBlock();
    lib = db->findLib("lib1");

    root_mod = dbModule::create(block, "root_mod") ;

    inv1_mod_master  = dbModule::create(block, "inv1_master_mod");
    dbModBTerm* inv1_mod_i0_port  = dbModBTerm::create(inv1_mod_master,"i0");
    dbModBTerm* inv1_mod_o0_port  = dbModBTerm::create(inv1_mod_master,"o0");    
    inv1_mod_inst = dbModInst::create(root_mod, inv1_mod_master, "inv1_mod_inst");
    
    
    inv4_mod_level0_master = dbModule::create(block, "inv4_master_level0");
    dbModBTerm* inv_mod_level0_master_i0_port  = dbModBTerm::create(inv4_mod_level0_master,"i0");
    dbModBTerm* inv_mod_level0_master_i1_port  = dbModBTerm::create(inv4_mod_level0_master,"i1");
    dbModBTerm* inv_mod_level0_master_i2_port  = dbModBTerm::create(inv4_mod_level0_master,"i2");
    dbModBTerm* inv_mod_level0_master_i3_port = dbModBTerm::create(inv4_mod_level0_master,"i3");
    dbModBTerm* inv_mod_level0_master_o0_port=  dbModBTerm::create(inv4_mod_level0_master,"o0");    
    
    
    inv4_mod_level1_master = dbModule::create(block, "inv4_master_level1");
    inv_mod_level1_master_i0_port  = dbModBTerm::create(inv4_mod_level1_master,"i0");
    inv_mod_level1_master_i1_port  = dbModBTerm::create(inv4_mod_level1_master,"i1");
    inv_mod_level1_master_i2_port  = dbModBTerm::create(inv4_mod_level1_master,"i2");
    inv_mod_level1_master_i3_port = dbModBTerm::create(inv4_mod_level1_master,"i3");
    inv_mod_level1_master_o0_port=  dbModBTerm::create(inv4_mod_level1_master,"o0");    
    
    
    inv4_mod_level2_master = dbModule::create(block, "inv4_master_level2");
    inv_mod_level2_master_i0_port  = dbModBTerm::create(inv4_mod_level2_master,"i0");
    inv_mod_level2_master_i1_port  = dbModBTerm::create(inv4_mod_level2_master,"i1");
    inv_mod_level2_master_i2_port  = dbModBTerm::create(inv4_mod_level2_master,"i2");
    inv_mod_level2_master_i3_port = dbModBTerm::create(inv4_mod_level2_master,"i3");
    inv_mod_level2_master_o0_port=  dbModBTerm::create(inv4_mod_level2_master,"o0");    
    


    //During modinst creation we set the parent.
    inv4_mod_level0_inst = dbModInst::create(root_mod, //parent
						  inv4_mod_level0_master,
						  "inv4_mod_level0_inst");
    inv4_mod_level1_inst = dbModInst::create(inv4_mod_level0_master, //parent
						  inv4_mod_level1_master,
						  "inv4_mod_level1_inst");
    inv4_mod_level2_inst = dbModInst::create(inv4_mod_level1_master, //parent
						  inv4_mod_level2_master,
						  "inv4_mod_level2_inst");


    //Use full scoped names for instances for this test
    inv1_1 = dbInst::create(block, lib->findMaster("inv1"), "inv1_mod_inst/inst1",false,inv1_mod_master);
    inv1_1_inst_ip0 = block -> findITerm("inv1_mod_inst/inst1/ip0");
    inv1_1_inst_op0 = block -> findITerm("inv1_mod_inst/inst1/op0");    


    //
    //create the low level inverter instances, for now uniquely name them in the scope of a block.
    //
    inv4_1 = dbInst::create(block, lib->findMaster("inv1"),
			    "inv4_mod_level0_inst/inv4_mod_level1_inst/inv4_mod_level2_inst/inst1");


    //get the iterm off the instance. Just give the terminal name
    //offset used to find iterm.
    inv4_1_ip = inv4_1 -> findITerm(
	    "ip0");
    inv4_1_op = inv4_1 -> findITerm(
				    "op0");
    
    inv4_2 = dbInst::create(block, lib->findMaster("inv1"),
			    "inv4_mod_level0_inst/inv4_mod_level1_inst/inv4_mod_level2_inst/inst2");   
    inv4_2_ip = inv4_2 -> findITerm("ip0");
    inv4_2_op = inv4_2 -> findITerm("op0");
    
    inv4_3 = dbInst::create(block, lib->findMaster("inv1"),
			    "inv4_mod_level0_inst/inv4_mod_level1_inst/inv4_mod_level2_inst/inst3");
    inv4_3_ip = inv4_3 -> findITerm("ip0");
    inv4_3_op = inv4_3 -> findITerm("op0");
    
    
    inv4_4 = dbInst::create(block, lib->findMaster("inv1"),
			    "inv4_mod_level0_inst/inv4_mod_level1_inst/inv4_mod_level2_inst/inst4");
    inv4_4_ip = inv4_4 -> findITerm("ip0");
    inv4_4_op = inv4_4 -> findITerm("op0");

    
    inv4_mod_level2_master -> addInst(inv4_1);
    inv4_mod_level2_master -> addInst(inv4_2);
    inv4_mod_level2_master -> addInst(inv4_3);
    inv4_mod_level2_master -> addInst(inv4_4);


    //First make the flat view
    dbNet* ip0_net = dbNet::create(block,"ip0_flat_net",false);
    dbNet* inv_op_net = dbNet::create(block,"inv_op_flat_net",false);
    dbNet* op0_net = dbNet::create(block,"op0_flat_net",false);
    dbNet* op1_net = dbNet::create(block,"op1_flat_net",false);
    dbNet* op2_net = dbNet::create(block,"op2_flat_net",false);
    dbNet* op3_net = dbNet::create(block,"op3_flat_net",false);        

    //connections to the primary (root) ports
    ip0_bterm = dbBTerm::create(ip0_net,"ip0");
    op0_bterm = dbBTerm::create(op0_net,"op0");
    op1_bterm = dbBTerm::create(op1_net,"op1");
    op2_bterm = dbBTerm::create(op2_net,"op2");
    op3_bterm = dbBTerm::create(op3_net,"op3");            


    //flat connections:
    //inverter 1 in module inv1_1
    inv1_1_inst_ip0 -> connect(ip0_net) ;
    inv1_1_inst_op0 -> connect(inv_op_net);

    //now the 4 inverters in inv4_1
    inv4_1_ip -> connect(inv_op_net);
    inv4_2_ip -> connect(inv_op_net);
    inv4_3_ip -> connect(inv_op_net);
    inv4_4_ip -> connect(inv_op_net);        
    
    //now core to external connections
    inv4_1_op -> connect(op0_net);
    inv4_2_op -> connect(op1_net);
    inv4_3_op -> connect(op2_net);
    inv4_4_op -> connect(op3_net);            

    std::stringstream str_str;
    DbStrDebugHierarchy(block, str_str);

    printf("The Flat design created %s\n", str_str.str().c_str());
    
  }

  ~F_HCONNECT() { dbDatabase::destroy(db); }

  dbDatabase* db;
  dbLib* lib;
  dbBlock* block;

  dbModule* root_mod;  
  dbModule* inv1_mod_master;
  dbModule* inv4_mod_level0_master;
  dbModule* inv4_mod_level1_master;
  dbModule* inv4_mod_level2_master;
  
  dbModBTerm* inv_mod_level0_master_i0_port;
  dbModBTerm* inv_mod_level0_master_i1_port;
  dbModBTerm* inv_mod_level0_master_i2_port;
  dbModBTerm* inv_mod_level0_master_i3_port;
  dbModBTerm* inv_mod_level0_master_o0_port;


  dbModBTerm* inv_mod_level1_master_i0_port;
  dbModBTerm* inv_mod_level1_master_i1_port;
  dbModBTerm* inv_mod_level1_master_i2_port;
  dbModBTerm* inv_mod_level1_master_i3_port;
  dbModBTerm* inv_mod_level1_master_o0_port;

  dbModBTerm* inv_mod_level2_master_i0_port;
  dbModBTerm* inv_mod_level2_master_i1_port;
  dbModBTerm* inv_mod_level2_master_i2_port;
  dbModBTerm* inv_mod_level2_master_i3_port;
  dbModBTerm* inv_mod_level2_master_o0_port;
  
  
  dbModInst* inv1_mod_inst;
  dbModInst* inv4_mod_level0_inst;
  dbModInst* inv4_mod_level1_inst;
  dbModInst* inv4_mod_level2_inst;
  
  
  dbInst* inv1_1;
  dbInst* inv4_1;
  dbInst* inv4_2;
  dbInst* inv4_3;
  dbInst* inv4_4;

  dbNet* ip0_net;
  dbNet* inv_op__net;
  dbNet* op0_net;
  dbNet* op1_net;
  dbNet* op2_net;
  dbNet* op3_net;

  dbBTerm* ip0_bterm;
  dbBTerm* op0_bterm;
  dbBTerm* op1_bterm;
  dbBTerm* op2_bterm;
  dbBTerm* op3_bterm;    

  dbModITerm* inv1_1_ip;
  dbModITerm* inv1_1_op;

  dbITerm* inv1_1_inst_ip0;
  dbITerm* inv1_1_inst_op0;

  dbITerm* inv4_1_ip;
  dbITerm* inv4_2_ip;
  dbITerm* inv4_3_ip;
  dbITerm* inv4_4_ip;        
  dbITerm* inv4_1_op;
  dbITerm* inv4_2_op;
  dbITerm* inv4_3_op;
  dbITerm* inv4_4_op;        

  
};

  
BOOST_FIXTURE_TEST_CASE(test_hier, F_HCONNECT)
{
  auto top = block->getTopModule();
  BOOST_TEST(top != nullptr);
  auto master1 = odb::dbModule::create(block, "master1");
  
  odb::dbModInst::create(top, master1, "minst1");
  
  auto master2 = odb::dbModule::create(block, "master2");
  auto minst2 = odb::dbModInst::create(master1, master2, "minst2");
  
  BOOST_TEST(block->findModInst("minst1/minst2") == minst2);

}


BOOST_FIXTURE_TEST_CASE(test_hierconnect, F_HCONNECT)
{

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
    inst3 = dbInst::create(block, lib->findMaster("and2"), "inst4");

    parent_mod->addInst(inst1);
    parent_mod->addInst(inst2);
    parent_mod->addInst(inst3);
    std::stringstream str_str;
    DbStrDebugHierarchy(block, str_str);
    unsigned signature = 0;
    for (const char* p = str_str.str().c_str(); p && *p != '\0'; p++) {
      signature += ((*p) * 5);
    }
    (void) (signature);
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
