// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cmath>
#include <cstdint>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "endpoint_slack.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
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

// The bins are upper-exclusive and so is select_net_length_bin, so the
// longest net must fall strictly inside the last bin.  When it only lands
// there by clamping, the bar counts nets that double-clicking it then fails
// to select.  The bug needs the longest net to be an exact multiple of the
// snapped bin width, so drive HPWL to chosen round values rather than hoping
// a geometry sweep lands on one.
TEST_F(NetLengthTest, LongestNetIsBelowTheLastBinUpperEdge)
{
  block_->setDieArea(odb::Rect(0, 0, 400000, 400000));
  odb::dbInst* src = placeInst("BUF_X16", "src", 0, 0);
  odb::dbInst* sink = placeInst("BUF_X16", "sink", 100000, 0);
  odb::dbNet* net = odb::dbNet::create(block_, "n");
  net->setSigType(odb::dbSigType::SIGNAL);
  iterm(src, odb::dbIoType::OUTPUT)->connect(net);
  iterm(sink, odb::dbIoType::INPUT)->connect(net);

  // HPWL tracks the sink's x with a fixed offset (pin geometry plus the
  // constant dy), so solve for the placement that yields an exact length.
  const int64_t offset = netHpwlDbu(net) - 100000;

  for (const int64_t target : {10000, 20000, 50000, 100000, 200000}) {
    sink->setLocation(static_cast<int>(target - offset), 0);
    ASSERT_EQ(netHpwlDbu(net), target);

    const NetLengthHistogramResult h = computeNetLengthHistogram(block_, true);
    ASSERT_FALSE(h.bins.empty()) << "target=" << target;
    const double bin_width = h.bins.front().upper - h.bins.front().lower;
    ASSERT_GT(bin_width, 0) << "target=" << target;
    // The case that used to break: the longest net sits exactly on a bin edge.
    ASSERT_EQ(std::fmod(static_cast<double>(target), bin_width), 0.0)
        << "target=" << target << " bin_width=" << bin_width;

    EXPECT_LT(static_cast<double>(target), h.bins.back().upper)
        << "target=" << target;
    EXPECT_GT(h.bins.back().count, 0)
        << "empty trailing bar, target=" << target;

    int sum = 0;
    for (const auto& bin : h.bins) {
      sum += bin.count;
    }
    EXPECT_EQ(sum, h.total_nets) << "target=" << target;
  }
}

TEST(NetHpwlTest, NullNetIsZero)
{
  EXPECT_EQ(netHpwlDbu(nullptr), 0);
}

//------------------------------------------------------------------------------
// assignEndpointsToWorstGroup — shared by the Qt and web stacked histograms.
//------------------------------------------------------------------------------

// sta::Pin is opaque, so any distinct address works as an endpoint key.
const sta::Pin* fakePin(int id)
{
  static char pins[4];
  return reinterpret_cast<const sta::Pin*>(&pins[id]);
}

TEST(EndpointWorstGroupTest, DisjointGroupsKeepEveryEndpoint)
{
  const EndpointSlacks a{{fakePin(1), 1.0f}, {fakePin(2), 2.0f}};
  const EndpointSlacks b{{fakePin(3), -1.0f}};

  const auto winners = assignEndpointsToWorstGroup({&a, &b});
  ASSERT_EQ(winners.size(), 3u);
  EXPECT_EQ(winners.at(fakePin(1)).group, 0u);
  EXPECT_EQ(winners.at(fakePin(2)).group, 0u);
  EXPECT_EQ(winners.at(fakePin(3)).group, 1u);
}

TEST(EndpointWorstGroupTest, SharedEndpointGoesToWorstSlack)
{
  const EndpointSlacks a{{fakePin(1), 0.5f}};
  const EndpointSlacks b{{fakePin(1), -0.25f}};

  const auto winners = assignEndpointsToWorstGroup({&a, &b});
  ASSERT_EQ(winners.size(), 1u);
  EXPECT_EQ(winners.at(fakePin(1)).group, 1u);
  EXPECT_EQ(winners.at(fakePin(1)).slack, -0.25f);
}

