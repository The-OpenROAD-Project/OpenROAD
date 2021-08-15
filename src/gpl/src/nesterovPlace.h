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

#ifndef __REPLACE_NESTEROV_PLACE__
#define __REPLACE_NESTEROV_PLACE__

#include "point.h"
#include <memory>
#include <vector>
#include <string>

namespace utl {
class Logger;
}

namespace gpl 
{

class PlacerBase;
class Instance;
class NesterovBase;
class RouteBase;
class TimingBase;
class Graphics;

class NesterovPlaceVars {
  public:
  int maxNesterovIter;
  int maxBackTrack;
  float initDensityPenalty; // INIT_LAMBDA
  float initWireLengthCoef; // base_wcof
  float targetOverflow; // overflow
  float minPhiCoef; // pcof_min
  float maxPhiCoef; // pcof_max
  float minPreconditioner; // MIN_PRE
  float initialPrevCoordiUpdateCoef; // z_ref_alpha
  float referenceHpwl; // refDeltaHpwl
  float routabilityCheckOverflow;

  int routabilityMaxBloatIter;
  int routabilityMaxInflationIter;

  bool timingDrivenMode;
  bool routabilityDrivenMode;
  bool debug;
  int debug_pause_iterations;
  int debug_update_iterations;
  bool debug_draw_bins;

  NesterovPlaceVars();
  void reset();
};

class NesterovPlace {
public:
  NesterovPlace();
  NesterovPlace(NesterovPlaceVars npVars,
      std::shared_ptr<PlacerBase> pb,
      std::shared_ptr<NesterovBase> nb,
      std::shared_ptr<RouteBase> rb,
      std::shared_ptr<TimingBase> tb,
      utl::Logger* log);
  ~NesterovPlace();

  void doNesterovPlace();

  void updateGradients(
      std::vector<FloatPoint>& sumGrads,
      std::vector<FloatPoint>& wireLengthGrads,
      std::vector<FloatPoint>& densityGrads );

  void updateWireLengthCoef(float overflow);

  void updateInitialPrevSLPCoordi();

  float getStepLength(
      const std::vector<FloatPoint>& prevCoordi_,
      const std::vector<FloatPoint>& prevSumGrads_,
      const std::vector<FloatPoint>& curCoordi_,
      const std::vector<FloatPoint>& curSumGrads_ );

  void updateNextIter();
  float getPhiCoef(float scaledDiffHpwl) const;

  void updateDb();

  float getWireLengthCoefX() const { return wireLengthCoefX_; }
  float getWireLengthCoefY() const { return wireLengthCoefY_; }
  float getDensityPenalty() const { return densityPenalty_; }

private:
  std::shared_ptr<PlacerBase> pb_;
  std::shared_ptr<NesterovBase> nb_;
  utl::Logger* log_;
  std::shared_ptr<RouteBase> rb_;
  std::shared_ptr<TimingBase> tb_;
  NesterovPlaceVars npVars_;
  std::unique_ptr<Graphics> graphics_;
  
  // SLP is Step Length Prediction.
  //
  // y_st, y_dst, y_wdst, w_pdst
  std::vector<FloatPoint> curSLPCoordi_;
  std::vector<FloatPoint> curSLPWireLengthGrads_;
  std::vector<FloatPoint> curSLPDensityGrads_;
  std::vector<FloatPoint> curSLPSumGrads_;

  // y0_st, y0_dst, y0_wdst, y0_pdst
  std::vector<FloatPoint> nextSLPCoordi_;
  std::vector<FloatPoint> nextSLPWireLengthGrads_;
  std::vector<FloatPoint> nextSLPDensityGrads_;
  std::vector<FloatPoint> nextSLPSumGrads_;

  // z_st, z_dst, z_wdst, z_pdst
  std::vector<FloatPoint> prevSLPCoordi_;
  std::vector<FloatPoint> prevSLPWireLengthGrads_;
  std::vector<FloatPoint> prevSLPDensityGrads_;
  std::vector<FloatPoint> prevSLPSumGrads_;

  // x_st and x0_st
  std::vector<FloatPoint> curCoordi_;
  std::vector<FloatPoint> nextCoordi_;

  // save initial coordinates -- needed for RD
  std::vector<FloatPoint> initCoordi_;

  // densityPenalty stor
  std::vector<float> densityPenaltyStor_;

  float wireLengthGradSum_;
  float densityGradSum_;

  // alpha
  float stepLength_;

  // opt_phi_cof
  float densityPenalty_;

  // base_wcof
  float baseWireLengthCoef_;

  // wlen_cof
  float wireLengthCoefX_;
  float wireLengthCoefY_;

  // phi is described in ePlace paper.
  float sumPhi_;
  float sumOverflow_;

  // half-parameter-wire-length
  int64_t prevHpwl_;

  float isDiverged_;
  float isRoutabilityNeed_;

  std::string divergeMsg_;
  int divergeCode_; 

  void cutFillerCoordinates();

  void init();
  void reset();

};
}

#endif
