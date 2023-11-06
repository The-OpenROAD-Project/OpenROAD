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
  double x;
  double y;
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
  double prob;

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
  void Run(int mx_sz, double alpha, double beta);

 private:
  int GetRows(int slot_cnt, int bitmask);
  int GetBitCnt(int bit_idx);
  int GetBitIdx(int bit_cnt);
  double GetDist(const Point& a, const Point& b);

  std::map<sta::LibertyPort*, std::pair<sta::LibertyPort*, sta::LibertyPort*>>
  GetPinMapping(odb::dbInst* tray);

  void SeparateFlops(std::vector<std::vector<Flop>>& ffs);

  bool IsSupplyPin(odb::dbITerm* iterm);
  bool IsClockPin(odb::dbITerm* iterm);
  bool IsDPin(odb::dbITerm* iterm);
  bool IsQPin(odb::dbITerm* iterm);
  bool IsInverting(odb::dbInst* inst);
  int GetNumD(odb::dbInst* inst);
  int GetNumQ(odb::dbInst* inst);

  bool IsValidFlop(odb::dbInst* inst);
  bool IsValidTray(odb::dbInst* tray);


  int GetBitMask(odb::dbInst* inst);
  bool HasSet(odb::dbInst* inst);
  bool HasReset(odb::dbInst* inst);

  Flop GetNewFlop(const std::vector<Flop>& prob_dist, double tot_dist);
  void GetStartTrays(std::vector<Flop> flops,
                     int num_trays,
                     double AR,
                     std::vector<Tray>& trays);
  Tray GetOneBit(const Point& pt);
  void GetSlots(const Point& tray,
                int rows,
                int cols,
                std::vector<Point>& slots,
                int bitmask);

  double doit(const std::vector<Flop>& flops,
              int mx_sz,
              double alpha,
              double beta,
              int bitmask);

  
  /*
    shreyas (august 2023):
    method to decompose a pointset into multiple "mini"-pointsets of size <=
  MAX_SZ.
    basic implementation of K-means++ (with K = 4) is used.
  */
  void KMeans(const std::vector<Flop>& flops,
              std::vector<std::vector<Flop>>& clusters);
  void KMeansDecomp(const std::vector<Flop>& flops,
                    int MAX_SZ,
                    std::vector<std::vector<Flop>>& pointsets);

  double GetSilh(const std::vector<Flop>& flops,
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
                            std::vector<std::pair<int, int>>& cluster,
                            int bitmask);
  void RunSilh(std::vector<std::vector<Tray>>& trays,
               const std::vector<Flop>& pointset,
               std::vector<std::vector<std::vector<Tray>>>& start_trays,
               int bitmask);

  void ReadLibs();
  void ReadFFs();
  void ReadPaths();
  void SetVars(const std::vector<Flop>& flops);
  void SetRatios(int bitmask);
  void SetTrayNames();

  /*
  This LP finds new tray centers such that the sum of displacements from the new
  trays' slots to their matching flops is minimized
  */
  double RunLP(const std::vector<Flop>& flops,
               std::vector<Tray>& trays,
               const std::vector<std::pair<int, int>>& clusters);

  /*
  this ILP finds a set of trays (given all of the tray candidates from capacitated
  k-means) such that
  (1) each flop gets mapped to exactly one slot (or, stays a single bit flop)
  (2) [(a) + (b)] is minimized, where
          (a) = sum of all displacements from a flop to its slot
          (b) = alpha * (sum of the chosen tray costs)

  we ignore timing-critical path constraints /
  objectives so that the algorithm is scalable
  */
  double RunILP(const std::vector<Flop>& flops,
                const std::vector<Tray>& trays,
                std::vector<std::pair<int, int>>& final_flop_to_slot,
                double alpha,
                int bitmask);
  void ModifyPinConnections(const std::vector<Flop>& flops,
                            const std::vector<Tray>& trays,
                            std::vector<std::pair<int, int>>& mapping,
                            int bitmask);
  double GetTCPDisplacement();

  odb::dbDatabase* db_;
  odb::dbBlock* block_;
  sta::dbSta* sta_;
  sta::dbNetwork* network_;
  utl::Logger* log_;

  std::vector<Flop> flops_;
  std::vector<odb::dbInst*> insts_;
  std::vector<double> slot_disp_x_;
  std::vector<double> slot_disp_y_;
  double height_;
  double width_;

  std::vector<Path> paths_;
  std::set<int> flops_in_path_;

  /*
  Let B = a bitmask representing an instance
  The 1st bit of B is on if #Q - #D pins > 0
  The 2nd bit of B is on if the SET pin exists
  The 3rd bit of B is on if the RESET pin exists
  The 4th bit of B is on if the instance is inverting
  max(B) = 2^3 + 2^2 + 2^1 + 2^0 = 15
  */

  std::vector<odb::dbMaster*> best_master_[16];
  std::vector<std::map<sta::LibertyPort*,
                       std::pair<sta::LibertyPort*, sta::LibertyPort*>>>
      pin_mappings_[16];
  std::vector<double> tray_area_[16];
  std::vector<double> tray_width_[16];
  std::vector<std::vector<double>> slot_to_tray_x_[16];
  std::vector<std::vector<double>> slot_to_tray_y_[16];
  std::vector<double> ratios_;
  std::vector<int> unused_;

  int num_threads_;
  int multistart_;
  int knn_;
  double multiplier_;
  // max tray size: 1 << (7 - 1) = 64 bits
  int num_sizes_;
};
}  // namespace gpl
