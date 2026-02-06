// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "sta/Corner.hh"
#include "sta/NetworkClass.hh"

namespace utl {
class Logger;
}

namespace rsz {
class Resizer;
}

namespace sta {
class dbNetwork;
class dbSta;
class FuncExpr;
class LibertyCell;
class LibertyPort;
}  // namespace sta

namespace gpl {

struct Point;
struct Tray;
struct Flop;
class AbstractGraphics;

class MBFF
{
  friend class MBFFTestPeer;

 public:
  MBFF(odb::dbDatabase* db,
       sta::dbSta* sta,
       utl::Logger* log,
       rsz::Resizer* resizer,
       int threads,
       int multistart,
       int num_paths,
       bool debug_graphics,
       std::unique_ptr<AbstractGraphics> graphics);

  ~MBFF();
  void Run(int mx_sz, float alpha, float beta);

 private:
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

  // get the respective q/qn pins for a d pin
  struct FlopOutputs
  {
    const sta::LibertyPort* q;
    const sta::LibertyPort* qn;
  };
  struct Mask
  {
    int func_idx{0};
    bool clock_polarity{false};
    bool has_clear{false};
    bool has_preset{false};
    bool pos_output{false};
    bool inv_output{false};
    bool is_scan_cell{false};

    std::string to_string() const;
    bool operator<(const Mask& rhs) const;
  };
  using DataToOutputsMap = std::map<const sta::LibertyPort*, FlopOutputs>;
  DataToOutputsMap GetPinMapping(odb::dbInst* tray);

  // MBFF functions
  const sta::LibertyCell* getLibertyCell(const sta::Cell* cell);
  float GetDist(const Point& a, const Point& b);
  float GetDistAR(const Point& a, const Point& b, float AR);
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
  PortName PortType(const sta::LibertyPort* lib_port, odb::dbInst* inst);
  bool IsSame(const sta::FuncExpr* expr1,
              odb::dbInst* inst1,
              const sta::FuncExpr* expr2,
              odb::dbInst* inst2);
  int GetMatchingFunc(const sta::FuncExpr* expr,
                      odb::dbInst* inst,
                      bool create_new);
  Mask GetArrayMask(odb::dbInst* inst, bool isTray);

  Tray GetOneBit(const Point& pt);
  Point GetTrayCenter(const Mask& array_mask, int idx);
  // get slots w.r.t. tray center
  void GetSlots(const Point& tray,
                int bit_cnt,
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
              const std::vector<int>& rand_nums);
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

  void displayFlopClusters(const char* stage,
                           std::vector<std::vector<Flop>>& clusters);

  float getLeakage(odb::dbMaster* master);

  // OpenROAD vars
  odb::dbDatabase* db_;
  odb::dbBlock* block_;
  sta::dbSta* sta_;
  sta::dbNetwork* network_;
  sta::Corner* corner_;
  std::unique_ptr<AbstractGraphics> graphics_;
  utl::Logger* log_;
  rsz::Resizer* resizer_;
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
  const int num_sizes_ = 7;
  // ind of last test tray
  int test_idx_;
  // all MBFF next_states
  std::vector<std::pair<const sta::FuncExpr*, odb::dbInst*>> funcs_;
};

}  // namespace gpl
