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
#include "sta/ExceptionPath.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Sdc.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "timing_report.h"
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
      opts.iterms = {{root_in.c_str(), "A"}, {clk.c_str(), "Z"}};
      makeInst(block_, buf, root.c_str(), opts);
      opts.iterms = {{clk.c_str(), "CK"}, {q1.c_str(), "D"}, {q0.c_str(), "Q"}};
      makeInst(block_, dff, (std::string(d) + "0").c_str(), opts);
      opts.iterms = {{clk.c_str(), "CK"}, {q0.c_str(), "D"}, {q1.c_str(), "Q"}};
      makeInst(block_, dff, (std::string(d) + "1").c_str(), opts);
    }

    sta_->postReadDef(block_);
    sta_->getDbNetwork()->setBlock(block_);

    makeClock("clk_a", "ca/Z", 1.0);
    makeClock("clk_b", "cb/Z", 2.0);
    makeGroupPath("to_b1", {"b1/D"});
  }

  void makeGroupPath(const char* name, const std::vector<const char*>& to_pins)
  {
    sta::dbNetwork* network = sta_->getDbNetwork();
    auto* pins = new sta::PinSet(network);
    for (const char* pin_name : to_pins) {
      pins->insert(network->dbToSta(block_->findITerm(pin_name)));
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

  void makeClock(const char* name, const char* pin_name, const double period_ns)
  {
    sta::dbNetwork* network = sta_->getDbNetwork();
    sta::PinSet pins(network);
    pins.insert(network->dbToSta(block_->findITerm(pin_name)));
    const float period = sta_->units()->timeUnit()->userToSta(period_ns);
    sta_->makeClock(name,
                    pins,
                    /*add_to_pins=*/false,
                    period,
                    {0, period / 2},
                    /*comment=*/"",
                    sta_->cmdMode());
  }

  std::vector<TimingPathSummary> report(
      const std::vector<std::string>& path_groups,
      const std::string& clock_name) const
  {
    TimingReport timing_report(getSta());
    return timing_report.getReport(/*is_setup=*/true,
                                   /*max_paths=*/100,
                                   -std::numeric_limits<float>::max(),
                                   std::numeric_limits<float>::max(),
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
  TimingReport timing_report(getSta());
  for (const Filter& filter : filters) {
    SCOPED_TRACE("path_groups=" + testing::PrintToString(filter.path_groups)
                 + " clock_name=" + filter.clock_name);
    const SlackHistogramResult histogram
        = timing_report.getSlackHistogram(/*is_setup=*/true,
                                          /*path_group=*/"",
                                          filter.clock_name,
                                          filter.path_groups);
    int counted = 0;
    for (const auto& bin : histogram.bins) {
      const std::vector<TimingPathSummary> paths
          = timing_report.getReport(/*is_setup=*/true,
                                    /*max_paths=*/100,
                                    bin.lower,
                                    bin.upper,
                                    filter.path_groups,
                                    filter.clock_name);
      EXPECT_EQ(static_cast<int>(paths.size()), bin.count)
          << "[" << bin.lower << ", " << bin.upper << ")";
      counted += bin.count;
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
  TimingReport timing_report(getSta());
  EXPECT_EQ(endPins(timing_report.getReport(/*is_setup=*/true,
                                            /*max_paths=*/1,
                                            -std::numeric_limits<float>::max(),
                                            std::numeric_limits<float>::max(),
                                            {"mixed"},
                                            "clk_b")),
            (std::multiset<std::string>{"b0/D"}));
}

TEST_F(TimingReportTest, UnknownClockReportsNothing)
{
  EXPECT_TRUE(report({}, "clk_c").empty());
}

TEST_F(TimingReportTest, DisjointFiltersReportNothing)
{
  EXPECT_TRUE(report({"to_b1"}, "clk_a").empty());
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
      const sta::Clock* clk
          = sta_->cmdSdc()->findClock((std::string("clk_") + d).c_str());
      ASSERT_NE(clk, nullptr);
      setPortDelay((std::string("in_") + d).c_str(), clk, /*input=*/true);
      setPortDelay((std::string("out_") + d).c_str(), clk, /*input=*/false);
    }
  }

  void setPortDelay(const char* port, const sta::Clock* clk, const bool input)
  {
    sta::dbNetwork* network = sta_->getDbNetwork();
    const sta::Pin* pin = network->dbToSta(block_->findBTerm(port));
    ASSERT_NE(pin, nullptr);
    if (input) {
      sta_->setInputDelay(pin,
                          sta::RiseFallBoth::riseFall(),
                          clk,
                          sta::RiseFall::rise(),
                          /*ref_pin=*/nullptr,
                          /*source_latency_included=*/false,
                          /*network_latency_included=*/false,
                          sta::MinMaxAll::all(),
                          /*add=*/false,
                          /*delay=*/0.0f,
                          sta_->cmdSdc());
    } else {
      sta_->setOutputDelay(pin,
                           sta::RiseFallBoth::riseFall(),
                           clk,
                           sta::RiseFall::rise(),
                           /*ref_pin=*/nullptr,
                           /*source_latency_included=*/false,
                           /*network_latency_included=*/false,
                           sta::MinMaxAll::all(),
                           /*add=*/false,
                           /*delay=*/0.0f,
                           sta_->cmdSdc());
    }
  }
};

// Every endpoint the histogram counts is listed by exactly one bar's
// drill-down, and each bar lists as many paths as it counts, also for the
// port paths whose slack sits on an edge.
TEST_F(TimingReportBinEdgeTest, EachPathIsListedUnderOneBar)
{
  TimingReport timing_report(getSta());
  const SlackHistogramResult histogram
      = timing_report.getSlackHistogram(/*is_setup=*/true,
                                        /*path_group=*/"",
                                        /*clock_name=*/"",
                                        /*path_groups=*/{});
  ASSERT_FALSE(histogram.bins.empty());
  std::map<std::string, int> bars_listing;
  int counted = 0;
  for (const auto& bin : histogram.bins) {
    const std::vector<TimingPathSummary> paths
        = timing_report.getReport(/*is_setup=*/true,
                                  /*max_paths=*/100,
                                  bin.lower,
                                  bin.upper,
                                  /*path_groups=*/{},
                                  /*clock_name=*/"");
    EXPECT_EQ(static_cast<int>(paths.size()), bin.count)
        << "[" << bin.lower << ", " << bin.upper << ")";
    for (const TimingPathSummary& path : paths) {
      EXPECT_GE(path.slack, bin.lower) << path.end_pin;
      EXPECT_LT(path.slack, bin.upper) << path.end_pin;
      bars_listing[path.end_pin]++;
    }
    counted += bin.count;
  }
  EXPECT_EQ(counted, 6);
  for (const char* end_pin :
       {"a0/D", "a1/D", "b0/D", "b1/D", "out_a", "out_b"}) {
    EXPECT_EQ(bars_listing[end_pin], 1) << end_pin;
  }
}

}  // namespace
}  // namespace web