TEST(EndpointWorstGroupTest, TieGoesToEarlierGroup)
{
  const EndpointSlacks a{{fakePin(1), 0.5f}};
  const EndpointSlacks b{{fakePin(1), 0.5f}};

  EXPECT_EQ(assignEndpointsToWorstGroup({&a, &b}).at(fakePin(1)).group, 0u);
  EXPECT_EQ(assignEndpointsToWorstGroup({&b, &a}).at(fakePin(1)).group, 0u);
}

TEST(EndpointWorstGroupTest, EmptyGroupKeepsItsIndex)
{
  const EndpointSlacks a{{fakePin(1), 0.5f}};
  const EndpointSlacks empty;
  const EndpointSlacks c{{fakePin(1), -0.5f}, {fakePin(2), 0.0f}};

  const auto winners = assignEndpointsToWorstGroup({&a, &empty, &c});
  ASSERT_EQ(winners.size(), 2u);
  EXPECT_EQ(winners.at(fakePin(1)).group, 2u);
  EXPECT_EQ(winners.at(fakePin(2)).group, 2u);
}

//------------------------------------------------------------------------------
// Stacked slack histogram over path groups that share an endpoint.
//
//   ff1/QN -> ff1/D                    (reg2reg)
//   ff1/Q  -> a1/A1, a1/ZN -> ff2/D    (reg2reg)
//   in     -> a1/A2, a1/ZN -> ff2/D    (in2reg)
//
// r2r = -from ff1/CK, i2r = -from in, g1 = g2 = -to ff2/D.
//------------------------------------------------------------------------------

class SlackHistogramGroupsTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    readLiberty("_main/test/Nangate45/Nangate45_typ.lib");

    makeBTerm(block_, "clk");
    makeBTerm(block_, "in");

    odb::dbMaster* dff = lib_->findMaster("DFF_X1");
    odb::dbMaster* and2 = lib_->findMaster("AND2_X1");
    ASSERT_NE(dff, nullptr);
    ASSERT_NE(and2, nullptr);

    tst::InstOptions ff1;
    ff1.iterms
        = {{"clk", "CK"}, {"ff1_q", "Q"}, {"ff1_qn", "QN"}, {"ff1_qn", "D"}};
    makeInst(block_, dff, "ff1", ff1);

    tst::InstOptions ff2;
    ff2.iterms = {{"clk", "CK"}, {"d2", "D"}};
    makeInst(block_, dff, "ff2", ff2);

    tst::InstOptions a1;
    a1.iterms = {{"ff1_q", "A1"}, {"in", "A2"}, {"d2", "ZN"}};
    makeInst(block_, and2, "a1", a1);

    sta_->postReadDef(block_);
    network_ = sta_->getDbNetwork();
    network_->setBlock(block_);
    sdc_ = sta_->cmdSdc();

    const float period = sta_->units()->timeUnit()->userToSta(1.0);
    sta::PinSet clk_pins(network_);
    clk_pins.insert(bterm("clk"));
    sta_->makeClock("clk",
                    clk_pins,
                    false,
                    period,
                    {0.0f, period / 2},
                    "",
                    sta_->cmdMode());

    // Late enough that in2reg is the worse path into ff2/D.
    sta_->setInputDelay(bterm("in"),
                        sta::RiseFallBoth::riseFall(),
                        sdc_->findClock("clk"),
                        sta::RiseFall::rise(),
                        nullptr,
                        false,
                        false,
                        sta::MinMaxAll::all(),
                        true,
                        0.8f * period,
                        sdc_);

    groupPathFrom("r2r", iterm("ff1", "CK"));
    groupPathFrom("i2r", bterm("in"));
    groupPathTo("g1", iterm("ff2", "D"));
    groupPathTo("g2", iterm("ff2", "D"));

    // The filtered histogram visits existing paths; the GUIs have found the
    // arrivals (unfiltered histogram) before any group is selected.
    sta_->updateTiming(false);
  }

  sta::Pin* bterm(const char* name) const
  {
    return network_->dbToSta(block_->findBTerm(name));
  }

  sta::Pin* iterm(const char* inst, const char* term) const
  {
    return network_->dbToSta(block_->findInst(inst)->findITerm(term));
  }

  void groupPathFrom(const char* name, const sta::Pin* pin)
  {
    auto* pins = new sta::PinSet(network_);
    pins->insert(pin);
    sta::ExceptionFrom* from = sta_->makeExceptionFrom(
        pins, nullptr, nullptr, sta::RiseFallBoth::riseFall(), sdc_);
    sta_->makeGroupPath(name, false, from, nullptr, nullptr, "", sdc_);
  }

  void groupPathTo(const char* name, const sta::Pin* pin)
  {
    auto* pins = new sta::PinSet(network_);
    pins->insert(pin);
    sta::ExceptionTo* to = sta_->makeExceptionTo(pins,
                                                 nullptr,
                                                 nullptr,
                                                 sta::RiseFallBoth::riseFall(),
                                                 sta::RiseFallBoth::riseFall(),
                                                 sdc_);
    sta_->makeGroupPath(name, false, nullptr, nullptr, to, "", sdc_);
  }

  static int binTotal(const SlackHistogramResult& h)
  {
    int sum = 0;
    for (const SlackHistogramBin& bin : h.bins) {
      sum += bin.count;
    }
    return sum;
  }

  static int seriesTotal(const SlackHistogramSeries& series)
  {
    int sum = 0;
    for (const int count : series.counts) {
      sum += count;
    }
    return sum;
  }

  sta::dbNetwork* network_ = nullptr;
  sta::Sdc* sdc_ = nullptr;
};

