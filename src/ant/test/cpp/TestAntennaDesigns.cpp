// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Antenna checks on real LEF/DEF designs: ant_check.lef/def (hand-tuned
// sky130 rules), merged_spacing.lef with sw130_random.def (sky130 rules as
// released), and small nets built on ant_check.lef. Each test asserts every
// ratio on its nets, read from the DesignCheck.

#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include "AntennaReport.hh"
#include "AntennaTestUtil.h"
#include "ant/AntennaChecker.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "tst/db_fixture.h"
#include "tst/physical_builder.h"

namespace ant::test {
namespace {

// Expected ratios, to the two decimals the report prints.
const std::vector<ExpectedPin> kAntCheckPins = {
    {"output50/A",
     {
         {"mcon",
          Ratio{0.14, 3.0, false},
          Ratio{0.14, 0.0, false},
          std::nullopt,
          std::nullopt},
         {"met1",
          Ratio{55.12, 0.0, false},
          Ratio{55.12, 0.0, false},
          Ratio{275.2, 10.0, true},
          Ratio{275.2, 0.0, false}},
         {"via",
          Ratio{0.11, 6.0, false},
          Ratio{0.25, 0.0, false},
          std::nullopt,
          std::nullopt},
         {"met2",
          Ratio{42.07, 0.0, false},
          Ratio{97.18, 0.0, false},
          Ratio{209.46, 10.0, true},
          Ratio{484.66, 0.0, false}},
         {"via2",
          Ratio{0.09, 6.0, false},
          Ratio{0.34, 0.0, false},
          std::nullopt,
          std::nullopt},
         {"met3",
          Ratio{11.7, 0.0, false},
          Ratio{108.88, 0.0, false},
          Ratio{43.22, 15.15, true},
          Ratio{527.89, 0.0, false}},
     }},
    {"_264_/B2",
     {
         {"mcon",
          Ratio{0.12, 3.0, false},
          Ratio{0.12, 0.0, false},
          std::nullopt,
          std::nullopt},
         {"met1",
          Ratio{0.45, 0.0, false},
          Ratio{0.45, 0.0, false},
          Ratio{2.21, 10.0, false},
          Ratio{2.21, 0.0, false}},
         {"via",
          Ratio{0.09, 6.0, false},
          Ratio{0.21, 0.0, false},
          std::nullopt,
          std::nullopt},
         {"met2",
          Ratio{42.07, 0.0, false},
          Ratio{42.52, 0.0, false},
          Ratio{209.46, 10.0, true},
          Ratio{211.67, 0.0, false}},
         {"via2",
          Ratio{0.09, 6.0, false},
          Ratio{0.3, 0.0, false},
          std::nullopt,
          std::nullopt},
         {"met3",
          Ratio{11.7, 0.0, false},
          Ratio{54.21, 0.0, false},
          Ratio{43.22, 15.15, true},
          Ratio{254.89, 0.0, false}},
     }},
};

const std::vector<ExpectedPin> kApi1Pins = {
    {"output50/A",
     {
         {"mcon",
          Ratio{0.14, 3.0, false},
          Ratio{0.14, 0.0, false},
          std::nullopt,
          std::nullopt},
         {"met1",
          Ratio{55.56, 0.0, false},
          Ratio{55.56, 0.0, false},
          Ratio{277.26, 400.0, false},
          Ratio{277.26, 0.0, false}},
         {"via",
          Ratio{0.11, 6.0, false},
          Ratio{0.25, 0.0, false},
          std::nullopt,
          std::nullopt},
         {"met2",
          Ratio{84.8, 0.0, false},
          Ratio{140.36, 0.0, false},
          Ratio{421.83, 400.0, true},
          Ratio{699.09, 0.0, false}},
         {"via2",
          Ratio{0.38, 6.0, false},
          Ratio{0.63, 0.0, false},
          std::nullopt,
          std::nullopt},
         {"met3",
          Ratio{40.49, 0.0, false},
          Ratio{180.85, 0.0, false},
          Ratio{218.01, 2878.88, false},
          Ratio{917.1, 0.0, false}},
     }},
    {"_264_/B2",
     {
         {"mcon",
          Ratio{0.12, 3.0, false},
          Ratio{0.12, 0.0, false},
          std::nullopt,
          std::nullopt},
         {"met1",
          Ratio{0.45, 0.0, false},
          Ratio{0.45, 0.0, false},
          Ratio{2.21, 400.0, false},
          Ratio{2.21, 0.0, false}},
         {"via",
          Ratio{0.09, 6.0, false},
          Ratio{0.21, 0.0, false},
          std::nullopt,
          std::nullopt},
         {"met2",
          Ratio{14.43, 0.0, false},
          Ratio{14.87, 0.0, false},
          Ratio{70.95, 400.0, false},
          Ratio{73.16, 0.0, false}},
         {"via2",
          Ratio{0.16, 6.0, false},
          Ratio{0.37, 0.0, false},
          std::nullopt,
          std::nullopt},
         {"met3",
          Ratio{40.49, 0.0, false},
          Ratio{55.36, 0.0, false},
          Ratio{218.01, 2878.88, false},
          Ratio{291.17, 0.0, false}},
     }},
};

const std::vector<ExpectedPin> kDisjointStubsPins = {
    {"u1/A",
     {
         {"li1",
          Ratio{16.93, 0.0, false},
          Ratio{16.93, 0.0, false},
          Ratio{20.08, 5.0, true},
          Ratio{20.08, 0.0, false}},
     }},
    {"u2/A",
     {
         {"li1",
          Ratio{16.93, 0.0, false},
          Ratio{16.93, 0.0, false},
          Ratio{20.08, 5.0, true},
          Ratio{20.08, 0.0, false}},
     }},
};

const std::vector<ExpectedPin> kOutputPinBridgePins = {
    {"u2/A",
     {
         {"li1",
          Ratio{34.28, 0.0, false},
          Ratio{34.28, 0.0, false},
          Ratio{26.83, 10.24, true},
          Ratio{26.83, 0.0, false}},
     }},
    {"u3/A",
     {
         {"li1",
          Ratio{34.28, 0.0, false},
          Ratio{34.28, 0.0, false},
          Ratio{26.83, 10.24, true},
          Ratio{26.83, 0.0, false}},
     }},
};

constexpr const char* kTestDir = "_main/src/ant/test/";

std::string testFile(const char* name)
{
  return std::string(kTestDir) + name;
}

class AntennaDesignTest : public tst::DbFixture
{
 protected:
  void load(const char* lef, const char* def)
  {
    ASSERT_NE(loadTechAndLibLef("tech", "lib", testFile(lef)), nullptr);
    block_ = readDef(testFile(def));
    ASSERT_NE(block_, nullptr);
  }

