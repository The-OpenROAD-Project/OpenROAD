// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <unistd.h>

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/lefin.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "tst/fixture.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace odb {

static const std::string prefix("_main/src/dbSta/test/");
static constexpr bool kDebugMsgs = false;

/*
  Extract the hierarchical information in human readable format.
  Shows the dbNet and dbModNet view of the database.
*/

static void DbStrDebugHierarchy(dbBlock* block, std::stringstream& str_db)
{
  str_db << fmt::format("Debug: Data base tables for block at {}:\n",
                        block->getName());

  str_db << "Db nets (The Flat db view)\n";

  for (auto dbnet : block->getNets()) {
    str_db << fmt::format(
        "dbnet {} (id {})\n", dbnet->getName(), dbnet->getId());

    for (auto db_iterm : dbnet->getITerms()) {
      str_db << fmt::format(
          "\t-> Db Iterm {} ({})\n", db_iterm->getId(), db_iterm->getName());
    }
    for (auto db_bterm : dbnet->getBTerms()) {
      str_db << fmt::format("\t-> Db Bterm {}\n", db_bterm->getId());
    }
  }

  // got through the ports and their owner
  str_db << "Block ports\n\t\tBTerm Ports +++\n";
  dbSet<dbBTerm> block_bterms = block->getBTerms();
  for (auto bt : block_bterms) {
    str_db << fmt::format("\t\tBterm ({}) {} Net {} ({})  Mod Net {} ({}) \n",
                          bt->getId(),
                          bt->getName(),
                          bt->getNet() ? bt->getNet()->getName() : "",
                          bt->getNet() ? bt->getNet()->getId() : 0,
                          bt->getModNet() ? bt->getModNet()->getName() : "",
                          bt->getModNet() ? bt->getModNet()->getId() : 0);
  }
  str_db << "\t\tBTerm Ports ---\n";

  str_db << "The hierarchical db view:\n";
  dbSet<dbModule> block_modules = block->getModules();
  str_db << fmt::format("Content size {} modules\n", block_modules.size());
  for (auto mi : block_modules) {
    dbModule* cur_obj = mi;
    if (cur_obj == block->getTopModule()) {
      str_db << "Top Module\n";
    }
    str_db << fmt::format(
        "\tModule {} {}\n",
        (cur_obj == block->getTopModule()) ? "(Top Module)" : "",
        cur_obj->getName());
    // in case of top level, care as the bterms double up as pins
    if (cur_obj == block->getTopModule()) {
      for (auto bterm : block->getBTerms()) {
        str_db << fmt::format(
            "Top B-term {} dbNet {} ({}) modNet {} ({})\n",
            bterm->getName(),
            bterm->getNet() ? bterm->getNet()->getName() : "",
            bterm->getNet() ? bterm->getNet()->getId() : -1,
            bterm->getModNet() ? bterm->getModNet()->getName() : "",
            bterm->getModNet() ? bterm->getModNet()->getId() : -1);
      }
    }
    // got through the module ports and their owner
    str_db << "\t\tModBTerm Ports +++\n";

    dbSet<dbModBTerm> module_ports = cur_obj->getModBTerms();
    for (auto module_port : module_ports) {
      str_db << fmt::format(
          "\t\tPort {} Net {} ({})\n",
          module_port->getName(),
          (module_port->getModNet()) ? (module_port->getModNet()->getName())
                                     : "No-modnet",
          (module_port->getModNet()) ? module_port->getModNet()->getId() : -1);
      str_db << fmt::format("\t\tPort parent {}\n\n",
                            module_port->getParent()->getName());
    }
    str_db << "\t\tModBTermPorts ---\n";

    str_db << "\t\tModule instances +++\n";
    dbSet<dbModInst> module_instances = mi->getModInsts();
    for (auto module_inst : module_instances) {
      str_db << fmt::format("\t\tMod inst {} ", module_inst->getName());
      dbModule* master = module_inst->getMaster();
      str_db << fmt::format("\t\tMaster {}\n\n",
                            module_inst->getMaster()->getName());
      dbBlock* owner = master->getOwner();
      if (owner != block) {
        str_db << "\t\t\tMaster owner in wrong block\n";
      }
      str_db << "\t\tConnections\n";
      for (dbModITerm* miterm_pin : module_inst->getModITerms()) {
        str_db << fmt::format(
            "\t\t\tModIterm : {} ({}) Mod Net {} ({}) \n",
            miterm_pin->getName(),
            miterm_pin->getId(),
            miterm_pin->getModNet() ? (miterm_pin->getModNet()->getName())
                                    : "No-net",
            miterm_pin->getModNet() ? miterm_pin->getModNet()->getId() : 0);
      }
    }
    str_db << "\t\tModule instances ---\n";
    str_db << "\t\tDb instances +++\n";
    for (dbInst* db_inst : cur_obj->getInsts()) {
      str_db << fmt::format("\t\tdb inst {}\n", db_inst->getName());
      str_db << "\t\tdb iterms:\n";
      dbSet<dbITerm> iterms = db_inst->getITerms();
      for (auto iterm : iterms) {
        dbMTerm* mterm = iterm->getMTerm();
        str_db << fmt::format(
            "\t\t\t\t iterm: {} ({}) Net: {} Mod net : {} ({})\n",
            mterm->getName(),
            iterm->getId(),
            iterm->getNet() ? iterm->getNet()->getName() : "unk-dbnet",
            iterm->getModNet() ? iterm->getModNet()->getName() : "unk-modnet",
            iterm->getModNet() ? iterm->getModNet()->getId() : -1);
      }
    }
    str_db << "\t\tDb instances ---\n";
    str_db << "\tModule nets (modnets) +++ \n";
    str_db << fmt::format("\t# mod nets {} in {} \n",
                          cur_obj->getModNets().size(),
                          cur_obj->getName());

    dbSet<dbModNet> mod_nets = cur_obj->getModNets();
    for (auto mod_net : mod_nets) {
      str_db << fmt::format(
          "\t\tNet: {} ({})\n", mod_net->getName(), mod_net->getId());
      str_db << "\t\tConnections -> modIterms/modbterms/bterms/iterms:\n";
      str_db << fmt::format("\t\t -> {} moditerms\n",
                            mod_net->getModITerms().size());
      for (dbModITerm* modi_term : mod_net->getModITerms()) {
        str_db << fmt::format("\t\t\t{}\n", modi_term->getName());
      }
      str_db << fmt::format("\t\t -> {} modbterms\n",
                            mod_net->getModBTerms().size());
      for (dbModBTerm* modb_term : mod_net->getModBTerms()) {
        str_db << fmt::format("\t\t\t{}\n", modb_term->getName());
      }
      str_db << fmt::format("\t\t -> {} iterms\n", mod_net->getITerms().size());
      for (dbITerm* db_iterm : mod_net->getITerms()) {
        str_db << fmt::format("\t\t\t{}\n", db_iterm->getName());
      }
      str_db << fmt::format("\t\t -> {} bterms\n", mod_net->getBTerms().size());
      for (dbBTerm* db_bterm : mod_net->getBTerms()) {
        str_db << fmt::format("\t\t\t{}\n", db_bterm->getName());
      }
    }
  }
}

