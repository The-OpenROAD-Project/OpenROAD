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

#include <odb/db.h>
#include "util.h" 
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cuda_runtime.h>
#include <thrust/copy.h>
#include <thrust/device_vector.h>
#include <thrust/fill.h>
#include <thrust/host_vector.h>
#include <thrust/sequence.h>
#include <thrust/for_each.h>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>

namespace odb {
class dbDatabase;

class dbInst;
class dbITerm;
class dbBTerm;
class dbNet;

class dbPlacementStatus;
class dbSigType;

class dbBox;

class Rect;
class Point;

}  // namespace odb

namespace utl {
class Logger;
}

namespace gpl2 {

class Instance;
class Net;
class Pin;
class PlacerBaseCommon;
class PlacerBase;

// The Instance is the union of
// Instance and Gcell
// We have four types of instances
// (a) macro
// (b) std cell
// (c) dummy : a dummy instance is a virtual
//             instance to fill in unusable sites
// (d) filler 
enum class InstanceType
{
  MACRO,
  STDCELL,
  DUMMY,
  FILLER
};


class Instance {
  public:
    // constructor
    Instance();

    // macros or std cells
    Instance(odb::dbInst* inst, 
             int padLeft,
             int padRight,
             int siteHeight,
             int rowLimit,
             utl::Logger* logger);

    // dummy or filler instances
    Instance(int cx, 
             int cy, 
             int width, 
             int height, 
             bool isDummy = false);

    ~Instance();

    void dbSetPlaced();
    void dbSetLocation();  
     // Some partical blocked sited need to be fully blocked 
    void snapOutward(const odb::Point& origin, int step_x, int step_y);
    void dbSetPlacementStatus(odb::dbPlacementStatus ps); 

    float wireLengthPreconditioner() const { return numPins(); }
    float densityPreconditioner() const {
      return static_cast<float>(dx())
              * static_cast<float>(dy());
    }

    odb::dbInst* dbInst() const { return inst_; }

    int instId() const { return instId_; }
    void setInstId(int instId) { instId_ = instId; }
    void setHaloWidth(int haloWidth) { 
      haloWidth_ = haloWidth; 
      dx_ = dx_ + 2 * haloWidth;
      dy_ = dy_ + 2 * haloWidth;
      dDx_ = dx_;
      dDy_ = dy_;
    }        

    int lx() const { return cx_ - dx_ / 2; }
    int ly() const { return cy_ - dy_ / 2; }
    int ux() const { return cx_ + dx_ / 2; }
    int uy() const { return cy_ + dy_ / 2; }
    int dx() const { return dx_; }
    int dy() const { return dy_; }
    int cx() const { return cx_; }
    int cy() const { return cy_; }

    // Instance type
    // Dummy is virtual instance to fill in
    // unusable sites.  
    bool isDummy() const {  return type_ == InstanceType::DUMMY; }
    // Filler instance (see RePlAce work)
    bool isFiller() const { return type_ == InstanceType::FILLER; }
    // a cell that is a macro.
    // Here the macro is the not necessary ``block'' type.
    // Some large standard cells are also identified as macros
    bool isMacro() const { return type_ == InstanceType::MACRO; }
    bool isStdInstance() const { return type_ == InstanceType::STDCELL; }
    bool isInstance() const { return isMacro() || isStdInstance(); }

    void setDummy() { type_ = InstanceType::DUMMY; }
    void setFiller() { type_ = InstanceType::FILLER; }
    void setMacro() { type_ = InstanceType::MACRO; }
    void setStdInstance() { type_ = InstanceType::STDCELL; }
      
    int dLx() const { return cx_ - dDx_ / 2; }
    int dLy() const { return cy_ - dDy_ / 2; }
    int dUx() const { return cx_ + dDx_ / 2; }
    int dUy() const { return cy_ + dDy_ / 2; }
    int dDx() const { return dDx_; }
    int dDy() const { return dDy_; }

    float densityScale() const { return densityScale_; }
    int64_t area() const { return 
      static_cast<int64_t>(dx_) * static_cast<int64_t>(dy_); 
    }

    void setFixed() { isFixed_ = true; }
    void unsetFixed() { isFixed_ = false; }
    bool isFixed() const { return isFixed_; }
    bool isPlaceInstance() const { 
      return (isInstance() && !isFixed());
    }  

    void setCenterLocation(int cx, int cy);
    void setDensitySize(int dDx, int dDy, float densityScale);

    void addPin(Pin* pin) { pins_.push_back(pin); }
    const std::vector<Pin*>& pins() const { return pins_; }
    int numPins() const { return pins_.size();  }

  private:
    odb::dbInst* inst_ = nullptr;
    // We need to arrange instances in some order
    int instId_ = -1;
    
