// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "boost/unordered/unordered_flat_map.hpp"
#include "gpl/Replace.h"
#include "odb/db.h"
#include "placerBase.h"
#include "point.h"
#include "routeBase.h"
#include "utl/Logger.h"
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
class GCellHandle;

class Instance;
class Pin;
class Net;

class GPin;
class FFT;
class nesterovDbCbk;

class GCell
{
 public:
  enum class GCellChange : uint8_t
  {
    kNone,
    kRoutability,
    kTimingDriven,
    kNewInstance,
    kDownsize,
    kUpsize,
    kResizeNoChange
  };

  // instance cells
  GCell(Instance* inst);
  GCell(const std::vector<Instance*>& insts);

  // filler cells
  GCell(int cx, int cy, int dx, int dy);

  const std::vector<Instance*>& insts() const { return insts_; }
  const std::vector<GPin*>& gPins() const { return gPins_; }

  std::string getName() const;

  void addGPin(GPin* gPin);
  void clearGPins() { gPins_.clear(); }

  void updateLocations();

  bool isLocked() const;
  void lock();

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
  // void setLocation(int x, int y);
  void setSize(int dx, int dy, GCellChange change = GCellChange::kNone);
  void setAreaChangeType(GCellChange change) { change_ = change; }
  GCellChange changeType() const { return change_; }
  void setAllLocations(int lx, int ly, int ux, int uy);

  void setDensityLocation(int dLx, int dLy);
  void setDensityCenterLocation(int dCx, int dCy);
  void setDensitySize(int dDx, int dDy);

  void setDensityScale(float densityScale);
  void setGradientX(float gradientX);
  void setGradientY(float gradientY);

  float getGradientX() const { return gradientX_; }
  float getGradientY() const { return gradientY_; }
  float getDensityScale() const { return densityScale_; }

  bool isInstance() const;
  bool isFiller() const;
  bool isMacroInstance() const;
  bool isStdInstance() const;
  bool contains(odb::dbInst* db_inst) const;

  void print(utl::Logger* logger, bool print_only_name) const;
  void writeAttributesToCSV(std::ostream& out) const;

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

  GCellChange change_ = GCellChange::kNone;
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

  Net* getPbNet() const;
  const std::vector<Net*>& getPbNets() const { return nets_; }
  const std::vector<GPin*>& getGPins() const { return gPins_; }

  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;

  void setTimingWeight(float timingWeight);
  void setCustomWeight(float customWeight);

  float getTotalWeight() const { return timingWeight_ * customWeight_; }
  float getTimingWeight() const { return timingWeight_; }
  float getCustomWeight() const { return customWeight_; }

  void addGPin(GPin* gPin);
  void clearGPins() { gPins_.clear(); }
  void updateBox();
  int64_t getHpwl() const;

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

  void print(utl::Logger* log) const;

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

  Pin* getPbPin() const;
  const std::vector<Pin*>& getPbPins() const { return pins_; }

  GCell* getGCell() const { return gCell_; }
  GNet* getGNet() const { return gNet_; }

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

  void print(utl::Logger* log) const;
  void updateCoordi();

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
  Bin() = default;
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
  float getTargetDensity() const;
  float getDensity() const;

  void setDensity(float density);
  void setBinTargetDensity(float density);
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

  int64_t getBinArea() const;
  int64_t getNonPlaceArea() const { return nonPlaceArea_; }
  int64_t instPlacedArea() const { return instPlacedArea_; }
  int64_t getNonPlaceAreaUnscaled() const { return nonPlaceAreaUnscaled_; }
  int64_t getInstPlacedAreaUnscaled() const { return instPlacedAreaUnscaled_; }

  int64_t getFillerArea() const { return fillerArea_; }

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
  BinGrid(int lx, int ly, int ux, int uy);

  void setPlacerBase(std::shared_ptr<PlacerBase> pb);
  void setLogger(utl::Logger* log);
  void setRegionPoints(int lx, int ly, int ux, int uy);
  void setBinCnt(int binCntX, int binCntY);
  void setBinTargetDensity(float density);
  void updateBinsGCellDensityArea(const std::vector<GCellHandle>& cells);
  void setNumThreads(int num_threads) { num_threads_ = num_threads; }

  void initBins();

  // lx, ly, ux, uy will hold region area
  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;
  int cx() const;
  int cy() const;
  int dx() const;
  int dy() const;

  int getBinCntX() const;
  int getBinCntY() const;
  double getBinSizeX() const;
  double getBinSizeY() const;

  int64_t getOverflowArea() const;
  int64_t getOverflowAreaUnscaled() const;

