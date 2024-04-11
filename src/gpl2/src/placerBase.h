///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2023, The Regents of the University of California
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

// Keep in mind that CUDA is C not C++ !!!
// To clarify, a device function in CUDA is a function that is executed on the GPU, 
// and it can be called from the host or from the device. 
// Thrust package is called from the host perspective.
// All the functions should be defined as struct.
// In our CUDA implementation, we may borrow some implementation from the original c++ implementation,
// so we always include original definition
// Compared to original C++ implementation, we merge the placerBase and nesterovBase 
// into general-purpose placer database:  GpuPlacerBase
// So we just need to maintain one database in cuda.


#pragma once

#include "placerObjects.h"
#include "poissonSolver.h"
#include "wirelengthOp.h"
#include "densityOp.h"
#include "util.h" 
#include "cudaUtil.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <cuda.h>
#include <cuda_runtime.h>
#include <thrust/copy.h>
#include <thrust/device_vector.h>
#include <thrust/fill.h>
#include <thrust/host_vector.h>
#include <thrust/sequence.h>
#include <thrust/for_each.h>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include "db_sta/dbNetwork.hh"
#include "sta/Liberty.hh"


namespace odb {
class dbDatabase;

class dbInst;
class dbITerm;
class dbBTerm;
class dbNet;
class dbGroup;

class dbPlacementStatus;
class dbSigType;

class dbBox;

class Rect;
class Point;

}  // namespace odb

namespace utl {
class Logger;
}


namespace sta {
class dbNetwork;
class dbSta;
}  // namespace sta
 

namespace gpl2 {


class Net;
class Pin;
class Instance;
class BinGrid;
class Bin;
class WirelengthOp;
class DensityOp;

// Variables used for Initial placement
class PlacerBaseVars
{
 public:
  int padLeft;
  int padRight;
  bool skipIoMode;
  int row_limit; // if the height of a standard cell is larger than row_limit, 
                 // we think it is a macro

  PlacerBaseVars();
  void reset();
};


class NesterovBaseVars
{
 public:
  float targetDensity;
  int binCntX;
  int binCntY;
  float minWireLengthForceBar;
  // temp variables
  bool isSetBinCnt = true;
  bool useUniformTargetDensity = true;

  NesterovBaseVars();
  void reset();
};



// Variables used for Nesterov placement
class NesterovPlaceVars
{
 public:
  int maxNesterovIter;
  int maxBackTrack;
  float initDensityPenalty;           // INIT_LAMBDA
  float initWireLengthCoef;           // base_wcof
  float targetOverflow;               // overflow
  float minPhiCoef;                   // pcof_min
  float maxPhiCoef;                   // pcof_max
  float minPreconditioner;            // MIN_PRE
  float initialPrevCoordiUpdateCoef;  // z_ref_alpha
  float referenceHpwl;                // refDeltaHpwl
  float routabilityCheckOverflow;

  static const int maxRecursionWlCoef = 10;
  static const int maxRecursionInitSLPCoef = 10;

  bool timingDrivenMode;
  bool routabilityDrivenMode;
 
  NesterovPlaceVars();
  void reset();
};

// Class includes everything from PlacerBase that is not region specific
// All the tricks of data structure is on how we design the layout 
// of pins, insts, nets.
// Stores all pins, nets, and actual instances (static and movable)
// Used to calculate wirelength gradient (wirelength is global)
// density gradient (density is local, so it's in GpuPlacerBase class)
// We try to follow the convention in the original implementation
class PlacerBaseCommon {
  public:
    PlacerBaseCommon();
    PlacerBaseCommon(sta::dbNetwork* network,
                     odb::dbDatabase* db,
                     PlacerBaseVars pbVars,
                     utl::Logger* log,
                     float haloWidth,  // in um
                     int virtualIter = 0, // virtual iteration
                     int numHops = 4,
                     float bloatFactor = 1.0, 
                     bool clusterFlag = false,
                     bool dataflowFlag = false,
                     bool datapathFlag = false,
                     bool clusterConstraintFlag = false);
    ~PlacerBaseCommon();

    // accessor to host objects
    // Mainly for initialization  
    Die& die() { return die_; }
    odb::dbDatabase* db() const { return db_; }
    utl::Logger* logger() const { return log_; }
    odb::dbBlock* getBlock() const { return db_->getChip()->getBlock(); }