    bool isFixed_ = false; // if the instance is fixed

    int cx_;
    int cy_;

    // real size
    int dx_;
    int dy_;

    // density size
    int dDx_;
    int dDy_;
    float densityScale_;

    int haloWidth_ = 0;

    // instance type
    InstanceType type_ = InstanceType::STDCELL;  
    std::vector<Pin*> pins_;
};




class Net {
  public:
    Net();
    Net(int netId);
    Net(odb::dbNet* net);
    ~Net();

    int lx() const { return lx_; }
    int ly() const { return ly_; }
    int ux() const { return ux_; }
    int uy() const { return uy_; }
    int netId() const { return netId_; }

    int numPins() const {  return pins_.size(); }
    int hpwl() const {  return (ux_ - lx_) + (uy_ - ly_); }
    float weight() const {  return weight_; }
    float virtualWeight() const { return virtualWeight_; }
    bool isDontCare() const { return isDontCare_; }
    const std::vector<Pin*>& pins() const { return pins_; }
    
    void setDontCare() { isDontCare_ = true; }
    void setWeight(float weight) { weight_ = weight; }
    void setVirtualWeight(float virtualWeight) { virtualWeight_ = virtualWeight; }
    void addPin(Pin* pin) { pins_.push_back(pin); }

    odb::dbNet* dbNet() const { return net_; }
    odb::dbSigType getSigType() const {
      return net_->getSigType();
    }

    void updateBBox();

  private:
    odb::dbNet* net_ = nullptr;
    std::vector<Pin*> pins_;

    int netId_ = -1;
    int lx_;
    int ly_;
    int ux_;
    int uy_;

    bool isDontCare_ = false;
    float virtualWeight_ = 0.0;
    float weight_ = 1.0;
};

class Pin {
  public:
    Pin();
    Pin(int pinId);
    Pin(odb::dbITerm* iTerm, utl::Logger* logger);
    Pin(odb::dbBTerm* bTerm, utl::Logger* logger);
    ~Pin();

    odb::dbITerm* dbITerm() const;
    odb::dbBTerm* dbBTerm() const;
    std::string name() const;

    void setInstance(Instance* inst) { inst_ = inst; }
    void setNet(Net* net) { net_ = net; }

    Instance* instance() const { return inst_; }
    Net* net() const { return net_; }

    int instId() const { 
      if (inst_ != nullptr) {
        return inst_->instId();
      } else {
        return -1;
      } 
    }
    
    bool isPlaceInstConnected() const {
      if (inst_ != nullptr) {
        return inst_->isPlaceInstance();
      } else {
        return false;
      }
    }

    int netId() const { 
      if (net_ != nullptr) {
        return net_->netId();
      } else {
        return -1;
      }
    }  
      
    int pinId() const { return pinId_; }

    int cx() const { return cx_; }
    int cy() const { return cy_; }

    void updateLocation(int instCx, int instCy);
    void updateLocation(Instance* inst);

    // offset coordinates inside instance.
    // origin point is center point of instance.
    // (e.g. (DX/2,DY/2) )
    // This will increase efficiency for bloating
    int offsetCx() const { return offsetCx_; }
    int offsetCy() const { return offsetCy_; }

    bool isITerm() const { return iTermField_; }
    bool isBTerm() const { return bTermField_; }

    // Identify if the pin is on the bounding box
    // of the net 
    bool isMinPinX() const { return minPinXField_; }
    bool isMaxPinX() const { return maxPinXField_; }
    bool isMinPinY() const { return minPinYField_; }
    bool isMaxPinY() const { return maxPinYField_; }
 
    void setMinPinX() { minPinXField_ = true; }
    void setMinPinY() { minPinYField_ = true; }
    void setMaxPinX() { maxPinXField_ = true; }
    void setMaxPinY() { maxPinYField_ = true; }
  
    void unsetMinPinX() { minPinXField_ = false; }
    void unsetMinPinY() { minPinYField_ = false; }
    void unsetMaxPinX() { maxPinXField_ = false; }
    void unsetMaxPinY() { maxPinYField_ = false; }


  private:
    void* pin_ = nullptr; // dbITerm or dbBTerm
    int pinId_ = -1;  // the unique id of the pin

    Instance* inst_;
    Net* net_;
    int cx_;
    int cy_;

    int offsetCx_;
    int offsetCy_;

    bool iTermField_ = true;
    bool bTermField_ = false;

    bool minPinXField_ = true;
    bool minPinYField_ = true;
    bool maxPinXField_ = true;
    bool maxPinYField_ = true;

