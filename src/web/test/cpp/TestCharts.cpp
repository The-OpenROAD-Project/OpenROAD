// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "gtest/gtest.h"
#include "odb/db.h"
#include "timing_report.h"
#include "tst/nangate45_fixture.h"

namespace web {
namespace {

class NetLengthTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    block_->setDieArea(odb::Rect(0, 0, 200000, 200000));
    block_->setCoreArea(odb::Rect(0, 0, 200000, 200000));
  }

  odb::dbInst* placeInst(const char* master, const char* name, int x, int y)
  {
    odb::dbMaster* m = lib_->findMaster(master);
    EXPECT_NE(m, nullptr);
    odb::dbInst* inst = odb::dbInst::create(block_, m, name);
    inst->setLocation(x, y);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    return inst;
  }

  // Any iterm of `inst` matching `io`, falling back to the first iterm.
  static odb::dbITerm* iterm(odb::dbInst* inst, odb::dbIoType io)
  {
    odb::dbITerm* first = nullptr;
    for (odb::dbITerm* it : inst->getITerms()) {
      if (it->getSigType().isSupply()) {
        continue;
      }
      if (!first) {
        first = it;
      }
      if (it->getIoType() == io) {
        return it;
      }
    }
    return first;
  }
};

TEST_F(NetLengthTest, HpwlMatchesTermBBox)
{
  odb::dbInst* b1 = placeInst("BUF_X16", "b1", 0, 0);
  odb::dbInst* b2 = placeInst("BUF_X16", "b2", 20000, 8000);
  odb::dbNet* net = odb::dbNet::create(block_, "n1");
  net->setSigType(odb::dbSigType::SIGNAL);
  iterm(b1, odb::dbIoType::OUTPUT)->connect(net);
  iterm(b2, odb::dbIoType::INPUT)->connect(net);

  const odb::Rect bbox = net->getTermBBox();
  EXPECT_EQ(netHpwlDbu(net), bbox.dx() + bbox.dy());
  EXPECT_GT(netHpwlDbu(net), 0);
}

TEST_F(NetLengthTest, HistogramCountsSignalNetsOnly)
{
  odb::dbInst* b1 = placeInst("BUF_X16", "b1", 0, 0);
  odb::dbInst* b2 = placeInst("BUF_X16", "b2", 20000, 8000);

  odb::dbNet* sig = odb::dbNet::create(block_, "sig");
  sig->setSigType(odb::dbSigType::SIGNAL);
  iterm(b1, odb::dbIoType::OUTPUT)->connect(sig);
  iterm(b2, odb::dbIoType::INPUT)->connect(sig);

  odb::dbNet* pwr = odb::dbNet::create(block_, "VDD");
  pwr->setSigType(odb::dbSigType::POWER);

  NetLengthHistogramResult h = computeNetLengthHistogram(block_, true);
  EXPECT_EQ(h.total_nets, 1);  // supply net excluded
  EXPECT_EQ(h.length_unit, "DBU");

  int sum = 0;
  for (const auto& bin : h.bins) {
    sum += bin.count;
  }
  EXPECT_EQ(sum, h.total_nets);
}

TEST_F(NetLengthTest, UnitLabelFollowsUseDbu)
{
  EXPECT_EQ(computeNetLengthHistogram(block_, true).length_unit, "DBU");
  EXPECT_EQ(computeNetLengthHistogram(block_, false).length_unit, "µm");
}

TEST_F(NetLengthTest, EmptyBlockHasNoNets)
{
  NetLengthHistogramResult h = computeNetLengthHistogram(block_, false);
  EXPECT_EQ(h.total_nets, 0);
  EXPECT_TRUE(h.bins.empty());
}

TEST(NetHpwlTest, NullNetIsZero)
{
  EXPECT_EQ(netHpwlDbu(nullptr), 0);
}

}  // namespace
}  // namespace web
