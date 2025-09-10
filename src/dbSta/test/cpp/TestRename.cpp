// Copyright 2024 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <tcl.h>
#include <unistd.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>

#include "dbBlock.h"
#include "dbJournal.h"
#include "db_sta/MakeDbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/lefin.h"
#include "sta/Corner.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/PathAnalysisPt.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace odb {

std::once_flag init_sta_flag_rename;

class TestRename : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    db_ = utl::UniquePtrWithDeleter<odb::dbDatabase>(odb::dbDatabase::create(),
                                                     &odb::dbDatabase::destroy);
    std::call_once(init_sta_flag_rename, []() { sta::initSta(); });
    sta_ = std::unique_ptr<sta::dbSta>(sta::makeDbSta());
    sta_->initVars(Tcl_CreateInterp(), db_.get(), &logger_);
    auto path = std::filesystem::canonical("./Nangate45/Nangate45_typ.lib");
    library_ = sta_->readLiberty(path.string().c_str(),
                                 sta_->findCorner("default"),
                                 sta::MinMaxAll::all(),
                                 false);
    odb::lefin lef_parser(db_.get(), &logger_, false);
    const char* lib_name = "Nangate45.lef";
    lib_ = lef_parser.createTechAndLib(
        "tech", lib_name, "./Nangate45/Nangate45.lef");

    sta_->postReadLef(nullptr, lib_);

    db_network_ = sta_->getDbNetwork();
    db_->setLogger(&logger_);

    odb::dbChip* chip = odb::dbChip::create(db_.get());
    block_ = odb::dbBlock::create(chip, "top");
    db_network_->setBlock(block_);
    block_->setDieArea(odb::Rect(0, 0, 1000, 1000));
    sta_->postReadDef(block_);
  }

  utl::UniquePtrWithDeleter<odb::dbDatabase> db_;
  std::unique_ptr<sta::dbSta> sta_;
  sta::LibertyLibrary* library_;
  utl::Logger logger_;
  sta::dbNetwork* db_network_;
  dbBlock* block_;
  odb::dbLib* lib_;
};

TEST_F(TestRename, RenameNet)
{
  dbNet* net = dbNet::create(block_, "original_net_name");
  ASSERT_NE(net, nullptr);
  EXPECT_EQ(net->getName(), "original_net_name");

  odb::dbDatabase::beginEco(block_);
  net->rename("new_net_name");
  EXPECT_EQ(net->getName(), "new_net_name");
  odb::dbDatabase::endEco(block_);

  odb::dbDatabase::undoEco(block_);
  EXPECT_EQ(net->getName(), "original_net_name");

  dbNet::destroy(net);
}

TEST_F(TestRename, RenameInst)
{
  dbMaster* master = lib_->findMaster("INV_X1");
  ASSERT_NE(master, nullptr);
  dbInst* inst = dbInst::create(block_, master, "original_inst_name");
  ASSERT_NE(inst, nullptr);
  EXPECT_EQ(inst->getName(), "original_inst_name");

  odb::dbDatabase::beginEco(block_);
  inst->rename("new_inst_name");
  EXPECT_EQ(inst->getName(), "new_inst_name");
  odb::dbDatabase::endEco(block_);

  odb::dbDatabase::undoEco(block_);
  EXPECT_EQ(inst->getName(), "original_inst_name");

  dbInst::destroy(inst);
}

TEST_F(TestRename, RenameModNet)
{
  dbModule* module = dbModule::create(block_, "original_module_name");
  ASSERT_NE(module, nullptr);

  dbModNet* mod_net = dbModNet::create(module, "original_mod_net_name");
  ASSERT_NE(mod_net, nullptr);
  EXPECT_EQ(mod_net->getName(), "original_mod_net_name");

  odb::dbDatabase::beginEco(block_);
  mod_net->rename("new_mod_net_name");
  EXPECT_EQ(mod_net->getName(), "new_mod_net_name");
  odb::dbDatabase::endEco(block_);

  odb::dbDatabase::undoEco(block_);
  EXPECT_EQ(mod_net->getName(), "original_mod_net_name");

  dbModNet::destroy(mod_net);
  dbModule::destroy(module);
}

}  // namespace odb
