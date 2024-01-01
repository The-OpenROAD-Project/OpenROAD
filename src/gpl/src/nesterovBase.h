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
#include <string>
#include <unordered_map>
#include <vector>

#include "point.h"

namespace odb {
class dbInst;
class dbITerm;
class dbBTerm;
class dbNet;
}  // namespace odb

namespace utl {
class Logger;
}

namespace gpl {

class Instance;
class Die;
class PlacerBaseCommon;
class PlacerBase;

class Instance;
class Pin;
class Net;

class GPin;
class FFT;

class GCell
{
 public:
  // instance cells
  GCell(Instance* inst);
  GCell(const std::vector<Instance*>& insts);

  // filler cells
  GCell(int cx, int cy, int dx, int dy);

  Instance* instance() const;
  const std::vector<Instance*>& insts() const { return insts_; }
  const std::vector<GPin*>& gPins() const { return gPins_; }

  void addGPin(GPin* gPin);

  void setClusteredInstance(const std::vector<Instance*>& insts);
  void setInstance(Instance* inst);
  void clearInstances();
  void setFiller();

  // normal coordinates
  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;
  int cx() const;
  int cy() const;
  int dx() const;
  int dy() const;

  // virtual density coordinates
  int dLx() const;
  int dLy() const;
  int dUx() const;
  int dUy() const;
  int dCx() const;
  int dCy() const;
  int dDx() const;
  int dDy() const;

  void setCenterLocation(int cx, int cy);
  void setSize(int dx, int dy);

  void setDensityLocation(int dLx, int dLy);
  void setDensityCenterLocation(int dCx, int dCy);
  void setDensitySize(int dDx, int dDy);

  void setDensityScale(float densityScale);
  void setGradientX(float gradientX);
  void setGradientY(float gradientY);

  float gradientX() const { return gradientX_; }
  float gradientY() const { return gradientY_; }
  float densityScale() const { return densityScale_; }

  bool isInstance() const;
  bool isClusteredInstance() const;
  bool isFiller() const;
  bool isMacroInstance() const;
  bool isStdInstance() const;

 private:
  std::vector<Instance*> insts_;
  std::vector<GPin*> gPins_;
  int lx_ = 0;
  int ly_ = 0;
  int ux_ = 0;
  int uy_ = 0;

  int dLx_ = 0;
  int dLy_ = 0;
  int dUx_ = 0;
  int dUy_ = 0;

  float densityScale_ = 0;
  float gradientX_ = 0;
  float gradientY_ = 0;
};

inline int GCell::lx() const
{
  return lx_;
}
inline int GCell::ly() const
{
  return ly_;
}

inline int GCell::ux() const
{
  return ux_;
}

inline int GCell::uy() const
{
  return uy_;
}

inline int GCell::cx() const
{
  return (lx_ + ux_) / 2;
}

inline int GCell::cy() const
{
  return (ly_ + uy_) / 2;
}

inline int GCell::dx() const
{
  return ux_ - lx_;
}

inline int GCell::dy() const
{
  return uy_ - ly_;
}

inline int GCell::dLx() const
{
  return dLx_;
}

inline int GCell::dLy() const
{
  return dLy_;
}

inline int GCell::dUx() const
{
  return dUx_;
}

inline int GCell::dUy() const
{
  return dUy_;
}

inline int GCell::dCx() const
{
  return (dUx_ + dLx_) / 2;
}

inline int GCell::dCy() const
{
  return (dUy_ + dLy_) / 2;
}

inline int GCell::dDx() const
{
  return dUx_ - dLx_;
}

inline int GCell::dDy() const
{
  return dUy_ - dLy_;
}

class GNet
{
 public:
  GNet(Net* net);
  GNet(const std::vector<Net*>& nets);

  Net* net() const;
  const std::vector<Net*>& nets() const { return nets_; }
  const std::vector<GPin*>& gPins() const { return gPins_; }

  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;

  void setTimingWeight(float timingWeight);
  void setCustomWeight(float customWeight);

  float totalWeight() const { return timingWeight_ * customWeight_; }
  float timingWeight() const { return timingWeight_; }
  float customWeight() const { return customWeight_; }