TEST_F(SlackHistogramGroupsTest, SharedEndpointCountsOnceInWorstGroup)
{
  TimingReport report(sta_.get());
  const SlackHistogramResult h
      = report.getSlackHistogram(true, "", "", {"r2r", "i2r"});

  // ff1/D (r2r) and ff2/D (r2r and i2r).
  EXPECT_EQ(h.total_endpoints, 2);
  EXPECT_EQ(h.unconstrained_count, 0);
  EXPECT_EQ(binTotal(h), 2);
  ASSERT_EQ(h.series.size(), 2u);
  EXPECT_EQ(h.series[0].name, "r2r");
  EXPECT_EQ(seriesTotal(h.series[0]), 1);
  EXPECT_EQ(seriesTotal(h.series[1]), 1);
}

TEST_F(SlackHistogramGroupsTest, SamePathEndInTwoGroupsGoesToFirstSelected)
{
  TimingReport report(sta_.get());

  const SlackHistogramResult g1_first
      = report.getSlackHistogram(true, "", "", {"g1", "g2"});
  EXPECT_EQ(g1_first.total_endpoints, 1);
  EXPECT_EQ(binTotal(g1_first), 1);
  ASSERT_EQ(g1_first.series.size(), 2u);
  EXPECT_EQ(seriesTotal(g1_first.series[0]), 1);
  EXPECT_EQ(seriesTotal(g1_first.series[1]), 0);

  const SlackHistogramResult g2_first
      = report.getSlackHistogram(true, "", "", {"g2", "g1"});
  ASSERT_EQ(g2_first.series.size(), 2u);
  EXPECT_EQ(g2_first.series[0].name, "g2");
  EXPECT_EQ(seriesTotal(g2_first.series[0]), 1);
  EXPECT_EQ(seriesTotal(g2_first.series[1]), 0);
}

}  // namespace
}  // namespace web
