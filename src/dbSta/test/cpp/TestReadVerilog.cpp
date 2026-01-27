// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <string>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "tst/IntegratedFixture.h"

namespace sta {

class TestReadVerilog : public tst::IntegratedFixture
{
 public:
  TestReadVerilog()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::Nangate45,
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

}  // namespace sta
