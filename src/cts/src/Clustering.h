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

namespace cts::CKMeans {

using utl::Logger;

struct Sink;

class Clustering
{
 public:
  Clustering(const std::vector<std::pair<float, float>>& sinks,
             float xBranch,
             float yBranch,
             Logger* logger);
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

  Logger* logger_;
  std::vector<Sink> sinks_;
  std::vector<std::vector<Sink*>> clusters_;

  float segment_length_ = 0.0;
  std::pair<float, float> branching_point_;
};

}  // namespace cts::CKMeans