    const std::vector<Instance*>& placeInsts() const { return placeInsts_; }
    // here the instances consist of placeInsts, fixedInsts, and dummyInsts
    const std::vector<Instance*>& insts() const { return insts_; }
    // All the pins
    const std::vector<Pin*>& pins() const { return pins_; }
    // all the nets
    const std::vector<Net*>& nets() const { return nets_; }

    void setVirtualWeightFactor(float weight) {
      virtualWeightFactor_ = weight;
      std::cout << "[INFO] Set virtualWeightFactor to " << virtualWeightFactor_ << std::endl;
    }

    // basic information
    void printInfo() const;
    int siteSizeX() const { return siteSizeX_; }
    int siteSizeY() const { return siteSizeY_; }
    int64_t placeInstsArea() const { return placeInstsArea_; }
    int64_t nonPlaceInstsArea() const { return nonPlaceInstsArea_; }
    int64_t macroInstsArea() const { return macroInstsArea_; }
    int64_t stdInstsArea() const { return stdCellInstsArea_; }
    int numInsts() const { return numInsts_; }
    int numPlaceInsts() const { return numPlaceInsts_; }
    int numFixedInsts() const { return numFixedInsts_; }
    int numDummyInsts() const { return numDummyInsts_; }
    int numNets() const { return nets_.size(); }
    int numPins() const { return pins_.size(); }

    // WL force update based on WeightedAverage model
    // wlCoeffX : WireLengthCoefficient for X.
    //            equal to 1 / gamma_x
    // wlCoeffY : WireLengthCoefficient for Y.
    //            equal to 1 / gamma_y
    //
    // Gamma is described in the ePlaceMS paper.
    // We calculate the weighted average wirelength and
    // its gradient for each pin at the same time.
    // Following functions are designed for nestrovPlace
    void updateWireLengthForce(float wlCoeffX, float wlCoeffY);
    void updatePinLocation();
    int* dInstDCxPtr() const {  return dInstDCxPtr_; }
    int* dInstDCyPtr() const {  return dInstDCyPtr_; }
    float* dWLGradXPtr() const { return dWLGradXPtr_; }
    float* dWLGradYPtr() const { return dWLGradYPtr_; }    
    int64_t hpwl() const;
    
    void updateDB();
    void updateDBCluster();
    void evaluateHPWL();

    // This is called during nesterovPlace::init
    void initCUDAKernel();
    void enableDREAMPlaceFlag() { wlGradOp_->enableDREAMPlaceFlag(); };


    void updateVirtualWeightFactor(int iter);
    void reset();

  private:
    sta::dbNetwork* network_;
    odb::dbDatabase* db_;
    utl::Logger* log_;
    PlacerBaseVars pbVars_;
    odb::dbBlock* block_ = nullptr;
    WirelengthOp* wlGradOp_;

    int siteSizeX_;
    int siteSizeY_;
    Die die_;

    int haloWidth_;
    int virtualIter_;
    float bloatFactor_ = 1.0;

    // store basic objects on host 
    std::vector<odb::dbInst*> dbInstStor_;
    std::vector<void*> dbPinStor_;
    std::vector<odb::dbNet*> dbNetStor_;

    // store the pointers to host objects on host
    // The order of instances is free insts, fixed insts, dummy instances
    std::vector<Instance> instStor_;
    std::vector<Pin> pinStor_;  // including ITerms and BTerms
    std::vector<Net> netStor_;

    std::vector<Instance*> placeInsts_;
    std::vector<Net*> nets_;
    std::vector<Instance*> insts_;
    std::vector<Pin*> pins_;

    // statistics
    int numInsts_ = 0;
    int numPlaceInsts_ = 0;
    int numFixedInsts_ = 0;
    int numDummyInsts_ = 0; // for placement blockages

    int64_t placeInstsArea_; // the area of place instances
    int64_t nonPlaceInstsArea_; // the area of fixed instances and dummy instances
    // macroInstsArea_ + stdInstsArea_ = placeInstsArea_;
    // macroInstsArea_ should be separated
    // because of target_density tuning
    int64_t macroInstsArea_;
    int64_t stdCellInstsArea_;

    // placable instances (no filler, no nonplace instances)
    thrust::device_vector<int> dInstDCx_;
    thrust::device_vector<int> dInstDCy_;
    thrust::host_vector<int> hInstDCx_;
    thrust::host_vector<int> hInstDCy_;

    int* dInstDCxPtr_;
    int* dInstDCyPtr_;

    float virtualWeightFactor_ = 0.0;
    float initVirtualWeightFactor_ = 0.0;
    thrust::device_vector<float> dWLGradX_;
    thrust::device_vector<float> dWLGradY_;
  
