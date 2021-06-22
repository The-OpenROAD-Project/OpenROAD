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

class flop
{
 public:
  // location
  float x, y;
  unsigned x_idx, y_idx;
  std::vector<float> dists;
  unsigned idx;
  std::vector<std::pair<int, int>> match_idx;
  std::vector<float> silhs;
  unsigned sinkIdx;
  flop(const float x, const float y, unsigned idx)
      : x(x), y(y), x_idx(0), y_idx(0), idx(0), sinkIdx(idx){};
};

class clustering
{
  Logger* _logger;
  std::vector<flop> flops;
  std::vector<std::vector<flop*>> clusters;

  int verbose = 1;
  int TEST_LAYOUT = 1;
  int TEST_ITER = 1;
  std::string plotFile;

  float segmentLength;
  std::pair<float, float> branchingPoint;

 public:
  clustering(const std::vector<std::pair<float, float>>&,
             float,
             float,
             Logger*);
  ~clustering();
  float Kmeans(unsigned,
               unsigned,
               unsigned,
               std::vector<std::pair<float, float>>&,
               unsigned,
               unsigned);
  void iterKmeans(unsigned,
                  unsigned,
                  unsigned,
                  unsigned,
                  std::vector<std::pair<float, float>>&,
                  unsigned MAX = 15,
                  unsigned power = 4);
  float calcSilh(const std::vector<std::pair<float, float>>&,
                 unsigned,
                 unsigned);
  void minCostFlow(const std::vector<std::pair<float, float>>&,
                   unsigned,
                   unsigned,
                   float,
                   unsigned);
  void setPlotFileName(const std::string fileName) { plotFile = fileName; }
  void getClusters(std::vector<std::vector<unsigned>>&);
  void fixSegmentLengths(std::vector<std::pair<float, float>>&);
  void fixSegment(const std::pair<float, float>& fixedPoint,
                  std::pair<float, float>& movablePoint,
                  float targetDist);

  inline float calcDist(const std::pair<float, float>& loc, flop* f) const
  {
    return (fabs(loc.first - f->x) + fabs(loc.second - f->y));
  }

  inline float calcDist(const std::pair<float, float>& loc1,
                        std::pair<float, float>& loc2) const
  {
    return (fabs(loc1.first - loc2.first) + fabs(loc1.second - loc2.second));
  }

  void plotClusters(const std::vector<std::vector<flop*>>&,
                    const std::vector<std::pair<float, float>>&,
                    const std::vector<std::pair<float, float>>&,
                    int) const;
};

}  // namespace CKMeans