  // return bins_ index with given gcell
  std::pair<int, int> getDensityMinMaxIdxX(const GCell* gcell) const;
  std::pair<int, int> getDensityMinMaxIdxY(const GCell* gcell) const;

  std::pair<int, int> getMinMaxIdxX(const Instance* inst) const;
  std::pair<int, int> getMinMaxIdxY(const Instance* inst) const;

  std::vector<Bin>& getBins();
  const std::vector<Bin>& getBinsConst() const { return bins_; };

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
  double binSizeX_ = 0;
  double binSizeY_ = 0;
  float targetDensity_ = 0;
  int64_t sumOverflowArea_ = 0;
  int64_t sumOverflowAreaUnscaled_ = 0;
  bool isSetBinCnt_ = false;
  int num_threads_ = 1;
};

inline std::vector<Bin>& BinGrid::getBins()
{
  return bins_;
}

struct NesterovBaseVars
{
  NesterovBaseVars(const PlaceOptions& options);

  const bool isSetBinCnt;
  const bool useUniformTargetDensity;
  bool isMaxPhiCoefChanged = false;  // not user config
  const float targetDensity;
  const int binCntX;
  const int binCntY;

  const float minPhiCoef;
  float maxPhiCoef;  // may be updated after initialization

  static constexpr float minWireLengthForceBar = -300;
};

struct NesterovPlaceVars
{
  NesterovPlaceVars(const PlaceOptions& options);

  int maxNesterovIter;
  static constexpr int maxBackTrack = 10;
  const float initDensityPenalty;                  // INIT_LAMBDA
  const float initWireLengthCoef;                  // base_wcof
  float targetOverflow;                            // overflow
  static constexpr float minPreconditioner = 1.0;  // MIN_PRE
  float initialPrevCoordiUpdateCoef = 100;         // z_ref_alpha
  const float referenceHpwl;                       // refDeltaHpwl
  const float routability_end_overflow;
  const float routability_snapshot_overflow;
  const float keepResizeBelowOverflow;

  static constexpr int maxRecursionWlCoef = 10;
  static constexpr int maxRecursionInitSLPCoef = 10;

  bool timingDrivenMode;
  int timingDrivenIterCounter = 0;
  const bool routability_driven_mode;
  const bool disableRevertIfDiverge;

  bool debug = false;
  int debug_pause_iterations = 10;
  int debug_update_iterations = 10;
  bool debug_draw_bins = true;
  odb::dbInst* debug_inst = nullptr;
  int debug_start_iter = 0;
  int debug_rudy_start = 5000;
  int debug_rudy_stride = 1;
  bool debug_generate_images = false;
  std::string debug_images_path = "REPORTS_DIR";
};

// Stores all pins, nets, and actual instances (static and movable)
// Used for calculating WL gradient
class NesterovBaseCommon
{
 public:
  NesterovBaseCommon(NesterovBaseVars nbVars,
                     std::shared_ptr<PlacerBaseCommon> pb,
                     utl::Logger* log,
                     int num_threads,
                     const Clusters& clusters);

  void reportInstanceExtensionByPinDensity() const;
  const std::vector<GCell*>& getGCells() const { return nbc_gcells_; }
  const std::vector<GNet*>& getGNets() const { return gNets_; }
  const std::vector<GPin*>& getGPins() const { return gPins_; }

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

  // Number of threads of execution
  size_t getNumThreads() { return num_threads_; }

  GCell* getGCellByIndex(size_t i);

  void setCbk(nesterovDbCbk* cbk) { db_cbk_ = cbk; }
  size_t createCbkGCell(odb::dbInst* db_inst);
  void createCbkGNet(odb::dbNet* net, bool skip_io_mode);
  void createCbkITerm(odb::dbITerm* iTerm);
  std::pair<odb::dbInst*, size_t> destroyCbkGCell(odb::dbInst* db_inst);
  void destroyCbkGNet(odb::dbNet*);
  void destroyCbkITerm(odb::dbITerm*);
  void resizeGCell(odb::dbInst* db_inst);
  void moveGCell(odb::dbInst* db_inst);
  void fixPointers();

  void resetMinRcCellSize();
  void resizeMinRcCellSize();
  void updateMinRcCellSize();
  void revertGCellSizeToMinRc();

  GCell& getGCell(size_t index);
  size_t getGCellIndex(const GCell* gCell) const;

  void printGCells();
  void printGPins();

  // TODO do this for each region? Also, manage this properly if other callbacks
  // are implemented.
  int64_t getDeltaArea() { return delta_area_; }
  void resetDeltaArea() { delta_area_ = 0; }
  int getNewGcellsCount() { return new_gcells_count_; }
  int getDeletedGcellsCount() { return deleted_gcells_count_; }
  void resetNewGcellsCount()
  {
    new_gcells_count_ = 0;
    deleted_gcells_count_ = 0;
  }

