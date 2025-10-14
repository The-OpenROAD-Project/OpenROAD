// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <utility>
#include <vector>

#include "cts/TritonCTS.h"
#include "gtest/gtest.h"
#include "src/cts/src/Clock.h"
#include "src/cts/src/Clustering.h"
#include "src/cts/src/HTreeBuilder.h"
#include "src/cts/src/SinkClustering.h"
#include "utl/Logger.h"

namespace cts {

TEST(HTreeBuilderTest, Instantiates)
{
  Clock clock(/*netName=*/"clk",
              /*clockPin=*/"p0",
              /*sdcClockName=*/"clk",
              /*clockPinX=*/0,
              /*clockPinY=*/0);

  utl::Logger logger;
  HTreeBuilder instance(
      /*options=*/nullptr, clock, /*parent=*/nullptr, &logger, nullptr);
}

TEST(Cluster, BranchingPointIsDisabledIfNoBranchesAreProvided)
{
  utl::Logger logger;
  CKMeans::Clustering clustering_engine(/*sinks=*/
                                        {{10, 20},
                                         {10, 10},
                                         {40, 40},
                                         {50, 50}},
                                        &logger);
  std::vector<std::pair<float, float>> means_without_branching
      = {{16, 41}, {33, 18}};

  // Better names
  //  iter
  //  number_of_clusters
  //  max_cluster_size
  //  maximum_iteration_count
  //  power
  clustering_engine.iterKmeans(/*iter=*/1,
                               /*n=*/2,
                               /*cap=*/2,
                               /*max*/ 5,
                               /*power*/ 4,
                               means_without_branching);

  CKMeans::Clustering clustering_engine_with_branching(/*sinks=*/
                                                       {{10, 20},
                                                        {10, 10},
                                                        {40, 40},
                                                        {50, 50}},
                                                       0,
                                                       0,
                                                       &logger);
  std::vector<std::pair<float, float>> means_with_branching
      = {{16, 41}, {33, 18}};
  clustering_engine_with_branching.iterKmeans(/*iter=*/1,
                                              /*n=*/2,
                                              /*cap=*/2,
                                              /*max*/ 5,
                                              /*power*/ 4,
                                              means_with_branching);

  EXPECT_NE(means_without_branching[0].first, means_with_branching[0].first);
}

// Check that we avoid a divide-by-zero error when the region has
// zero height.
TEST(SinkClusteringTest, ZeroHeightRegion)
{
  // Setup
  utl::Logger logger;
  CtsOptions options(&logger, nullptr);
  TechChar techChar(
      &options, nullptr, nullptr, nullptr, nullptr, nullptr, &logger);
  Clock net("clock", "clock", "clock", 0, 0);
  HTreeBuilder HTree(&options, net, nullptr, &logger, nullptr);
  SinkClustering clustering(&options, &techChar, &HTree);
  for (int i = 0; i < 3; ++i) {
    clustering.addPoint(i, 0);
    clustering.addCap(0);
  }

  // Run
  unsigned best_size;
  float best_diameter;
  clustering.run(1, 1, 1, best_size, best_diameter);

  // Pass == no crash
}

}  // namespace cts
