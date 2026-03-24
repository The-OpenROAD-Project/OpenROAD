// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "utl/Logger.h"

namespace cts::CKMeans {

struct Sink;

class Clustering
{
 public:
  Clustering(const std::vector<std::pair<float, float>>& sinks,
             utl::Logger* logger);
  Clustering(const std::vector<std::pair<float, float>>& sinks,
             float xBranch,
             float yBranch,
             utl::Logger* logger);
  ~Clustering();

  void iterKmeans(unsigned iter,
                  unsigned n,
                  unsigned cap,
                  unsigned max,
                  unsigned power,
                  std::vector<std::pair<float, float>>& means);

  void getClusters(std::vector<std::vector<unsigned>>& newClusters) const;

 private:
  float Kmeans(unsigned n,
               unsigned cap,
               unsigned max,
               unsigned power,
               std::vector<std::pair<float, float>>& means);
  float calcSilh(const std::vector<std::pair<float, float>>& means) const;
  void minCostFlow(const std::vector<std::pair<float, float>>& means,
                   unsigned cap,
                   float dist,
                   unsigned power);
  void fixSegmentLengths(std::vector<std::pair<float, float>>& means);
  void fixSegment(const std::pair<float, float>& fixedPoint,
                  float targetDist,
                  std::pair<float, float>& movablePoint);

  static float calcDist(const std::pair<float, float>& loc, const Sink* sink);
  static float calcDist(const std::pair<float, float>& loc1,
                        const std::pair<float, float>& loc2);

  utl::Logger* logger_;
  std::vector<Sink> sinks_;
  std::vector<std::vector<Sink*>> clusters_;

  float segment_length_ = 0.0;
  std::optional<std::pair<float, float>> branching_point_;
};

}  // namespace cts::CKMeans
