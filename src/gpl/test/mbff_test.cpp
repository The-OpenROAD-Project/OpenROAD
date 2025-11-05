// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-, The OpenROAD Authors

#include "src/gpl/src/mbff.h"

#include <cstdint>
#include <memory>
#include <string>

#include "absl/strings/str_cat.h"
#include "ant/AntennaChecker.hh"
#include "db_sta/dbReadVerilog.hh"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "src/gpl/src/graphicsNone.h"
#include "stt/SteinerTreeBuilder.h"
#include "tst/fixture.h"
#include "utl/CallBackHandler.h"

namespace gpl {

class MBFFTestPeer
{
 public:
  static bool IsValidTray(MBFF* uut, odb::dbInst* tray)
  {
    return uut->IsValidTray(tray);
  }
};

namespace {

class MBFFTestFixture : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    logger_ = getLogger();
    callback_handler_ = std::make_unique<utl::CallBackHandler>(logger_);
    verilog_network_ = std::make_unique<ord::dbVerilogNetwork>(getSta());
    stt_builder_ = std::make_unique<stt::SteinerTreeBuilder>(getDb(), logger_);
    antenna_checker_ = std::make_unique<ant::AntennaChecker>(getDb(), logger_);
    opendp_ = std::make_unique<dpl::Opendp>(getDb(), logger_);
    global_router_
        = std::make_unique<grt::GlobalRouter>(logger_,
                                              callback_handler_.get(),
                                              stt_builder_.get(),
                                              getDb(),
                                              getSta(),
                                              antenna_checker_.get(),
                                              opendp_.get());
    estimate_parasitics_
        = std::make_unique<est::EstimateParasitics>(logger_,
                                                    callback_handler_.get(),
                                                    getDb(),
                                                    getSta(),
                                                    stt_builder_.get(),
                                                    global_router_.get());

    resizer_ = std::make_unique<rsz::Resizer>(getLogger(),
                                              getDb(),
                                              getSta(),
                                              stt_builder_.get(),
                                              global_router_.get(),
                                              opendp_.get(),
                                              estimate_parasitics_.get());

    readLiberty(getFilePath("openroad/src/gpl/test/library/test/test0.lib"));
    loadTechAndLib("test0",
                   "test0",
                   getFilePath("openroad/src/gpl/test/library/test/test0.lef"));

    chip_ = odb::dbChip::create(db_.get(), db_->getTech());
    block_ = odb::dbBlock::create(chip_, "top");
    block_->setDefUnits(1000);
    block_->setDieArea(odb::Rect(0, 0, 10000, 10000));

    mbff_
        = std::make_unique<MBFF>(getDb(),
                                 getSta(),
                                 logger_,
                                 resizer_.get(),
                                 /*threads=*/1,
                                 /*multistart=*/20,
                                 /*num_paths=*/0,
                                 /*debug_graphics=*/false,
                                 /*graphics=*/std::make_unique<GraphicsNone>());
  }

  // Create a cell insidde the main block of database db.
  odb::dbInst* CreateTmpCell(const char* name,
                             const char* lib_name,
                             const char* master_name)
  {
    // Find a unique name.
    int64_t idx = 0;
    std::string cell_name = name;

    while (block_->findInst(cell_name.c_str()) != nullptr) {
      cell_name = absl::StrCat(name, ++idx);
    }

    odb::dbLib* lib = db_->findLib(lib_name);
    odb::dbMaster* master = lib->findMaster(master_name);
    return odb::dbInst::create(block_, master, cell_name.c_str());
  }

  utl::Logger* logger_;
  std::unique_ptr<utl::CallBackHandler> callback_handler_;
  std::unique_ptr<ord::dbVerilogNetwork> verilog_network_;
  std::unique_ptr<stt::SteinerTreeBuilder> stt_builder_;
  std::unique_ptr<ant::AntennaChecker> antenna_checker_;
  std::unique_ptr<dpl::Opendp> opendp_;
  std::unique_ptr<grt::GlobalRouter> global_router_;
  std::unique_ptr<est::EstimateParasitics> estimate_parasitics_;
  std::unique_ptr<rsz::Resizer> resizer_;
  std::unique_ptr<MBFF> mbff_;

  odb::dbLib* lib_;
  odb::dbChip* chip_;
  odb::dbBlock* block_;
};

TEST_F(MBFFTestFixture, FlopsCanBeIdentifiedAsATrayAndNot)
{
  // Retreive masters, create a test cell, and assert that they are correctly
  // identified as either a tray or not a tray.
  EXPECT_EQ(db_->findLib("test0")->getMasters().size(), 6);

  EXPECT_FALSE(MBFFTestPeer::IsValidTray(
      mbff_.get(), CreateTmpCell("test_tray", "test0", "INV")));
  EXPECT_FALSE(MBFFTestPeer::IsValidTray(
      mbff_.get(), CreateTmpCell("test_tray", "test0", "DFF")));
  EXPECT_TRUE(MBFFTestPeer::IsValidTray(
      mbff_.get(), CreateTmpCell("test_tray", "test0", "MBFF2")));
  EXPECT_TRUE(MBFFTestPeer::IsValidTray(
      mbff_.get(), CreateTmpCell("test_tray", "test0", "MBFF2SE")));
  EXPECT_TRUE(MBFFTestPeer::IsValidTray(
      mbff_.get(), CreateTmpCell("test_tray", "test0", "MBFF2CLPS")));
  EXPECT_TRUE(MBFFTestPeer::IsValidTray(
      mbff_.get(), CreateTmpCell("test_tray", "test0", "MBFF2SECLPS")));
}

}  // namespace
}  // namespace gpl
