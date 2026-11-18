// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "timing_report.h"
#include "tst/db_fixture.h"
#include "tst/nangate45_fixture.h"

namespace web {
namespace {

// Two clock domains, each a pair of flops feeding each other:
//
//   clk_a: a0/Q -> a1/D, a1/Q -> a0/D
//   clk_b: b0/Q -> b1/D, b1/Q -> b0/D
//
// Each clock gets its default path group, named after it, and a group_path,
// to_b1, takes b1/D out of clk_b's, as the chart's path group filter lists
// only group_path groups.  One setup path ends on each flop.
class TimingReportTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    readLiberty("_main/test/Nangate45/Nangate45_typ.lib");
    block_->setDieArea(odb::Rect(0, 0, 100000, 100000));

    odb::dbMaster* buf = lib_->findMaster("BUF_X1");
    odb::dbMaster* dff = lib_->findMaster("DFF_X1");
    ASSERT_NE(buf, nullptr);
    ASSERT_NE(dff, nullptr);
    for (const char* d : {"a", "b"}) {
      const std::string clk = std::string("clk_") + d;
      const std::string root = std::string("c") + d;
      const std::string q0 = std::string("q") + d + "0";
      const std::string q1 = std::string("q") + d + "1";
      const std::string root_in = root + "_in";
      tst::InstOptions opts;
      opts.iterms = {
          {.net_name = root_in.c_str(), .term_name = "A"},
          {.net_name = clk.c_str(), .term_name = "Z"},
      };
      makeInst(block_, buf, root.c_str(), opts);
      opts.iterms = {
          {.net_name = clk.c_str(), .term_name = "CK"},
          {.net_name = q1.c_str(), .term_name = "D"},
          {.net_name = q0.c_str(), .term_name = "Q"},
      };
      makeInst(block_, dff, (std::string(d) + "0").c_str(), opts);
      opts.iterms = {
          {.net_name = clk.c_str(), .term_name = "CK"},
          {.net_name = q0.c_str(), .term_name = "D"},
          {.net_name = q1.c_str(), .term_name = "Q"},
      };
      makeInst(block_, dff, (std::string(d) + "1").c_str(), opts);
    }

    sta_->postReadDef(block_);
    sta_->getDbNetwork()->setBlock(block_);

    makeClock("clk_a", "ca/Z", clk_a_period_ns_);
    makeClock("clk_b", "cb/Z", 2.0);
    makeGroupPath("to_b1", {"b1/D"});
  }

  void makeGroupPath(const char* name, const std::vector<const char*>& to_pins)
  {
    sta::dbNetwork* network = sta_->getDbNetwork();
    auto* pins = new sta::PinSet(network);
    for (const char* pin_name : to_pins) {
      pins->insert(network->findPin(pin_name));
    }
    sta::ExceptionTo* to = sta_->makeExceptionTo(pins,
                                                 /*to_clks=*/nullptr,
                                                 /*to_insts=*/nullptr,
                                                 sta::RiseFallBoth::riseFall(),
                                                 sta::RiseFallBoth::riseFall(),
                                                 sta_->cmdSdc());
    sta_->makeGroupPath(name,
                        /*is_default=*/false,
                        /*from=*/nullptr,
                        /*thrus=*/nullptr,
                        to,
                        /*comment=*/"",
                        sta_->cmdSdc());
  }

  void makeClock(const char* name,
                 const char* pin_name,
                 const double period_ns,
                 const bool add_to_pins = false)
  {
    sta::dbNetwork* network = sta_->getDbNetwork();
    sta::PinSet pins(network);
    pins.insert(network->findPin(pin_name));
    const float period = sta_->units()->timeUnit()->userToSta(period_ns);
    sta_->makeClock(name,
                    pins,
                    add_to_pins,
                    period,
                    {0, period / 2},
                    /*comment=*/"",
                    sta_->cmdMode());
  }

