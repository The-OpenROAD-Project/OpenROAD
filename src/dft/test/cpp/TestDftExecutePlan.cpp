// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// End-to-end tests of execute_dft_plan: scan replace, architect and stitch a
// design, then check the scan chains written to ODB.

#include <string>
#include <variant>

#include "DftConfig.hh"
#include "ScanArchitectConfig.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dft/Dft.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/NetworkClass.hh"
#include "sta/SdcClass.hh"
#include "sta/Units.hh"
#include "tst/fixture.h"
#include "tst/loaded_design.h"

namespace dft::test {
namespace {

bool isConnected(const std::variant<odb::dbBTerm*, odb::dbITerm*>& term)
{
  return std::visit([](auto* t) { return t != nullptr; }, term);
}

class TestDftExecutePlan : public tst::Fixture
{
 protected:
  TestDftExecutePlan()
  {
    readLiberty(kTestRoot + "sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib");
    lib_ = loadTechAndLib(
        "sky130", "sky130hd", kTestRoot + "sky130hd/sky130hd.tlef");
    updateLib(lib_, kTestRoot + "sky130hd/sky130_fd_sc_hd_merged.lef");
    sta_->postReadLef(/*tech=*/nullptr, lib_);
    db_network_ = sta_->getDbNetwork();
  }

  static inline const std::string kTestRoot = "_main/src/dft/test/";

  // Reads the design and defines a clock on its "clock" port
  void readDesign(const std::string& verilog_file, const char* top)
  {
    block_ = tst::readVerilogAndLink(getFilePath(kTestRoot + verilog_file),
                                     top,
                                     /*hierarchy=*/false,
                                     db_.get(),
                                     sta_.get(),
                                     &logger_,
                                     lib_);

    sta::Instance* top_inst = db_network_->topInstance();
    sta::Port* clk_port
        = db_network_->findPort(db_network_->cell(top_inst), "clock");
    ASSERT_NE(clk_port, nullptr);
    sta::PinSet clk_pins(db_network_);
    clk_pins.insert(db_network_->findPin(top_inst, clk_port));

    const float period = sta_->units()->timeUnit()->userToSta(2.0);
    sta::FloatSeq waveform{0.0f, period / 2.0f};
    sta_->makeClock("main_clock",
                    clk_pins,
                    /*add_to_pins=*/false,
                    period,
                    waveform,
                    /*comment=*/"",
                    sta_->cmdMode());
  }

  odb::dbLib* lib_{nullptr};
  odb::dbBlock* block_{nullptr};
  sta::dbNetwork* db_network_{nullptr};
};

// 10 flops with -max_length 1 -max_chains 6 have a chain length of 2, so only
// 5 chains can be filled. No empty sixth chain may reach stitching.
TEST_F(TestDftExecutePlan, MaxChainsAboveBalancedCountStitchesNoEmptyChain)
{
  readDesign("max_chain_count_sky130.v", "max_chain_count");

  Dft dft(db_.get(), sta_.get(), /*resizer=*/nullptr, &logger_);
  ScanArchitectConfig* config
      = dft.getMutableDftConfig()->getMutableScanArchitectConfig();
  config->setMaxLength(1);
  config->setMaxChains(6);

  dft.scanReplace();
  dft.executeDftPlan();

  odb::dbSet<odb::dbScanChain> chains = block_->getDft()->getScanChains();
  ASSERT_EQ(chains.size(), 5);
  for (odb::dbScanChain* chain : chains) {
    EXPECT_TRUE(isConnected(chain->getScanIn())) << chain->getName();
    EXPECT_TRUE(isConnected(chain->getScanOut())) << chain->getName();
    EXPECT_TRUE(isConnected(chain->getScanEnable())) << chain->getName();

    int num_insts = 0;
    for (odb::dbScanPartition* partition : chain->getScanPartitions()) {
      for (odb::dbScanList* list : partition->getScanLists()) {
        num_insts += list->getScanInsts().size();
      }
    }
    EXPECT_EQ(num_insts, 2) << chain->getName();
  }
}

}  // namespace
}  // namespace dft::test
