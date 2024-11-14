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

#include <climits>
#include <memory>
#include <unordered_map>
#include <vector>

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

namespace gpl {

class Pin;
class Net;
class GCell;

class Instance
{
 public:
  Instance();
  Instance(odb::dbInst* inst,
           int padLeft,
           int padRight,
           int site_height,
           utl::Logger* logger);
  Instance(int lx, int ly, int ux, int uy);  // dummy instance
  ~Instance();

  odb::dbInst* dbInst() const { return inst_; }

  // a cell that no need to be moved.
  bool isFixed() const;

  // a instance that need to be moved.
  bool isInstance() const;

  bool isPlaceInstance() const;

  bool isMacro() const;

  // A placeable instance may be fixed during part of incremental placement.
  // It remains in the set of placeable objects though so as to simplify
  // the unlocking process.
  bool isLocked() const;
  void lock();
  void unlock();

  // Dummy is virtual instance to fill in
  // unusable sites.  It will have inst_ as nullptr
  bool isDummy() const;

  void setLocation(int x, int y);
  void setCenterLocation(int x, int y);

  void dbSetPlaced();
  void dbSetPlacementStatus(const odb::dbPlacementStatus& ps);
  void dbSetLocation();
  void dbSetLocation(int x, int y);
  void dbSetCenterLocation(int x, int y);

  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;
  int cx() const;
  int cy() const;
  int dx() const;
  int dy() const;
  int64_t area() const;

  void setExtId(int extId);
  int extId() const { return extId_; }

  void addPin(Pin* pin);
  const std::vector<Pin*>& pins() const { return pins_; }
  void snapOutward(const odb::Point& origin, int step_x, int step_y);

 private:
  odb::dbInst* inst_ = nullptr;
  std::vector<Pin*> pins_;
  int lx_ = 0;
  int ly_ = 0;
  int ux_ = 0;
  int uy_ = 0;
  int extId_ = INT_MIN;
  bool is_macro_ = false;
  bool is_locked_ = false;
};

class Pin
{
 public:
  Pin();
  Pin(odb::dbITerm* iTerm);
  Pin(odb::dbBTerm* bTerm, utl::Logger* logger);
  ~Pin();

  odb::dbITerm* dbITerm() const;
  odb::dbBTerm* dbBTerm() const;

  bool isITerm() const;
  bool isBTerm() const;
  bool isMinPinX() const;
  bool isMaxPinX() const;
  bool isMinPinY() const;
  bool isMaxPinY() const;

  void setITerm();
  void setBTerm();
  void setMinPinX();
  void setMinPinY();
  void setMaxPinX();
  void setMaxPinY();
  void unsetMinPinX();
  void unsetMinPinY();
  void unsetMaxPinX();
  void unsetMaxPinY();

  int cx() const;
  int cy() const;

  int offsetCx() const;
  int offsetCy() const;

  void updateLocation(const Instance* inst);

  void setInstance(Instance* inst);
  void setNet(Net* net);

  bool isPlaceInstConnected() const;

  Instance* instance() const { return inst_; }
  Net* net() const { return net_; }
  std::string name() const;

  void updateCoordi(odb::dbITerm* iTerm);

 private:
  void* term_ = nullptr;
  Instance* inst_ = nullptr;
  Net* net_ = nullptr;

  // pin center coordinate is enough
  // Pins' placed location.
  int cx_ = 0;
  int cy_ = 0;

  // offset coordinates inside instance.
  // origin point is center point of instance.
  // (e.g. (DX/2,DY/2) )
  // This will increase efficiency for bloating
  int offsetCx_ = 0;
  int offsetCy_ = 0;

  unsigned char iTermField_ : 1;
  unsigned char bTermField_ : 1;
  unsigned char minPinXField_ : 1;
  unsigned char minPinYField_ : 1;
  unsigned char maxPinXField_ : 1;
  unsigned char maxPinYField_ : 1;

  void updateCoordi(odb::dbBTerm* bTerm, utl::Logger* logger);
};

class Net
{
 public:
  Net();
  Net(odb::dbNet* net, bool skipIoMode);
  ~Net();

  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;
  int cx() const;
  int cy() const;

  // HPWL: half-parameter-wire-length
  int64_t hpwl() const;

  void updateBox(bool skipIoMode = false);

  const std::vector<Pin*>& pins() const { return pins_; }

  odb::dbNet* dbNet() const { return net_; }
  odb::dbSigType getSigType() const;

  void addPin(Pin* pin);

 private:
  odb::dbNet* net_ = nullptr;
  std::vector<Pin*> pins_;
  int lx_ = 0;
  int ly_ = 0;
  int ux_ = 0;
  int uy_ = 0;
};

class Die
{
 public:
  Die();
  Die(const odb::Rect& dieRect, const odb::Rect& coreRect);
  ~Die();

  void setDieBox(const odb::Rect& dieRect);
  void setCoreBox(const odb::Rect& coreRect);

  int dieLx() const { return dieLx_; }
  int dieLy() const { return dieLy_; }
  int dieUx() const { return dieUx_; }
  int dieUy() const { return dieUy_; }

  int coreLx() const { return coreLx_; }
  int coreLy() const { return coreLy_; }
  int coreUx() const { return coreUx_; }
  int coreUy() const { return coreUy_; }

  int dieCx() const;
  int dieCy() const;
  int dieDx() const;
  int dieDy() const;
  int coreCx() const;
  int coreCy() const;
  int coreDx() const;
  int coreDy() const;

