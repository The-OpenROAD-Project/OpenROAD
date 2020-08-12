#ifndef __REPLACE_NESTEROV_PLACE__
#define __REPLACE_NESTEROV_PLACE__

#include "point.h"
#include <memory>
#include <vector>

namespace replace
{

class PlacerBase;
class Instance;
class NesterovBase;
class Logger;
class RouteBase;

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
      std::shared_ptr<Logger> log);
  ~NesterovPlace();

  void doNesterovPlace();

  void updateCoordi(
      std::vector<FloatPoint>& coordi);
  void updateBins();
  void updateWireLength();

  void updateGradients(
      std::vector<FloatPoint>& sumGrads,
      std::vector<FloatPoint>& wireLengthGrads,
      std::vector<FloatPoint>& densityGrads );

  void updateWireLengthCoef(float overflow);

  void updateInitialPrevSLPCoordi();

  float getStepLength(
      std::vector<FloatPoint>& prevCoordi_,
      std::vector<FloatPoint>& prevSumGrads_,
      std::vector<FloatPoint>& curCoordi_,
      std::vector<FloatPoint>& curSumGrads_ );

  void updateNextIter();
  float getPhiCoef(float scaledDiffHpwl);

  void updateDb();

private:
  std::shared_ptr<PlacerBase> pb_;
  std::shared_ptr<NesterovBase> nb_;
  std::shared_ptr<Logger> log_;
  std::shared_ptr<RouteBase> rb_;
  NesterovPlaceVars npVars_;

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

  void cutFillerCoordinates();

  void init();
  void reset();

};
}

#endif