  void addGPin(GPin* gPin);
  void updateBox();
  int64_t hpwl() const;

  void setDontCare();
  bool isDontCare() const;

  // clear WA(Weighted Average) variables.
  void clearWaVars();

  void addWaExpMinSumX(float waExpMinSumX);
  void addWaXExpMinSumX(float waXExpMinSumX);

  void addWaExpMinSumY(float waExpMinSumY);
  void addWaYExpMinSumY(float waYExpMinSumY);

  void addWaExpMaxSumX(float waExpMaxSumX);
  void addWaXExpMaxSumX(float waXExpMaxSumX);

  void addWaExpMaxSumY(float waExpMaxSumY);
  void addWaYExpMaxSumY(float waYExpMaxSumY);

  float waExpMinSumX() const;
  float waXExpMinSumX() const;

  float waExpMinSumY() const;
  float waYExpMinSumY() const;

  float waExpMaxSumX() const;
  float waXExpMaxSumX() const;

  float waExpMaxSumY() const;
  float waYExpMaxSumY() const;

 private:
  std::vector<GPin*> gPins_;
  std::vector<Net*> nets_;
  int lx_ = 0;
  int ly_ = 0;
  int ux_ = 0;
  int uy_ = 0;

  float timingWeight_ = 1;
  float customWeight_ = 1;

  //
  // weighted average WL model stor for better indexing
  // Please check the equation (4) in the ePlace-MS paper.
  //
  // WA: weighted Average
  // saving four variable will be helpful for
  // calculating the WA gradients/wirelengths.
  //
  // gamma: modeling accuracy.
  //
  // X forces.
  //
  // waExpMinSumX_: store sigma {exp(x_i/gamma)}
  // waXExpMinSumX_: store signa {x_i*exp(e_i/gamma)}
  // waExpMaxSumX_ : store sigma {exp(-x_i/gamma)}
  // waXExpMaxSumX_: store sigma {x_i*exp(-x_i/gamma)}
  //
  float waExpMinSumX_ = 0;
  float waXExpMinSumX_ = 0;

  float waExpMaxSumX_ = 0;
  float waXExpMaxSumX_ = 0;

  //
  // Y forces.
  //
  // waExpMinSumY_: store sigma {exp(y_i/gamma)}
  // waYExpMinSumY_: store signa {y_i*exp(e_i/gamma)}
  // waExpMaxSumY_ : store sigma {exp(-y_i/gamma)}
  // waYExpMaxSumY_: store sigma {y_i*exp(-y_i/gamma)}
  //
  float waExpMinSumY_ = 0;
  float waYExpMinSumY_ = 0;

  float waExpMaxSumY_ = 0;
  float waYExpMaxSumY_ = 0;

  bool isDontCare_ = false;
};

inline int GNet::lx() const
{
  return lx_;
}

inline int GNet::ly() const
{
  return ly_;
}

inline int GNet::ux() const
{
  return ux_;
}

inline int GNet::uy() const
{
  return uy_;
}

// eight add functions
inline void GNet::addWaExpMinSumX(float waExpMinSumX)
{
  waExpMinSumX_ += waExpMinSumX;
}

inline void GNet::addWaXExpMinSumX(float waXExpMinSumX)
{
  waXExpMinSumX_ += waXExpMinSumX;
}

inline void GNet::addWaExpMinSumY(float waExpMinSumY)
{
  waExpMinSumY_ += waExpMinSumY;
}

inline void GNet::addWaYExpMinSumY(float waYExpMinSumY)
{
  waYExpMinSumY_ += waYExpMinSumY;
}

inline void GNet::addWaExpMaxSumX(float waExpMaxSumX)
{
  waExpMaxSumX_ += waExpMaxSumX;
}

inline void GNet::addWaXExpMaxSumX(float waXExpMaxSumX)
{
  waXExpMaxSumX_ += waXExpMaxSumX;
}

inline void GNet::addWaExpMaxSumY(float waExpMaxSumY)
{
  waExpMaxSumY_ += waExpMaxSumY;
}

inline void GNet::addWaYExpMaxSumY(float waYExpMaxSumY)
{
  waYExpMaxSumY_ += waYExpMaxSumY;
}

inline float GNet::waExpMinSumX() const
{
  return waExpMinSumX_;
}

inline float GNet::waXExpMinSumX() const
{
  return waXExpMinSumX_;
}

inline float GNet::waExpMinSumY() const
{
  return waExpMinSumY_;
}

inline float GNet::waYExpMinSumY() const
{
  return waYExpMinSumY_;
}

inline float GNet::waExpMaxSumX() const
{
  return waExpMaxSumX_;
}

inline float GNet::waXExpMaxSumX() const
{
  return waXExpMaxSumX_;
}

inline float GNet::waExpMaxSumY() const
{
  return waExpMaxSumY_;
}

inline float GNet::waYExpMaxSumY() const
{
  return waYExpMaxSumY_;
}

class GPin
{
 public:
  GPin(Pin* pin);
  GPin(const std::vector<Pin*>& pins);