/*
   Test fixture for hierarchical connect test
   ------------------------------------------

   Two modules, 1 with 1 inverter, second with 4 inverters.

   inv1 is instantiated in the top level
   inv4 is instantiated three levels down:
   level0 -- instantiation in root
   level1
   level2 -- contains 4 inverters

   Objective of this test is to stress the hierarchical connection code.

   root {inputs: i0; outputs: o0, o1, o2,o3}
       --> inv1 (module containing 1 inverter) this drives inv1_level0_inst
       --> inv4_level0_inst  {wrapper input: i0, outputs o0,o1,o2,o3}
           --> inv4_level1_inst {wrapper input: i0, outputs o0,o1,o2,o3}
               --> inv4_level2_inst {contains 4 inverters. Has input i0, outputs
   o0,01,o2,o3}
*/

class TestHconn : public ::tst::Fixture
{
 protected:
  void SetUp() override
  {
    // this will be so much easier with read_def
    library_ = readLiberty(prefix + "Nangate45/Nangate45_typ.lib");
    lib_ = loadTechAndLib(
        "tech", "Nangate45", prefix + "Nangate45/Nangate45.lef");

    db_network_ = sta_->getDbNetwork();
    // turn on hierarchy
    db_network_->setHierarchy();

    // create a chain consisting of 4 buffers
    odb::dbChip* chip = odb::dbChip::create(db_.get(), db_->getTech());
    block_ = odb::dbBlock::create(chip, "top");
    db_network_->setBlock(block_);
    block_->setDieArea(odb::Rect(0, 0, 1000, 1000));
    // register proper callbacks for timer like read_def
    sta_->postReadDef(block_);

    root_mod_ = block_->getTopModule();

    // The bterms are created below during wiring
    // Note a bterm without a parent is a root bterm.

    // Make the inv1 module (contains 1 inverter).
    inv1_mod_master_ = dbModule::create(block_, "inv1_master_mod");
    inv1_mod_i0_port_ = dbModBTerm::create(inv1_mod_master_, "i0");
    inv1_mod_o0_port_ = dbModBTerm::create(inv1_mod_master_, "o0");

    inv1_mod_inst_
        = dbModInst::create(root_mod_, inv1_mod_master_, "inv1_mod_inst");

    inv1_mod_inst_i0_miterm_
        = dbModITerm::create(inv1_mod_inst_, "i0", inv1_mod_i0_port_);
    inv1_mod_inst_o0_miterm_
        = dbModITerm::create(inv1_mod_inst_, "o0", inv1_mod_o0_port_);
    // correlate the iterms and bterms
    inv1_mod_inst_i0_miterm_->setChildModBTerm(inv1_mod_i0_port_);
    inv1_mod_i0_port_->setParentModITerm(inv1_mod_inst_i0_miterm_);

    inv1_mod_inst_o0_miterm_->setChildModBTerm(inv1_mod_o0_port_);
    inv1_mod_o0_port_->setParentModITerm(inv1_mod_inst_o0_miterm_);

    inv1_1_ = dbInst::create(block_,
                             lib_->findMaster("INV_X1"),
                             "inv1_mod_inst/inst1",
                             false,
                             inv1_mod_master_);

    inv1_1_inst_ip0_ = inv1_1_->findITerm("A");
    inv1_1_inst_op0_ = inv1_1_->findITerm("ZN");

    inv4_mod_level0_master_ = dbModule::create(block_, "inv4_master_level0");
    inv4_mod_level0_master_i0_port_
        = dbModBTerm::create(inv4_mod_level0_master_, "i0");
    inv4_mod_level0_master_o0_port_
        = dbModBTerm::create(inv4_mod_level0_master_, "o0");
    inv4_mod_level0_master_o1_port_
        = dbModBTerm::create(inv4_mod_level0_master_, "o1");
    inv4_mod_level0_master_o2_port_
        = dbModBTerm::create(inv4_mod_level0_master_, "o2");
    inv4_mod_level0_master_o3_port_
        = dbModBTerm::create(inv4_mod_level0_master_, "o3");

    inv4_mod_level1_master_ = dbModule::create(block_, "inv4_master_level1");
    inv4_mod_level1_master_i0_port_
        = dbModBTerm::create(inv4_mod_level1_master_, "i0");
    inv4_mod_level1_master_o0_port_
        = dbModBTerm::create(inv4_mod_level1_master_, "o0");
    inv4_mod_level1_master_o1_port_
        = dbModBTerm::create(inv4_mod_level1_master_, "o1");
    inv4_mod_level1_master_o2_port_
        = dbModBTerm::create(inv4_mod_level1_master_, "o2");
    inv4_mod_level1_master_o3_port_
        = dbModBTerm::create(inv4_mod_level1_master_, "o3");

    inv4_mod_level2_master_ = dbModule::create(block_, "inv4_master_level2");
    inv4_mod_level2_master_i0_port_
        = dbModBTerm::create(inv4_mod_level2_master_, "i0");
    inv4_mod_level2_master_o0_port_
        = dbModBTerm::create(inv4_mod_level2_master_, "o0");
    inv4_mod_level2_master_o1_port_
        = dbModBTerm::create(inv4_mod_level2_master_, "o1");
    inv4_mod_level2_master_o2_port_
        = dbModBTerm::create(inv4_mod_level2_master_, "o2");
    inv4_mod_level2_master_o3_port_
        = dbModBTerm::create(inv4_mod_level2_master_, "o3");

    // During modinst creation we set the parent.
    inv4_mod_level0_inst_ = dbModInst::create(root_mod_,  // parent
                                              inv4_mod_level0_master_,
                                              "inv4_mod_level0_inst");
    inv4_mod_level0_inst_i0_miterm_
        = dbModITerm::create(inv4_mod_level0_inst_, "i0");
    inv4_mod_level0_inst_i0_miterm_->setChildModBTerm(
        inv4_mod_level0_master_i0_port_);
    inv4_mod_level0_master_i0_port_->setParentModITerm(
        inv4_mod_level0_inst_i0_miterm_);

    inv4_mod_level0_inst_o0_miterm_
        = dbModITerm::create(inv4_mod_level0_inst_, "o0");
    inv4_mod_level0_inst_o0_miterm_->setChildModBTerm(
        inv4_mod_level0_master_o0_port_);
    inv4_mod_level0_master_o0_port_->setParentModITerm(
        inv4_mod_level0_inst_o0_miterm_);

    inv4_mod_level0_inst_o1_miterm_
        = dbModITerm::create(inv4_mod_level0_inst_, "o1");
    inv4_mod_level0_inst_o1_miterm_->setChildModBTerm(
        inv4_mod_level0_master_o1_port_);
    inv4_mod_level0_master_o1_port_->setParentModITerm(
        inv4_mod_level0_inst_o1_miterm_);

    inv4_mod_level0_inst_o2_miterm_
        = dbModITerm::create(inv4_mod_level0_inst_, "o2");
    inv4_mod_level0_inst_o2_miterm_->setChildModBTerm(
        inv4_mod_level0_master_o2_port_);
    inv4_mod_level0_master_o2_port_->setParentModITerm(
        inv4_mod_level0_inst_o2_miterm_);

    inv4_mod_level0_inst_o3_miterm_
        = dbModITerm::create(inv4_mod_level0_inst_, "o3");
    inv4_mod_level0_inst_o3_miterm_->setChildModBTerm(
        inv4_mod_level0_master_o3_port_);
    inv4_mod_level0_master_o3_port_->setParentModITerm(
        inv4_mod_level0_inst_o3_miterm_);

    inv4_mod_level1_inst_
        = dbModInst::create(inv4_mod_level0_master_,  // parent
                            inv4_mod_level1_master_,
                            "inv4_mod_level1_inst");

    inv4_mod_level1_inst_i0_miterm_
        = dbModITerm::create(inv4_mod_level1_inst_, "i0");
    inv4_mod_level1_inst_i0_miterm_->setChildModBTerm(
        inv4_mod_level1_master_i0_port_);
    inv4_mod_level1_master_i0_port_->setParentModITerm(
        inv4_mod_level1_inst_i0_miterm_);

    inv4_mod_level1_inst_o0_miterm_
        = dbModITerm::create(inv4_mod_level1_inst_, "o0");
    inv4_mod_level1_inst_o0_miterm_->setChildModBTerm(
        inv4_mod_level1_master_o0_port_);
    inv4_mod_level1_master_o0_port_->setParentModITerm(
        inv4_mod_level1_inst_o0_miterm_);

    inv4_mod_level1_inst_o1_miterm_
        = dbModITerm::create(inv4_mod_level1_inst_, "o1");
    inv4_mod_level1_inst_o1_miterm_->setChildModBTerm(
        inv4_mod_level1_master_o1_port_);
    inv4_mod_level1_master_o1_port_->setParentModITerm(
        inv4_mod_level1_inst_o1_miterm_);

    inv4_mod_level1_inst_o2_miterm_
        = dbModITerm::create(inv4_mod_level1_inst_, "o2");
    inv4_mod_level1_inst_o2_miterm_->setChildModBTerm(
        inv4_mod_level1_master_o2_port_);
    inv4_mod_level1_master_o2_port_->setParentModITerm(
        inv4_mod_level1_inst_o2_miterm_);

    inv4_mod_level1_inst_o3_miterm_
        = dbModITerm::create(inv4_mod_level1_inst_, "o3");
    inv4_mod_level1_inst_o3_miterm_->setChildModBTerm(
        inv4_mod_level1_master_o3_port_);
    inv4_mod_level1_master_o3_port_->setParentModITerm(
        inv4_mod_level1_inst_o3_miterm_);

    inv4_mod_level2_inst_
        = dbModInst::create(inv4_mod_level1_master_,  // parent
                            inv4_mod_level2_master_,
                            "inv4_mod_level2_inst");

    inv4_mod_level2_inst_i0_miterm_
        = dbModITerm::create(inv4_mod_level2_inst_, "i0");
    inv4_mod_level2_inst_i0_miterm_->setChildModBTerm(
        inv4_mod_level2_master_i0_port_);
    inv4_mod_level2_master_i0_port_->setParentModITerm(
        inv4_mod_level2_inst_i0_miterm_);

    inv4_mod_level2_inst_o0_miterm_
        = dbModITerm::create(inv4_mod_level2_inst_, "o0");
    inv4_mod_level2_inst_o0_miterm_->setChildModBTerm(
        inv4_mod_level2_master_o0_port_);
    inv4_mod_level2_master_o0_port_->setParentModITerm(
        inv4_mod_level2_inst_o0_miterm_);

    inv4_mod_level2_inst_o1_miterm_
        = dbModITerm::create(inv4_mod_level2_inst_, "o1");
    inv4_mod_level2_inst_o1_miterm_->setChildModBTerm(
        inv4_mod_level2_master_o1_port_);
    inv4_mod_level2_master_o1_port_->setParentModITerm(
        inv4_mod_level2_inst_o1_miterm_);

    inv4_mod_level2_inst_o2_miterm_
        = dbModITerm::create(inv4_mod_level2_inst_, "o2");
    inv4_mod_level2_inst_o2_miterm_->setChildModBTerm(
        inv4_mod_level2_master_o2_port_);
    inv4_mod_level2_master_o2_port_->setParentModITerm(
        inv4_mod_level2_inst_o2_miterm_);

    inv4_mod_level2_inst_o3_miterm_
        = dbModITerm::create(inv4_mod_level2_inst_, "o3");
    inv4_mod_level2_inst_o3_miterm_->setChildModBTerm(
        inv4_mod_level2_master_o3_port_);
    inv4_mod_level2_master_o3_port_->setParentModITerm(
        inv4_mod_level2_inst_o3_miterm_);

    //
    // create the low level inverter instances, for now uniquely name them in
    // the scope of a block.

    inv4_1_ = dbInst::create(
        block_,
        lib_->findMaster("INV_X1"),
        "inv4_mod_level0_inst/inv4_mod_level1_inst/inv4_mod_level2_inst/inst1");

    // get the iterm off the instance. Just give the terminal name
    // offset used to find iterm.
    inv4_1_ip_ = inv4_1_->findITerm("A");
    inv4_1_op_ = inv4_1_->findITerm("ZN");

    inv4_2_ = dbInst::create(
        block_,
        lib_->findMaster("INV_X1"),
        "inv4_mod_level0_inst/inv4_mod_level1_inst/inv4_mod_level2_inst/inst2");
    inv4_2_ip_ = inv4_2_->findITerm("A");
    inv4_2_op_ = inv4_2_->findITerm("ZN");

    inv4_3_ = dbInst::create(
        block_,
        lib_->findMaster("INV_X1"),
        "inv4_mod_level0_inst/inv4_mod_level1_inst/inv4_mod_level2_inst/inst3");
    inv4_3_ip_ = inv4_3_->findITerm("A");
    inv4_3_op_ = inv4_3_->findITerm("ZN");

    inv4_4_ = dbInst::create(
        block_,
        lib_->findMaster("INV_X1"),
        "inv4_mod_level0_inst/inv4_mod_level1_inst/inv4_mod_level2_inst/inst4");
    inv4_4_ip_ = inv4_4_->findITerm("A");
    inv4_4_op_ = inv4_4_->findITerm("ZN");

    //
    // inv4_mod_level2_master is lowest level in hierarchy
    //
    inv4_mod_level2_master_->addInst(inv4_1_);
    inv4_mod_level2_master_->addInst(inv4_2_);
    inv4_mod_level2_master_->addInst(inv4_3_);
    inv4_mod_level2_master_->addInst(inv4_4_);

    inv4_mod_level2_inst_i0_mnet_ = dbModNet::create(
        inv4_mod_level2_master_, "inv4_mod_level2_inst_i0_mnet");
    inv4_mod_level2_inst_o0_mnet_ = dbModNet::create(
        inv4_mod_level2_master_, "inv4_mod_level2_inst_o0_mnet");
    inv4_mod_level2_inst_o1_mnet_ = dbModNet::create(
        inv4_mod_level2_master_, "inv4_mod_level2_inst_o1_mnet");
    inv4_mod_level2_inst_o2_mnet_ = dbModNet::create(
        inv4_mod_level2_master_, "inv4_mod_level2_inst_o2_mnet");
    inv4_mod_level2_inst_o3_mnet_ = dbModNet::create(
        inv4_mod_level2_master_, "inv4_mod_level2_inst_o3_mnet");
    inv4_1_ip_->connect(inv4_mod_level2_inst_i0_mnet_);
    inv4_2_ip_->connect(inv4_mod_level2_inst_i0_mnet_);
    inv4_3_ip_->connect(inv4_mod_level2_inst_i0_mnet_);
    inv4_4_ip_->connect(inv4_mod_level2_inst_i0_mnet_);

    inv4_1_op_->connect(inv4_mod_level2_inst_o0_mnet_);
    inv4_2_op_->connect(inv4_mod_level2_inst_o1_mnet_);
    inv4_3_op_->connect(inv4_mod_level2_inst_o2_mnet_);
    inv4_4_op_->connect(inv4_mod_level2_inst_o3_mnet_);

    inv4_mod_level2_master_i0_port_->connect(inv4_mod_level2_inst_i0_mnet_);
    inv4_mod_level2_master_o0_port_->connect(inv4_mod_level2_inst_o0_mnet_);
    inv4_mod_level2_master_o1_port_->connect(inv4_mod_level2_inst_o1_mnet_);
    inv4_mod_level2_master_o2_port_->connect(inv4_mod_level2_inst_o2_mnet_);
    inv4_mod_level2_master_o3_port_->connect(inv4_mod_level2_inst_o3_mnet_);

    // First make the flat view connectivity
    ip0_net_ = dbNet::create(block_, "ip0_flat_net", false);
    inv_op_net_ = dbNet::create(block_, "inv_op_flat_net", false);
    op0_net_ = dbNet::create(block_, "op0_flat_net", false);
    op1_net_ = dbNet::create(block_, "op1_flat_net", false);
    op2_net_ = dbNet::create(block_, "op2_flat_net", false);
    op3_net_ = dbNet::create(block_, "op3_flat_net", false);

    //
    // connections to the primary (root) ports
    // Note: a bterm without a parent is a root port.
    //
    ip0_bterm_ = dbBTerm::create(ip0_net_, "ip0");
    op0_bterm_ = dbBTerm::create(op0_net_, "op0");
    op1_bterm_ = dbBTerm::create(op1_net_, "op1");
    op2_bterm_ = dbBTerm::create(op2_net_, "op2");
    op3_bterm_ = dbBTerm::create(op3_net_, "op3");

    op0_bterm_->connect(inv4_mod_level2_inst_o0_mnet_);
    op1_bterm_->connect(inv4_mod_level2_inst_o1_mnet_);
    op2_bterm_->connect(inv4_mod_level2_inst_o2_mnet_);
    op3_bterm_->connect(inv4_mod_level2_inst_o3_mnet_);

    // flat connections:
    // inverter 1 in module inv1_1
    inv1_1_inst_ip0_->connect(ip0_net_);
    inv1_1_inst_op0_->connect(inv_op_net_);

    // now the 4 inverters in inv4_1
    inv4_1_ip_->connect(inv_op_net_);
    inv4_2_ip_->connect(inv_op_net_);
    inv4_3_ip_->connect(inv_op_net_);
    inv4_4_ip_->connect(inv_op_net_);

    // now core to external connections
    inv4_1_op_->connect(op0_net_);
    inv4_2_op_->connect(op1_net_);
    inv4_3_op_->connect(op2_net_);
    inv4_4_op_->connect(op3_net_);

    if (kDebugMsgs) {
      std::stringstream str_str;
      DbStrDebugHierarchy(block_, str_str);
      printf("The Flat design created %s\n", str_str.str().c_str());
    }

    // Now build the hierarchical "overlay"
    // What we are doing here is adding the modnets which hook up

    // wire in hierarchy for inv1

    // Contents of inv1 (modnets from ports to internals).
    inv1_mod_i0_modnet_
        = dbModNet::create(inv1_mod_master_, "inv1_mod_i0_mnet");
    inv1_mod_o0_modnet_
        = dbModNet::create(inv1_mod_master_, "inv1_mod_o0_mnet");
    // connection from port to net in module
    inv1_mod_i0_port_->connect(inv1_mod_i0_modnet_);
    // connection to inverter ip
    inv1_1_inst_ip0_->connect(inv1_mod_i0_modnet_);
    // from inverter op to port
    inv1_1_inst_op0_->connect(inv1_mod_o0_modnet_);
    inv1_mod_o0_port_->connect(inv1_mod_o0_modnet_);

    // Instantiation of inv1 connections to top level (root bterms to modnets,
    // modnets to moditerms on inv1).
    root_inv1_i0_mnet_ = dbModNet::create(root_mod_, "inv1_inst_i0_mnet");
    root_inv1_o0_mnet_ = dbModNet::create(root_mod_, "inv1_inst_o0_mnet");
    inv1_mod_inst_i0_miterm_->connect(root_inv1_i0_mnet_);
    inv1_mod_inst_o0_miterm_->connect(root_inv1_o0_mnet_);

    // top level connections for inv1.
    // root input to instance, via a modnet.
    ip0_bterm_->connect(root_inv1_i0_mnet_);
    // note the output from out inverter module is inv1_inst_o0_mnet
    // which now needs to be hooked up the inv4 hierarchy.

    // The inv4 hierarchy connections
    // inv1 -> inv4 connection
    inv4_mod_level0_inst_i0_miterm_->connect(root_inv1_o0_mnet_);

    // level 0 is top of hierarchy, in root
    // the level 0 instance -> root connections
    root_inv4_o0_mnet_ = dbModNet::create(root_mod_, "inv4_inst_o0_mnet");
    inv4_mod_level0_inst_o0_miterm_->connect(root_inv4_o0_mnet_);
    op0_bterm_->connect(root_inv4_o0_mnet_);

    root_inv4_o1_mnet_ = dbModNet::create(root_mod_, "inv4_inst_o1_mnet");
    inv4_mod_level0_inst_o1_miterm_->connect(root_inv4_o1_mnet_);
    op1_bterm_->connect(root_inv4_o1_mnet_);

    root_inv4_o2_mnet_ = dbModNet::create(root_mod_, "inv4_inst_o2_mnet");
    inv4_mod_level0_inst_o2_miterm_->connect(root_inv4_o2_mnet_);
    op2_bterm_->connect(root_inv4_o2_mnet_);

    root_inv4_o3_mnet_ = dbModNet::create(root_mod_, "inv4_inst_o3_mnet");
    inv4_mod_level0_inst_o3_miterm_->connect(root_inv4_o3_mnet_);
    op3_bterm_->connect(root_inv4_o3_mnet_);

    // level 1 is next level down
    // The level 1 instance connections within the scope of the
    // inv4_mod_level0_master
    inv4_mod_level1_inst_i0_mnet_ = dbModNet::create(
        inv4_mod_level0_master_, "inv4_mod_level1_inst_i0_mnet");

    inv4_mod_level1_inst_i0_miterm_->connect(inv4_mod_level1_inst_i0_mnet_);
    inv4_mod_level0_master_i0_port_->connect(inv4_mod_level1_inst_i0_mnet_);

    inv4_mod_level1_inst_o0_mnet_ = dbModNet::create(
        inv4_mod_level0_master_, "inv4_mod_level1_inst_o0_mnet");
    inv4_mod_level1_inst_o0_miterm_->connect(inv4_mod_level1_inst_o0_mnet_);
    inv4_mod_level0_master_o0_port_->connect(inv4_mod_level1_inst_o0_mnet_);

    inv4_mod_level1_inst_o1_mnet_ = dbModNet::create(
        inv4_mod_level0_master_, "inv4_mod_level1_inst_o1_mnet");
    inv4_mod_level1_inst_o1_miterm_->connect(inv4_mod_level1_inst_o1_mnet_);
    inv4_mod_level0_master_o1_port_->connect(inv4_mod_level1_inst_o1_mnet_);

    inv4_mod_level1_inst_o2_mnet_ = dbModNet::create(
        inv4_mod_level0_master_, "inv4_mod_level1_inst_o2_mnet");
    inv4_mod_level1_inst_o2_miterm_->connect(inv4_mod_level1_inst_o2_mnet_);
    inv4_mod_level0_master_o2_port_->connect(inv4_mod_level1_inst_o2_mnet_);

    inv4_mod_level1_inst_o3_mnet_ = dbModNet::create(
        inv4_mod_level0_master_, "inv4_mod_level1_inst_o3_mnet");
    inv4_mod_level1_inst_o3_miterm_->connect(inv4_mod_level1_inst_o3_mnet_);
    inv4_mod_level0_master_o3_port_->connect(inv4_mod_level1_inst_o3_mnet_);

    // The level 2 instance connections within the scope of the
    // inv4_mod_level1_master level 2 is the cell which contains the 4 inverters
    inv4_mod_level2_inst_i0_mnet_ = dbModNet::create(
        inv4_mod_level1_master_, "inv4_mod_level2_inst_i0_mnet");

    inv4_mod_level2_inst_i0_miterm_->connect(inv4_mod_level2_inst_i0_mnet_);
    inv4_mod_level1_master_i0_port_->connect(inv4_mod_level2_inst_i0_mnet_);

    inv4_mod_level2_inst_o0_mnet_ = dbModNet::create(
        inv4_mod_level1_master_, "inv4_mod_level2_inst_o0_mnet");

    inv4_mod_level2_inst_o0_miterm_->connect(inv4_mod_level2_inst_o0_mnet_);
    inv4_mod_level1_master_o0_port_->connect(inv4_mod_level2_inst_o0_mnet_);

    inv4_mod_level2_inst_o1_mnet_ = dbModNet::create(
        inv4_mod_level1_master_, "inv4_mod_level2_inst_o1_mnet");

    inv4_mod_level2_inst_o1_miterm_->connect(inv4_mod_level2_inst_o1_mnet_);
    inv4_mod_level1_master_o1_port_->connect(inv4_mod_level2_inst_o1_mnet_);

    inv4_mod_level2_inst_o2_mnet_ = dbModNet::create(
        inv4_mod_level1_master_, "inv4_mod_level2_inst_o2_mnet");

    inv4_mod_level2_inst_o2_miterm_->connect(inv4_mod_level2_inst_o2_mnet_);
    inv4_mod_level1_master_o2_port_->connect(inv4_mod_level2_inst_o2_mnet_);

    inv4_mod_level2_inst_o3_mnet_ = dbModNet::create(
        inv4_mod_level1_master_, "inv4_mod_level2_inst_o3_mnet");

    inv4_mod_level2_inst_o3_miterm_->connect(inv4_mod_level2_inst_o3_mnet_);
    inv4_mod_level1_master_o3_port_->connect(inv4_mod_level2_inst_o3_mnet_);

    if (kDebugMsgs) {
      std::stringstream full_design;
      DbStrDebugHierarchy(block_, full_design);
      printf("The  design created (flat and hierarchical) %s\n",
             full_design.str().c_str());
    }
  }

