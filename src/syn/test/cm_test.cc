// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <sstream>
#include <string>
#include <string_view>

#include "equivalence_check.h"
#include "gtest/gtest.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/synthesis.h"
#include "tst/fixture.h"

// ABC ships its own main() that wins over gtest_main when linked in.
// Provide our own to force the gtest entry point.
int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

namespace syn {

struct CmTestCase
{
  std::string_view name;
  std::string_view ir;
  // Best observed post-mapping area + 20% headroom.
  float area_threshold;
};

// Net IDs 0/1/2 are the constant tie-off slots; instance IDs start at %3.
static const CmTestCase kCmTestCases[] = {
    {.name = "SingleAnd",
     .ir = R"(
%3:1 = input "a"
%4:1 = input "b"
%5:1 = and %3 %4
%6:0 = output "y" %5
)",
     .area_threshold = 3.6f},

    {.name = "NandFromAndThenNot",
     .ir = R"(
%3:1 = input "a"
%4:1 = input "b"
%5:1 = and %3 %4
%6:1 = not %5
%7:0 = output "y" %6
)",
     .area_threshold = 2.4f},

    {.name = "NorFromOrThenNot",
     .ir = R"(
%3:1 = input "a"
%4:1 = input "b"
%5:1 = or %3 %4
%6:1 = not %5
%7:0 = output "y" %6
)",
     .area_threshold = 2.4f},

    {.name = "Andnot",
     .ir = R"(
%3:1 = input "a"
%4:1 = input "b"
%5:1 = andnot %3 %4
%6:0 = output "y" %5
)",
     .area_threshold = 3.6f},

    {.name = "AoiPattern",
     .ir = R"(
%3:1 = input "a"
%4:1 = input "b"
%5:1 = input "c"
%6:1 = or %4 %5
%7:1 = and %3 %6
%8:1 = not %7
%9:0 = output "y" %8
)",
     .area_threshold = 4.8f},

    {.name = "OrOfAnds",
     .ir = R"(
%3:1 = input "a"
%4:1 = input "b"
%5:1 = input "c"
%6:1 = input "d"
%7:1 = and %3 %4
%8:1 = and %5 %6
%9:1 = or %7 %8
%10:0 = output "y" %9
)",
     .area_threshold = 7.2f},

    {.name = "SharedFanin",
     .ir = R"(
%3:1 = input "a"
%4:1 = input "b"
%5:1 = input "c"
%6:1 = and %3 %4
%7:1 = and %3 %5
%8:0 = output "y1" %6
%9:0 = output "y2" %7
)",
     .area_threshold = 7.2f},

    {.name = "FourInputAndTree",
     .ir = R"(
%3:1 = input "a"
%4:1 = input "b"
%5:1 = input "c"
%6:1 = input "d"
%7:1 = and %3 %4
%8:1 = and %5 %6
%9:1 = and %7 %8
%10:0 = output "y" %9
)",
     .area_threshold = 7.2f},

    // y = (a & !s) | (b & s)
    {.name = "MuxFromAig",
     .ir = R"(
%3:1 = input "a"
%4:1 = input "b"
%5:1 = input "s"
%6:1 = andnot %3 %5
%7:1 = and %4 %5
%8:1 = or %6 %7
%9:0 = output "y" %8
)",
     .area_threshold = 8.4f},

    {.name = "ConstantOutput",
     .ir = R"(
%3:0 = output "y0" 0
%4:0 = output "y1" 1
)",
     .area_threshold = 2.4f},

    {.name = "InverterOnly",
     .ir = R"(
%3:1 = input "a"
%4:1 = not %3
%5:0 = output "y" %4
)",
     .area_threshold = 1.2f},

    // 12-bit adder: bitblast lowers `adc` to Han-Carlson AIG. Output is
    // the 12-bit sum, dropping the carry-out at bit 12.
    {.name = "Adder12",
     .ir = R"(
%3:12 = input "a"
%15:12 = input "b"
%27:13 = adc %3:12 %15:12 0
%40:0 = output "y" %27+0:12
)",
     .area_threshold = 280.8f},

    // Truncated 6x6 unsigned multiplier (syn::Mul outputWidth = a.width()).
    // bitblast lowers it to a Wallace tree feeding a Han-Carlson adder.
    {.name = "Multiplier6",
     .ir = R"(
%3:6 = input "a"
%9:6 = input "b"
%15:6 = mul %3:6 %9:6
%21:0 = output "y" %15:6
)",
     .area_threshold = 265.2f},
};

class CmTest : public tst::Fixture,
               public ::testing::WithParamInterface<CmTestCase>
{
 protected:
  void SetUp() override
  {
    Fixture::SetUp();
    readLiberty(getFilePath("_main/src/syn/test/cm_test_cells.lib"));
    synthesis_ = std::make_unique<Synthesis>(getDb(),
                                             getSta(),
                                             /*resizer=*/nullptr,
                                             getLogger());
  }

  std::unique_ptr<Synthesis> synthesis_;
};

TEST_P(CmTest, MapsBelowAreaAndIsEquivalent)
{
  const CmTestCase& tc = GetParam();

  // bitblast lowers xor/adc/mul/… to AIG; mapper only handles And/Andnot/Or.
  std::istringstream gold_is{std::string(tc.ir)};
  std::unique_ptr<Graph> gold = Graph::parse(gold_is);
  bitblast(*gold);
  gold->normalize();

  std::istringstream gate_is{std::string(tc.ir)};
  std::unique_ptr<Graph> gate = Graph::parse(gate_is);
  bitblast(*gate);

  mapCombinationals(*gate, getSta()->network(), getLogger(), *synthesis_);

  // Surviving instances must be Targets, plus IR scaffolding and the
  // naming-only Nots that cm.cc:integrate() leaves behind.
  float area = 0.0f;
  gate->forEachInstance([&](const Instance* inst) {
    if (inst->is<Input>() || inst->is<Output>() || inst->is<Name>()
        || inst->is<Not>() || inst->is<TieLow>() || inst->is<TieHigh>()
        || inst->is<TieX>()) {
      return;
    }
    if (!inst->is<Target>()) {
      std::ostringstream os;
      gate->dumpInstance(os, inst);
      ADD_FAILURE() << "Non-Target instance survived combinational mapping in "
                       "case "
                    << tc.name << ": " << os.str();
      return;
    }
    if (auto* t = inst->try_as<Target>()) {
      area += t->cell()->area();
    }
  });

  EXPECT_LE(area, tc.area_threshold)
      << "Mapped area " << area << " exceeds threshold " << tc.area_threshold
      << " (best observed + 20% headroom) for case " << tc.name;

  EXPECT_TRUE(equivalenceCheck(
      *gold, *gate, /*inputsDefined=*/true, /*allowRefinement=*/false))
      << "Combinational mapping changed the function for case " << tc.name;
}

INSTANTIATE_TEST_SUITE_P(Cm,
                         CmTest,
                         ::testing::ValuesIn(kCmTestCases),
                         [](const ::testing::TestParamInfo<CmTestCase>& info) {
                           return std::string(info.param.name);
                         });

}  // namespace syn