  NesterovBaseVars& getNbVars() { return nbVars_; }

 private:
  NesterovBaseVars nbVars_;
  std::shared_ptr<PlacerBaseCommon> pbc_;
  utl::Logger* log_ = nullptr;

  std::vector<GCell> gCellStor_;
  std::vector<GNet> gNetStor_;
  std::vector<GPin> gPinStor_;

  std::vector<GCell*> nbc_gcells_;
  // For usage in routability mode, parallel to nbc_gcells_
  std::vector<odb::Rect> minRcCellSize_;
  std::vector<GNet*> gNets_;
  std::vector<GPin*> gPins_;

  boost::unordered::unordered_flat_map<Instance*, GCell*> gCellMap_;
  boost::unordered::unordered_flat_map<Pin*, GPin*> gPinMap_;
  boost::unordered::unordered_flat_map<Net*, GNet*> gNetMap_;

  boost::unordered::unordered_flat_map<odb::dbInst*, size_t>
      db_inst_to_nbc_index_map_;
  boost::unordered::unordered_flat_map<odb::dbNet*, size_t>
      db_net_to_index_map_;
  boost::unordered::unordered_flat_map<odb::dbITerm*, size_t>
      db_iterm_to_index_map_;
  boost::unordered::unordered_flat_map<odb::dbBTerm*, size_t>
      db_bterm_to_index_map_;

  // These three deques should not be required if placerBase allows for dynamic
  // modifications on its vectors.
  std::deque<Instance> pb_insts_stor_;
  std::deque<Net> pb_nets_stor_;
  std::deque<Pin> pb_pins_stor_;

  int num_threads_;
  int64_t delta_area_;
  int new_gcells_count_;
  int deleted_gcells_count_;
  nesterovDbCbk* db_cbk_{nullptr};
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

  GCell& getFillerGCell(size_t index);

  const std::vector<GCellHandle>& getGCells() const { return nb_gcells_; }

  float getSumOverflow() const { return sum_overflow_; }
  float getSumOverflowUnscaled() const { return sum_overflow_unscaled_; }
  float getBaseWireLengthCoef() const { return baseWireLengthCoef_; }
  float getDensityPenalty() const { return densityPenalty_; }

  float getWireLengthGradSum() const { return wireLengthGradSum_; }
  float getDensityGradSum() const { return densityGradSum_; }

  // update gCells with cx, cy
  void updateGCellCenterLocation(const std::vector<FloatPoint>& coordis);

  void updateGCellDensityCenterLocation(const std::vector<FloatPoint>& coordis);

  int getBinCntX() const;
  int getBinCntY() const;
  double getBinSizeX() const;
  double getBinSizeY() const;
  int64_t getOverflowArea() const;
  int64_t getOverflowAreaUnscaled() const;

  std::vector<Bin>& getBins();
  const std::vector<Bin>& getBinsConst() const { return bg_.getBinsConst(); };

  // filler cells / area control
  // will be used in Routability-driven loop
  int getFillerDx() const;
  int getFillerDy() const;
  int getFillerCnt() const;
  int64_t getFillerCellArea() const;
  int64_t getWhiteSpaceArea() const;
  int64_t getMovableArea() const;
  int64_t getTotalFillerArea() const;

  void setMovableArea(int64_t area) { movableArea_ = area; }

  // update
  // fillerArea, whiteSpaceArea, movableArea
  // and totalFillerArea after changing gCell's size
  void updateAreas();

  // update density sizes with changed dx and dy
  void updateDensitySize();

  // should be separately defined.
  // This is mainly used for NesterovLoop
  int64_t getNesterovInstsArea() const;
  int64_t getStdInstArea() const { return this->stdInstsArea_; }
  int64_t getMacroInstArea() const { return this->macroInstsArea_; }

  // sum phi and target density
  // used in NesterovPlace
  float getSumPhi() const;

  //
  // return uniform (lower bound) target density
  // LB of target density is required for massive runs.
  //
  float getUniformTargetDensity() const;

  // initTargetDensity is set by users
  // targetDensity is equal to initTargetDensity and
  // would be changed dynamically in RD loop
  //
  float initTargetDensity() const;
  float getTargetDensity() const;

  void setTargetDensity(float targetDensity);
  void checkConsistency();

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
  void setNpVars(const NesterovPlaceVars* npVars) { npVars_ = npVars; }
  void setIter(int iter) { iter_ = iter; }
  void setMaxPhiCoefChanged(bool maxPhiCoefChanged)
  {
    nbVars_.isMaxPhiCoefChanged = maxPhiCoefChanged;
  }