    // Utilities function
    void updateCoordi(odb::dbITerm* iTerm, utl::Logger* logger);
    void updateCoordi(odb::dbBTerm* bTerm, utl::Logger* logger);
};





// Density force is specified on the GpuBin
// Besides, we need to apply density scaling
// so we can handle the fixed instances and dark nodes
// dark nodes are used to handle rectilinear placement shapes
// We also need to enable bin-wise density screening
// So we need to include the targetDensity
// x: bin x index, y: bin y index
class Bin
{
  public:
    Bin();
    Bin(int x, int y, int lx, int ly, int ux, int uy, float targetDensity);

    int x() const { return x_; }
    int y() const { return y_; }

    int lx() const { return lx_; }
    int ly() const { return ly_; }
    int ux() const { return ux_; }
    int uy() const { return uy_; }
    int cx() const { return (lx_ + ux_) / 2; }
    int cy() const { return (ly_ + uy_) / 2; }
    int dx() const { return ux_ - lx_; }
    int dy() const { return uy_ - ly_; }

    float density() const { return density_; }
    float targetDensity() const { return targetDensity_; }
    void setDensity(float density) { density_ = density; }
    
    // density force
    float electroPhi() const { return electroPhi_; }
    void setElectroPhi(float electroPhi) { electroPhi_ = electroPhi; }

    float electroForceX() const { return electroForceX_; }
    float electroForceY() const { return electroForceY_; }
    void setElectroForce(float electroForceX, float electroForceY) {
      electroForceX_ = electroForceX;
      electroForceY_ = electroForceY;
    }

    int64_t area() const { return static_cast<int64_t>(dx()) * static_cast<int64_t>(dy()); }
    int64_t nonPlaceArea() const { return nonPlaceArea_; }
    int64_t instPlacedArea() const { return instPlacedArea_; }
    int64_t fillerArea() const { return fillerArea_; }

    void setNonPlaceArea(int64_t area) { nonPlaceArea_ = area; }
    void setInstPlacedArea(int64_t area) { instPlacedArea_ = area; }
    void setFillerArea(int64_t area) { fillerArea_ = area; }

    void addNonPlaceArea(int64_t area) { nonPlaceArea_ += area; }
    void addInstPlacedArea(int64_t area) { instPlacedArea_ += area; }
    void addFillerArea(int64_t area) { fillerArea_ += area; }

  private:
    // member variable
    // index
    int x_;  // col_id
    int y_;  // row_id

    // coordinate
    int lx_;
    int ly_;
    int ux_;
    int uy_;
  
    // density scaling is used 
    int64_t nonPlaceArea_; // Area occupied by the dark nodes and fixed instances
    int64_t instPlacedArea_; // Area occupied by the placeable (movable) instances
    int64_t fillerArea_; // Area occupied by fillers

    float density_; // real density
    float targetDensity_;  // will enable bin-wise density screening
    float electroPhi_;
    float electroForceX_;
    float electroForceY_;
};

class Die
{
  public:
  Die();  
  Die(const odb::Rect& dieBox, const odb::Rect& coreRect);
  void setDieBox(const odb::Rect& dieBox);
  void setCoreBox(const odb::Rect& coreBox);


  int dieLx() const { return dieLx_; }
  int dieLy() const { return dieLy_; }
  int dieUx() const { return dieUx_; }
  int dieUy() const { return dieUy_; }
  int dieCx() const { return (dieLx_ + dieUx_) / 2; }
  int dieCy() const { return (dieLy_ + dieUy_) / 2; }
  int dieDx() const { return dieUx_ - dieLx_; }
  int dieDy() const { return dieUy_ - dieLy_; }
  int64_t dieArea() const { 
    return static_cast<int64_t>(dieDx()) * static_cast<int64_t>(dieDy()); 
  }

  int coreLx() const { return coreLx_; }
  int coreLy() const { return coreLy_; }
  int coreUx() const { return coreUx_; }
  int coreUy() const { return coreUy_; }
  int coreCx() const { return (coreLx_ + coreUx_) / 2; }
  int coreCy() const { return (coreLy_ + coreUy_) / 2; }
  int coreDx() const { return coreUx_ - coreLx_; }
  int coreDy() const { return coreUy_ - coreLy_; }
  int64_t coreArea() const { 
    return static_cast<int64_t>(coreDx()) * static_cast<int64_t>(coreDy()); 
  }

  private:
    int dieLx_;
    int dieLy_;
    int dieUx_;
    int dieUy_;
    int coreLx_;
    int coreLy_;
    int coreUx_;
    int coreUy_;
};



struct IntPoint
{
  int x;
  int y;
  __host__ __device__ IntPoint() : x(0), y(0) {}
  __host__ __device__ IntPoint(int _x, int _y) : x(_x), y(_y) {}
};


struct FloatPoint
{
  float x;
  float y;
  __host__ __device__ FloatPoint() : x(0), y(0) {}
  __host__ __device__ FloatPoint(float _x, float _y) : x(_x), y(_y) {}
};



struct Int64Point
{
  int64_t x;
  int64_t y;
  __host__ __device__ Int64Point() : x(0), y(0) {}
  __host__ __device__ Int64Point(int64_t _x, int64_t _y) : x(_x), y(_y) {}
};


struct IntRect
{ 
  __host__ __device__
  IntRect() {
    lx = 0;
    ly = 0;
    ux = 0;
    uy = 0;
  }
  