  void setPortDelay(const char* port,
                    const char* clk_name,
                    const bool input,
                    const sta::RiseFallBoth* rf = sta::RiseFallBoth::riseFall(),
                    const double delay_ns = 0.0)
  {
    const sta::Pin* pin = sta_->getDbNetwork()->findPin(port);
    ASSERT_NE(pin, nullptr);
    const sta::Clock* clk = sta_->cmdSdc()->findClock(clk_name);
    ASSERT_NE(clk, nullptr);
    const float delay = sta_->units()->timeUnit()->userToSta(delay_ns);
    if (input) {
      sta_->setInputDelay(pin,
                          rf,
                          clk,
                          sta::RiseFall::rise(),
                          /*ref_pin=*/nullptr,
                          /*source_latency_included=*/false,
                          /*network_latency_included=*/false,
                          sta::MinMaxAll::all(),
                          /*add=*/false,
                          delay,
                          sta_->cmdSdc());
    } else {
      sta_->setOutputDelay(pin,
                           rf,
                           clk,
                           sta::RiseFall::rise(),
                           /*ref_pin=*/nullptr,
                           /*source_latency_included=*/false,
                           /*network_latency_included=*/false,
                           sta::MinMaxAll::all(),
                           /*add=*/false,
                           delay,
                           sta_->cmdSdc());
    }
  }

  // Expects each bar of the filtered setup histogram to list as many paths as
  // it counts; returns how many bars list each end pin.
  std::map<std::string, int> barsListing(
      const std::vector<std::string>& path_groups,
      const std::string& clock_name) const
  {
    TimingReport timing_report(getSta());
    const SlackHistogramResult histogram = timing_report.getSlackHistogram(
        /*is_setup=*/true, clock_name, path_groups);
    std::map<std::string, int> listing;
    for (const auto& bin : histogram.bins) {
      const std::vector<TimingPathSummary> paths
          = timing_report.getReport(/*is_setup=*/true,
                                    /*max_paths=*/100,
                                    bin.lower,
                                    bin.upper,
                                    /*include_unconstrained=*/false,
                                    path_groups,
                                    clock_name);
      EXPECT_EQ(static_cast<int>(paths.size()), bin.count)
          << "[" << bin.lower << ", " << bin.upper << ")";
      for (const TimingPathSummary& path : paths) {
        EXPECT_GE(path.slack, bin.lower) << path.end_pin;
        EXPECT_LT(path.slack, bin.upper) << path.end_pin;
        listing[path.end_pin]++;
      }
    }
    return listing;
  }

  // The listing of `pins` under one bar each.
  static std::map<std::string, int> onceEach(
      const std::vector<std::string>& pins)
  {
    std::map<std::string, int> listing;
    for (const std::string& pin : pins) {
      listing[pin] = 1;
    }
    return listing;
  }

  std::vector<TimingPathSummary> report(
      const std::vector<std::string>& path_groups,
      const std::string& clock_name,
      const int max_paths = 100) const
  {
    TimingReport timing_report(getSta());
    return timing_report.getReport(/*is_setup=*/true,
                                   max_paths,
                                   -std::numeric_limits<float>::max(),
                                   std::numeric_limits<float>::max(),
                                   /*include_unconstrained=*/false,
                                   path_groups,
                                   clock_name);
  }

  static std::multiset<std::string> endPins(
      const std::vector<TimingPathSummary>& paths)
  {
    std::multiset<std::string> pins;
    for (const TimingPathSummary& path : paths) {
      pins.insert(path.end_pin);
    }
    return pins;
  }

  static std::multiset<std::string> endClocks(
      const std::vector<TimingPathSummary>& paths)
  {
    std::multiset<std::string> clocks;
    for (const TimingPathSummary& path : paths) {
      clocks.insert(path.end_clk);
    }
    return clocks;
  }

  double clk_a_period_ns_ = 1.0;
};

TEST_F(TimingReportTest, NoFilterReportsEveryClock)
{
  EXPECT_EQ(endClocks(report({}, "")),
            (std::multiset<std::string>{"clk_a", "clk_a", "clk_b", "clk_b"}));
}