    float* dWLGradXPtr_;
    float* dWLGradYPtr_;

    bool clusterFlag_ = false; // Create clustered netlist
    bool clusterConstraintFlag_ = false; // Create clustered netlist with constraints
    bool dataflowFlag_ = true;
    bool datapathFlag_ = true;
    
    
    void init();
    //void reset();
    void initClusterNetlist();

    // CUDA related functions   
    void freeCUDAKernel();

    // Dataflow Information
    int largeNetThreshold_ = 50; // threshold for large nets
    int busLimit_ = 2; // threshold for bus nets
    int numHops_ = 4; 
    std::vector<std::map<int, float> > adjMatrix_; // for dataflow constraints
    std::vector<DVertex> dataflowVertices_; // for datapath constraints

    bool debugFlag_ = false;
  
    void createDataFlow();
    void clearPinProperty();
    void clearInstProperty();
    void createSeqGraph(
      std::map<int, odb::dbBTerm*>& ioPinVertex,
      std::map<int, odb::dbInst*>&  instVertex,
      std::vector<Vertex>& seqVertices,
      // create the original netlist
      std::vector<std::vector<int> >& vertices,
      std::vector<std::vector<int> >& sinkHyperedges  // dircted hypergraph
    );
};


// Bin Grid is only defined on the host side
// But each bin is defined on the device side
// The bin can be non-uniform becuase of
// "integer" coordinates
// 
class BinGrid
{
  public:
    // functions
    BinGrid();
    BinGrid(Die* die);
    ~BinGrid();

    // set up functions
    void setPlacerBase(PlacerBase* pb) {
      pb_ = pb;
    }

    void setLogger(utl::Logger* log) {
      log_ = log;
    }

    void setCorePoints(const Die* die);
    void setBinCnt(int binCntX, int binCntY);
    void initBins();
  

    // accessor functions
    const std::vector<Bin*>& bins() const { return bins_; }
    void setTargetDensity(float density) {
      targetDensity_ = density;
    }

    // lx, ly, ux, uy will hold coreArea
    int lx() const { return lx_; }
    int ly() const { return ly_; }
    int ux() const { return ux_; }
    int uy() const { return uy_; }
    int cx() const { return (lx_ + ux_) / 2; }
    int cy() const { return (ly_ + uy_) / 2; }
    int dx() const { return ux_ - lx_; }
    int dy() const { return uy_ - ly_; }

    int binCntX() const { return binCntX_; }
    int binCntY() const { return binCntY_; }
    int numBins() const { return numBins_; }
    int binSizeX() const { return binSizeX_; }
    int binSizeY() const { return binSizeY_; }

  private:
    // variables
    PlacerBase* pb_;
    utl::Logger* log_;
    std::vector<Bin> binStor_;
    std::vector<Bin*> bins_;
 
    int numBins_ = 0;
    int lx_;
    int ly_;
    int ux_;
    int uy_;
    int binCntX_;
    int binCntY_;
    int binSizeX_;
    int binSizeY_;
    float targetDensity_;
    
    bool isSetBinCnt_;  // if binCntX_ and binCntY_ get specified

    std::pair<int, int> getMinMaxIdxX(const Instance* inst) const;
    std::pair<int, int> getMinMaxIdxY(const Instance* inst) const;

    void updateBinsNonPlaceArea();
};



// Our PlacerBase contains the PlacerBase
// and NesterovBase in the original implementation
// Stores instances belonging to a specific power domain
// along with fillers and virtual blockages
// Also stores the bin grid for the power domain
// Used to calculate density gradient
// We try to follow the convention in the original implementation
// But some functions may be different
// Density is local in each region
// Each PlacerBase only consists of placeInsts and fillers
class PlacerBase
{
  public:
    PlacerBase();
    // temp padLeft/Right before OpenDB supporting...
    PlacerBase(NesterovBaseVars nbVars,
               odb::dbDatabase* db,
               std::shared_ptr<PlacerBaseCommon> pbCommon,
               utl::Logger* log,
               odb::dbGroup* group = nullptr);
    ~PlacerBase();

    // Basic accessors
    // insts_ = placeInsts_ + fillerInsts_
    const std::vector<Instance*>& insts() const { return insts_; }
    const std::vector<Instance*>& placeInsts() const { return placeInsts_; }
    // nonPlaceInsts_ = fixedInsts_ + dummyInsts_
    const std::vector<Instance*>& nonPlaceInsts() const { return nonPlaceInsts_; }
    
