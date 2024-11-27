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

#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "odb/db.h"
#include "sta/Corner.hh"

namespace utl {
class Logger;
}

namespace sta {
class dbNetwork;
class dbSta;
class FuncExpr;
class LibertyPort;
}  // namespace sta

namespace gpl {

struct Point;
struct Tray;
struct Flop;
class Graphics;
enum PortName
{
  d,
  si,
  se,
  preset,
  clear,
  q,
  qn,
  vss,
  vdd,
  func,
  ifunc
};

class MBFF
{
 public:
  MBFF(odb::dbDatabase* db,
       sta::dbSta* sta,
       utl::Logger* log,
       int threads,
       int multistart,
       int num_paths,
       bool debug_graphics = false);
  ~MBFF();
  void Run(int mx_sz, float alpha, float beta);

 private:
  // get the respective q/qn pins for a d pin
  struct FlopOutputs
  {
    sta::LibertyPort* q;
    sta::LibertyPort* qn;
  };
  using Mask = std::array<int, 7>;
  using DataToOutputsMap = std::map<sta::LibertyPort*, FlopOutputs>;
  DataToOutputsMap GetPinMapping(odb::dbInst* tray);

  // MBFF functions
  float GetDist(const Point& a, const Point& b);
  float GetDistAR(const Point& a, const Point& b, float AR);
  int GetRows(int slot_cnt, const Mask& array_mask);
  int GetBitCnt(int bit_idx);
  int GetBitIdx(int bit_cnt);

  // clock pin functions
  bool IsClockPin(odb::dbITerm* iterm);
  bool ClockOn(odb::dbInst* inst);

  // d pin functions
  bool IsDPin(odb::dbITerm* iterm);
  int GetNumD(odb::dbInst* inst);

  // q(n) pin functions
  bool IsQPin(odb::dbITerm* iterm);
  bool IsInvertingQPin(odb::dbITerm* iterm);
  int GetNumQ(odb::dbInst* inst);

  // clear/preset pin functions
  bool HasClear(odb::dbInst* inst);
  bool IsClearPin(odb::dbITerm* iterm);
  bool HasPreset(odb::dbInst* inst);
  bool IsPresetPin(odb::dbITerm* iterm);

  // scan cell/pin functions
  bool IsScanCell(odb::dbInst* inst);
  bool IsScanIn(odb::dbITerm* iterm);
  odb::dbITerm* GetScanIn(odb::dbInst* inst);
  bool IsScanEnable(odb::dbITerm* iterm);
  odb::dbITerm* GetScanEnable(odb::dbInst* inst);

  // supply pin functions
  bool IsSupplyPin(odb::dbITerm* iterm);

  bool IsValidFlop(odb::dbInst* FF);
  bool IsValidTray(odb::dbInst* tray);

  // (MB)FF funcs
  PortName PortType(sta::LibertyPort* lib_port, odb::dbInst* inst);
  bool IsSame(sta::FuncExpr* expr1,
              odb::dbInst* inst1,
              sta::FuncExpr* expr2,
              odb::dbInst* inst2);
  int GetMatchingFunc(sta::FuncExpr* expr, odb::dbInst* inst, bool create_new);
  Mask GetArrayMask(odb::dbInst* inst, bool isTray);

  Tray GetOneBit(const Point& pt);
  Point GetTrayCenter(const Mask& array_mask, int idx);
  // get slots w.r.t. tray center
  void GetSlots(const Point& tray,
                int rows,
                int cols,
                std::vector<Point>& slots,
                const Mask& array_mask);

  // one iteration of K-Means++ for starting tray generation
  Flop GetNewFlop(const std::vector<Flop>& prob_dist, float tot_dist);
  void GetStartTrays(std::vector<Flop> flops,
                     int num_trays,
                     float AR,
                     std::vector<Tray>& trays);

  // multistart for starting tray locations
  void RunMultistart(std::vector<std::vector<Tray>>& trays,
                     const std::vector<Flop>& flops,
                     std::vector<std::vector<std::vector<Tray>>>& start_trays,
                     const Mask& array_mask);

  // get silhouette metric for a run of multistart
  float GetSilh(const std::vector<Flop>& flops,
                const std::vector<Tray>& trays,
                const std::vector<std::pair<int, int>>& clusters);

