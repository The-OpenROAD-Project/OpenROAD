// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "src/par/src/Evaluator.h"
#include "src/par/src/Hypergraph.h"
#include "src/par/src/Partitioner.h"
#include "utl/Logger.h"

namespace par {
namespace {

TEST(PartitionerTest, ILPRespectsFixedVertices)
{
  utl::Logger logger;
  auto hgraph = std::make_shared<Hypergraph>(1,
                                             1,
                                             0,
                                             Matrix<int>{{0, 1}, {2, 3}},
                                             Matrix<float>{{1}, {1}, {1}, {1}},
                                             Matrix<float>{{1}, {1}},
                                             std::vector<int>{0, 1, 0, 1},
                                             std::vector<int>{},
                                             Matrix<float>{},
                                             &logger);
  auto evaluator = std::make_shared<GoldenEvaluator>(2,
                                                     std::vector<float>{1},
                                                     std::vector<float>{1},
                                                     std::vector<float>{},
                                                     0,
                                                     0,
                                                     0,
                                                     0,
                                                     0,
                                                     hgraph,
                                                     &logger);
  Partitioner partitioner(2, 0, evaluator, &logger);
  std::vector<int> solution(4, -1);

  partitioner.Partition(hgraph,
                        Matrix<float>{{2}, {2}},
                        Matrix<float>{{2}, {2}},
                        solution,
                        PartitionType::kInitDirectIlp);

  EXPECT_EQ(solution, (std::vector<int>{0, 1, 0, 1}));
}

}  // namespace
}  // namespace par
