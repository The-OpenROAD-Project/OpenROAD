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
#include <vector>

#include "nesterovBase.h"
#include "point.h"
#include "odb/dbBlockCallBackObj.h"

namespace utl {
class Logger;
}

namespace odb {
class dbInst;
}

namespace gpl {

class PlacerBase;
class PlacerBaseCommon;
class Instance;
class RouteBase;
class TimingBase;
class Graphics;

class NesterovPlace
{
 public:
  NesterovPlace();
  NesterovPlace(const NesterovPlaceVars& npVars,
                const std::shared_ptr<PlacerBaseCommon>& pbc,
                const std::shared_ptr<NesterovBaseCommon>& nbc,
                std::vector<std::shared_ptr<PlacerBase>>& pbVec,
                std::vector<std::shared_ptr<NesterovBase>>& nbVec,
                std::shared_ptr<RouteBase> rb,
                std::shared_ptr<TimingBase> tb,
                utl::Logger* log);
  ~NesterovPlace();

  // return iteration count
  int doNesterovPlace(int start_iter = 0);

  void updateWireLengthCoef(float overflow);

  void updateNextIter(int iter);

  void updateDb();

  float getWireLengthCoefX() const { return wireLengthCoefX_; }
  float getWireLengthCoefY() const { return wireLengthCoefY_; }

  void setTargetOverflow(float overflow) { npVars_.targetOverflow = overflow; }
  void setMaxIters(int limit) { npVars_.maxNesterovIter = limit; }

  void updatePrevGradient(const std::shared_ptr<NesterovBase>& nb);
  void updateCurGradient(const std::shared_ptr<NesterovBase>& nb);
  void updateNextGradient(const std::shared_ptr<NesterovBase>& nb);

  void resizeGCell(odb::dbInst*);
  void moveGCell(odb::dbInst*);

  void createGCell(odb::dbInst*);  
  void createGNet(odb::dbNet*);
  void createITerm(odb::dbITerm*);

  void destroyGCell(odb::dbInst*);
  void destroyGNet(odb::dbNet*);
  void destroyITerm(odb::dbITerm*);

 private:
  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::vector<std::shared_ptr<PlacerBase>> pbVec_;
  std::vector<std::shared_ptr<NesterovBase>> nbVec_;
  utl::Logger* log_ = nullptr;
  std::shared_ptr<RouteBase> rb_;
  std::shared_ptr<TimingBase> tb_;
  NesterovPlaceVars npVars_;
  std::unique_ptr<Graphics> graphics_;

  float total_sum_overflow_ = 0;
  float total_sum_overflow_unscaled_ = 0;
  float average_overflow_ = 0;
  float average_overflow_unscaled_ = 0;

  // densityPenalty stor
  std::vector<float> densityPenaltyStor_;

  // base_wcof
  float baseWireLengthCoef_ = 0;

  // wlen_cof
  float wireLengthCoefX_ = 0;
  float wireLengthCoefY_ = 0;

  // half-parameter-wire-length
  int64_t prevHpwl_ = 0;

  bool isDiverged_ = false;
  bool isRoutabilityNeed_ = true;

  std::string divergeMsg_;
  int divergeCode_ = 0;

  int recursionCntWlCoef_ = 0;
  int recursionCntInitSLPCoef_ = 0;

  void cutFillerCoordinates();

  void init();
  void reset();

  std::unique_ptr<nesterovDbCbk> db_cbk_;
};

class nesterovDbCbk : public odb::dbBlockCallBackObj
{
 public:
  nesterovDbCbk(NesterovPlace* nesterov_place_);

  //buffer insertion
  virtual void inDbInstCreate(odb::dbInst*);
  virtual void inDbInstCreate(odb::dbInst*, odb::dbRegion*);
  virtual void inDbInstDestroy(odb::dbInst*);

