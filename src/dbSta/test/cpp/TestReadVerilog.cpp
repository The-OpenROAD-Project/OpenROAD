// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <set>
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
// Verilog2db::staToDb. The reader must NOT attach a deep-descendant
// pin (e.g. u_block/u_l2/u_consumer/clk) to a same-named modBTerm
// of an ancestor module (aliased_port_block::clk). The structural
// invariant we assert: every dbModNet whose getModBTerms() has size
// >= 2 must have all those modBTerms' parent modITerms resolve to
// the same flat dbNet. >= 2 distinct flat nets means the boundary
// is aliasing electrically-distinct external nets.
TEST_F(TestReadVerilog, DeepDescendantModBTermCollision)
{
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
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

  // Their parent-side flat dbNets must be electrically distinct
  // (clk_a vs clk_b at the top scope).
  auto parent_flat_id = [](odb::dbModBTerm* mbt) -> uint {
    odb::dbModITerm* pmi = mbt->getParentModITerm();
    if (pmi == nullptr) {
      return 0;
    }
    odb::dbModNet* pmn = pmi->getModNet();
    if (pmn == nullptr) {
      return 0;
    }
    odb::dbNet* pflat = pmn->findRelatedNet();
    return pflat ? pflat->getId() : 0;
  };
  uint clk_parent = parent_flat_id(clk_mbt);
  uint txclk_parent = parent_flat_id(txclk_mbt);
  ASSERT_NE(clk_parent, 0u);
  ASSERT_NE(txclk_parent, 0u);
  EXPECT_NE(clk_parent, txclk_parent)
      << "clk and txclk modBTerms resolve to the same parent flat dbNet "
         "(id="
      << clk_parent
      << "), but top drives them with two distinct external nets.";

  // Walk every modnet in every module and assert the structural
  // invariant. This is the same check checkSanityModNetPortAliasing
  // performs at runtime, but here we fail the test directly instead
  // of relying on a warning code being emitted.
  for (odb::dbModule* module : block_->getModules()) {
    for (odb::dbModNet* mn : module->getModNets()) {
      auto mbts = mn->getModBTerms();
      if (mbts.size() < 2) {
        continue;
      }
      std::set<uint> parent_flat_ids;
      for (odb::dbModBTerm* mbt : mbts) {
        odb::dbModITerm* pmi = mbt->getParentModITerm();
        if (pmi == nullptr) {
          continue;
        }
        odb::dbModNet* pmn = pmi->getModNet();
        if (pmn == nullptr) {
          continue;
        }
        odb::dbNet* pflat = pmn->findRelatedNet();
        if (pflat != nullptr) {
          parent_flat_ids.insert(pflat->getId());
        }
      }
      EXPECT_LT(parent_flat_ids.size(), 2u)
          << "dbModNet '" << mn->getName() << "' in module '"
          << module->getHierarchicalName()
          << "' has modBTerms whose parent modITerms resolve to "
          << parent_flat_ids.size()
          << " distinct flat dbNets. Boundary aliases unrelated "
             "external nets.";
    }
  }
}

}  // namespace sta
