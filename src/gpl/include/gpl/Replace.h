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

#ifndef __REPLACE_HEADER__
#define __REPLACE_HEADER__

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

class Replace
{
 public:
  Replace();
  ~Replace();

  void init();
  void reset();

  void setDb(odb::dbDatabase* odb);
  void setResizer(rsz::Resizer* resizer);
  void setGlobalRouter(grt::GlobalRouter* fr);
  void setLogger(utl::Logger* log);

  void doIncrementalPlace();
  void doInitialPlace();

  bool initNesterovPlace();
  int doNesterovPlace(int start_iter = 0);

  // Initial Place param settings
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

  float getUniformTargetDensity();

  // HPWL: half-parameter wire length.
  void setReferenceHpwl(float deltaHpwl);

  // temp funcs; OpenDB should have these values.
  void setPadLeft(int padding);
  void setPadRight(int padding);

  void setForceCPU(bool force_cpu);
  void setTimingDrivenMode(bool mode);

  void setSkipIoMode(bool mode);

  void setRoutabilityDrivenMode(bool mode);
  void setRoutabilityCheckOverflow(float overflow);
  void setRoutabilityMaxDensity(float density);

  void setRoutabilityMaxBloatIter(int iter);
  void setRoutabilityMaxInflationIter(int iter);

  void setRoutabilityTargetRcMetric(float rc);
  void setRoutabilityInflationRatioCoef(float ratio);
  void setRoutabilityMaxInflationRatio(float ratio);

  void setRoutabilityRcCoefficients(float k1, float k2, float k3, float k4);

  void addTimingNetWeightOverflow(int overflow);
  void setTimingNetWeightMax(float max);

  void setDebug(int pause_iterations,
                int update_iterations,
                bool draw_bins,
                bool initial,
                odb::dbInst* inst = nullptr);

 private:
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

  int initialPlaceMaxIter_;
  int initialPlaceMinDiffLength_;
  int initialPlaceMaxSolverIter_;
  int initialPlaceMaxFanout_;
  float initialPlaceNetWeightScale_;
  bool forceCPU_;

  int nesterovPlaceMaxIter_;
  int binGridCntX_;
  int binGridCntY_;
  float overflow_;
  float density_;
  float initDensityPenalityFactor_;
  float initWireLengthCoef_;
  float minPhiCoef_;
  float maxPhiCoef_;
  float referenceHpwl_;

  float routabilityCheckOverflow_;
  float routabilityMaxDensity_;
  float routabilityTargetRcMetric_;
  float routabilityInflationRatioCoef_;
  float routabilityMaxInflationRatio_;

  // routability RC metric coefficients
  float routabilityRcK1_, routabilityRcK2_, routabilityRcK3_, routabilityRcK4_;

  int routabilityMaxBloatIter_;
  int routabilityMaxInflationIter_;

  float timingNetWeightMax_;

  bool timingDrivenMode_;
  bool routabilityDrivenMode_;
  bool uniformTargetDensityMode_;
  bool skipIoMode_;

  std::vector<int> timingNetWeightOverflows_;

  // temp variable; OpenDB should have these values.
  int padLeft_;
  int padRight_;

  bool gui_debug_;
  int gui_debug_pause_iterations_;
  int gui_debug_update_iterations_;
  int gui_debug_draw_bins_;
  int gui_debug_initial_;
  odb::dbInst* gui_debug_inst_;
};
}  // namespace gpl

#endif