  Pin* pin() const;
  const std::vector<Pin*>& pins() const { return pins_; }

  GCell* gCell() const { return gCell_; }
  GNet* gNet() const { return gNet_; }

  void setGCell(GCell* gCell);
  void setGNet(GNet* gNet);

  int cx() const { return cx_; }
  int cy() const { return cy_; }

  // clear WA(Weighted Average) variables.
  void clearWaVars();

  void setMaxExpSumX(float maxExpSumX);
  void setMaxExpSumY(float maxExpSumY);
  void setMinExpSumX(float minExpSumX);
  void setMinExpSumY(float minExpSumY);

  float maxExpSumX() const { return maxExpSumX_; }
  float maxExpSumY() const { return maxExpSumY_; }
  float minExpSumX() const { return minExpSumX_; }
  float minExpSumY() const { return minExpSumY_; }

  bool hasMaxExpSumX() const { return hasMaxExpSumX_; }
  bool hasMaxExpSumY() const { return hasMaxExpSumY_; }
  bool hasMinExpSumX() const { return hasMinExpSumX_; }
  bool hasMinExpSumY() const { return hasMinExpSumY_; }

  void setCenterLocation(int cx, int cy);
  void updateLocation(const GCell* gCell);
  void updateDensityLocation(const GCell* gCell);

 private:
  GCell* gCell_ = nullptr;
  GNet* gNet_ = nullptr;
  std::vector<Pin*> pins_;

  int offsetCx_ = 0;
  int offsetCy_ = 0;
  int cx_ = 0;
  int cy_ = 0;

  // weighted average WL vals stor for better indexing
  // Please check the equation (4) in the ePlace-MS paper.
  //
  // maxExpSum_: holds exp(x_i/gamma)
  // minExpSum_: holds exp(-x_i/gamma)
  // the x_i is equal to cx_ variable.
  //
  float maxExpSumX_ = 0;
  float maxExpSumY_ = 0;

  float minExpSumX_ = 0;
  float minExpSumY_ = 0;

  // flag variables
  //
  // check whether
  // this pin is considered in a WA models.
  bool hasMaxExpSumX_ = false;
  bool hasMaxExpSumY_ = false;

  bool hasMinExpSumX_ = false;
  bool hasMinExpSumY_ = false;
};

class Bin
{
 public:
  Bin(int x, int y, int lx, int ly, int ux, int uy, float targetDensity);

  int x() const { return x_; };
  int y() const { return y_; };

  int lx() const { return lx_; };
  int ly() const { return ly_; };
  int ux() const { return ux_; };
  int uy() const { return uy_; };
  int cx() const;
  int cy() const;
  int dx() const;
  int dy() const;

  float electroPhi() const;
  float electroForceX() const;
  float electroForceY() const;
  float targetDensity() const;
  float density() const;

  void setDensity(float density);
  void setTargetDensity(float density);
  void setElectroForce(float electroForceX, float electroForceY);
  void setElectroPhi(float phi);

  void setNonPlaceArea(int64_t area);
  void setInstPlacedArea(int64_t area);
  void setFillerArea(int64_t area);

  void setNonPlaceAreaUnscaled(int64_t area);
  void setInstPlacedAreaUnscaled(int64_t area);

