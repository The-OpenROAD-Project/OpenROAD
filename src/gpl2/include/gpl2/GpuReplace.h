///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
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

#pragma once

#include <memory>
#include <vector>

namespace odb {
class dbDatabase;
class dbInst;
}  // namespace odb
namespace sta {
class dbSta;
class dbNetwork;
}  // namespace sta

namespace grt {
class GlobalRouter;
}

namespace rsz {
class Resizer;
}

namespace utl {
class Logger;
}

namespace gpl2 {

class PlacerBaseCommon;
class PlacerBase;
class GpuRouteBase;
class GpuTimingBase;
class InitialPlace;
class NesterovPlace;

class GpuReplace
{
 public:
  GpuReplace();
  ~GpuReplace();

  void init(sta::dbNetwork* network,
            odb::dbDatabase* odb,
            rsz::Resizer* resizer,
            grt::GlobalRouter* router,
            utl::Logger* logger);
  void reset();

  // The three main functions
  void doInitialPlace();
  int doNesterovPlace(int start_iter = 0);

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

  float getUniformTargetDensity();

  // HPWL: half-parameter wire length.
  void setReferenceHpwl(float deltaHpwl);
  void setHaloWidth(float haloWidth);
  void setVirtualIter(int iter);
  void setNumHops(int hops);

  // temp funcs; OpenDB should have these values.
  void setPadLeft(int padding);
  void setPadRight(int padding);
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

  void setDatapathFlag(bool flag);
  void setDataflowFlag(bool flag);
  void setClusterConstraintFlag(bool flag);

 private:
  bool initNesterovPlace();

  sta::dbNetwork* network_;
  odb::dbDatabase* db_;
  rsz::Resizer* rs_;
  grt::GlobalRouter* fr_;
  utl::Logger* log_;

  // We should only have one placerBaseCommon, timingBase and routeBase
  // But we need multiple placerBases to handle fences and multiple domains
  float haloWidth_ = 0.0f;
  int virtualIter_ = 0;
  int numHops_ = 0;

  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<GpuRouteBase> rb_;
  std::shared_ptr<GpuTimingBase> tb_;
  std::vector<std::shared_ptr<PlacerBase>> pbVec_;

  std::unique_ptr<InitialPlace> ip_;
  std::unique_ptr<NesterovPlace> np_;

  int initialPlaceMaxIter_;
  int initialPlaceMinDiffLength_;
  int initialPlaceMaxSolverIter_;
  int initialPlaceMaxFanout_;
  float initialPlaceNetWeightScale_;
  int totalPlaceableInsts_;

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

  bool datapathFlag_;
  bool dataflowFlag_;
  bool clusterConstraintFlag_;

  std::vector<int> timingNetWeightOverflows_;

  // temp variable; OpenDB should have these values.
  int padLeft_;
  int padRight_;
};

}  // namespace gpl2