  sta::LibertyLibrary* library_;

  sta::dbNetwork* db_network_;

  dbBlock* block_;
  odb::dbLib* lib_;

  dbModule* root_mod_;
  dbMTerm* root_mod_i0_mterm_;
  dbMTerm* root_mod_i1_mterm_;
  dbMTerm* root_mod_i2_mterm_;
  dbMTerm* root_mod_i3_mterm_;
  dbMTerm* root_mod_o0_mterm_;

  dbBTerm* root_mod_i0_bterm_;
  dbBTerm* root_mod_i1_bterm_;
  dbBTerm* root_mod_i2_bterm_;
  dbBTerm* root_mod_i3_bterm_;
  dbBTerm* root_mod_o0_bterm_;

  dbModule* inv1_mod_master_;
  dbModInst* inv1_mod_inst_;
  dbModITerm* inv1_mod_inst_i0_miterm_;
  dbModITerm* inv1_mod_inst_o0_miterm_;
  dbModBTerm* inv1_mod_i0_port_;
  dbModBTerm* inv1_mod_o0_port_;
  dbModNet* inv1_mod_i0_modnet_;
  dbModNet* inv1_mod_o0_modnet_;

  dbModule* inv4_mod_level0_master_;
  dbModule* inv4_mod_level1_master_;
  dbModule* inv4_mod_level2_master_;

