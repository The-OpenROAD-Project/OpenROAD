// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "src/cts/src/Clock.h"
#include "src/cts/src/Clustering.h"
#include "src/cts/src/HTreeBuilder.h"
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
                                              /*number_of_clusters=*/2,
                                              /*max_cluster_size=*/2,
                                              /*maximum_iteration_count*/ 5,
                                              /*power*/ 4,
                                              means_with_branching);

  EXPECT_NE(means_without_branching[0].first, means_with_branching[0].first);
}

}  // namespace cts
