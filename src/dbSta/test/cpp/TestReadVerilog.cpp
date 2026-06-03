// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <string>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "tst/IntegratedFixture.h"

namespace sta {

namespace {

odb::dbModITerm* findChildModITerm(odb::dbModule* module,
                                   const char* instance_name,
                                   const char* port_name)
{
  odb::dbModInst* mod_inst = module->findModInst(instance_name);
  if (mod_inst == nullptr) {
    return nullptr;
  }
  return mod_inst->findModITerm(port_name);
}

}  // namespace

class TestReadVerilog : public tst::IntegratedFixture
{
 public:
  TestReadVerilog()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/dbSta/test/")
  {
  }
};

TEST_F(TestReadVerilog, FeedThrough)
{
  // FeedThrough test:
  // Tests hierarchical design with an intermediate module that has a
  // feedthrough path (assign out = in).
  //
  // Hierarchy:
  // - top (top module with output 'ass_out')
  //   - impl (top_impl instance)
  //     - U1 (INV_X1, driver)
  //     - U2 (INV_X1, load)
  //     - ass (ASSIGN_MODULE instance with feedthrough)
  //   - U2 (INV_X1, load at top level)
  //
  // Signal flow:
  // - U1/ZN drives 'ass_in' wire
  // - 'ass_in' connects to ass/in
  // - ass module has: assign out = in (feedthrough)
  // - ass/out connects to 'ass_out' port
  // - 'ass_out' is consumed by impl/U2/A and top/U2/A

  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  // Read verilog without SDC setup (this test has intentionally unconnected
  // pins that would cause checkAxioms to fail)
  readVerilogAndSetup(test_name + "_pre.v", /*init_default_sdc=*/false);

  //----------------------------------------------------
  // Verify Hierarchy Structure
  //----------------------------------------------------

  // Check top module
  odb::dbModule* top_module = block_->getTopModule();
  ASSERT_NE(top_module, nullptr);
  EXPECT_EQ(std::string(top_module->getName()), "top");

  // Check sub-modules exist
  odb::dbModule* top_impl_module = block_->findModule("top_impl");
  ASSERT_NE(top_impl_module, nullptr);
  odb::dbModule* assign_module = block_->findModule("ASSIGN_MODULE");
  ASSERT_NE(assign_module, nullptr);

  // Check module instances
  odb::dbModInst* impl_modinst = block_->findModInst("impl");
  ASSERT_NE(impl_modinst, nullptr);
  EXPECT_EQ(impl_modinst->getMaster(), top_impl_module);

  odb::dbModInst* ass_modinst = block_->findModInst("impl/ass");
  ASSERT_NE(ass_modinst, nullptr);
  EXPECT_EQ(ass_modinst->getMaster(), assign_module);

  //----------------------------------------------------
  // Verify Instances
  //----------------------------------------------------

  // U1 in top_impl (driver)
  odb::dbInst* impl_u1 = block_->findInst("impl/U1");
  ASSERT_NE(impl_u1, nullptr);

  // U2 in top_impl (load inside impl)
  odb::dbInst* impl_u2 = block_->findInst("impl/U2");
  ASSERT_NE(impl_u2, nullptr);

  // U2 in top (load at top level)
  odb::dbInst* top_u2 = block_->findInst("U2");
  ASSERT_NE(top_u2, nullptr);

  //----------------------------------------------------
  // Verify Connectivity
  //----------------------------------------------------

  // Get ass_out port at top level
  odb::dbBTerm* ass_out_bterm = block_->findBTerm("ass_out");
  ASSERT_NE(ass_out_bterm, nullptr);
  EXPECT_EQ(ass_out_bterm->getIoType(), odb::dbIoType::OUTPUT);

  // Verify ass_out net exists and connects to relevant loads
  odb::dbNet* ass_out_net = ass_out_bterm->getNet();
  ASSERT_NE(ass_out_net, nullptr);

  // top/U2/A should be connected to ass_out
  odb::dbITerm* top_u2_a = top_u2->findITerm("A");
  ASSERT_NE(top_u2_a, nullptr);
  EXPECT_EQ(top_u2_a->getNet(), ass_out_net);

  // Verify through-assignment path: impl/ass has in and out ports
  odb::dbModBTerm* ass_in_modbterm
      = ass_modinst->getMaster()->findModBTerm("in");
  ASSERT_NE(ass_in_modbterm, nullptr);
  EXPECT_EQ(ass_in_modbterm->getIoType(), odb::dbIoType::INPUT);

  odb::dbModBTerm* ass_out_modbterm
      = ass_modinst->getMaster()->findModBTerm("out");
  ASSERT_NE(ass_out_modbterm, nullptr);
  EXPECT_EQ(ass_out_modbterm->getIoType(), odb::dbIoType::OUTPUT);

  //----------------------------------------------------
  // Write and Compare Verilog Output
  //----------------------------------------------------
  // We avoid checkAxioms() here because it triggers ORD-2041 on the
  // intentionally unconnected pins (U1/A, U2/ZN), which terminates the test.
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Regression for the deep-descendant modBTerm false-attach bug in
// Verilog2db::staToDb. Escaped child instance names may contain '/'
// characters, so the reader must resolve them as local dbModInst names
// and connect the child pin to a dbModITerm instead of falling through
// to a same-named ancestor dbModBTerm.
TEST_F(TestReadVerilog, DeepDescendantModBTermCollision)
{
  const testing::TestInfo* test_info
      = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  readVerilogAndSetup(test_name + ".v", /*init_default_sdc=*/false);

  odb::dbModule* mid = block_->findModule("mid");
  ASSERT_NE(mid, nullptr);

  // Both clk and txclk modBTerms must exist and remain distinct.
  odb::dbModBTerm* clk_mbt = mid->findModBTerm("clk");
  odb::dbModBTerm* txclk_mbt = mid->findModBTerm("txclk");
  ASSERT_NE(clk_mbt, nullptr);
  ASSERT_NE(txclk_mbt, nullptr);
  EXPECT_NE(clk_mbt, txclk_mbt);

  odb::dbModNet* clk_modnet = clk_mbt->getModNet();
  odb::dbModNet* txclk_modnet = txclk_mbt->getModNet();
  ASSERT_NE(clk_modnet, nullptr);
  ASSERT_NE(txclk_modnet, nullptr);
  EXPECT_NE(clk_modnet, txclk_modnet);

  // The escaped names are local child instance names in module "mid".
  odb::dbModITerm* child0_clk
      = findChildModITerm(mid, "iclkdiv\\/gen_phases\\[0\\].iclk", "clk");
  odb::dbModITerm* child1_clk
      = findChildModITerm(mid, "iclkdiv\\/gen_phases\\[1\\].iclk", "clk");
  ASSERT_NE(child0_clk, nullptr);
  ASSERT_NE(child1_clk, nullptr);

  EXPECT_EQ(child0_clk->getModNet(), clk_modnet);
  EXPECT_EQ(child1_clk->getModNet(), txclk_modnet);
  EXPECT_EQ(clk_modnet->getModBTerms().size(), 1u);
  EXPECT_EQ(txclk_modnet->getModBTerms().size(), 1u);
}

}  // namespace sta