  dbModBTerm* inv4_mod_level0_master_i0_port_;
  dbModBTerm* inv4_mod_level0_master_o0_port_;
  dbModBTerm* inv4_mod_level0_master_o1_port_;
  dbModBTerm* inv4_mod_level0_master_o2_port_;
  dbModBTerm* inv4_mod_level0_master_o3_port_;

  dbModBTerm* inv4_mod_level1_master_i0_port_;
  dbModBTerm* inv4_mod_level1_master_o0_port_;
  dbModBTerm* inv4_mod_level1_master_o1_port_;
  dbModBTerm* inv4_mod_level1_master_o2_port_;
  dbModBTerm* inv4_mod_level1_master_o3_port_;

  dbModBTerm* inv4_mod_level2_master_i0_port_;
  dbModBTerm* inv4_mod_level2_master_o0_port_;
  dbModBTerm* inv4_mod_level2_master_o1_port_;
  dbModBTerm* inv4_mod_level2_master_o2_port_;
  dbModBTerm* inv4_mod_level2_master_o3_port_;

  dbModInst* inv4_mod_level0_inst_;
  dbModInst* inv4_mod_level1_inst_;
  dbModInst* inv4_mod_level2_inst_;

  dbModITerm* inv4_mod_level0_inst_i0_miterm_;
  dbModITerm* inv4_mod_level0_inst_o0_miterm_;
  dbModITerm* inv4_mod_level0_inst_o1_miterm_;
  dbModITerm* inv4_mod_level0_inst_o2_miterm_;
  dbModITerm* inv4_mod_level0_inst_o3_miterm_;