// As the histogram counts a clock with no path group: its default group,
// which to_b1 took b1/D out of.
TEST_F(TimingReportTest, ClockFilterKeepsItsDefaultGroup)
{
  EXPECT_EQ(endPins(report({}, "clk_b")), (std::multiset<std::string>{"b0/D"}));
}

TEST_F(TimingReportTest, PathGroupFilterKeepsItsPaths)
{
  EXPECT_EQ(endPins(report({"to_b1"}, "")),
            (std::multiset<std::string>{"b1/D"}));
}

// For every filter the chart sends, each bar of the slack histogram lists as
// many paths as it counts, and the report lists every path the histogram
// counted.
TEST_F(TimingReportTest, ReportListsWhatHistogramCounts)
{
  struct Filter
  {
    std::vector<std::string> path_groups;
    std::string clock_name;
  };
  const std::vector<Filter> filters = {
      {.path_groups = {}, .clock_name = ""},
      {.path_groups = {}, .clock_name = "clk_a"},
      {.path_groups = {}, .clock_name = "clk_b"},
      {.path_groups = {"to_b1"}, .clock_name = ""},
      {.path_groups = {"to_b1"}, .clock_name = "clk_a"},
      {.path_groups = {"to_b1"}, .clock_name = "clk_b"},
      {.path_groups = {}, .clock_name = "clk_c"},
      {.path_groups = {"to_b1"}, .clock_name = "clk_c"},
  };
  for (const Filter& filter : filters) {
    SCOPED_TRACE("path_groups=" + testing::PrintToString(filter.path_groups)
                 + " clock_name=" + filter.clock_name);
    int counted = 0;
    for (const auto& [end_pin, bars] :
         barsListing(filter.path_groups, filter.clock_name)) {
      counted += bars;
    }
    EXPECT_EQ(
        static_cast<int>(report(filter.path_groups, filter.clock_name).size()),
        counted);
  }
}

// max_paths caps the paths of the clock filtered on: a group spanning two
// clocks, its worst path on the other clock, still lists the clock's.
TEST_F(TimingReportTest, MaxPathsCapsTheClocksPaths)
{
  makeGroupPath("mixed", {"a0/D", "b0/D"});
  EXPECT_EQ(endPins(report({"mixed"}, "clk_b", /*max_paths=*/1)),
            (std::multiset<std::string>{"b0/D"}));
}

// A bar of a histogram of several path groups lists every group's endpoints.
TEST_F(TimingReportTest, SeveralPathGroupsListEveryGroup)
{
  makeGroupPath("to_a", {"a0/D", "a1/D"});
  EXPECT_EQ(barsListing({"to_a", "to_b1"}, ""),
            onceEach({"a0/D", "a1/D", "b1/D"}));
}

TEST_F(TimingReportTest, UnknownClockReportsNothing)
{
  EXPECT_TRUE(report({}, "clk_c").empty());
}

TEST_F(TimingReportTest, DisjointFiltersReportNothing)
{
  EXPECT_TRUE(report({"to_b1"}, "clk_a").empty());
}

// The flops above, clk_a's period shorter than its flops' clock-to-Q and
// setup, so every clk_a endpoint fails.
class TimingReportFailingTest : public TimingReportTest
{
 protected:
  void SetUp() override
  {
    clk_a_period_ns_ = 0.05;
    TimingReportTest::SetUp();
  }
};

// A histogram of negative slacks only ends at 0, with no positive bar.
TEST_F(TimingReportFailingTest, NegativeSlacksHaveNoPositiveBar)
{
  TimingReport timing_report(getSta());
  const SlackHistogramResult histogram
      = timing_report.getSlackHistogram(/*is_setup=*/true,
                                        /*clock_name=*/"clk_a");
  // The first filtered histogram since makeClock invalidated the arrivals.
  ASSERT_FALSE(histogram.bins.empty());
  for (const auto& bin : histogram.bins) {
    EXPECT_LT(bin.lower, 0.0f) << "[" << bin.lower << ", " << bin.upper << ")";
  }
}