  odb::dbNet* net(const char* name) const { return block_->findNet(name); }

  odb::dbBlock* block_{nullptr};
  AntennaChecker checker_{getDb(), getLogger()};
};

// ant_check.lef: hand-tuned rules where only side area ratios can violate.
TEST_F(AntennaDesignTest, AntCheck)
{
  load("ant_check.lef", "ant_check.def");

  const DesignCheck design = checker_.check();
  EXPECT_EQ(design.violatingNets(), 1);
  expectNetCheck(netCheck(design, "net50"), kAntCheckPins);
}

// The report file holds the violating nets' detail.
TEST_F(AntennaDesignTest, AntReport)
{
  load("ant_check.lef", "ant_check.def");
  const std::string report_file = ::testing::TempDir() + "ant_report.rpt";
  checker_.setReportFileName(report_file.c_str());
  EXPECT_EQ(checker_.checkAntennas(), 1);

  std::ifstream file(report_file);
  const std::string report((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
  EXPECT_EQ(report, formatNetCheck(checker_.checkNet(net("net50"))));
}

// sky130 rules as released, where only one of two gates violates.
TEST_F(AntennaDesignTest, Api1)
{
  load("merged_spacing.lef", "sw130_random.def");

  const DesignCheck design = checker_.check();
  EXPECT_EQ(design.violatingNets(), 1);
  expectNetCheck(netCheck(design, "net50"), kApi1Pins);
  EXPECT_FALSE(netCheck(design, "net51").violated);
}

// The report holds the violating nets, and a single net can be checked on
// its own.
TEST_F(AntennaDesignTest, Drt1)
{
  load("merged_spacing.lef", "sw130_random.def");

  // The report lists the violating net unless asked for every net.
  const DesignCheck design = checker_.check();
  const std::string violating = formatReport(design, false);
  EXPECT_NE(violating.find("Net: net50"), std::string::npos);
  EXPECT_EQ(violating.find("Net: net51"), std::string::npos);
  EXPECT_NE(formatReport(design, true).find("Net: net51"), std::string::npos);

  // One net can be checked on its own.
  const DesignCheck net50 = checker_.check(net("net50"));
  ASSERT_EQ(net50.nets.size(), 1);
  EXPECT_EQ(net50.violatingNets(), 1);
  expectNetCheck(net50.nets[0], kApi1Pins);
}

// These designs place sky130 buf_1 cells from ant_check.lef and route net n1
// on li1.
class AntennaStubTest : public AntennaDesignTest
{
 protected:
  void SetUp() override
  {
    lib_ = loadTechAndLibLef("tech", "lib", testFile("ant_check.lef"));
    ASSERT_NE(lib_, nullptr);
    block_ = tst::makeBlock(getDb(), lib_->getTech(), {0, 0, 250000, 100000});
    buf_ = lib_->findMaster("sky130_fd_sc_ms__buf_1");
    li1_ = lib_->getTech()->findLayer("li1");
  }

  void place(const char* name, int x, const char* term)
  {
    makeInst(block_,
             buf_,
             name,
             {.location = {x, 10000},
              .status = odb::dbPlacementStatus::PLACED,
              .iterms = {{"n1", term}}});
  }

  odb::dbLib* lib_{nullptr};
  odb::dbMaster* buf_{nullptr};
  odb::dbTechLayer* li1_{nullptr};
};

// Two li1 stubs joined only through input pin u1/A form one piece of metal,
// whichever order the stubs are built in.
TEST_F(AntennaStubTest, DisjointStubs)
{
  place("u1", 10000, "A");
  place("u2", 50000, "A");
  tst::RoutePath(net("n1"))
      .on(li1_, {8000, 11615})
      .to({10200, 11615})
      .on(li1_, {10800, 11615})
      .to({50200, 11615});

  const DesignCheck design = checker_.check();
  EXPECT_EQ(design.violatingNets(), 1);
  expectNetCheck(netCheck(design, "n1"), kDisjointStubsPins);
}

TEST_F(AntennaStubTest, DisjointStubsReversed)
{
  place("u1", 10000, "A");
  place("u2", 50000, "A");
  tst::RoutePath(net("n1"))
      .on(li1_, {10800, 11615})
      .to({50200, 11615})
      .on(li1_, {8000, 11615})
      .to({10200, 11615});

  expectNetCheck(checker_.checkNet(net("n1")), kDisjointStubsPins);
}

// Stubs joined only by the driver's output pin u1/X are one node, so both gates
// see the whole area and the driver's diffusion (the 10.24 required ratio).
TEST_F(AntennaStubTest, OutputPinBridge)
{
  place("u1", 10000, "X");
  place("u2", 2000, "A");
  place("u3", 200000, "A");
  tst::RoutePath(net("n1"))
      .on(li1_, {2500, 11615})
      .to({11700, 11615})
      .on(li1_, {11790, 11615})
      .to({200500, 11615});

  const DesignCheck design = checker_.check();
  EXPECT_EQ(design.violatingNets(), 1);
  expectNetCheck(netCheck(design, "n1"), kOutputPinBridgePins);
}

}  // namespace
}  // namespace ant::test
