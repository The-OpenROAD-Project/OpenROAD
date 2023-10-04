///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Precision Innovations Inc.
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include <set>
#include <tuple>
#include <vector>

#include "odb/db.h"
#include "utl/Logger.h"

namespace utl {
class Logger;
}

namespace gpl {

struct Tray
{
  odb::Point pt;
  std::vector<odb::Point> slots;
  std::vector<int> cand;
};

struct Flop
{
  odb::Point pt;
  int idx;
  float prob;

  bool operator<(const Flop& a) const
  {
    return std::tie(prob, idx) < std::tie(a.prob, a.idx);
  }
};

struct Path
{
  int start_point;
  int end_point;
};

class MBFF
{
 public:
  MBFF(int num_flops,
       int num_paths,
       const std::vector<odb::Point>& points,
       const std::vector<Path>& paths,
       int threads,
       int knn,
       int multistart,
       utl::Logger* log);
  ~MBFF();

  void Run(int mx_sz, float alpha, float beta);

 private:
  int GetRows(int slot_cnt);
  int GetBitCnt(int bit_idx);
  float GetDist(const odb::Point& a, const odb::Point& b);

  void GetSlots(const odb::Point& tray,
                int rows,
                int cols,
                std::vector<odb::Point>& slots);
  Flop GetNewFlop(const std::vector<Flop>& prob_dist, float tot_dist);
  void GetStartTrays(std::vector<Flop> flops,
                     int num_trays,
                     float AR,
                     std::vector<Tray>& trays);
  Tray GetOneBit(const odb::Point& pt);

  void MinCostFlow(const std::vector<Flop>& flops,
                   std::vector<Tray>& trays,
                   int sz,
                   std::vector<std::pair<int, int>>& clusters);

  float GetSilh(const std::vector<Flop>& flops,
                const std::vector<Tray>& trays,
                const std::vector<std::pair<int, int>>& clusters);

  void KMeans(const std::vector<Flop>& flops,
              std::vector<std::vector<Flop>>& clusters);
  void KMeansDecomp(const std::vector<Flop>& flops,
                    int MAX_SZ,
                    std::vector<std::vector<Flop>>& pointsets);

  void RunCapacitatedKMeans(const std::vector<Flop>& flops,
                            std::vector<Tray>& trays,
                            int sz,
                            int iter,
                            std::vector<std::pair<int, int>>& cluster);

  std::vector<std::vector<Tray>>& RunSilh(
      const std::vector<Flop>& pointset,
      std::vector<std::vector<std::vector<Tray>>>& start_trays);

  float RunLP(const std::vector<Flop>& flops,
              std::vector<Tray>& trays,
              const std::vector<std::pair<int, int>>& clusters);
  float RunILP(const std::vector<Flop>& flops,
               const std::vector<std::vector<Tray>>& all_trays,
               float alpha);

  double GetTCPDisplacement(float beta);

  std::vector<Flop> flops_;
  std::vector<Path> paths_;
  std::set<int> flops_in_path_;
  std::vector<int> slot_disp_x_;
  std::vector<int> slot_disp_y_;
  int num_threads_;
  int knn_;
  int multistart_;
  utl::Logger* log_;
};
}  // namespace gpl
