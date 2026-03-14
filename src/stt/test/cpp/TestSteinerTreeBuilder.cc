// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace stt {
namespace {

class SteinerTreeBuilderTest : public ::testing::Test
{
 protected:
  SteinerTreeBuilderTest()
      : db_(odb::dbDatabase::create()), builder_(db_, &logger_)
  {
  }

  ~SteinerTreeBuilderTest() override { odb::dbDatabase::destroy(db_); }

  utl::Logger logger_;
  odb::dbDatabase* db_;
  SteinerTreeBuilder builder_;
};

TEST_F(SteinerTreeBuilderTest, MakeSteinerTreeDefaultAlpha)
{
  std::vector<int> x = {0, 100, 200};
  std::vector<int> y = {0, 0, 0};

  Tree tree = builder_.makeSteinerTree(x, y, 0);

  EXPECT_EQ(tree.deg, 3);
  EXPECT_EQ(tree.length, 200);
}

TEST_F(SteinerTreeBuilderTest, MakeSteinerTreeAlphaZero)
{
  // Alpha=0 should use FLUTE (pure Steiner)
  std::vector<int> x = {0, 100, 50};
  std::vector<int> y = {0, 0, 100};

  Tree tree = builder_.makeSteinerTree(x, y, 0, 0.0);

  EXPECT_EQ(tree.deg, 3);
  EXPECT_GE(tree.length, 200);  // HPWL
}

TEST_F(SteinerTreeBuilderTest, MakeSteinerTreeAlphaPositive)
{
  // Alpha>0 tries PD first, falls back to FLUTE if checkTree fails
  std::vector<int> x = {0, 100, 200, 100};
  std::vector<int> y = {0, 0, 0, 100};

  Tree tree = builder_.makeSteinerTree(x, y, 0, 0.3);

  EXPECT_EQ(tree.deg, 4);
  EXPECT_GE(tree.length, 300);  // HPWL
}

TEST_F(SteinerTreeBuilderTest, CheckTreeValid)
{
  std::vector<int> x = {0, 100, 200};
  std::vector<int> y = {0, 0, 0};

  Tree tree = builder_.makeSteinerTree(x, y, 0);

  EXPECT_TRUE(builder_.checkTree(tree));
}

TEST_F(SteinerTreeBuilderTest, SetAlpha)
{
  builder_.setAlpha(0.8);
  EXPECT_FLOAT_EQ(builder_.getAlpha(), 0.8);

  builder_.setAlpha(0.0);
  EXPECT_FLOAT_EQ(builder_.getAlpha(), 0.0);
}

TEST_F(SteinerTreeBuilderTest, FluteDirectAccess)
{
  std::vector<int> x = {0, 100, 200};
  std::vector<int> y = {0, 0, 0};

  Tree tree = builder_.flute(x, y, 3);

  EXPECT_EQ(tree.deg, 3);
  EXPECT_EQ(tree.length, 200);
}

TEST_F(SteinerTreeBuilderTest, Wirelength)
{
  std::vector<int> x = {0, 100};
  std::vector<int> y = {0, 200};

  Tree tree = builder_.flute(x, y, 3);

  EXPECT_EQ(builder_.wirelength(tree), 300);
}

TEST_F(SteinerTreeBuilderTest, LargeNet)
{
  // 20-pin net to exercise decomposition
  std::vector<int> x, y;
  for (int i = 0; i < 20; ++i) {
    x.push_back(i * 100);
    y.push_back((i % 4) * 100);
  }

  Tree tree = builder_.makeSteinerTree(x, y, 0, 0.3);

  EXPECT_EQ(tree.deg, 20);
  EXPECT_GT(tree.length, 0);
  // Branch indices must be valid
  for (int i = 0; i < tree.branchCount(); ++i) {
    EXPECT_GE(tree.branch[i].n, 0);
    EXPECT_LT(tree.branch[i].n, tree.branchCount());
  }
}

}  // namespace
}  // namespace stt
