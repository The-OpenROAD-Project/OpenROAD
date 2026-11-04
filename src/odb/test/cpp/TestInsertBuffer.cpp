// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <filesystem>
#include <string>
#include <tuple>

#include "gtest/gtest.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "tst/db_fixture.h"
#include "tst/loaded_design.h"

namespace odb {
namespace {

class InsertBufferPortNamesTest
    : public testing::TestWithParam<std::tuple<bool, bool, bool>>
{
 protected:
  void checkConnectivity(dbBlock* block,
                         const std::string& buffer_name,
                         bool move_both)
  {
    auto* buffer = block->findInst(buffer_name.c_str());
    auto* driver = block->findInst("drv");
    auto* load = block->findInst("ld");
    ASSERT_NE(buffer, nullptr);
    ASSERT_NE(driver, nullptr);
    ASSERT_NE(load, nullptr);
    auto* input_net = buffer->findITerm("A")->getNet();
    auto* output_net = buffer->findITerm("Z")->getNet();
    ASSERT_NE(input_net, nullptr);
    ASSERT_NE(output_net, nullptr);
    EXPECT_NE(input_net, output_net);
    EXPECT_EQ(driver->findITerm("ZN")->getNet(), input_net);
    EXPECT_EQ(load->findITerm("A")->getNet(), input_net);
    EXPECT_EQ(block->findBTerm("p")->getNet(), output_net);
    EXPECT_EQ(block->findBTerm("q")->getNet(),
              move_both ? output_net : input_net);
    EXPECT_NE(input_net->getName(), "p");
    if (move_both) {
      EXPECT_NE(input_net->getName(), "q");
    }
  }
};

TEST_P(InsertBufferPortNamesTest, PreservesConnectivityThroughVerilog)
{
  const auto [hierarchy, named_after_q, move_both] = GetParam();
  const auto output_path
      = std::filesystem::path(testing::TempDir())
        / (std::string("insert_buffer_") + (hierarchy ? "hier_" : "flat_")
           + (named_after_q ? "q_" : "p_") + (move_both ? "both.v" : "one.v"));
  std::string buffer_name;
  {
    tst::LoadedDesign design(
        tst::Technology::kNangate45,
        tst::getRunfilePath(
            "_main/src/odb/test/cpp/insert_buffer_port_alias.v"),
        "top",
        hierarchy);
    auto* block = design.getBlock();
    auto* p = block->findBTerm("p");
    auto* q = block->findBTerm("q");
    ASSERT_NE(p, nullptr);
    ASSERT_NE(q, nullptr);
    auto* original_net = p->getNet();
    ASSERT_EQ(original_net, q->getNet());
    const char* original_name = named_after_q ? "q" : "p";
    if (original_net->getName() != original_name) {
      ASSERT_TRUE(original_net->rename(original_name));
    }

    PtrSet<dbObject> loads{p};
    if (move_both) {
      loads.insert(q);
    }
    auto* buffer_master = design.getDb()->findMaster("BUF_X1");
    ASSERT_NE(buffer_master, nullptr);
    const Point location{20000, 10000};
    auto* buffer = original_net->insertBufferBeforeLoads(
        loads, buffer_master, &location, "rbuf");
    ASSERT_NE(buffer, nullptr);
    buffer_name = buffer->getName();
    checkConnectivity(block, buffer_name, move_both);
    if (!move_both && named_after_q) {
      EXPECT_EQ(original_net->getName(), "q");
    }
    design.writeVerilog(output_path.string());
  }

  {
    tst::LoadedDesign reread(
        tst::Technology::kNangate45, output_path.string(), "top", hierarchy);
    checkConnectivity(reread.getBlock(), buffer_name, move_both);
  }
  std::filesystem::remove(output_path);
}

INSTANTIATE_TEST_SUITE_P(FlatAndHierarchical,
                         InsertBufferPortNamesTest,
                         testing::Combine(testing::Bool(),
                                          testing::Bool(),
                                          testing::Bool()));

}  // namespace
}  // namespace odb