  void updateGradients(std::vector<FloatPoint>& sumGrads,
                       std::vector<FloatPoint>& wireLengthGrads,
                       std::vector<FloatPoint>& densityGrads,
                       float wlCoeffX,
                       float wlCoeffY);

  void nbUpdatePrevGradient(float wlCoeffX, float wlCoeffY);
  void nbUpdateCurGradient(float wlCoeffX, float wlCoeffY);
  void nbUpdateNextGradient(float wlCoeffX, float wlCoeffY);

  // Used for updates based on callbacks
  void updateSingleGradient(size_t gCellIndex,
                            std::vector<FloatPoint>& sumGrads,
                            std::vector<FloatPoint>& wireLengthGrads,
                            std::vector<FloatPoint>& densityGrads,
                            float wlCoeffX,
                            float wlCoeffY);

  void updateSinglePrevGradient(size_t gCellIndex,
                                float wlCoeffX,
                                float wlCoeffY);
  void updateSingleCurGradient(size_t gCellIndex,
                               float wlCoeffX,
                               float wlCoeffY);

  void updateInitialPrevSLPCoordi();

  float getStepLength(const std::vector<FloatPoint>& prevSLPCoordi_,
                      const std::vector<FloatPoint>& prevSLPSumGrads_,
                      const std::vector<FloatPoint>& curSLPCoordi_,
                      const std::vector<FloatPoint>& curSLPSumGrads_);

  void updateNextIter(int iter);
  void setTrueReprintIterHeader() { reprint_iter_header_ = true; }
  float getPhiCoef(float scaledDiffHpwl) const;
  float getStoredPhiCoef() const { return phiCoef_; }
  float getStoredStepLength() const { return stepLength_; }
  float getStoredCoordiDistance() const { return coordiDistance_; }
  float getStoredGradDistance() const { return gradDistance_; }

  bool checkConvergence(int gpl_iter_count,
                        int routability_gpl_iter_count,
                        RouteBase* rb);

  bool checkDivergence();
  void saveSnapshot();
  bool revertToSnapshot();

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

  void createCbkGCell(odb::dbInst* db_inst, size_t stor_index);
  std::pair<odb::dbInst*, size_t> destroyCbkGCell(odb::dbInst* db_inst);
  bool updateHandle(odb::dbInst* db_inst, size_t handle);

  // Must be called after fixPointers() to initialize internal values of gcells,
  // including parallel vectors.
  void updateGCellState(float wlCoeffX, float wlCoeffY);

  void destroyFillerGCell(size_t index_remove);
  void restoreRemovedFillers();
  void clearRemovedFillers() { removed_fillers_.clear(); }

  void appendGCellCSVNote(const std::string& filename,
                          int iteration,
                          const std::string& message) const;
  // Helper to be used at nesterovPlace.cpp, inside core nesterov loop
  // Example:
  // for(auto& nb : nbVec_) {
  //   nb->writeGCellVectorsToCSV("gcells_vector.csv", nesterov_iter, 320, 1,
  //   1);
  // }
  void writeGCellVectorsToCSV(const std::string& filename,
                              int iteration,
                              int start_iteration = 0,
                              int iteration_stride = 50,
                              int gcell_index_stride = 10) const;

  std::shared_ptr<PlacerBase> getPb() const { return pb_; }

  odb::dbGroup* getGroup() const { return pb_->getGroup(); }

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
  int64_t initial_filler_area_ = 0;

  int64_t stdInstsArea_ = 0;
  int64_t macroInstsArea_ = 0;

  std::vector<GCell> fillerStor_;
  std::vector<GCellHandle> nb_gcells_;

  std::unordered_map<odb::dbInst*, size_t> db_inst_to_nb_index_;
  std::unordered_map<size_t, size_t> filler_stor_index_to_nb_index_;

  // used to update gcell states after fixPointers() is called
  std::vector<odb::dbInst*> new_instances_;

  struct RemovedFillerState
  {
    GCell gcell;
    FloatPoint curSLPCoordi;
    FloatPoint curSLPWireLengthGrads;
    FloatPoint curSLPDensityGrads;
    FloatPoint curSLPSumGrads;
    FloatPoint nextSLPCoordi;
    FloatPoint nextSLPWireLengthGrads;
    FloatPoint nextSLPDensityGrads;
    FloatPoint nextSLPSumGrads;
    FloatPoint prevSLPCoordi;
    FloatPoint prevSLPWireLengthGrads;
    FloatPoint prevSLPDensityGrads;
    FloatPoint prevSLPSumGrads;
    FloatPoint curCoordi;
    FloatPoint nextCoordi;
    FloatPoint initCoordi;
    FloatPoint snapshotCoordi;
    FloatPoint snapshotSLPCoordi;
    FloatPoint snapshotSLPSumGrads;
  };

