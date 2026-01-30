// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <climits>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "boost/unordered/unordered_flat_map.hpp"
#include "odb/geom.h"

namespace odb {
class dbDatabase;

class dbBTerm;
class dbGroup;
class dbITerm;
class dbInst;
class dbNet;
class dbObject;

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
class PlacerBaseCommon;
struct PlaceOptions;

class Instance
{
 public:
  Instance();
  Instance(odb::dbInst* inst, PlacerBaseCommon* pbc, utl::Logger* logger);
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

  void copyDbLocation(PlacerBaseCommon* pbc);
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
  int64_t getArea() const;

  void setExtId(int extId);
  int getExtId() const { return extId_; }

  void addPin(Pin* pin);
  const std::vector<Pin*>& getPins() const { return pins_; }
  void snapOutward(const odb::Point& origin, int step_x, int step_y);
  int64_t extendSizeByScale(double scale, utl::Logger* logger);

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

  odb::dbITerm* getDbITerm() const;
  odb::dbBTerm* getDbBTerm() const;

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

  int getOffsetCx() const;
  int getOffsetCy() const;

  void updateLocation(const Instance* inst);

  void setInstance(Instance* inst);
  void setNet(Net* net);

  bool isPlaceInstConnected() const;

  Instance* getInstance() const { return inst_; }
  Net* getNet() const { return net_; }
  std::string getName() const;

  void updateCoordi(odb::dbITerm* iTerm);

 private:
  odb::dbObject* term_ = nullptr;
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
  int64_t getHpwl() const;

  void updateBox(bool skipIoMode = false);

  const std::vector<Pin*>& getPins() const { return pins_; }

  odb::dbNet* getDbNet() const { return net_; }
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

struct PlacerBaseVars
{
  PlacerBaseVars(const PlaceOptions& options);

  const int padLeft;
  const int padRight;
  const bool skipIoMode;
  const bool disablePinDensityAdjust;
};

// Class includes everything from PlacerBase that is not region specific
class PlacerBaseCommon
{
 public:
  // temp padLeft/Right before OpenDB supporting...
  PlacerBaseCommon(odb::dbDatabase* db,
                   PlacerBaseVars pbVars,
                   utl::Logger* log);
  ~PlacerBaseCommon();

  const std::vector<Instance*>& placeInsts() const { return placeInsts_; }
  const std::vector<Instance*>& getInsts() const { return insts_; }
  const std::vector<Pin*>& getPins() const { return pins_; }
  const std::vector<Net*>& getNets() const { return nets_; }

  Die& getDie() { return die_; }

  // Pb : PlacerBase
  Instance* dbToPb(odb::dbInst* inst) const;
  Pin* dbToPb(odb::dbITerm* term) const;
  Pin* dbToPb(odb::dbBTerm* term) const;
  Net* dbToPb(odb::dbNet* net) const;

  int siteSizeX() const { return siteSizeX_; }
  int siteSizeY() const { return siteSizeY_; }

  int getPadLeft() const { return pbVars_.padLeft; }
  int getPadRight() const { return pbVars_.padRight; }
  bool isSkipIoMode() const { return pbVars_.skipIoMode; }

  int64_t getHpwl() const;
  void printInfo() const;

  int64_t getMacroInstsArea() const { return macroInstsArea_; }

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

  boost::unordered::unordered_flat_map<odb::dbInst*, Instance*> instMap_;
  // The key is a dbITerm or a dbBTerm
  boost::unordered::unordered_flat_map<odb::dbObject*, Pin*> pinMap_;
  boost::unordered::unordered_flat_map<odb::dbNet*, Net*> netMap_;

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

  const std::vector<Instance*>& getInsts() const { return pb_insts_; }

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

  Die& getDie() { return die_; }
  int64_t getRegionArea() const { return region_area_; }
  const odb::Rect& getRegionBBox() const { return region_bbox_; }

  int getSiteSizeX() const { return siteSizeX_; }
  int getSiteSizeY() const { return siteSizeY_; }

  int64_t getHpwl() const;
  void printInfo() const;

  int64_t placeInstsArea() const { return placeInstsArea_; }
  int64_t nonPlaceInstsArea() const { return nonPlaceInstsArea_; }
  int64_t macroInstsArea() const;
  int64_t stdInstsArea() const { return stdInstsArea_; }

  odb::dbDatabase* db() const { return db_; }
  odb::dbGroup* getGroup() const { return group_; }

  void unlockAll();

 private:
  odb::dbDatabase* db_ = nullptr;
  utl::Logger* log_ = nullptr;

  Die die_;
  int64_t region_area_;
  odb::Rect region_bbox_;

  std::vector<Instance> instStor_;

  std::vector<Instance*> pb_insts_;

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
