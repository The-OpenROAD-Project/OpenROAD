// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <set>
#include <tuple>
#include <vector>

#include "db/obj/frBlockObject.h"
#include "db/obj/frMarker.h"
#include "db/tech/frTechObject.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "global.h"
#include "utl/Logger.h"

namespace drt {
class drNet;
class drPatchWire;
class FlexDRWorker;
class gcNet;
class gcPin;

class FlexGCWorker
{
 public:
  // constructors
  FlexGCWorker(frTechObject* techIn,
               utl::Logger* logger,
               RouterConfiguration* router_cfg,
               FlexDRWorker* drWorkerIn = nullptr);
  ~FlexGCWorker();
  // setters
  void setExtBox(const odb::Rect& in);
  void setDrcBox(const odb::Rect& in);
  bool setTargetNet(frBlockObject* in);
  bool setTargetNet(drNet* in);
  gcNet* getTargetNet();
  void resetTargetNet();
  void addTargetObj(frBlockObject* in);
  void setTargetObjs(const std::set<frBlockObject*>& targetObjs);
  void setIgnoreDB();
  void setIgnoreMinArea();
  void setIgnoreLongSideEOL();
  void setIgnoreCornerSpacing();
  void setEnableSurgicalFix(bool in);
  void addPAObj(frConnFig* obj, frBlockObject* owner);
  // getters
  std::vector<std::unique_ptr<gcNet>>& getNets();
  gcNet* getNet(frNet* net);
  frDesign* getDesign() const;
  const std::vector<std::unique_ptr<frMarker>>& getMarkers() const;
  const std::vector<std::unique_ptr<drPatchWire>>& getPWires() const;
  // others
  void init(const frDesign* design);
  int main();
  void clearPWires();
  // initialization from FlexPA, initPA0 --> addPAObj --> initPA1
  void initPA0(const frDesign* design);
  void initPA1();
  void updateDRNet(drNet* net);
  // used in rp_prep
  void checkMinStep(gcPin* pin);
  void updateGCWorker();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
struct MarkerId
{
  odb::Rect box;
  frLayerNum lNum;
  frConstraint* con;
  std::set<frBlockObject*> srcs;
  bool operator<(const MarkerId& rhs) const
  {
    return std::tie(box, lNum, con, srcs)
           < std::tie(rhs.box, rhs.lNum, rhs.con, rhs.srcs);
  }
};
}  // namespace drt
