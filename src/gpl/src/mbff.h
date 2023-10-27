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

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Bfs.hh"
#include "sta/Clock.hh"
#include "sta/Corner.hh"
#include "sta/FuncExpr.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InputDrive.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/Parasitics.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchPred.hh"
#include "sta/Sequential.hh"
#include "sta/Sta.hh"
#include "sta/StaMain.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingModel.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace utl {
class Logger;
}

namespace gpl {

struct Point
{
  float x;
  float y;
};

struct Tray
{
  Point pt;
  std::vector<Point> slots;
  std::vector<int> cand;
};

struct Flop
{
  Point pt;
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
  MBFF(odb::dbDatabase* db,
       sta::dbSta* sta,
       utl::Logger* log,
       int threads,
       int knn,
       int multistart);
  ~MBFF();
  void Run(int mx_sz, float alpha, float beta);

 private:
  int GetRows(int slot_cnt);
  int GetBitCnt(int bit_idx);
  int GetBitIdx(int bit_cnt);
  float GetDist(const Point& a, const Point& b);

  std::map<std::string, std::string> GetPinMapping(odb::dbInst* tray);

  void SeparateFlops(std::vector<std::vector<odb::dbInst*>>& blocks,
                     std::vector<std::vector<Flop>>& ffs);

  bool IsGroundPin(odb::dbITerm* iterm);
  bool IsClockPin(odb::dbITerm* iterm);
  bool IsDPin(odb::dbITerm* iterm);
  bool IsQPin(odb::dbITerm* iterm);
  int GetNumSlots(odb::dbInst* inst);

  Flop GetNewFlop(const std::vector<Flop>& prob_dist, float tot_dist);
  void GetStartTrays(std::vector<Flop> flops,
                     int num_trays,
                     float AR,
                     std::vector<Tray>& trays);
  Tray GetOneBit(const Point& pt);
  void GetSlots(const Point& tray,
                int rows,
                int cols,
                std::vector<Point>& slots);

  double doit(const std::vector<Flop>& flops,
              std::vector<odb::dbInst*> block,
              int mx_sz,
              float alpha,
              float beta);

  void KMeans(const std::vector<Flop>& flops,
              std::vector<std::vector<Flop>>& clusters);
  void KMeansDecomp(const std::vector<Flop>& flops,
                    int MAX_SZ,
                    std::vector<std::vector<Flop>>& pointsets);

  float GetSilh(const std::vector<Flop>& flops,
                const std::vector<Tray>& trays,
                const std::vector<std::pair<int, int>>& clusters);
  void MinCostFlow(const std::vector<Flop>& flops,
                   std::vector<Tray>& trays,
                   int sz,
                   std::vector<std::pair<int, int>>& clusters);
  void RunCapacitatedKMeans(const std::vector<Flop>& flops,
                            std::vector<Tray>& trays,
                            int sz,
                            int iter,
                            std::vector<std::pair<int, int>>& cluster);
  std::vector<std::vector<Tray>>& RunSilh(
      const std::vector<Flop>& pointset,
      std::vector<std::vector<std::vector<Tray>>>& start_trays);

  void ReadLibs();
  void ReadFFs();
  void ReadPaths();
  void SetVars(std::vector<odb::dbInst*> block);
  void SetRatios();

  float RunLP(const std::vector<Flop>& flops,
              std::vector<Tray>& trays,
              const std::vector<std::pair<int, int>>& clusters);
  float RunILP(std::vector<odb::dbInst*> block,
               const std::vector<Flop>& flops,
               const std::vector<std::vector<Tray>>& all_trays,
               float alpha);
  void ModifyPinConnections(std::vector<odb::dbInst*> block,
                            const std::vector<Flop>& flops,
                            const std::vector<Tray>& trays,
                            std::vector<std::pair<int, int>>& mapping);
  double GetTCPDisplacement(float beta);

  odb::dbDatabase* db_;
  odb::dbBlock* block_;
  sta::dbSta* sta_;
  utl::Logger* log_;

  std::vector<Flop> flops_;
  std::vector<int> slot_disp_x_;
  std::vector<int> slot_disp_y_;
  float height_;
  float width_;

  std::vector<Path> paths_;
  std::set<int> flops_in_path_;

  std::vector<odb::dbMaster*> best_master_;
  std::vector<std::map<std::string, std::string>> pin_mappings_;
  std::vector<float> tray_area_;
  std::vector<float> tray_width_;
  std::vector<float> ratios_;

  int num_threads_;
  int multistart_;
  int knn_;
  float multiplier_;
  int num_sizes_;
};
}  // namespace gpl