  //buffer removal
  virtual void inDbITermCreate(odb::dbITerm*) ;
  virtual void inDbITermDestroy(odb::dbITerm*) ;
//  virtual void inDbITermPreDisconnect(odb::dbITerm*) ;
  virtual void inDbITermPostDisconnect(odb::dbITerm*, odb::dbNet*) ;
//  virtual void inDbITermPreConnect(odb::dbITerm*, odb::dbNet*) ;
  virtual void inDbITermPostConnect(odb::dbITerm*) ;


// virtual void inDbPreMoveInst(odb::dbInst*) override;
virtual void inDbPostMoveInst(odb::dbInst*) override;
virtual void inDbNetCreate(odb::dbNet*) override;
virtual void inDbNetDestroy(odb::dbNet*) override;
virtual void inDbNetPreMerge(odb::dbNet*, odb::dbNet*) override;
virtual void inDbBTermCreate(odb::dbBTerm*) override;
virtual void inDbBTermDestroy(odb::dbBTerm*) override;
virtual void inDbBTermPreConnect(odb::dbBTerm*, odb::dbNet*) override;
virtual void inDbBTermPostConnect(odb::dbBTerm*) override;
virtual void inDbBTermPreDisconnect(odb::dbBTerm*) override;
virtual void inDbBTermPostDisConnect(odb::dbBTerm*, odb::dbNet*) override;
virtual void inDbBTermSetIoType(odb::dbBTerm*, const odb::dbIoType&) override;
virtual void inDbBPinCreate(odb::dbBPin*) override;
virtual void inDbBPinDestroy(odb::dbBPin*) override;
virtual void inDbBlockageCreate(odb::dbBlockage*) override;
virtual void inDbObstructionCreate(odb::dbObstruction*) override;
virtual void inDbObstructionDestroy(odb::dbObstruction*) override;
virtual void inDbRegionCreate(odb::dbRegion*) override;
virtual void inDbRegionAddBox(odb::dbRegion*, odb::dbBox*) override;
virtual void inDbRegionDestroy(odb::dbRegion*) override;
virtual void inDbRowCreate(odb::dbRow*) override;
virtual void inDbRowDestroy(odb::dbRow*) override;

  //cell resizing
//  virtual void inDbInstSwapMasterBefore(odb::dbInst*, odb::dbMaster*);
  virtual void inDbInstSwapMasterAfter(odb::dbInst*);

  void printCallCounts();
  void resetCallCounts();
 private:
  NesterovPlace* nesterov_place_;
  utl::Logger log_;

    int inDbInstSwapMasterAfterCount = 0;
    int inDbInstCreateCount = 0;
    int inDbInstDestroyCount = 0;
    int inDbITermCreateCount = 0;
    int inDbITermDestroyCount = 0;
    int inDbITermPreDisconnectCount = 0;
    int inDbITermPostDisconnectCount = 0;
    int inDbITermPreConnectCount = 0;
    int inDbITermPostConnectCount = 0;
    int inDbPreMoveInstCount = 0;
    int inDbPostMoveInstCount = 0;
    int inDbNetCreateCount = 0;
    int inDbNetDestroyCount = 0;
    int inDbNetPreMergeCount = 0;
    int inDbBTermCreateCount = 0;
    int inDbBTermDestroyCount = 0;
    int inDbBTermPreConnectCount = 0;
    int inDbBTermPostConnectCount = 0;
    int inDbBTermPreDisconnectCount = 0;
    int inDbBTermPostDisConnectCount = 0;
    int inDbBTermSetIoTypeCount = 0;
    int inDbBPinCreateCount = 0;
    int inDbBPinDestroyCount = 0;
    int inDbBlockageCreateCount = 0;
    int inDbObstructionCreateCount = 0;
    int inDbObstructionDestroyCount = 0;
    int inDbRegionCreateCount = 0;
    int inDbRegionAddBoxCount = 0;
    int inDbRegionDestroyCount = 0;
    int inDbRowCreateCount = 0;
    int inDbRowDestroyCount = 0;
};

}  // namespace gpl
