// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The report text check_antennas prints and writes.  Users and scripts read
// this format, so it is pinned here once; everything else asserts on the
// structured results.

#include <string>

#include "AntennaReport.hh"
#include "ant/AntennaChecker.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "tst/db_fixture.h"
#include "tst/physical_builder.h"

namespace ant::test {
namespace {

class AntennaReportTest : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    odb::dbTech* tech = tst::makeTech(getDb());
    stack_ = tst::makeLayerStack(tech, 2);
    odb::dbLib* lib = tst::makeLib(getDb(), tech);
    block_ = tst::makeBlock(getDb(), tech);
    odb::dbMaster* gate = tst::makeMaster(
        lib, "GATE", 100, 100, {{.name = "A", .gate_area = 0.5}});
    makeInst(block_, gate, "u1", {.iterms = {{"n1", "A"}}});
  }

  tst::LayerStack stack_;
  odb::dbBlock* block_{nullptr};
};

TEST_F(AntennaReportTest, Format)
{
  NetCheck net_check;
  net_check.net = block_->findNet("n1");
  net_check.violated = true;

  PinCheck pin;
  pin.gate = block_->findInst("u1")->findITerm("A");
  pin.violated = true;

  // A routing layer, where all four ratios are checked.
  LayerCheck metal;
  metal.layer = stack_.metal(1);
  metal.par = {.actual = 55.123, .required = 0, .violated = false};
  metal.car = {.actual = 55.123, .required = 0, .violated = false};
  metal.psr = RatioCheck{.actual = 275.2, .required = 10, .violated = true};
  metal.csr = RatioCheck{.actual = 275.2, .required = 0, .violated = false};
  metal.violated = true;
  pin.layers.push_back(metal);

  // A cut layer, which has no side area ratios.
  LayerCheck cut;
  cut.layer = stack_.cut(1);
  cut.par = {.actual = 0.14, .required = 0.1, .violated = true};
  cut.car = {.actual = 0.25, .required = 0, .violated = false};
  cut.violated = true;
  pin.layers.push_back(cut);

  net_check.pins.push_back(pin);

  // Unflagged ratios keep a trailing space after the rule name.
  const std::string expected
      = "Net: n1\n"
        "  Pin:   u1/A (GATE)\n"
        "    Layer: M1\n"
        "      Partial area ratio:   55.12\n"
        "      Required ratio:    0.00 (Gate area) \n"
        "      Cumulative area ratio:   55.12\n"
        "      Required ratio:    0.00 (Cumulative area) \n"
        "      Partial area ratio:  275.20\n"
        "      Required ratio:   10.00 (Side area) (VIOLATED)\n"
        "      Cumulative area ratio:  275.20\n"
        "      Required ratio:    0.00 (Cumulative side area) \n"
        "\n"
        "    Layer: V1\n"
        "      Partial area ratio:    0.14\n"
        "      Required ratio:    0.10 (Gate area) (VIOLATED)\n"
        "      Cumulative area ratio:    0.25\n"
        "      Required ratio:    0.00 (Cumulative area) \n"
        "\n"
        "\n";
  EXPECT_EQ(formatNetCheck(net_check), expected);
}

TEST_F(AntennaReportTest, NetWithoutPins)
{
  NetCheck net_check;
  net_check.net = block_->findNet("n1");
  EXPECT_EQ(formatNetCheck(net_check), "Net: n1\n");
}

}  // namespace
}  // namespace ant::test
