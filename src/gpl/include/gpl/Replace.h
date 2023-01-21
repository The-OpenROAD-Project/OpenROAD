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

class PlacerBase;
class NesterovBase;
class RouteBase;
class TimingBase;

class InitialPlace;
class NesterovPlace;
class Debug;

class ReplaceOptions
{
 public:
  bool getIncremental() const;
  bool getDoNesterovPlace() const;
  bool getForceCpu() const;

  int getInitialPlaceMaxIter() const;
  int getInitialPlaceMinDiffLength() const;
  int getInitialPlaceMaxSolverIter() const;
  int getInitialPlaceMaxFanout() const;
  float getInitialPlaceNetWeightScale() const;

  int getNesterovPlaceMaxIter() const;

  int getBinGridCntX() const;
  int getBinGridCntY() const;

  float getTargetDensity() const;
  bool getUniformTargetDensityMode() const;
  float getTargetOverflow() const;
  float getInitDensityPenalityFactor() const;
  float getInitWireLengthCoef() const;
  float getMinPhiCoef() const;
  float getMaxPhiCoef() const;

  int getPadLeft() const;
  int getPadRight() const;

  float getRoutabilityCheckOverflow() const;
  float getRoutabilityMaxDensity() const;

  int getRoutabilityMaxBloatIter() const;
  int getRoutabilityMaxInflationIter() const;

  float getRoutabilityTargetRcMetric() const;
  float getRoutabilityInflationRatioCoef() const;
  float getRoutabilityMaxInflationRatio() const;

  float getRoutabilityRcK1() const;
  float getRoutabilityRcK2() const;
  float getRoutabilityRcK3() const;
  float getRoutabilityRcK4() const;

  float getReferenceHpwl() const;
  const std::vector<int>& getTimingNetWeightOverflows() const;
  float getTimingNetWeightMax() const;

  bool getRoutabilityDrivenMode() const;
  bool getTimingDrivenMode() const;
  bool getSkipIoMode() const;

  bool getGuiDebug() const;
  int getGuiDebugPauseIterations() const;
  int getGuiDebugUpdateIterations() const;
  int getGuiDebugDrawBins() const;
  int getGuiDebugInitial() const;
  odb::dbInst* getGuiDebugInst() const;

  // setters

  void setIncremental(bool value);
  void setDoNesterovPlace(bool value);
  void setForceCpu(bool value);
  void skipInitialPlace() {
    setInitialPlaceMaxIter(0);
  }

  void setInitialPlaceMaxIter(int iter);
  void setInitialPlaceMinDiffLength(int length);
  void setInitialPlaceMaxSolverIter(int iter);
  void setInitialPlaceMaxFanout(int fanout);
  void setInitialPlaceNetWeightScale(float scale);

  void setNesterovPlaceMaxIter(int iter);

  void setBinGridCntX(int binGridCntX);
  void setBinGridCntY(int binGridCntY);

  void setTargetDensity(float density);
  void setUniformTargetDensityMode(bool mode);
  void setTargetOverflow(float overflow);
  void setInitDensityPenalityFactor(float penaltyFactor);
  void setInitWireLengthCoef(float coef);
  void setMinPhiCoef(float minPhiCoef);
  void setMaxPhiCoef(float maxPhiCoef);

  void setPadLeft(int padding);
  void setPadRight(int padding);

  void setRoutabilityCheckOverflow(float overflow);
  void setRoutabilityMaxDensity(float density);

  void setRoutabilityMaxBloatIter(int iter);
  void setRoutabilityMaxInflationIter(int iter);

  void setRoutabilityTargetRcMetric(float rc);
  void setRoutabilityInflationRatioCoef(float ratio);
  void setRoutabilityMaxInflationRatio(float ratio);

  void setRoutabilityRcCoefficients(float k1, float k2, float k3, float k4);

  // HPWL: half-parameter wire length.
  void setReferenceHpwl(float deltaHpwl);

  void addTimingNetWeightOverflow(int overflow);
  void setTimingNetWeightMax(float max);

  void setRoutabilityDrivenMode(bool mode);
  void setTimingDrivenMode(bool mode);
  void setSkipIoMode(bool mode);

  void setDebug(int pause_iterations,
                int update_iterations,
                bool draw_bins,
                bool initial,
                odb::dbInst* inst = nullptr);

 private:
  bool incremental_ = false;
  bool do_nesterov_place_ = true;
  bool force_cpu_ = false;

  int initialPlaceMaxIter_ = 20;
  int initialPlaceMinDiffLength_ = 1500;
  int initialPlaceMaxSolverIter_ = 100;
  int initialPlaceMaxFanout_ = 200;
  float initialPlaceNetWeightScale_ = 800;

  int nesterovPlaceMaxIter_ = 5000;
  int binGridCntX_ = 0;
  int binGridCntY_ = 0;

  float overflow_ = 0.1;
  float density_ = 0.7;
  float initDensityPenalityFactor_ = 0.00008;
  float initWireLengthCoef_ = 0.25;
  float minPhiCoef_ = 0.95;
  float maxPhiCoef_ = 1.05;
  bool uniformTargetDensityMode_ = false;

  int padLeft_ = 0;
  int padRight_ = 0;

  float routabilityCheckOverflow_ = 0.20;
  float routabilityMaxDensity_ = 0.99;
  float routabilityTargetRcMetric_ = 1.25;
  float routabilityInflationRatioCoef_ = 2.5;
  float routabilityMaxInflationRatio_ = 2.5;

  // routability RC metric coefficients
  float routabilityRcK1_ = 1.0;
  float routabilityRcK2_ = 1.0;
  float routabilityRcK3_ = 0.0;
  float routabilityRcK4_ = 0.0;

  int routabilityMaxBloatIter_ = 1;
  int routabilityMaxInflationIter_ = 4;

  float referenceHpwl_ = 446000000;

  float timingNetWeightMax_ = 1.9;
  std::vector<int> timingNetWeightOverflows_ = {79, 64, 49, 29, 21, 15};
  bool hasDefaultTimingNetWeightOverflows_ = true;

  bool gui_debug_ = false;
  int gui_debug_pause_iterations_ = 10;
  int gui_debug_update_iterations_ = 10;
  int gui_debug_draw_bins_ = false;
  int gui_debug_initial_ = false;
  odb::dbInst* gui_debug_inst_ = nullptr;

  bool timingDrivenMode_ = false;
  bool routabilityDrivenMode_ = false;
  bool skipIoMode_ = false;
};

class Replace
{
 public:
  Replace();
  ~Replace();

  void init(odb::dbDatabase* odb,
            rsz::Resizer* resizer,
            grt::GlobalRouter* router,
            utl::Logger* logger);
  void reset();

  void place(const ReplaceOptions& options = ReplaceOptions());

  float getUniformTargetDensity(const ReplaceOptions& options
                                = ReplaceOptions());

 private:
  void doIncrementalPlace();
  void doInitialPlace();
  int doNesterovPlace(int start_iter = 0);
  void validate(const ReplaceOptions& options);

  bool initNesterovPlace();

  odb::dbDatabase* db_;
  rsz::Resizer* rs_;
  grt::GlobalRouter* fr_;
  utl::Logger* log_;

  std::shared_ptr<PlacerBase> pb_;
  std::shared_ptr<NesterovBase> nb_;
  std::shared_ptr<RouteBase> rb_;
  std::shared_ptr<TimingBase> tb_;

  std::unique_ptr<InitialPlace> ip_;
  std::unique_ptr<NesterovPlace> np_;

  ReplaceOptions options_;
};
}  // namespace gpl