  void addNonPlaceArea(int64_t area);
  void addInstPlacedArea(int64_t area);
  void addFillerArea(int64_t area);

  void addNonPlaceAreaUnscaled(int64_t area);
  void addInstPlacedAreaUnscaled(int64_t area);

  const int64_t binArea() const;
  const int64_t nonPlaceArea() const { return nonPlaceArea_; }
  const int64_t instPlacedArea() const { return instPlacedArea_; }
  const int64_t nonPlaceAreaUnscaled() const { return nonPlaceAreaUnscaled_; }
  const int64_t instPlacedAreaUnscaled() const
  {
    return instPlacedAreaUnscaled_;
  }

  const int64_t fillerArea() const { return fillerArea_; }

 private:
  // index
  int x_ = 0;
  int y_ = 0;

  // coordinate
  int lx_ = 0;
  int ly_ = 0;
  int ux_ = 0;
  int uy_ = 0;

  int64_t nonPlaceArea_ = 0;
  int64_t instPlacedArea_ = 0;

  int64_t instPlacedAreaUnscaled_ = 0;
  int64_t nonPlaceAreaUnscaled_ = 0;
  int64_t fillerArea_ = 0;

  float density_ = 0;
  float targetDensity_ = 0;  // will enable bin-wise density screening
  float electroPhi_ = 0;
  float electroForceX_ = 0;
  float electroForceY_ = 0;
};

inline int Bin::cx() const
{
  return (ux_ + lx_) / 2;
}

inline int Bin::cy() const
{
  return (uy_ + ly_) / 2;
}

inline int Bin::dx() const
{
  return (ux_ - lx_);
}

inline int Bin::dy() const
{
  return (uy_ - ly_);
}

inline void Bin::setNonPlaceArea(int64_t area)
{
  nonPlaceArea_ = area;
}

inline void Bin::setNonPlaceAreaUnscaled(int64_t area)
{
  nonPlaceAreaUnscaled_ = area;
}

inline void Bin::setInstPlacedArea(int64_t area)
{
  instPlacedArea_ = area;
}

inline void Bin::setInstPlacedAreaUnscaled(int64_t area)
{
  instPlacedAreaUnscaled_ = area;
}

inline void Bin::setFillerArea(int64_t area)
{
  fillerArea_ = area;
}

inline void Bin::addNonPlaceArea(int64_t area)
{
  nonPlaceArea_ += area;
}

inline void Bin::addInstPlacedArea(int64_t area)
{
  instPlacedArea_ += area;
}

inline void Bin::addNonPlaceAreaUnscaled(int64_t area)
{
  nonPlaceAreaUnscaled_ += area;
}

inline void Bin::addInstPlacedAreaUnscaled(int64_t area)
{
  instPlacedAreaUnscaled_ += area;
}

inline void Bin::addFillerArea(int64_t area)
{
  fillerArea_ += area;
}

//
// The bin can be non-uniform because of
// "integer" coordinates
//
class BinGrid
{
 public:
  BinGrid() = default;
  BinGrid(Die* die);

  void setPlacerBase(std::shared_ptr<PlacerBase> pb);
  void setLogger(utl::Logger* log);
  void setCorePoints(const Die* die);
  void setBinCnt(int binCntX, int binCntY);
  void setTargetDensity(float density);
  void updateBinsGCellDensityArea(const std::vector<GCell*>& cells);

  void initBins();

  // lx, ly, ux, uy will hold coreArea
  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;
  int cx() const;
  int cy() const;
  int dx() const;
  int dy() const;

  int binCntX() const;
  int binCntY() const;
  int binSizeX() const;
  int binSizeY() const;

  int64_t overflowArea() const;
  int64_t overflowAreaUnscaled() const;

  // return bins_ index with given gcell
  std::pair<int, int> getDensityMinMaxIdxX(const GCell* gcell) const;
  std::pair<int, int> getDensityMinMaxIdxY(const GCell* gcell) const;

  std::pair<int, int> getMinMaxIdxX(const Instance* inst) const;
  std::pair<int, int> getMinMaxIdxY(const Instance* inst) const;