// A second clock on clk_a's root captures the clk_a flops too; each is still
// counted once, so it is listed once.
TEST_F(TimingReportTest, TwoClocksOnOneRootListEachEndpointOnce)
{
  makeClock("clk_a2", "ca/Z", 0.5, /*add_to_pins=*/true);
  EXPECT_EQ(barsListing({}, ""), onceEach({"a0/D", "a1/D", "b0/D", "b1/D"}));
}

// An output delay makes a0/Q an endpoint with fanout: its own slack (about
// 2.9) puts it in a bar, not a1/D's downstream slack (about 0.9).
TEST_F(TimingReportTest, FanoutEndpointUsesItsOwnSlack)
{
  setPortDelay("a0/Q", "clk_a", false, sta::RiseFallBoth::riseFall(), -2.0);
  EXPECT_EQ(barsListing({}, ""),
            onceEach({"a0/D", "a1/D", "b0/D", "b1/D", "a0/Q"}));
}

// The flops above, plus a wire from an input port to an output port per
// clock, both delays 0, so that path's slack is the clock's period: 1 and
// 2, edges of the histogram's bars.
class TimingReportBinEdgeTest : public TimingReportTest
{
 protected:
  void SetUp() override
  {
    for (const char* d : {"a", "b"}) {
      odb::dbNet* net
          = odb::dbNet::create(block_, (std::string("p") + d).c_str());
      odb::dbBTerm::create(net, (std::string("in_") + d).c_str())
          ->setIoType(odb::dbIoType::INPUT);
      odb::dbBTerm::create(net, (std::string("out_") + d).c_str())
          ->setIoType(odb::dbIoType::OUTPUT);
    }
    TimingReportTest::SetUp();
    for (const char* d : {"a", "b"}) {
      const std::string clk = std::string("clk_") + d;
      setPortDelay((std::string("in_") + d).c_str(), clk.c_str(), true);
      setPortDelay((std::string("out_") + d).c_str(), clk.c_str(), false);
    }
  }
};

// Every endpoint the histogram counts is listed by exactly one bar's
// drill-down, and each bar lists as many paths as it counts, also for the
// port paths whose slack sits on an edge.
TEST_F(TimingReportBinEdgeTest, EachPathIsListedUnderOneBar)
{
  EXPECT_EQ(barsListing({}, ""),
            onceEach({"a0/D", "a1/D", "b0/D", "b1/D", "out_a", "out_b"}));
}

// out_a's fall slack (0.5) and rise slack (1) fall in different bars; the
// endpoint is listed once, under its worst slack's bar.
TEST_F(TimingReportBinEdgeTest, RiseAndFallInDifferentBarsListOnce)
{
  setPortDelay("in_a", "clk_a", true, sta::RiseFallBoth::fall(), 0.5);
  EXPECT_EQ(barsListing({}, ""),
            onceEach({"a0/D", "a1/D", "b0/D", "b1/D", "out_a", "out_b"}));
}

// The flops above, plus r0, a DFFR_X1 on clk_a whose RN comes from the port
// rst: a recovery check ends on r0/RN.
class TimingReportRecoveryTest : public TimingReportTest
{
 protected:
  void SetUp() override
  {
    makeBTerm(block_, "rst");
    odb::dbMaster* dffr = lib_->findMaster("DFFR_X1");
    ASSERT_NE(dffr, nullptr);
    tst::InstOptions opts;
    opts.iterms = {
        {.net_name = "clk_a", .term_name = "CK"},
        {.net_name = "rst", .term_name = "RN"},
        {.net_name = "qa0", .term_name = "D"},
        {.net_name = "qr", .term_name = "Q"},
    };
    makeInst(block_, dffr, "r0", opts);
    TimingReportTest::SetUp();
    setPortDelay("rst", "clk_a", true);
  }
};

// The unfiltered histogram counts the recovery check on r0/RN, so a bar
// lists it.
TEST_F(TimingReportRecoveryTest, RecoveryEndpointIsListed)
{
  EXPECT_EQ(barsListing({}, ""),
            onceEach({"a0/D", "a1/D", "b0/D", "b1/D", "r0/D", "r0/RN"}));
}

}  // namespace
}  // namespace web