  std::vector<RemovedFillerState> removed_fillers_;

  float sumPhi_ = 0;
  float phiCoef_ = 0;
  float targetDensity_ = 0;
  float uniformTargetDensity_ = 0;

  // StepLength parameters (also included in the np debugPrint)
  // alpha
  float stepLength_ = 0;
  float coordiDistance_ = 0;
  float gradDistance_ = 0;

  // Nesterov loop data for each region, using parallel vectors
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

  // Snapshot data for routability, parallel vectors
  std::vector<FloatPoint> snapshotCoordi_;
  std::vector<FloatPoint> snapshotSLPCoordi_;
  std::vector<FloatPoint> snapshotSLPSumGrads_;
  float snapshotDensityPenalty_ = 0;
  float snapshotStepLength_ = 0;

  // For destroying elements in parallel vectors
  void swapAndPop(std::vector<FloatPoint>& vec,
                  size_t remove_index,
                  size_t last_index);
  void swapAndPopParallelVectors(size_t remove_index, size_t last_index);
  void appendParallelVectors();

  float wireLengthGradSum_ = 0;
  float densityGradSum_ = 0;

  // opt_phi_cof
  float densityPenalty_ = 0;

  // base_wcof
  float baseWireLengthCoef_ = 0;

  // phi is described in ePlace paper.
  float sum_overflow_ = 0;
  float sum_overflow_unscaled_ = 0;
  float prev_reported_overflow_unscaled_ = 0;

  // half-parameter-wire-length
  int64_t prev_hpwl_ = 0;
  int64_t prev_reported_hpwl_ = 0;

  bool isDiverged_ = false;

  const NesterovPlaceVars* npVars_ = nullptr;

  float minSumOverflow_ = 1e30;
  float hpwlWithMinSumOverflow_ = 1e30;
  int iter_ = 0;
  bool isConverged_ = false;
  bool reprint_iter_header_ = false;

  void initFillerGCells();
};

inline std::vector<Bin>& NesterovBase::getBins()
{
  return bg_.getBins();
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

class GCellHandle
{
 public:
  GCellHandle(NesterovBaseCommon* nbc, size_t idx)
      : storage_(nbc), storage_index_(idx)
  {
  }

  GCellHandle(NesterovBase* nb, size_t idx) : storage_(nb), storage_index_(idx)
  {
  }

  // Non-const versions
  GCell* operator->() { return &getGCell(); }
  GCell& operator*() { return getGCell(); }
  operator GCell*() { return &getGCell(); }

  // Const versions
  const GCell* operator->() const { return &getGCell(); }
  const GCell& operator*() const { return getGCell(); }
  operator const GCell*() const { return &getGCell(); }

  bool isNesterovBaseCommon() const
  {
    return std::holds_alternative<NesterovBaseCommon*>(storage_);
  }

  void updateHandle(NesterovBaseCommon* nbc, size_t new_index)
  {
    storage_ = nbc;
    storage_index_ = new_index;
  }

  void updateHandle(NesterovBase* nb, size_t new_index)
  {
    storage_ = nb;
    storage_index_ = new_index;
  }

  size_t getStorageIndex() const { return storage_index_; }

 private:
  using StorageVariant = std::variant<NesterovBaseCommon*, NesterovBase*>;

  GCell& getGCell() const
  {
    if (std::holds_alternative<NesterovBaseCommon*>(storage_)) {
      return std::get<NesterovBaseCommon*>(storage_)->getGCell(storage_index_);
    }
    return std::get<NesterovBase*>(storage_)->getFillerGCell(storage_index_);
  }

  StorageVariant storage_;
  size_t storage_index_;
};

inline bool isValidSigType(const odb::dbSigType& db_type)
{
  return (db_type == odb::dbSigType::SIGNAL
          || db_type == odb::dbSigType::CLOCK);
}

inline constexpr const char* format_label_int = "{:27} {:10}";
inline constexpr const char* format_label_float = "{:27} {:10.4f}";
inline constexpr const char* format_label_um2 = "{:27} {:10.3f} um^2";
inline constexpr const char* format_label_percent = "{:27} {:10.2f} %";
inline constexpr const char* format_label_um2_with_delta
    = "{:27} {:10.3f} um^2 ({:+.2f}%)";

}  // namespace gpl