  std::vector<Bin>& bins();
  const std::vector<Bin>& binsConst() const { return bins_; };

  void updateBinsNonPlaceArea();

 private:
  std::vector<Bin> bins_;
  std::shared_ptr<PlacerBase> pb_;
  utl::Logger* log_ = nullptr;
  int lx_ = 0;
  int ly_ = 0;
  int ux_ = 0;
  int uy_ = 0;
  int binCntX_ = 0;
  int binCntY_ = 0;
  int binSizeX_ = 0;
  int binSizeY_ = 0;
  float targetDensity_ = 0;
  int64_t overflowArea_ = 0;
  int64_t overflowAreaUnscaled_ = 0;
  bool isSetBinCnt_ = false;
};

inline std::vector<Bin>& BinGrid::bins()
{
  return bins_;
}

class NesterovBaseVars
{
 public:
  float targetDensity = 1.0;
  int binCntX = 0;
  int binCntY = 0;
  float minWireLengthForceBar = -300;
  // temp variables
  bool isSetBinCnt = false;
  bool useUniformTargetDensity = false;

  void reset();
};

class NesterovPlaceVars
{
 public:
  int maxNesterovIter = 5000;
  int maxBackTrack = 10;
  float initDensityPenalty = 0.00008;       // INIT_LAMBDA
  float initWireLengthCoef = 0.25;          // base_wcof
  float targetOverflow = 0.1;               // overflow
  float minPhiCoef = 0.95;                  // pcof_min
  float maxPhiCoef = 1.05;                  // pcof_max
  float minPreconditioner = 1.0;            // MIN_PRE
  float initialPrevCoordiUpdateCoef = 100;  // z_ref_alpha
  float referenceHpwl = 446000000;          // refDeltaHpwl
  float routabilityCheckOverflow = 0.20;

  static const int maxRecursionWlCoef = 10;
  static const int maxRecursionInitSLPCoef = 10;

  bool forceCPU = false;
  bool timingDrivenMode = true;
  bool routabilityDrivenMode = true;
  bool debug = false;
  int debug_pause_iterations = 10;
  int debug_update_iterations = 10;
  bool debug_draw_bins = true;
  odb::dbInst* debug_inst = nullptr;

  void reset();
};

// Stores all pins, nets, and actual instances (static and movable)
// Used for calculating WL gradient
class NesterovBaseCommon
{
 public:
  NesterovBaseCommon(NesterovBaseVars nbVars,
                     std::shared_ptr<PlacerBaseCommon> pb,
                     utl::Logger* log);

  const std::vector<GCell*>& gCells() const { return gCells_; }
  const std::vector<GNet*>& gNets() const { return gNets_; }
  const std::vector<GPin*>& gPins() const { return gPins_; }

  //
  // placerBase To NesterovBase functions
  //
  GCell* pbToNb(Instance* inst) const;
  GPin* pbToNb(Pin* pin) const;
  GNet* pbToNb(Net* net) const;

  //
  // OpenDB To NesterovBase functions
  //
  GCell* dbToNb(odb::dbInst* inst) const;
  GPin* dbToNb(odb::dbITerm* pin) const;
  GPin* dbToNb(odb::dbBTerm* pin) const;
  GNet* dbToNb(odb::dbNet* net) const;

  // WL force update based on WeightedAverage model
  // wlCoeffX : WireLengthCoefficient for X.
  //            equal to 1 / gamma_x
  // wlCoeffY : WireLengthCoefficient for Y.
  //            equal to 1 / gamma_y
  //
  // Gamma is described in the ePlaceMS paper.
  //
  void updateWireLengthForceWA(float wlCoeffX, float wlCoeffY);

  FloatPoint getWireLengthGradientPinWA(const GPin* gPin,
                                        float wlCoeffX,
                                        float wlCoeffY) const;

  FloatPoint getWireLengthGradientWA(const GCell* gCell,
                                     float wlCoeffX,
                                     float wlCoeffY) const;

  // for preconditioner
  FloatPoint getWireLengthPreconditioner(const GCell* gCell) const;

  int64_t getHpwl();

  void updateDbGCells();

