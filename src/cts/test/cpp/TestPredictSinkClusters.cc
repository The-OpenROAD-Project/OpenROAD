// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "CtsOptions.h"
#include "cts/TritonCTS.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Scene.hh"
#include "tst/IntegratedFixture.h"

namespace cts {

namespace {

// Sink instance names of each leaf cluster.
using SinkGroups = std::set<std::vector<std::string>>;

}  // namespace

// check_buffers.def: 288 placed flops on clk, enough for sink clustering.
class PredictSinkClustersTest : public tst::IntegratedFixture
{
 protected:
  PredictSinkClustersTest()
      : tst::IntegratedFixture(tst::Technology::kNangate45,
                               "_main/src/cts/test/")
  {
  }

  void SetUp() override
  {
    odb::dbChip* chip = odb::dbChip::create(db_.get(), lib_->getTech());
    std::vector<odb::dbLib*> libs{lib_};
    odb::defin reader(db_.get(), &logger_);
    const std::string def = getFilePath(test_root_path_ + "check_buffers.def");
    ASSERT_TRUE(reader.readChip(libs, def.c_str(), chip));
    block_ = chip->getBlock();
    sta_->postReadDef(block_);

    constexpr float kPeriod = 5e-9;
    sta::PinSet clk_pins(db_network_);
    clk_pins.insert(db_network_->dbToSta(block_->findBTerm("clk")));
    sta_->makeClock("clk",
                    clk_pins,
                    false,
                    kPeriod,
                    {0, kPeriod / 2},
                    "",
                    sta_->cmdScene()->mode());
    // set_wire_rc -clock
    constexpr double kRes = 2.5e5;  // ohms/meter
    constexpr double kCap = 2e-10;  // farads/meter
    ep_.setHWireClkRC(db_->getTech(), sta_->cmdScene(), kRes, kCap);
    ep_.setVWireClkRC(db_->getTech(), sta_->cmdScene(), kRes, kCap);

    cts_ = std::make_unique<TritonCTS>(
        &logger_, db_.get(), db_network_, sta_.get(), &stt_, &resizer_, &ep_);
    // clock_tree_synthesis -sink_clustering_enable
    cts_->getParms()->setSinkClustering(true);
  }

  SinkGroups predicted()
  {
    SinkGroups groups;
    for (const std::vector<odb::dbITerm*>& cluster :
         cts_->predictSinkClusters({block_->findNet("clk")})) {
      std::vector<std::string> names;
      for (odb::dbITerm* sink : cluster) {
        names.push_back(sink->getInst()->getName());
      }
      std::ranges::sort(names);
      groups.insert(names);
    }
    return groups;
  }

  // Flops grouped by the net that clocks them after CTS.
  SinkGroups built()
  {
    cts_->runTritonCts();
    std::map<std::string, std::vector<std::string>> by_net;
    for (odb::dbInst* inst : block_->getInsts()) {
      odb::dbITerm* ck = inst->findITerm("CK");
      if (ck != nullptr && ck->getNet() != nullptr) {
        by_net[ck->getNet()->getName()].push_back(inst->getName());
      }
    }
    SinkGroups groups;
    for (auto& [net, names] : by_net) {
      std::ranges::sort(names);
      groups.insert(names);
    }
    return groups;
  }

  std::unique_ptr<TritonCTS> cts_;
};

// Sizes and diameter given: clusters are bounded by them, not by cap.
TEST_F(PredictSinkClustersTest, MatchesCtsWithSizeAndDiameter)
{
  cts_->setBufferList("CLKBUF_X3");
  cts_->setRootBuffer("CLKBUF_X3");
  cts_->getParms()->setSinkClusteringSize(10);
  cts_->getParms()->setMaxDiameter(60);

  const SinkGroups prediction = predicted();
  ASSERT_GT(prediction.size(), 1);
  EXPECT_EQ(prediction, built());
}

// No size or diameter: clusters are bounded by the sink buffer cap budget.
// Buffers inferred and selected, as clock_tree_synthesis without -buf_list.
TEST_F(PredictSinkClustersTest, MatchesCtsWithCapBudget)
{
  cts_->setBufferList("");

  const SinkGroups prediction = predicted();
  ASSERT_GT(prediction.size(), 1);
  EXPECT_EQ(prediction, built());
}

}  // namespace cts
