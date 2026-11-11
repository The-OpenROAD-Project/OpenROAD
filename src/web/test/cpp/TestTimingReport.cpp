// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <limits>
#include <set>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
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
// Each clock gets its default path group, named after it, so one setup path
// ends on each flop and each group and clock holds two of them.
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
                                   /*include_unconstrained=*/false,
                                   path_groups,
                                   clock_name);
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

TEST_F(TimingReportTest, ClockFilterKeepsPathsEndingOnIt)
{
  EXPECT_EQ(endClocks(report({}, "clk_b")),
            (std::multiset<std::string>{"clk_b", "clk_b"}));
}

TEST_F(TimingReportTest, PathGroupFilterKeepsItsPaths)
{
  EXPECT_EQ(endClocks(report({"clk_a"}, "")),
            (std::multiset<std::string>{"clk_a", "clk_a"}));
}

TEST_F(TimingReportTest, UnknownClockReportsNothing)
{
  EXPECT_TRUE(report({}, "clk_c").empty());
}

TEST_F(TimingReportTest, DisjointFiltersReportNothing)
{
  EXPECT_TRUE(report({"clk_a"}, "clk_b").empty());
}

}  // namespace
}  // namespace web