 private:
  NesterovBaseVars nbVars_;
  std::shared_ptr<PlacerBaseCommon> pbc_;
  utl::Logger* log_ = nullptr;

  std::vector<GCell> gCellStor_;
  std::vector<GNet> gNetStor_;
  std::vector<GPin> gPinStor_;

  std::vector<GCell*> gCells_;
  std::vector<GNet*> gNets_;
  std::vector<GPin*> gPins_;

  std::unordered_map<Instance*, GCell*> gCellMap_;
  std::unordered_map<Pin*, GPin*> gPinMap_;
  std::unordered_map<Net*, GNet*> gNetMap_;
};

// Stores instances belonging to a specific power domain
// along with fillers and virtual blockages
// Also stores the bin grid for the power domain
// Used to calculate density gradient
class NesterovBase
{
 public:
  NesterovBase(NesterovBaseVars nbVars,
               std::shared_ptr<PlacerBase> pb,
               std::shared_ptr<NesterovBaseCommon> nbc,
               utl::Logger* log);
  ~NesterovBase();

  const std::vector<GCell*>& gCells() const { return gCells_; }
  const std::vector<GCell*>& gCellInsts() const { return gCellInsts_; }
  const std::vector<GCell*>& gCellFillers() const { return gCellFillers_; }

  float getSumOverflow() const { return sumOverflow_; }
  float getSumOverflowUnscaled() const { return sumOverflowUnscaled_; }
  float getBaseWireLengthCoef() const { return baseWireLengthCoef_; }
  float getDensityPenalty() const { return densityPenalty_; }

  float getWireLengthGradSum() const { return wireLengthGradSum_; }
  float getDensityGradSum() const { return densityGradSum_; }

  // update gCells with cx, cy
  void updateGCellCenterLocation(const std::vector<FloatPoint>& coordis);

  void updateGCellDensityCenterLocation(const std::vector<FloatPoint>& coordis);

  int binCntX() const;
  int binCntY() const;
  int binSizeX() const;
  int binSizeY() const;
  int64_t overflowArea() const;
  int64_t overflowAreaUnscaled() const;

  std::vector<Bin>& bins();
  const std::vector<Bin>& binsConst() const { return bg_.binsConst(); };

  // filler cells / area control
  // will be used in Routability-driven loop
  int fillerDx() const;
  int fillerDy() const;
  int fillerCnt() const;
  int64_t fillerCellArea() const;
  int64_t whiteSpaceArea() const;
  int64_t movableArea() const;
  int64_t totalFillerArea() const;

  // update
  // fillerArea, whiteSpaceArea, movableArea
  // and totalFillerArea after changing gCell's size
  void updateAreas();

  // update density sizes with changed dx and dy
  void updateDensitySize();

  // should be separately defined.
  // This is mainly used for NesterovLoop
  int64_t nesterovInstsArea() const;

  // sum phi and target density
  // used in NesterovPlace
  float sumPhi() const;

  //
  // return uniform (lower bound) target density
  // LB of target density is required for massive runs.
  //
  float uniformTargetDensity() const;

  // initTargetDensity is set by users
  // targetDensity is equal to initTargetDensity and
  // would be changed dynamically in RD loop
  //
  float initTargetDensity() const;
  float targetDensity() const;

  void setTargetDensity(float targetDensity);

  // RD can shrink the number of fillerCells.
  void cutFillerCells(int64_t targetFillerArea);

  void updateDensityCoordiLayoutInside(GCell* gcell);

  float getDensityCoordiLayoutInsideX(const GCell* gCell, float cx) const;
  float getDensityCoordiLayoutInsideY(const GCell* gCell, float cy) const;

  // FloatPoint getRegionGradient(const GCell* gCell, FloatPoint nextLocation)
  // const;

  // for preconditioner
  FloatPoint getDensityPreconditioner(const GCell* gCell) const;

  FloatPoint getDensityGradient(const GCell* gCell) const;

  // update electrostatic forces within Bin
  void updateDensityForceBin();

  BinGrid& getBinGrid() { return bg_; }