    // accessor
    odb::dbDatabase* db() const { return db_; }
    utl::Logger* logger() const { return log_; } 
    const Die& die() const { return die_; }


    int numInsts() const { return numInsts_; }
    int numNonPlaceInsts() const { return numNonPlaceInsts_; }
    int numPlaceInsts() const { return numPlaceInsts_; }
    int numFixedInsts() const { return numFixedInsts_; }
    int numDummyInsts() const { return numDummyInsts_; }
    int numFillerInsts() const { return numFillerInsts_; }

    // filler cells / area control
    // will be used in Routability-driven loop
    int fillerDx() const { return fillerDx_;}
    int fillerDy() const { return fillerDy_;}
    int fillerCnt() const { return numFillerInsts_; }
    int64_t fillerCellArea() const {
      return static_cast<int64_t>(fillerDx_) * static_cast<int64_t>(fillerDy_);
    }
    int64_t whiteSpaceArea() const { return whiteSpaceArea_; }
    int64_t movableArea() const { return movableArea_; }
    int64_t totalFillerArea() const { return totalFillerArea_; }
    int64_t placeInstsArea() const { return placeInstsArea_; }
    int64_t nonPlaceInstsArea() const { return nonPlaceInstsArea_; }
    int64_t nesterovInstsArea() const {
      return stdInstsArea_ +
        static_cast<int64_t>(round(macroInstsArea_ * targetDensity_));
    }

    int binCntX() const { return bg_.binCntX(); }
    int binCntY() const { return bg_.binCntY(); }
    int binSizeX() const { return bg_.binSizeX(); }
    int binSizeY() const { return bg_.binSizeY(); }
    int numBins() const { return bg_.numBins(); }
    int coreLx() const { return bg_.lx(); }
    int coreLy() const { return bg_.ly(); }
    int coreUx() const { return bg_.ux(); }
    int coreUy() const { return bg_.uy(); }
    const std::vector<Bin*>& bins() const { return bg_.bins(); }

    float uniformTargetDensity() const { return uniformTargetDensity_; }   
    // density penalty for density force
    float getDensityPenalty() const { return densityPenalty_; }    
    // sum phi in NesterovPlace
    float getSumPhi() const { return sumPhi_; }  
    // initTargetDensity is set by users
    // targetDensity is equal to initTargetDensity and
    // would be changed dynamically in the routability-driven loop
    float initTargetDensity() const { return nbVars_.targetDensity; }
    float targetDensity() const { return targetDensity_; }


    // Functions called by NesterovPlace
    // for nesterovplace initialization
    void initCUDAKernel(); 
    void setNpVars(NesterovPlaceVars npVars) { npVars_ = npVars; }
    void initDensity1();
    // overflow information 
    // overflow is defined on each grid
    // overflow is the area of overlap between instances and the grid 
    // minus grid area
    float getSumOverflow() const { return sumOverflow_; }
    float getSumOverflowUnscaled() const { return sumOverflowUnscaled_; }
    // wire length coefficient is defined as 1 / gamma
    float getBaseWireLengthCoef() const { return baseWireLengthCoef_; }
    void updateInitialPrevSLPCoordi();
    void updateDensityCenterPrevSLP();
    // update the density force on each instance
    // based on current instances location
    void updateDensityForceBin();
    float initDensity2(); 

    // for nestrovPlace:: doNestrovPlace 
    void setIter(int iter) { iter_ = iter; }
    void setMaxPhiCoefChanged(bool maxPhiCoefChanged)
    {
      isMaxPhiCoefChanged_ = maxPhiCoefChanged;
    }
    void resetMinSumOverflow() {
      minSumOverflow_ = 1e30;
      hpwlWithMinSumOverflow_ = 1e30;
    }
    // update next state based on current state
    void nesterovUpdateCoordinates(float coeff);
    bool isDiverged() const { return isDiverged_; }
    bool nesterovUpdateStepLength();
    void nesterovAdjustPhi();
    bool checkConvergence();
		

    // For nesterov:  UpdateNextIter
    // exchange the states:  prev -> current, 
    // current -> next
    // update the parameters
    void updateNextIter(int iter);

    // For nesterov  UpdatePrevGradient,
    // UpdateCurGradient, UpdateNextGradient
    void updatePrevGradient();
    void updateCurGradient();
    void updateNextGradient();
    // The force exerted on each instance consists of 
    // wirelength force and density force
    float getWireLengthGradSum() const { return wireLengthGradSum_; }
    float getDensityGradSum() const { return densityGradSum_; }