  int64_t dieArea() const;
  int64_t coreArea() const;

 private:
  int dieLx_ = 0;
  int dieLy_ = 0;
  int dieUx_ = 0;
  int dieUy_ = 0;
  int coreLx_ = 0;
  int coreLy_ = 0;
  int coreUx_ = 0;
  int coreUy_ = 0;
};

class PlacerBaseVars
{
 public:
  int padLeft;
  int padRight;
  bool skipIoMode;

  PlacerBaseVars();
  void reset();
};

// Class includes everything from PlacerBase that is not region specific
class PlacerBaseCommon
{
 public:
  PlacerBaseCommon();
  // temp padLeft/Right before OpenDB supporting...
  PlacerBaseCommon(odb::dbDatabase* db,
                   PlacerBaseVars pbVars,
                   utl::Logger* log);
  ~PlacerBaseCommon();

  const std::vector<Instance*>& placeInsts() const { return placeInsts_; }
  const std::vector<Instance*>& insts() const { return insts_; }
  const std::vector<Pin*>& pins() const { return pins_; }
  const std::vector<Net*>& nets() const { return nets_; }

  Die& die() { return die_; }

  // Pb : PlacerBase
  Instance* dbToPb(odb::dbInst* inst) const;
  Pin* dbToPb(odb::dbITerm* term) const;
  Pin* dbToPb(odb::dbBTerm* term) const;
  Net* dbToPb(odb::dbNet* net) const;

  int siteSizeX() const { return siteSizeX_; }
  int siteSizeY() const { return siteSizeY_; }

  int padLeft() const { return pbVars_.padLeft; }
  int padRight() const { return pbVars_.padRight; }
  bool skipIoMode() const { return pbVars_.skipIoMode; }

  int64_t hpwl() const;
  void printInfo() const;

  int64_t macroInstsArea() const { return macroInstsArea_; }

  odb::dbDatabase* db() const { return db_; }

  void unlockAll();

 private:
  odb::dbDatabase* db_ = nullptr;
  utl::Logger* log_ = nullptr;

  PlacerBaseVars pbVars_;

  Die die_;

  std::vector<Instance> instStor_;
  std::vector<Pin> pinStor_;
  std::vector<Net> netStor_;

  std::vector<Instance*> insts_;
  std::vector<Pin*> pins_;
  std::vector<Net*> nets_;

  std::vector<Instance*> placeInsts_;

  std::unordered_map<odb::dbInst*, Instance*> instMap_;
  std::unordered_map<void*, Pin*> pinMap_;
  std::unordered_map<odb::dbNet*, Net*> netMap_;

  int siteSizeX_ = 0;
  int siteSizeY_ = 0;

  int64_t macroInstsArea_ = 0;

  void init();
  void reset();
};

class PlacerBase
{
 public:
  PlacerBase();
  // temp padLeft/Right before OpenDB supporting...
  PlacerBase(odb::dbDatabase* db,
             std::shared_ptr<PlacerBaseCommon> pbCommon,
             utl::Logger* log,
             odb::dbGroup* group = nullptr);
  ~PlacerBase();

  const std::vector<Instance*>& insts() const { return insts_; }

  //
  // placeInsts : a real instance that need to be placed
  // fixedInsts : a real instance that is fixed (e.g. macros, tapcells)
  // dummyInsts : a fake instance that is for unusable site handling
  //
  // nonPlaceInsts : fixedInsts + dummyInsts to enable fast-iterate on Bin-init
  //
  const std::vector<Instance*>& placeInsts() const { return placeInsts_; }
  const std::vector<Instance*>& fixedInsts() const { return fixedInsts_; }
  const std::vector<Instance*>& dummyInsts() const { return dummyInsts_; }
  const std::vector<Instance*>& nonPlaceInsts() const { return nonPlaceInsts_; }

  Die& die() { return die_; }

  int siteSizeX() const { return siteSizeX_; }
  int siteSizeY() const { return siteSizeY_; }

  int64_t hpwl() const;
  void printInfo() const;

  int64_t placeInstsArea() const { return placeInstsArea_; }
  int64_t nonPlaceInstsArea() const { return nonPlaceInstsArea_; }
  int64_t macroInstsArea() const;
  int64_t stdInstsArea() const { return stdInstsArea_; }

  odb::dbDatabase* db() const { return db_; }
  odb::dbGroup* group() const { return group_; }

  void unlockAll();

 private:
  odb::dbDatabase* db_ = nullptr;
  utl::Logger* log_ = nullptr;

  Die die_;

  std::vector<Instance> instStor_;

  std::vector<Instance*> insts_;

  std::vector<Instance*> placeInsts_;
  std::vector<Instance*> fixedInsts_;
  std::vector<Instance*> dummyInsts_;
  std::vector<Instance*> nonPlaceInsts_;

  int siteSizeX_ = 0;
  int siteSizeY_ = 9;

  int64_t placeInstsArea_ = 0;
  int64_t nonPlaceInstsArea_ = 0;

  // macroInstsArea_ + stdInstsArea_ = placeInstsArea_;
  // macroInstsArea_ should be separated
  // because of target_density tuning
  int64_t macroInstsArea_ = 0;
  int64_t stdInstsArea_ = 0;

  std::shared_ptr<PlacerBaseCommon> pbCommon_;
  odb::dbGroup* group_ = nullptr;

  void init();
  void initInstsForUnusableSites();

  void reset();
};

}  // namespace gpl