  __host__ __device__
  IntRect(int _lx, int _ly, int _ux, int _uy)
    : lx(_lx),
      ly(_ly),
      ux(_ux),
      uy(_uy)
    {  }

  int lx = 0;
  int ly = 0;
  int ux = 0;
  int uy = 0;
};



struct Vertex {
  int src;
  bool isPin;
  std::map<int, int> sinks;

  Vertex(int _src, bool _isPin) : src(_src), isPin(_isPin) {}

  void addSink(int sink, int dist) {
    if (sinks.find(sink) != sinks.end()) {
      sinks[sink] = std::max(dist, sinks[sink]);
    } else {
      sinks[sink] = dist;
    }
  }
};


// dataflow vertex
struct DVertex {
  int dVertexId;
  int instId = -1;
  odb::dbInst* inst;
  odb::dbBTerm* bTerm;
  std::vector<odb::dbInst*> FFs;
  std::map<int, float> sinks; // dVertexId, distance

  DVertex() {
    dVertexId = -1;
    inst = nullptr;
    bTerm = nullptr;
  }

  DVertex(int _dVertexId) {
    dVertexId = _dVertexId;
    inst = nullptr;
    bTerm = nullptr;
  }

  DVertex(int _dVertexId, odb::dbInst* _inst) {
    dVertexId = _dVertexId;
    inst = _inst;
    bTerm = nullptr;
  }

  DVertex(int _dVertexId, odb::dbBTerm* _bTerm) {
    dVertexId = _dVertexId;
    inst = nullptr;
    bTerm = _bTerm;
  }

  void addSink(int sink, float weight) {
    if (sinks.find(sink) != sinks.end()) {
      sinks[sink] += weight;
    } else {
      sinks[sink] = weight;
    }
  }

  void addInst(odb::dbInst* inst) {
    FFs.push_back(inst);
  }

  bool isBTerm() const {
    return bTerm != nullptr;
  }

  odb::dbBTerm* getBTerm() const {
    return bTerm;
  }

  bool isMacro() const {
    return inst != nullptr;
  }

  odb::dbInst* getMacro() const {
    return inst;
  }

  std::vector<odb::dbInst*> getFFs() const {
    return FFs;
  }

  void printVirtualNets() const {
    if (sinks.size() < 1) {
      return;
    }
    
    std::cout << std::endl;
    std::cout << "dVertexId: " << dVertexId <<  "  "
              << "numFFs = " << FFs.size() << std::endl;
    for (auto& sink : sinks) {
      std::cout << "sink: " << sink.first << " weight: " << sink.second << "\n";
    }
  }
};


// Utilities function
bool isCoreAreaOverlap(Die& die,  const Instance* inst);
int64_t getOverlapWithCoreArea(Die& die, const Instance* inst);

// Check if the odb::dbInst is fixed
bool isFixedOdbInst(odb::dbInst* inst);

// utility functions for snaping the inst into site
int snapDown(int value, int origin, int step);
int snapUp(int value, int origin, int step);
std::string intToStringWithPrecision(int value, int precision);
std::string floatToStringWithPrecision(float value, int precision);

// utility functions
// To handle the case of mixed designs, 
// we need to do modification when calculating the overlap between
// GpuBin and Macro (local smoothness over discrete macros)
// The parameters passed to normal distribution cumulative probability density function (CDF)
struct biNormalParameters
{
  int meanX;
  int meanY;
  int sigmaX;
  int sigmaY;
  int lx;
  int ly;
  int ux;
  int uy;
};

// A function that does 2D integration to the density function of a
// bivariate normal distribution with 0 correlation.
// Essentially, the function being integrated is the product
// of 2 1D probability density functions (for x and y). The means and standard
// deviation of the probablity density functions are parametarized. In this
// function, I am using the closed-form solution of the integration. The limits
// of integration are lx->ux and ly->uy For reference: the equation that is
// being integrated is:
//      (1/(2*pi*sigmaX*sigmaY))*e^(-(y-meanY)^2/(2*sigmaY*sigmaY))*e^(-(x-meanX)^2/(2*sigmaX*sigmaX))
int calculateBiVariateNormalCDF(biNormalParameters i);

// numeric operators

__host__ __device__
int fastModulo(const int input, const int ceil);



}