  // Nesterov Loop
  void initDensity1();
  float initDensity2(float wlCoeffX, float wlCoeffY);
  void setNpVars(NesterovPlaceVars* npVars) { npVars_ = npVars; }
  void setIter(int iter) { iter_ = iter; }
  void setMaxPhiCoefChanged(bool maxPhiCoefChanged)
  {
    isMaxPhiCoefChanged_ = maxPhiCoefChanged;
  }

  void updateGradients(std::vector<FloatPoint>& sumGrads,
                       std::vector<FloatPoint>& wireLengthGrads,
                       std::vector<FloatPoint>& densityGrads,
                       float wlCoeffX,
                       float wlCoeffY);

  void updateInitialPrevSLPCoordi();

  float getStepLength(const std::vector<FloatPoint>& prevSLPCoordi_,
                      const std::vector<FloatPoint>& prevSLPSumGrads_,
                      const std::vector<FloatPoint>& curSLPCoordi_,
                      const std::vector<FloatPoint>& curSLPSumGrads_);

  void updateNextIter(int iter);
  float getPhiCoef(float scaledDiffHpwl) const;
  void cutFillerCoordinates();

  void snapshot();

  bool checkConvergence();
  bool checkDivergence();
  bool revertDivergence();

  void updatePrevGradient(float wlCoeffX, float wlCoeffY);
  void updateCurGradient(float wlCoeffX, float wlCoeffY);
  void updateNextGradient(float wlCoeffX, float wlCoeffY);

  void updateDensityCenterCur();
  void updateDensityCenterCurSLP();
  void updateDensityCenterPrevSLP();
  void updateDensityCenterNextSLP();

  void nesterovUpdateCoordinates(float coeff);
  bool nesterovUpdateStepLength();
  void nesterovAdjustPhi();

  void resetMinSumOverflow();

  void printStepLength() { printf("stepLength = %f\n", stepLength_); }

  bool isDiverged() const { return isDiverged_; }

 private:
  NesterovBaseVars nbVars_;
  std::shared_ptr<PlacerBase> pb_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  utl::Logger* log_ = nullptr;

  BinGrid bg_;
  std::unique_ptr<FFT> fft_;

  int fillerDx_ = 0;
  int fillerDy_ = 0;
  int64_t whiteSpaceArea_ = 0;
  int64_t movableArea_ = 0;
  int64_t totalFillerArea_ = 0;

  int64_t stdInstsArea_ = 0;
  int64_t macroInstsArea_ = 0;

  std::vector<GCell> gCellStor_;

  std::vector<GCell*> gCells_;
  std::vector<GCell*> gCellInsts_;
  std::vector<GCell*> gCellFillers_;

  float sumPhi_ = 0;
  float targetDensity_ = 0;
  float uniformTargetDensity_ = 0;

  // Nesterov loop data for each region
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

  float wireLengthGradSum_ = 0;
  float densityGradSum_ = 0;

  // alpha
  float stepLength_ = 0;

  // opt_phi_cof
  float densityPenalty_ = 0;

  // base_wcof
  float baseWireLengthCoef_ = 0;

  // phi is described in ePlace paper.
  float sumOverflow_ = 0;
  float sumOverflowUnscaled_ = 0;

  // half-parameter-wire-length
  int64_t prevHpwl_ = 0;

  float isDiverged_ = false;

  std::string divergeMsg_;
  int divergeCode_ = 0;

  NesterovPlaceVars* npVars_;

  bool isMaxPhiCoefChanged_ = false;

  float minSumOverflow_ = 1e30;
  float hpwlWithMinSumOverflow_ = 1e30;
  int iter_ = 0;
  bool isConverged_ = false;

  // Snapshot data
  std::vector<FloatPoint> snapshotCoordi_;
  std::vector<FloatPoint> snapshotSLPCoordi_;
  std::vector<FloatPoint> snapshotSLPSumGrads_;
  float snapshotDensityPenalty_ = 0;
  float snapshotStepLength_ = 0;

  void initFillerGCells();
};

inline std::vector<Bin>& NesterovBase::bins()
{
  return bg_.bins();
}

class biNormalParameters
{
 public:
  float meanX;
  float meanY;
  float sigmaX;
  float sigmaY;
  float lx;
  float ly;
  float ux;
  float uy;
};

}  // namespace gpl
