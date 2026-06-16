// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <sstream>

#include "gtest/gtest.h"
#include "syn/ir/Bundle.h"
#include "syn/ir/Const.h"
#include "syn/ir/ControlNet.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/synthesis.h"
#include "tst/fixture.h"

namespace syn {

class SmTest : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    Fixture::SetUp();
    readLiberty(getFilePath("_main/src/syn/test/sm_test_cells.lib"));
    synthesis_ = std::make_unique<Synthesis>(getDb(),
                                             getSta(),
                                             /*resizer=*/nullptr,
                                             getLogger());
  }

  std::string mapAndDump(Graph& g)
  {
    mapSequentials(g, getSta()->network(), getLogger(), *synthesis_);
    g.normalize();
    std::ostringstream os;
    g.dump(os);
    return os.str();
  }

  std::unique_ptr<Synthesis> synthesis_;
};

TEST_F(SmTest, BasicDff)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle clk = g.add<Input>("clk", 1);
  Bundle dff = g.add<Dff>(a,
                          ControlNet::pos(clk[0]),
                          ControlNet::zero(),
                          ControlNet::zero(),
                          ControlNet::one(),
                          Const::undef(1),
                          Const::undef(1),
                          Const::undef(1));
  g.add<Output>("out", dff);

  EXPECT_EQ(mapAndDump(g), R"(%3:1 = input "a"
%4:1 = input "clk"
%5:0 = output "out" %6
%6:1 = target "DFF" {
  input "D" = %3
  input "CLK" = %4
  %6:1 = output "Q"
}
)");
}

TEST_F(SmTest, DffWithClear)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle clk = g.add<Input>("clk", 1);
  Bundle clr = g.add<Input>("clr", 1);
  // Liberty has clear : "!RST_N" (active-low), so use neg polarity.
  Bundle dff = g.add<Dff>(a,
                          ControlNet::pos(clk[0]),
                          ControlNet::neg(clr[0]),
                          ControlNet::zero(),
                          ControlNet::one(),
                          Const::undef(1),
                          Const::undef(1),
                          Const::zero(1));
  g.add<Output>("out", dff);

  EXPECT_EQ(mapAndDump(g), R"(%3:1 = input "a"
%4:1 = input "clk"
%5:1 = input "clr"
%6:0 = output "out" %7
%7:1 = target "DFFRSN" {
  input "D" = %8
  input "CLK" = %4
  input "RST_N" = 1
  input "SET_N" = %5
  %7:1 = output "QN"
}
%8:1 = not %3
)");
}

TEST_F(SmTest, DffWithSet)
{
  Graph g;
  Bundle a = g.add<Input>("a", 1);
  Bundle clk = g.add<Input>("clk", 1);
  Bundle set = g.add<Input>("set", 1);
  // Liberty has preset : "!SET_N" (active-low), so use neg polarity.
  Bundle dff = g.add<Dff>(a,
                          ControlNet::pos(clk[0]),
                          ControlNet::neg(set[0]),
                          ControlNet::zero(),
                          ControlNet::one(),
                          Const::undef(1),
                          Const::undef(1),
                          Const::ones(1));
  g.add<Output>("out", dff);

  EXPECT_EQ(mapAndDump(g), R"(%3:1 = input "a"
%4:1 = input "clk"
%5:1 = input "set"
%6:0 = output "out" %7
%7:1 = target "DFFRSN" {
  input "D" = %8
  input "CLK" = %4
  input "RST_N" = %5
  input "SET_N" = 1
  %7:1 = output "QN"
}
%8:1 = not %3
)");
}

}  // namespace syn

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
