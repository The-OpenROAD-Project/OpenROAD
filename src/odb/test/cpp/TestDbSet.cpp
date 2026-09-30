// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>
#include <limits>

#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"
#include "odb/dbSet.h"

namespace odb {
namespace {

class TestDbSet : public SimpleDbFixture
{
 protected:
  TestDbSet()
  {
    create2LevetDbNoBTerms();
    block_ = db_->getChip()->getBlock();
  }

  dbBlock* block_;
};

TEST_F(TestDbSet, EmptySet)
{
  dbSet<dbITerm> iterms = dbNet::create(block_, "unconnected")->getITerms();
  EXPECT_TRUE(iterms.empty());
  EXPECT_FALSE(iterms.hasMoreThan(0));
  EXPECT_FALSE(iterms.hasMoreThan(1));
  EXPECT_TRUE(iterms.hasExactly(0));
  EXPECT_FALSE(iterms.hasExactly(1));
}

TEST_F(TestDbSet, SingleElementSet)
{
  // n7 is connected to i3/o only.
  dbSet<dbITerm> iterms = block_->findNet("n7")->getITerms();
  EXPECT_EQ(iterms.size(), 1u);
  EXPECT_TRUE(iterms.hasMoreThan(0));
  EXPECT_FALSE(iterms.hasMoreThan(1));
  EXPECT_FALSE(iterms.hasExactly(0));
  EXPECT_TRUE(iterms.hasExactly(1));
  EXPECT_FALSE(iterms.hasExactly(2));
}

TEST_F(TestDbSet, HasMoreThanBoundaries)
{
  // n5 is connected to i1/o and i3/a.
  dbSet<dbITerm> iterms = block_->findNet("n5")->getITerms();
  EXPECT_EQ(iterms.size(), 2u);
  EXPECT_TRUE(iterms.hasMoreThan(0));
  EXPECT_TRUE(iterms.hasMoreThan(1));
  EXPECT_FALSE(iterms.hasMoreThan(2));
  EXPECT_FALSE(iterms.hasMoreThan(3));
}

TEST_F(TestDbSet, HasExactlyBoundaries)
{
  dbSet<dbITerm> iterms = block_->findNet("n5")->getITerms();
  EXPECT_FALSE(iterms.hasExactly(0));
  EXPECT_FALSE(iterms.hasExactly(1));
  EXPECT_TRUE(iterms.hasExactly(2));
  EXPECT_FALSE(iterms.hasExactly(3));
}

TEST_F(TestDbSet, HugeCountTerminatesAtEnd)
{
  constexpr uint32_t kHuge = std::numeric_limits<uint32_t>::max();
  dbSet<dbITerm> iterms = block_->findNet("n5")->getITerms();
  EXPECT_FALSE(iterms.hasMoreThan(kHuge));
  EXPECT_FALSE(iterms.hasExactly(kHuge));
}

TEST_F(TestDbSet, SequentialIteratorSet)
{
  // getInsts() uses a different iterator than getITerms(); i1, i2, i3.
  dbSet<dbInst> insts = block_->getInsts();
  EXPECT_EQ(insts.size(), 3u);
  EXPECT_TRUE(insts.hasMoreThan(2));
  EXPECT_FALSE(insts.hasMoreThan(3));
  EXPECT_TRUE(insts.hasExactly(3));
  EXPECT_FALSE(insts.hasExactly(4));
}

}  // namespace
}  // namespace odb