    // for debug
    double densityTime() const {
      return densityOp_->densityTime(); 
    }
    
    double fftTime() const {
      return densityOp_->fftTime(); 
    }

    double updateGradientRuntime() const {
      return updateGradientRuntime_;
    }

    double updateGradientRuntime1() const {
      return updateGradientRuntime1_;
    }

    double updateGradientRuntime2() const {
      return updateGradientRuntime2_;
    }

    double updateGradientRuntime3() const {
      return updateGradientRuntime3_;
    }

     double updateGradientRuntime4() const {
      return updateGradientRuntime4_;
    }

    void printInstInfo() const;

    void reset();

  private:
    odb::dbDatabase* db_;
    utl::Logger* log_;
    std::shared_ptr<PlacerBaseCommon> pbCommon_;
    odb::dbGroup* group_;
    DensityOp* densityOp_;
    BinGrid bg_;

    Die die_;
    int siteSizeX_;
    int siteSizeY_;
    NesterovBaseVars nbVars_;
    NesterovPlaceVars npVars_;

    // We need to consider the density scaling
    int fillerDx_; // The width of filler cell
    int fillerDy_; // The height of filler cell  

    int64_t whiteSpaceArea_;  // total area - blockages - fixed instances
    int64_t movableArea_; // the area of instances that be moved
    int64_t totalFillerArea_; // the area occupied by filler

    int64_t placeInstsArea_;
    int64_t nonPlaceInstsArea_; // the area of fixed instances and dummy instances

    // macroInstsArea_ + stdInstsArea_ = placeInstsArea_;
    // macroInstsArea_ should be separated
    // because of target_density tuning
    int64_t macroInstsArea_;
    int64_t stdInstsArea_;

    // dummy instance for this region
    std::vector<Instance> fillerInsts_;
    std::vector<Instance> dummyInsts_;
    std::vector<Instance*> insts_; // placeInsts + fillerInsts
    std::vector<Instance*> placeInsts_; 
    std::vector<Instance*> nonPlaceInsts_; // dummy instances + fixed instances

    int numInsts_;  // place instances + filler instances
    int numNonPlaceInsts_; // fixed instances + dummy instances
    int numPlaceInsts_;
    int numFixedInsts_;
    int numDummyInsts_;
    int numFillerInsts_;

    // variables
    thrust::device_vector<int> dPlaceInstIds_;
    int* dPlaceInstIdsPtr_;

    thrust::device_vector<int> dInstDDx_;
    thrust::device_vector<int> dInstDDy_;

    int* dInstDDxPtr_;
    int* dInstDDyPtr_;

    thrust::device_vector<int> dInstDCx_;
    thrust::device_vector<int> dInstDCy_;

    int* dInstDCxPtr_;
    int* dInstDCyPtr_;

    thrust::device_vector<float> dWireLengthPrecondi_;
    float* dWireLengthPrecondiPtr_;

    thrust::device_vector<float> dDensityPrecondi_;
    float* dDensityPrecondiPtr_;

    // Following variables belong to NesterovBase
    // ***************************************************
    float sumPhi_;
    float targetDensity_; // including fillers
    float uniformTargetDensity_; // without considering fillers

    // opt_phi_cof
    float densityPenalty_;

    // base_wcof
    float baseWireLengthCoef_;

    // phi is described in ePlace paper.
    float sumOverflow_;
    float sumOverflowUnscaled_;

    // half-parameter-wire-length
    int64_t prevHpwl_;
    bool isDiverged_;
    bool isMaxPhiCoefChanged_; 
    float minSumOverflow_ = 1e30; // the divergence detect condition
    float hpwlWithMinSumOverflow_  = 1e30; // the divergence detect condition
    int iter_; // The iteration of Nesterov placement
    bool isConverged_;
    std::string divergeMsg_;
    int divergeCode_;

    // Nesterov loop data for each region
    // SLP is Step Length Prediction
    // SLP is used to predict

    // alpha
    float stepLength_;
    float wireLengthGradSum_;
    float densityGradSum_;

    // density gradient
    thrust::device_vector<float> dDensityGradX_;
    thrust::device_vector<float> dDensityGradY_;
    thrust::device_vector<float> dWireLengthGradX_;
    thrust::device_vector<float> dWireLengthGradY_;

    float* dDensityGradXPtr_;
    float* dDensityGradYPtr_;    
    float* dWireLengthGradXPtr_;
    float* dWireLengthGradYPtr_;

