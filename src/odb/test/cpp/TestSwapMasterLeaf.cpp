// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Unit tests for dbInst::swapMaster(dbMaster*), the leaf-cell master swap.
//
// This replaces the integration test swap_master.tcl, which only printed
// the post-swap master name and diffed it against a golden log.  Here we
// assert the documented contract directly: the return value, that the
// master actually changes, that existing iterm connections are preserved by
// MTerm name, and that an incompatible signature is rejected without
// mutating the instance.  (Binding-level coverage of the swapMaster command
// remains in test_inst.tcl / test_inst.py.)
//
// NOTE: dbModInst::swapMaster(dbModule*) (the module-instance overload) is a
// different API covered by TestSwapMaster.cpp.

#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"

namespace odb {

namespace {

class SwapMasterLeafTest : public SimpleDbFixture
{
 protected:
  void SetUp() override
  {
    SimpleDbFixture::SetUp();
    // createSimpleDB() builds:
    //   and2 : MTerms a, b, o   (2x1)
    //   or2  : MTerms a, b, o   (2x1, same signature as and2)
    //   inv1 : MTerms ip0, op0  (1x1, different signature)
    createSimpleDB();
    block_ = db_->getChip()->getBlock();
    lib_ = db_->findLib("lib1");
  }

  dbBlock* block_ = nullptr;
  dbLib* lib_ = nullptr;
};

// Swapping to a master with an identical MTerm signature succeeds, changes the
// master, and preserves every iterm's net connection (remapped by MTerm name).
TEST_F(SwapMasterLeafTest, SwapToCompatibleMasterPreservesConnections)
{
  dbMaster* and2 = lib_->findMaster("and2");
  dbMaster* or2 = lib_->findMaster("or2");
  ASSERT_NE(and2, nullptr);
  ASSERT_NE(or2, nullptr);

  dbInst* inst = dbInst::create(block_, and2, "i1");
  ASSERT_NE(inst, nullptr);
  auto [na, nb, no] = makeNets<3>(block_, {"na", "nb", "no"});
  dbITerm* a = inst->findITerm("a");
  dbITerm* b = inst->findITerm("b");
  dbITerm* o = inst->findITerm("o");
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  ASSERT_NE(o, nullptr);
  a->connect(na);
  b->connect(nb);
  o->connect(no);

  ASSERT_EQ(inst->getMaster(), and2);

  const bool swapped = inst->swapMaster(or2);

  EXPECT_TRUE(swapped);
  EXPECT_EQ(inst->getMaster(), or2);
  // Connections survive the swap, matched to the new master by MTerm name.
  dbITerm* swapped_a = inst->findITerm("a");
  dbITerm* swapped_b = inst->findITerm("b");
  dbITerm* swapped_o = inst->findITerm("o");
  ASSERT_NE(swapped_a, nullptr);
  ASSERT_NE(swapped_b, nullptr);
  ASSERT_NE(swapped_o, nullptr);
  EXPECT_EQ(swapped_a->getNet(), na);
  EXPECT_EQ(swapped_b->getNet(), nb);
  EXPECT_EQ(swapped_o->getNet(), no);
}

// Swapping to a master whose MTerm signature differs is rejected: swapMaster
// returns false and leaves the instance untouched.
TEST_F(SwapMasterLeafTest, SwapToIncompatibleMasterIsRejected)
{
  dbMaster* and2 = lib_->findMaster("and2");
  dbMaster* inv1 = lib_->findMaster("inv1");
  ASSERT_NE(and2, nullptr);
  ASSERT_NE(inv1, nullptr);

  dbInst* inst = dbInst::create(block_, and2, "i1");
  ASSERT_NE(inst, nullptr);

  const bool swapped = inst->swapMaster(inv1);

  EXPECT_FALSE(swapped);
  EXPECT_EQ(inst->getMaster(), and2);
}

}  // namespace

}  // namespace odb
