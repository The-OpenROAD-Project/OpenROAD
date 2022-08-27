/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <string>
#include <vector>

namespace utl {
class Logger;
}  // namespace utl

namespace CKMeans {

using utl::Logger;

struct Flop;

class Clustering
{
 public:
  Clustering(const std::vector<std::pair<float, float>>& sinks,
             const float xBranch,
             const float yBranch,
             Logger* logger);
  ~Clustering();

  void iterKmeans(const unsigned iter,
                  const unsigned n,
                  const unsigned cap,
                  const unsigned idx,
                  std::vector<std::pair<float, float>>& means,
                  const unsigned max = 15,
                  const unsigned power = 4);

  void getClusters(std::vector<std::vector<unsigned>>& newClusters) const;

  void setPlotFileName(const std::string fileName) { plotFile_ = fileName; }

 private:
  float Kmeans(const unsigned n,
               const unsigned cap,
               const unsigned idx,
               std::vector<std::pair<float, float>>& means,
               const unsigned max,
               const unsigned power);
  float calcSilh(const std::vector<std::pair<float, float>>& means,
                 const unsigned idx);
  void minCostFlow(const std::vector<std::pair<float, float>>& means,
                   const unsigned cap,
                   const unsigned idx,
                   const float dist,
                   const unsigned power);
  void fixSegmentLengths(std::vector<std::pair<float, float>>& means);
  void fixSegment(const std::pair<float, float>& fixedPoint,
                  std::pair<float, float>& movablePoint,
                  const float targetDist);

  float calcDist(const std::pair<float, float>& loc, Flop* f) const;
  float calcDist(const std::pair<float, float>& loc1,
                 std::pair<float, float>& loc2) const;

  void plotClusters(const std::vector<std::vector<Flop*>>& clusters,
                    const std::vector<std::pair<float, float>>& means,
                    const std::vector<std::pair<float, float>>& pre_means,
                    const int iter) const;

  Logger* logger_;
  std::vector<Flop> flops_;
  std::vector<std::vector<Flop*>> clusters_;

  std::string plotFile_;

  float segmentLength_;
  std::pair<float, float> branchingPoint_;

  static const bool test_layout_ = true;
  static const bool test_iter_ = true;
};

}  // namespace CKMeans