  void KMeans(const std::vector<Flop>& flops,
              int knn,
              std::vector<std::vector<Flop>>& clusters,
              std::vector<int>& rand_nums);
  float GetKSilh(const std::vector<std::vector<Flop>>& clusters,
                 const std::vector<Point>& centers);
  void KMeansDecomp(const std::vector<Flop>& flops,
                    int max_sz,
                    std::vector<std::vector<Flop>>& pointsets);

  void MinCostFlow(const std::vector<Flop>& flops,
                   std::vector<Tray>& trays,
                   int sz,
                   std::vector<std::pair<int, int>>& clusters);
  float RunLP(const std::vector<Flop>& flops,
              std::vector<Tray>& trays,
              const std::vector<std::pair<int, int>>& clusters);
  // iterate between MCF and LP until convergence or #iterations = iter
  void RunCapacitatedKMeans(const std::vector<Flop>& flops,
                            std::vector<Tray>& trays,
                            int sz,
                            int iter,
                            std::vector<std::pair<int, int>>& cluster,
                            const Mask& array_mask);

  /* MODIFIED ILP for fast MBFF clustering (compliments pointset decomposition
  results) minimize sum(alpha * tray_costs + occ(i) * (slot_disp_x(i) +
  slot_disp_y(i))) where occ(i) = max(1, sum(i in launch-caputre FF-pair j)) */
  double RunILP(const std::vector<Flop>& flops,
                const std::vector<Tray>& trays,
                std::vector<std::pair<int, int>>& final_flop_to_slot,
                float alpha,
                float beta,
                const Mask& array_mask);
  // calculate beta (1.00) * sum(relative displacements)
  float GetPairDisplacements();
  // place trays and modify nets
  void ModifyPinConnections(const std::vector<Flop>& flops,
                            const std::vector<Tray>& trays,
                            const std::vector<std::pair<int, int>>& mapping,
                            const Mask& array_mask);

  // seperate flops by clock net and ArrayMask
  void SeparateFlops(std::vector<std::vector<Flop>>& ffs);
  void SetVars(const std::vector<Flop>& flops);
  void SetRatios(const Mask& array_mask);
  float RunClustering(const std::vector<Flop>& flops,
                      int mx_sz,
                      float alpha,
                      float beta,
                      const Mask& array_mask);

  void ReadFFs();
  void ReadPaths();
  void ReadLibs();
  void SetTrayNames();

  // OpenROAD vars
  odb::dbDatabase* db_;
  odb::dbBlock* block_;
  sta::dbSta* sta_;
  sta::dbNetwork* network_;
  sta::Corner* corner_;
  utl::Logger* log_;
  std::unique_ptr<Graphics> graphics_;
  int num_threads_;
  int multistart_;
  int num_paths_;
  float multiplier_;

  // single-bit FF vars
  std::vector<Flop> flops_;
  std::vector<odb::dbInst*> insts_;
  std::vector<float> slot_disp_x_;
  std::vector<float> slot_disp_y_;
  float single_bit_height_;
  float single_bit_width_;
  float single_bit_power_;

  // launch-capture FF-pair vars
  std::map<std::string, int> name_to_idx_;
  std::map<int, int> tray_sizes_used_;
  std::vector<std::vector<int>> paths_;
  std::vector<std::set<int>> unique_;

  // MBFF vars
  template <typename T>
  /* ArrayMaskVector[i] = a vector for a certain (MB)FF structure.
  See GetArrayMask for details on how the structure of an instance is described.
*/
  using ArrayMaskVector = std::map<Mask, std::vector<T>>;
  ArrayMaskVector<odb::dbMaster*> best_master_;
  ArrayMaskVector<DataToOutputsMap> pin_mappings_;
  ArrayMaskVector<float> tray_area_;
  ArrayMaskVector<float> tray_power_;
  ArrayMaskVector<float> tray_width_;
  ArrayMaskVector<std::vector<float>> slot_to_tray_x_;
  ArrayMaskVector<std::vector<float>> slot_to_tray_y_;
  std::vector<float> norm_area_;
  std::vector<float> norm_power_;
  std::vector<int> unused_;
  // max tray size: 1 << (7 - 1) = 64 bits
  int num_sizes_ = 7;
  // ind of last test tray
  int test_idx_;
  // all MBFF next_states
  std::vector<std::pair<sta::FuncExpr*, odb::dbInst*>> funcs_;
};
}  // namespace gpl