    // SLP related variables
    thrust::device_vector<FloatPoint> dCurSLPCoordi_;
    thrust::device_vector<float> dCurSLPWireLengthGradX_;
    thrust::device_vector<float> dCurSLPWireLengthGradY_;
    thrust::device_vector<float> dCurSLPDensityGradX_;
    thrust::device_vector<float> dCurSLPDensityGradY_;
    thrust::device_vector<FloatPoint> dCurSLPSumGrads_;

    thrust::device_vector<FloatPoint> dPrevSLPCoordi_;
    thrust::device_vector<float> dPrevSLPWireLengthGradX_;
    thrust::device_vector<float> dPrevSLPWireLengthGradY_;
    thrust::device_vector<float> dPrevSLPDensityGradX_;
    thrust::device_vector<float> dPrevSLPDensityGradY_;
    thrust::device_vector<FloatPoint> dPrevSLPSumGrads_;

    thrust::device_vector<FloatPoint> dNextSLPCoordi_;
    thrust::device_vector<float> dNextSLPWireLengthGradX_;
    thrust::device_vector<float> dNextSLPWireLengthGradY_;
    thrust::device_vector<float> dNextSLPDensityGradX_;
    thrust::device_vector<float> dNextSLPDensityGradY_;
    thrust::device_vector<FloatPoint> dNextSLPSumGrads_;

    thrust::device_vector<FloatPoint> dCurCoordi_;
    thrust::device_vector<FloatPoint> dNextCoordi_;


    // For test
    thrust::device_vector<float> dSumGradsX_;
    thrust::device_vector<float> dSumGradsY_;

    float* dSumGradsXPtr_;
    float* dSumGradsYPtr_;

    FloatPoint* dCurSLPCoordiPtr_;
    float* dCurSLPWireLengthGradXPtr_;
    float* dCurSLPWireLengthGradYPtr_;
    float* dCurSLPDensityGradXPtr_;
    float* dCurSLPDensityGradYPtr_;
    FloatPoint* dCurSLPSumGradsPtr_;


    FloatPoint* dPrevSLPCoordiPtr_;
    float* dPrevSLPWireLengthGradXPtr_;
    float* dPrevSLPWireLengthGradYPtr_;
    float* dPrevSLPDensityGradXPtr_;
    float* dPrevSLPDensityGradYPtr_;
    FloatPoint* dPrevSLPSumGradsPtr_;

    FloatPoint* dNextSLPCoordiPtr_;
    float* dNextSLPWireLengthGradXPtr_;
    float* dNextSLPWireLengthGradYPtr_;
    float* dNextSLPDensityGradXPtr_;
    float* dNextSLPDensityGradYPtr_;    
    FloatPoint* dNextSLPSumGradsPtr_;

    FloatPoint* dCurCoordiPtr_;
    FloatPoint* dNextCoordiPtr_;

    // device memory management
    void freeCUDAKernel();

    float getStepLength(
      const FloatPoint* prevSLPCoordi,
      const FloatPoint* prevSLPSumGrads,
      const FloatPoint* curSLPCoordi,
      const FloatPoint* curSLPSumGrads) const;

    void updateGradients(
      float* wireLengthGradientsX,
      float* wireLengthGradientsY,
      float* densityGradientsX,
      float* densityGradientsY,
      FloatPoint* sumGrads); 

    void updateGCellDensityCenterLocation(const FloatPoint* coordis);
    
    void getWireLengthGradientWA(
      float* wireLengthGradientsX,
      float* wireLengthGradientsY);
    
    void getDensityGradient(
      float* densityGradientsX,
      float* densityGradientsY);

    float getPhiCoef(float scaledDiffHpwl) const;

    // update the instance location
    void updateDensityCenterCur();
    void updateDensityCenterCurSLP();
    void updateDensityCenterNextSLP();

    float overflowArea() const {
      return densityOp_->sumOverflow();
    }

    // TODO: do we need overflowAreaUnscaled?
    float overflowAreaUnscaled() const {
      return overflowArea();
      //return densityOp_->sumOverflowUnscaled();
    }

    void init();
    void initFillerGCells();
    //void reset();
    void initInstsForUnusableSites();
    void updateDensitySize();

    bool debugFlag_ = false;

     // for runtime debug
    double updateGradientRuntime_ = 0.0;
    double updateGradientRuntime1_ = 0.0;
    double updateGradientRuntime2_ = 0.0;
    double updateGradientRuntime3_ = 0.0;
    double updateGradientRuntime4_ = 0.0;
};





}



