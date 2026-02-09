// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <set>
#include <vector>

#include "detailed_objective.h"
#include "infrastructure/Objects.h"
#include "infrastructure/architecture.h"
#include "infrastructure/network.h"
#include "optimization/detailed_manager.h"

namespace dpl {

class DetailedOrient;

class DetailedABU : public DetailedObjective
{
  // This class maintains the ABU metric which can be used as part of a cost
  // function for detailed improvement.
 public:
  DetailedABU(Architecture* arch, Network* network);

  // Those that must be overridden.
  double curr() override;
  double delta(const Journal& journal) override;
  void accept() override;
  void reject() override;

  // Other.
  void init(DetailedMgr* mgr, DetailedOrient* orient);
  void init();
  double calculateABU(bool print = false);
  double measureABU(bool print = false);

  void updateBins(const Node* nd, double x, double y, int addSub);
  void acceptBins();
  void rejectBins();
  void clearBins();

  double delta();

  double freeSpaceThreshold();
  double binAreaThreshold();
  double alpha();

  void computeUtils();
  void clearUtils();

  void computeBuckets();
  void clearBuckets();
  int getBucketId(int binId, double occ);

 private:
  struct DensityBin
  {
    int id;
    double lx, hx;      // low/high x coordinate
    double ly, hy;      // low/high y coordinate
    double area;        // bin area
    double m_util;      // bin's movable cell area
    double c_util;      // bin's old movable cell area
    double f_util;      // bin's fixed cell area
    double free_space;  // bin's freespace area
  };

  DetailedMgr* mgrPtr_ = nullptr;
  DetailedOrient* orientPtr_ = nullptr;

  Architecture* arch_;
  Network* network_;

  // Utilization monitoring for ABU (if paying attention to ABU).
  std::vector<DensityBin> abuBins_;
  double abuGridUnit_ = 0;
  int abuGridNumX_ = 0;
  int abuGridNumY_ = 0;
  int abuNumBins_ = 0;

  double abuTargUt_ = 0;
  double abuTargUt02_ = 0;
  double abuTargUt05_ = 0;
  double abuTargUt10_ = 0;
  double abuTargUt20_ = 0;

  int abuChangedBinsCounter_ = 0;
  std::vector<int> abuChangedBins_;
  std::vector<int> abuChangedBinsMask_;

  std::vector<std::set<int>> utilBuckets_;
  std::vector<double> utilTotals_;
};

}  // namespace dpl