  dbModITerm* inv4_mod_level1_inst_i0_miterm_;
  dbModITerm* inv4_mod_level1_inst_o0_miterm_;
  dbModITerm* inv4_mod_level1_inst_o1_miterm_;
  dbModITerm* inv4_mod_level1_inst_o2_miterm_;
  dbModITerm* inv4_mod_level1_inst_o3_miterm_;

  dbModITerm* inv4_mod_level2_inst_i0_miterm_;
  dbModITerm* inv4_mod_level2_inst_o0_miterm_;
  dbModITerm* inv4_mod_level2_inst_o1_miterm_;
  dbModITerm* inv4_mod_level2_inst_o2_miterm_;
  dbModITerm* inv4_mod_level2_inst_o3_miterm_;

  dbInst* inv1_1_;
  dbInst* inv4_1_;
  dbInst* inv4_2_;
  dbInst* inv4_3_;
  dbInst* inv4_4_;

  dbNet* ip0_net_;
  dbNet* inv_op_net_;
  dbNet* op0_net_;
  dbNet* op1_net_;
  dbNet* op2_net_;
  dbNet* op3_net_;

  dbBTerm* ip0_bterm_;
  dbBTerm* op0_bterm_;
  dbBTerm* op1_bterm_;
  dbBTerm* op2_bterm_;
  dbBTerm* op3_bterm_;

