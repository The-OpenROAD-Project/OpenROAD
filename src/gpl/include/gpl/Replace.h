///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2020, The Regents of the University of California
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

#include <memory>
#include <vector>

namespace odb {
class dbDatabase;
class dbInst;

}  // namespace odb
namespace sta {
class dbSta;
}

namespace grt {
class GlobalRouter;
}

namespace rsz {
class Resizer;
}

namespace utl {
class Logger;
}

namespace gpl {

class PlacerBaseCommon;
class PlacerBase;
class NesterovBaseCommon;
class NesterovBase;
class RouteBase;
class TimingBase;

class InitialPlace;
class NesterovPlace;

class Replace
{
 public:
  Replace();
  ~Replace();

  void init(odb::dbDatabase* odb,
            sta::dbSta* sta,
            rsz::Resizer* resizer,
            grt::GlobalRouter* router,
            utl::Logger* logger);
  void reset();

  void doIncrementalPlace(int threads);
  void doInitialPlace();
  void runMBFF(int max_sz, float alpha, float beta, int threads, int num_paths);

  int doNesterovPlace(int threads, int start_iter = 0);

  // Initial Place param settings
  void setInitialPlaceMaxIter(int iter);
  void setInitialPlaceMinDiffLength(int length);
  void setInitialPlaceMaxSolverIter(int iter);
  void setInitialPlaceMaxFanout(int fanout);
  void setInitialPlaceNetWeightScale(float scale);

  void setNesterovPlaceMaxIter(int iter);

  void setBinGridCnt(int binGridCntX, int binGridCntY);

  void setTargetDensity(float density);
  void setUniformTargetDensityMode(bool mode);
  void setTargetOverflow(float overflow);
  void setInitDensityPenalityFactor(float penaltyFactor);
  void setInitWireLengthCoef(float coef);
  void setMinPhiCoef(float minPhiCoef);
  void setMaxPhiCoef(float maxPhiCoef);

  float getUniformTargetDensity(int threads);

  // HPWL: half-parameter wire length.
  void setReferenceHpwl(float refHpwl);

  // temp funcs; OpenDB should have these values.
  void setPadLeft(int padding);
  void setPadRight(int padding);

  void setTimingDrivenMode(bool mode);

  void setSkipIoMode(bool mode);

  void setRoutabilityDrivenMode(bool mode);
  void setRoutabilityUseGrt(bool mode);
  void setRoutabilityCheckOverflow(float overflow);
  void setRoutabilityMaxDensity(float density);

  void setRoutabilityMaxInflationIter(int iter);

  void setRoutabilityTargetRcMetric(float rc);
  void setRoutabilityInflationRatioCoef(float coef);
  void setRoutabilityMaxInflationRatio(float ratio);

  void setRoutabilityRcCoefficients(float k1, float k2, float k3, float k4);

  void addTimingNetWeightOverflow(int overflow);
  void setTimingNetWeightMax(float max);
  void setKeepResizeBelowOverflow(float overflow);

  void setDebug(int pause_iterations,
                int update_iterations,
                bool draw_bins,
                bool initial,
                odb::dbInst* inst = nullptr);

 private:
  bool initNesterovPlace(int threads);

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  rsz::Resizer* rs_ = nullptr;
  grt::GlobalRouter* fr_ = nullptr;
  utl::Logger* log_ = nullptr;

  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::vector<std::shared_ptr<PlacerBase>> pbVec_;
  std::vector<std::shared_ptr<NesterovBase>> nbVec_;
  std::shared_ptr<RouteBase> rb_;
  std::shared_ptr<TimingBase> tb_;

  std::unique_ptr<InitialPlace> ip_;
  std::unique_ptr<NesterovPlace> np_;

  int initialPlaceMaxIter_ = 20;
  int initialPlaceMinDiffLength_ = 1500;
  int initialPlaceMaxSolverIter_ = 100;
  int initialPlaceMaxFanout_ = 200;
  float initialPlaceNetWeightScale_ = 800;

  int total_placeable_insts_ = 0;

  int nesterovPlaceMaxIter_ = 5000;
  int binGridCntX_ = 0;
  int binGridCntY_ = 0;
  float overflow_ = 0.1;
  float density_ = 1.0;
  float initDensityPenalityFactor_ = 0.00008;
  float initWireLengthCoef_ = 0.25;
  float minPhiCoef_ = 0.95;
  float maxPhiCoef_ = 1.05;
  float referenceHpwl_ = 446000000;

  float routabilityCheckOverflow_ = 0.3;
  float routabilityMaxDensity_ = 0.99;
  float routabilityTargetRcMetric_ = 1.01;
  float routabilityInflationRatioCoef_ = 5;
  float routabilityMaxInflationRatio_ = 8;

  // routability RC metric coefficients
  float routabilityRcK1_ = 1.0;
  float routabilityRcK2_ = 1.0;
  float routabilityRcK3_ = 0.0;
  float routabilityRcK4_ = 0.0;

  int routabilityMaxBloatIter_ = 1;
  int routabilityMaxInflationIter_ = 4;

  float timingNetWeightMax_ = 1.9;
  float keepResizeBelowOverflow_ = 0.3;

  bool timingDrivenMode_ = true;
  bool routabilityDrivenMode_ = true;
  bool routabilityUseRudy_ = true;
  bool uniformTargetDensityMode_ = false;
  bool skipIoMode_ = false;

  std::vector<int> timingNetWeightOverflows_;

  // temp variable; OpenDB should have these values.
  int padLeft_ = 0;
  int padRight_ = 0;
  bool gui_debug_ = false;
  int gui_debug_pause_iterations_ = 10;
  int gui_debug_update_iterations_ = 10;
  int gui_debug_draw_bins_ = false;
  int gui_debug_initial_ = false;
  odb::dbInst* gui_debug_inst_ = nullptr;
};
}  // namespace gpl