  dbModITerm* inv1_1_ip_;
  dbModITerm* inv1_1_op_;

  dbModNet* root_inv1_i0_mnet_;
  dbModNet* root_inv1_o0_mnet_;

  dbModNet* root_inv4_o0_mnet_;
  dbModNet* root_inv4_o1_mnet_;
  dbModNet* root_inv4_o2_mnet_;
  dbModNet* root_inv4_o3_mnet_;

  // first input inv4_mod_level_i0 comes from root_inv1_o0_modnet
  dbModNet* inv4_mod_level0_inst_o0_mnet_;
  dbModNet* inv4_mod_level0_inst_o1_mnet_;
  dbModNet* inv4_mod_level0_inst_o2_mnet_;
  dbModNet* inv4_mod_level0_inst_o3_mnet_;

  dbModNet* inv4_mod_level1_inst_i0_mnet_;
  dbModNet* inv4_mod_level1_inst_o0_mnet_;
  dbModNet* inv4_mod_level1_inst_o1_mnet_;
  dbModNet* inv4_mod_level1_inst_o2_mnet_;
  dbModNet* inv4_mod_level1_inst_o3_mnet_;

  dbModNet* inv4_mod_level2_inst_i0_mnet_;
  dbModNet* inv4_mod_level2_inst_o0_mnet_;
  dbModNet* inv4_mod_level2_inst_o1_mnet_;
  dbModNet* inv4_mod_level2_inst_o2_mnet_;
  dbModNet* inv4_mod_level2_inst_o3_mnet_;

  dbITerm* inv1_1_inst_ip0_;
  dbITerm* inv1_1_inst_op0_;

  dbITerm* inv4_1_ip_;
  dbITerm* inv4_2_ip_;
  dbITerm* inv4_3_ip_;
  dbITerm* inv4_4_ip_;
  dbITerm* inv4_1_op_;
  dbITerm* inv4_2_op_;
  dbITerm* inv4_3_op_;
  dbITerm* inv4_4_op_;
};

TEST_F(TestHconn, ConnectionMade)
{
  if (kDebugMsgs) {
    std::stringstream str_str_initial;
    DbStrDebugHierarchy(block_, str_str_initial);
    printf("The initial design: %s\n", str_str_initial.str().c_str());
  }

  // ECO test: get initial state before we start modifying
  // the design. Then at end we undo everything and
  // validate initial state preserved

  size_t initial_db_net_count = block_->getNets().size();
  size_t initial_mod_net_count = block_->getModNets().size();
  odb::dbDatabase::beginEco(block_);

  //
  //
  // Hierarchical connection test case:
  //----------------------------------
  //
  // add a new inverter to inv1 and connect to the fanout of the
  // existing one.
  // remove the driver of the 4th inverter in inv4_4 leaf
  // and connect to it via hierarchy
  //
  //
  // Before
  //--inv1_1 -----------inv4_1_ip0
  //        |----------inv4_2_ip0
  //        |----------inv4_3_ip0
  //        |----------inv4_4_ip0
  //
  //
  // After:
  //
  //--inv1_1-----------inv4_1_ip0
  //        |----------inv4_2_ip0
  //        |----------inv4_3_ip0
  //
  //--inv1_2---------inv4_4_ip0
  //
  //
  //
  // Objects referenced in "before"
  //
  // inv1_1 -- dbInst in inv1_1
  // inv1_1_inst_op0 -- op iterm on inv1
  // inv1_mod_master -- dbModule parent for inv1_1
  // inv4_mod_level2_master -- dbModule parent for leaf level inverters in inv4
  // inv4_4_ip0 -- ip iterm on 4th inverter
  //
  //

  //
  // first create the new inverter in inv1_mod_master
  // and get its pins ready for connection.
  // Note we are declaring these new objects outside
  // of the test struct so it is obvious what is in the test harness
  // and what is in the test
  //
  dbInst* inv1_2 = dbInst::create(block_,
                                  lib_->findMaster("INV_X1"),
                                  "inv1_mod_inst/inst2",
                                  false,
                                  inv1_mod_master_);
  dbITerm* inv1_2_inst_ip0 = inv1_2->findITerm("A");
  dbITerm* inv1_2_inst_op0 = inv1_2->findITerm("ZN");

  // Plumb in the new input of the inverter
  // This is the ip0_net, which is hooked to the top level bterm.
  // And to the top level modnet (which connects the modbterm on
  // inv1_mod_master) to the inverter. Note the dual connection:
  // one flat, one hierarchical.

  inv1_2_inst_ip0->connect(ip0_net_);             // flat world
  inv1_2_inst_ip0->connect(inv1_mod_i0_modnet_);  // hierarchical world

  // now disconnect the 4th inverter in inv4_4 (this is in level 2, the 3rd
  // level, of the hierarchy).
  // This kills both the dbNet (flat) connection
  // and the dbModNet (hierarchical) connection.

  inv4_4_ip_->disconnect();

  //
  // Make the flat connection.
  // We keep two worlds: the modnet world
  // and the dbNet world. The modnet world
  // exists at the edges eg where a module ports
  //(dbModITerms) connect to core gates (eg dbITerms).
  //
  // The flat world is always there.
  //
  //
  std::string flat_net_name = inv1_2->getName() + inv4_4_ip_->getName('/');
  std::string hier_net_name = "test_hier_" + flat_net_name;

  dbNet* flat_net = dbNet::create(block_, flat_net_name.c_str(), false);

  inv1_2_inst_op0->connect(flat_net);

  inv4_4_ip_->connect(flat_net);

  //
  // The hierarchical test: connect the output of the inverter in
  // inv1_2_inst_op0 to the input of the inverter inv4_4_ip0;
  //

  db_network_->hierarchicalConnect(
      inv1_2_inst_op0, inv4_4_ip_, hier_net_name.c_str());

  if (kDebugMsgs) {
    std::stringstream str_str_final;
    DbStrDebugHierarchy(block_, str_str_final);
    printf("The final design: %s\n", str_str_final.str().c_str());
  }

  // Example of how to turn on the call backs for all the bterms/iterms
  // used by the sta
  dbSet<dbModNet> mod_nets = block_->getModNets();
  for (auto mnet : mod_nets) {
    sta::NetSet visited_nets;
    // given one mod net go get all its low level objects to
    // issue call back on
    sta::Net* cur_net = db_network_->dbToSta(mnet);
    sta::NetConnectedPinIterator* npi
        = db_network_->connectedPinIterator(cur_net);
    while (npi->hasNext()) {
      const sta::Pin* cur_pin = npi->next();
      odb::dbModITerm* moditerm;
      odb::dbITerm* iterm;
      odb::dbBTerm* bterm;
      db_network_->staToDb(cur_pin, iterm, bterm, moditerm);
      if (iterm) {
        db_network_->connectPinAfter(const_cast<sta::Pin*>(cur_pin));
        sta_->connectPinAfter(const_cast<sta::Pin*>(cur_pin));
      }
      if (bterm) {
        db_network_->connectPinAfter(const_cast<sta::Pin*>(cur_pin));
        sta_->connectPinAfter(const_cast<sta::Pin*>(cur_pin));
      }
      if (moditerm) {
        ;
      }
    }
  }

  // Get the final design state statistics

  size_t final_db_net_count = block_->getNets().size();
  size_t final_mod_net_count = block_->getModNets().size();

  EXPECT_EQ(initial_db_net_count, 6);
  EXPECT_EQ(initial_mod_net_count, 23);
  EXPECT_EQ(final_mod_net_count, 28);
  EXPECT_EQ(final_db_net_count, 7);

  //
  // Journalling test.
  // Undo everything and check initial state preserved
  //
  odb::dbDatabase::undoEco(block_);

  size_t restored_db_net_count = block_->getNets().size();
  size_t restored_mod_net_count = block_->getModNets().size();

  EXPECT_EQ(restored_mod_net_count, initial_mod_net_count);
  EXPECT_EQ(restored_db_net_count, initial_db_net_count);

  // Test deletion of a dbModInst
  size_t num_mod_insts_before_delete = block_->getModInsts().size();
  size_t num_mod_iterms_before_delete = block_->getModITerms().size();
  dbModInst::destroy(inv1_mod_inst_);
  size_t num_mod_insts_after_delete = block_->getModInsts().size();
  size_t num_mod_iterms_after_delete = block_->getModITerms().size();

  // Test deletion of a dbModule
  size_t num_mods_before_delete = block_->getModules().size();
  size_t num_mod_bterms_before_delete = block_->getModBTerms().size();
  dbModule::destroy(inv1_mod_master_);
  size_t num_mods_after_delete = block_->getModules().size();
  size_t num_mod_bterms_after_delete = block_->getModBTerms().size();

  // we have killed one module instance
  EXPECT_EQ((num_mods_before_delete - num_mods_after_delete), 1);
  // we have killed one module
  EXPECT_EQ((num_mod_insts_before_delete - num_mod_insts_after_delete), 1);
  // we are deleting an inverter, so expect to delete 2 ports
  EXPECT_EQ((num_mod_bterms_before_delete - num_mod_bterms_after_delete), 2);
  // and the number of moditerms reduced should be the same (2)
  EXPECT_EQ((num_mod_iterms_before_delete - num_mod_iterms_after_delete),
            (num_mod_bterms_before_delete - num_mod_bterms_after_delete));
}

}  // namespace odb
