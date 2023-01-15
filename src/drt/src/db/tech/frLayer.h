/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <set>

#include "db/infra/frSegStyle.h"
#include "db/obj/frVia.h"
#include "db/tech/frConstraint.h"
#include "frBaseTypes.h"
namespace odb {
class dbTechLayer;
}
namespace fr {
namespace io {
class Parser;
}

class frLayer
{
 public:
  friend class io::Parser;
  // constructor
  frLayer()
      : db_layer_(nullptr),
        fakeCut(false),
        fakeMasterslice(false),
        layerNum(0),
        width(0),
        wrongDirWidth(0),
        minWidth(0),
        defaultViaDef(nullptr),
        hasMinStepViol(false),
        unidirectional(false),
        minSpc(nullptr),
        spacingSamenet(nullptr),
        spacingInfluence(nullptr),
        eols(),
        cutConstraints(),
        cutSpacingSamenetConstraints(),
        interLayerCutSpacingConstraints(),
        interLayerCutSpacingSamenetConstraints(),
        interLayerCutSpacingConstraintsMap(),
        interLayerCutSpacingSamenetConstraintsMap(),
        lef58CutSpacingConstraints(),
        lef58CutSpacingSamenetConstraints(),
        recheckConstraint(nullptr),
        shortConstraint(nullptr),
        offGridConstraint(nullptr),
        minEnclosedAreaConstraints(),
        nonSufficientMetalConstraint(nullptr),
        areaConstraint(nullptr),
        minStepConstraint(nullptr),
        lef58MinStepConstraints(),
        minWidthConstraint(nullptr),
        minimumcutConstraints(),
        lef58MinimumcutConstraints(),
        lef58RectOnlyConstraint(nullptr),
        lef58RightWayOnGridOnlyConstraint(nullptr),
        lef58CutSpacingTableSameNetMetalConstraint(nullptr),
        lef58CutSpacingTableDiffNetConstraint(nullptr),
        lef58SameNetInterCutSpacingTableConstraint(nullptr),
        lef58SameMetalInterCutSpacingTableConstraint(nullptr),
        lef58DefaultInterCutSpacingTableConstraint(nullptr),
        drEolCon()
  {
  }
  frLayer(frLayerNum layerNumIn,
          const frString& nameIn)  // remove name from signature
      : db_layer_(nullptr),
        fakeCut(false),
        fakeMasterslice(false),
        layerNum(layerNumIn),
        width(0),
        wrongDirWidth(0),
        minWidth(-1),
        defaultViaDef(nullptr),
        minSpc(nullptr),
        spacingSamenet(nullptr),
        spacingInfluence(nullptr),
        eols(),
        cutConstraints(),
        cutSpacingSamenetConstraints(),
        interLayerCutSpacingConstraints(),
        interLayerCutSpacingSamenetConstraints(),
        interLayerCutSpacingConstraintsMap(),
        interLayerCutSpacingSamenetConstraintsMap(),
        lef58CutSpacingConstraints(),
        lef58CutSpacingSamenetConstraints(),
        recheckConstraint(nullptr),
        shortConstraint(nullptr),
        offGridConstraint(nullptr),
        nonSufficientMetalConstraint(nullptr),
        areaConstraint(nullptr),
        minStepConstraint(nullptr),
        lef58MinStepConstraints(),
        minWidthConstraint(nullptr),
        minimumcutConstraints(),
        lef58MinimumcutConstraints(),
        lef58RectOnlyConstraint(nullptr),
        lef58RightWayOnGridOnlyConstraint(nullptr)
  {
  }
  // setters
  void setDbLayer(odb::dbTechLayer* dbLayer) { db_layer_ = dbLayer; }
  void setFakeCut(bool fakeCutIn) { fakeCut = fakeCutIn; }
  void setFakeMasterslice(bool fakeMastersliceIn)
  {
    fakeMasterslice = fakeMastersliceIn;
  }
  void setLayerNum(frLayerNum layerNumIn) { layerNum = layerNumIn; }
  void setWidth(frUInt4 widthIn) { width = widthIn; }
  void setMinWidth(frUInt4 minWidthIn) { minWidth = minWidthIn; }
  void setDefaultViaDef(frViaDef* in) { defaultViaDef = in; }
  void addConstraint(frConstraint* consIn) { constraints.push_back(consIn); }
  void addViaDef(frViaDef* viaDefIn) { viaDefs.insert(viaDefIn); }
  void setHasVia2ViaMinStepViol(bool in) { hasMinStepViol = in; }
  void setUnidirectional(bool in) { unidirectional = in; }
  // getters
  odb::dbTechLayer* getDbLayer() const { return db_layer_; }
  bool isFakeCut() const { return fakeCut; }
  bool isFakeMasterslice() const { return fakeMasterslice; }
  frUInt4 getNumMasks() const
  {
    return (fakeCut || fakeMasterslice) ? 1 : db_layer_->getNumMasks();
  }
  frLayerNum getLayerNum() const { return layerNum; }
  void getName(frString& nameIn) const
  {
    nameIn = (fakeCut)           ? "FR_VIA"
             : (fakeMasterslice) ? "FR_MASTERSLICE"
                                 : db_layer_->getName();
  }
  frString getName() const
  {
    return (fakeCut)           ? "Fr_VIA"
           : (fakeMasterslice) ? "FR_MASTERSLICE"
                               : db_layer_->getName();
  }
  frUInt4 getPitch() const
  {
    return (fakeCut || fakeMasterslice) ? 0 : db_layer_->getPitch();
  }
  frUInt4 getWidth() const { return width; }
  frUInt4 getWrongDirWidth() const { return db_layer_->getWrongWayWidth(); }
  frUInt4 getMinWidth() const { return minWidth; }
  dbTechLayerDir getDir() const
  {
    if (fakeCut || fakeMasterslice)
      return dbTechLayerDir::NONE;
    return db_layer_->getDirection();
  }
  bool isVertical()
  {
    return (fakeCut || fakeMasterslice)
               ? false
               : db_layer_->getDirection() == dbTechLayerDir::VERTICAL;
  }
  bool isHorizontal()
  {
    return (fakeCut || fakeMasterslice)
               ? false
               : db_layer_->getDirection() == dbTechLayerDir::HORIZONTAL;
  }
  bool isUnidirectional() const
  {
    // We don't handle coloring so any double/triple patterned
    // layer is treated as unidirectional.
    // RectOnly could allow for a purely wrong-way rect but
    // we ignore that rare case and treat it as unidirectional.
    return getNumMasks() > 1 || getLef58RectOnlyConstraint() || unidirectional;
  }
  frSegStyle getDefaultSegStyle() const
  {
    frSegStyle style;
    style.setWidth(width);
    style.setBeginStyle(frcExtendEndStyle, width / 2);
    style.setEndStyle(frcExtendEndStyle, width / 2);
    return style;
  }
  frViaDef* getDefaultViaDef() const { return defaultViaDef; }
  bool hasVia2ViaMinStepViol() { return hasMinStepViol; }
  std::set<frViaDef*> getViaDefs() const { return viaDefs; }
  dbTechLayerType getType() const
  {
    if (fakeCut)
      return dbTechLayerType::CUT;

    if (fakeMasterslice)
      return dbTechLayerType::MASTERSLICE;

    return db_layer_->getType();
  }

  // cut class (new)
  void addCutClass(frLef58CutClass* in)
  {
    name2CutClassIdxMap[in->getName()] = cutClasses.size();
    cutClasses.push_back(in);
  }

  int getCutClassIdx(frCoord width, frCoord length) const
  {
    int cutClassIdx = -1;
    for (int i = 0; i < (int) cutClasses.size(); i++) {
      auto cutClass = cutClasses[i];
      if (cutClass->getViaWidth() == width
          && cutClass->getViaLength() == length) {
        cutClassIdx = i;
      }
    }
    return cutClassIdx;
  }

  int getCutClassIdx(std::string name)
  {
    if (name2CutClassIdxMap.find(name) != name2CutClassIdxMap.end()) {
      return name2CutClassIdxMap[name];
    } else {
      return -1;
    }
  }

  frLef58CutClass* getCutClass(frCoord width, frCoord length) const
  {
    int cutClassIdx = getCutClassIdx(width, length);
    if (cutClassIdx != -1) {
      return cutClasses[cutClassIdx];
    } else {
      return nullptr;
    }
  }

  frLef58CutClass* getCutClass(int cutClassIdx)
  {
    return cutClasses[cutClassIdx];
  }

  void printCutClasses()
  {
    for (auto& [name, idx] : name2CutClassIdxMap) {
      std::cout << "cutClass name: " << name << ", idx: " << idx << "\n";
    }
  }

  // cut spacing table

  void setLef58SameNetCutSpcTblConstraint(frLef58CutSpacingTableConstraint* con)
  {
    lef58CutSpacingTableSameNetMetalConstraint = con;
  }

  bool hasLef58SameNetCutSpcTblConstraint() const
  {
    return lef58CutSpacingTableSameNetMetalConstraint != nullptr
           && lef58CutSpacingTableSameNetMetalConstraint->getODBRule()
                  ->isSameNet();
  }

  frLef58CutSpacingTableConstraint* getLef58SameNetCutSpcTblConstraint() const
  {
    if (hasLef58SameNetCutSpcTblConstraint())
      return lef58CutSpacingTableSameNetMetalConstraint;
    return nullptr;
  }

  void setLef58SameMetalCutSpcTblConstraint(
      frLef58CutSpacingTableConstraint* con)
  {
    lef58CutSpacingTableSameNetMetalConstraint = con;
  }

  bool hasLef58SameMetalCutSpcTblConstraint() const
  {
    return lef58CutSpacingTableSameNetMetalConstraint != nullptr
           && lef58CutSpacingTableSameNetMetalConstraint->getODBRule()
                  ->isSameMetal();
  }

  frLef58CutSpacingTableConstraint* getLef58SameMetalCutSpcTblConstraint() const
  {
    if (hasLef58SameMetalCutSpcTblConstraint())
      return lef58CutSpacingTableSameNetMetalConstraint;
    return nullptr;
  }

  void setLef58DiffNetCutSpcTblConstraint(frLef58CutSpacingTableConstraint* con)
  {
    lef58CutSpacingTableDiffNetConstraint = con;
  }

  bool hasLef58DiffNetCutSpcTblConstraint() const
  {
    return lef58CutSpacingTableDiffNetConstraint != nullptr;
  }

  frLef58CutSpacingTableConstraint* getLef58DiffNetCutSpcTblConstraint() const
  {
    return lef58CutSpacingTableDiffNetConstraint;
  }

  // spacing end of line
  bool hasLef58SpacingEndOfLineConstraints() const
  {
    return !lef58SpacingEndOfLineConstraints.empty();
  }
  const frCollection<frLef58SpacingEndOfLineConstraint*>&
  getLef58SpacingEndOfLineConstraints() const
  {
    return lef58SpacingEndOfLineConstraints;
  }

  void addLef58SpacingEndOfLineConstraint(
      frLef58SpacingEndOfLineConstraint* constraintIn)
  {
    lef58SpacingEndOfLineConstraints.push_back(constraintIn);
  }
  // new functions
  bool hasMinSpacing() const { return (minSpc); }
  frConstraint* getMinSpacing() const { return minSpc; }
  frCoord getMinSpacingValue(frCoord width1,
                             frCoord width2,
                             frCoord prl,
                             bool use_min_spacing);
  void setMinSpacing(frConstraint* in, bool verbose = false)
  {
    if (verbose && minSpc != nullptr) {
      std::cout << "Warning: override minspacing rule, ";
      if (minSpc->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
        std::cout << "original type is SPACING, ";
      } else if (minSpc->typeId()
                 == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
        std::cout << "original type is SPACINGTABLE PARALLELRUNLENGTH, ";
      } else if (minSpc->typeId()
                 == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
        std::cout << "original type is SPACINGTABLE TWOWIDTHS, ";
      } else {
        std::cout << "original type is UNKNWON, ";
      }
      if (in->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
        std::cout << "new type is SPACING";
      } else if (in->typeId()
                 == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
        std::cout << "new type is SPACINGTABLE PARALLELRUNLENGTH";
      } else if (in->typeId()
                 == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
        std::cout << "new type is SPACINGTABLE TWOWIDTHS";
      } else {
        std::cout << "new type is UNKNWON";
      }
      std::cout << std::endl;
    }
    minSpc = in;
  }
  bool hasSpacingSamenet() const { return (spacingSamenet); }
  frSpacingSamenetConstraint* getSpacingSamenet() const
  {
    return spacingSamenet;
  }
  void setSpacingSamenet(frSpacingSamenetConstraint* in)
  {
    spacingSamenet = in;
  }
  bool hasSpacingTableInfluence() const { return (spacingInfluence); }
  frSpacingTableInfluenceConstraint* getSpacingTableInfluence() const
  {
    return spacingInfluence;
  }
  void setSpacingTableInfluence(frSpacingTableInfluenceConstraint* in)
  {
    spacingInfluence = in;
  }
  bool hasEolSpacing() const { return (eols.empty() ? false : true); }
  void addEolSpacing(frSpacingEndOfLineConstraint* in) { eols.push_back(in); }
  const std::vector<frSpacingEndOfLineConstraint*>& getEolSpacing() const
  {
    return eols;
  }
  // lef58
  void addLef58CutSpacingConstraint(frLef58CutSpacingConstraint* in)
  {
    if (in->isSameNet()) {
      lef58CutSpacingSamenetConstraints.push_back(in);
    } else {
      lef58CutSpacingConstraints.push_back(in);
    }
  }
  const std::vector<frLef58CutSpacingConstraint*>&
  getLef58CutSpacingConstraints(bool samenet = false) const
  {
    if (samenet) {
      return lef58CutSpacingSamenetConstraints;
    } else {
      return lef58CutSpacingConstraints;
    }
  }
  void addLef58MinStepConstraint(frLef58MinStepConstraint* in)
  {
    lef58MinStepConstraints.push_back(in);
  }
  const std::vector<frLef58MinStepConstraint*>& getLef58MinStepConstraints()
      const
  {
    return lef58MinStepConstraints;
  }

  void addLef58EolExtConstraint(frLef58EolExtensionConstraint* in)
  {
    lef58EolExtConstraints.push_back(in);
  }
  const std::vector<frLef58EolExtensionConstraint*>& getLef58EolExtConstraints()
      const
  {
    return lef58EolExtConstraints;
  }

  void addCutSpacingConstraint(frCutSpacingConstraint* in)
  {
    if (!(in->isLayer())) {
      if (in->hasSameNet()) {
        cutSpacingSamenetConstraints.push_back(in);
      } else {
        cutConstraints.push_back(in);
      }
    } else {
      if (!(in->hasSameNet())) {
        if (interLayerCutSpacingConstraintsMap.find(in->getSecondLayerName())
            != interLayerCutSpacingConstraintsMap.end()) {
          std::cout << "Error: Up to one diff-net inter-layer cut spacing rule "
                       "can be specified for one layer pair. Rule ignored\n";
        } else {
          interLayerCutSpacingConstraintsMap[in->getSecondLayerName()] = in;
        }
      } else {
        if (interLayerCutSpacingSamenetConstraintsMap.find(
                in->getSecondLayerName())
            != interLayerCutSpacingSamenetConstraintsMap.end()) {
          std::cout << "Error: Up to one same-net inter-layer cut spacing rule "
                       "can be specified for one layer pair. Rule ignored\n";
        } else {
          interLayerCutSpacingSamenetConstraintsMap[in->getSecondLayerName()]
              = in;
        }
      }
    }
  }
  frCutSpacingConstraint* getInterLayerCutSpacing(frLayerNum layerNum,
                                                  bool samenet = false) const
  {
    if (!samenet) {
      return interLayerCutSpacingConstraints[layerNum];
    } else {
      return interLayerCutSpacingSamenetConstraints[layerNum];
    }
  }
  const std::vector<frCutSpacingConstraint*>& getInterLayerCutSpacingConstraint(
      bool samenet = false) const
  {
    if (!samenet) {
      return interLayerCutSpacingConstraints;
    } else {
      return interLayerCutSpacingSamenetConstraints;
    }
  }
  // do not use this after initialization
  std::vector<frCutSpacingConstraint*>& getInterLayerCutSpacingConstraintRef(
      bool samenet = false)
  {
    if (!samenet) {
      return interLayerCutSpacingConstraints;
    } else {
      return interLayerCutSpacingSamenetConstraints;
    }
  }
  const std::map<std::string, frCutSpacingConstraint*>&
  getInterLayerCutSpacingConstraintMap(bool samenet = false) const
  {
    if (!samenet) {
      return interLayerCutSpacingConstraintsMap;
    } else {
      return interLayerCutSpacingSamenetConstraintsMap;
    }
  }
  const std::vector<frCutSpacingConstraint*>& getCutConstraint(bool samenet
                                                               = false) const
  {
    if (samenet) {
      return cutSpacingSamenetConstraints;
    } else {
      return cutConstraints;
    }
  }
  const std::vector<frCutSpacingConstraint*>& getCutSpacing(bool samenet
                                                            = false) const
  {
    if (samenet) {
      return cutSpacingSamenetConstraints;
    } else {
      return cutConstraints;
    }
  }
  frCoord getCutSpacingValue() const
  {
    frCoord s = 0;
    for (auto con : getCutSpacing()) {
      s = max(s, con->getCutSpacing());
    }
    return s;
  }
  bool hasCutSpacing(bool samenet = false) const
  {
    if (samenet) {
      return (!cutSpacingSamenetConstraints.empty());
    } else {
      return (!cutConstraints.empty());
    }
  }
  bool hasInterLayerCutSpacing(frLayerNum layerNum, bool samenet = false) const
  {
    if (samenet) {
      return (!(interLayerCutSpacingSamenetConstraints[layerNum] == nullptr));
    } else {
      return (!(interLayerCutSpacingConstraints[layerNum] == nullptr));
    }
  }
  void setRecheckConstraint(frRecheckConstraint* in) { recheckConstraint = in; }
  frRecheckConstraint* getRecheckConstraint() { return recheckConstraint; }
  void setShortConstraint(frShortConstraint* in) { shortConstraint = in; }
  frShortConstraint* getShortConstraint() { return shortConstraint; }
  void setOffGridConstraint(frOffGridConstraint* in) { offGridConstraint = in; }
  frOffGridConstraint* getOffGridConstraint() { return offGridConstraint; }
  void setNonSufficientMetalConstraint(frNonSufficientMetalConstraint* in)
  {
    nonSufficientMetalConstraint = in;
  }
  frNonSufficientMetalConstraint* getNonSufficientMetalConstraint()
  {
    return nonSufficientMetalConstraint;
  }
  void setAreaConstraint(frAreaConstraint* in) { areaConstraint = in; }
  frAreaConstraint* getAreaConstraint() { return areaConstraint; }
  void setMinStepConstraint(frMinStepConstraint* in) { minStepConstraint = in; }
  frMinStepConstraint* getMinStepConstraint() { return minStepConstraint; }
  void setMinWidthConstraint(frMinWidthConstraint* in)
  {
    minWidthConstraint = in;
  }
  frMinWidthConstraint* getMinWidthConstraint() { return minWidthConstraint; }

  void addMinimumcutConstraint(frMinimumcutConstraint* in)
  {
    minimumcutConstraints.push_back(in);
  }
  const std::vector<frMinimumcutConstraint*>& getMinimumcutConstraints() const
  {
    return minimumcutConstraints;
  }
  bool hasMinimumcut() const { return (!minimumcutConstraints.empty()); }

  void addLef58MinimumcutConstraint(frLef58MinimumcutConstraint* in)
  {
    lef58MinimumcutConstraints.push_back(in);
  }
  const std::vector<frLef58MinimumcutConstraint*>&
  getLef58MinimumcutConstraints() const
  {
    return lef58MinimumcutConstraints;
  }
  bool hasLef58Minimumcut() const
  {
    return (!lef58MinimumcutConstraints.empty());
  }

  void addMinEnclosedAreaConstraint(frMinEnclosedAreaConstraint* in)
  {
    minEnclosedAreaConstraints.push_back(in);
  }

  const std::vector<frMinEnclosedAreaConstraint*>&
  getMinEnclosedAreaConstraints() const
  {
    return minEnclosedAreaConstraints;
  }

  bool hasMinEnclosedArea() const
  {
    return (!minEnclosedAreaConstraints.empty());
  }

  void setLef58RectOnlyConstraint(frLef58RectOnlyConstraint* in)
  {
    lef58RectOnlyConstraint = in;
  }
  frLef58RectOnlyConstraint* getLef58RectOnlyConstraint() const
  {
    return lef58RectOnlyConstraint;
  }

  void setLef58RightWayOnGridOnlyConstraint(
      frLef58RightWayOnGridOnlyConstraint* in)
  {
    lef58RightWayOnGridOnlyConstraint = in;
  }
  frLef58RightWayOnGridOnlyConstraint* getLef58RightWayOnGridOnlyConstraint()
  {
    return lef58RightWayOnGridOnlyConstraint;
  }

  void addLef58CornerSpacingConstraint(frLef58CornerSpacingConstraint* in)
  {
    lef58CornerSpacingConstraints.push_back(in);
  }

  const std::vector<frLef58CornerSpacingConstraint*>&
  getLef58CornerSpacingConstraints() const
  {
    return lef58CornerSpacingConstraints;
  }

  bool hasLef58CornerSpacingConstraint() const
  {
    return (!lef58CornerSpacingConstraints.empty());
  }

  void addLef58EolKeepOutConstraint(frLef58EolKeepOutConstraint* in)
  {
    lef58EolKeepOutConstraints.push_back(in);
  }

  const std::vector<frLef58EolKeepOutConstraint*>&
  getLef58EolKeepOutConstraints() const
  {
    return lef58EolKeepOutConstraints;
  }

  bool hasLef58EolKeepOutConstraint() const
  {
    return (!lef58EolKeepOutConstraints.empty());
  }

  void addMetalWidthViaConstraint(frMetalWidthViaConstraint* in)
  {
    metalWidthViaConstraints.push_back(in);
  }

  const std::vector<frMetalWidthViaConstraint*>& getMetalWidthViaConstraints()
      const
  {
    return metalWidthViaConstraints;
  }
  void addLef58AreaConstraint(frLef58AreaConstraint* in)
  {
    lef58AreaConstraints.push_back(in);
  }

  const std::vector<frLef58AreaConstraint*>& getLef58AreaConstraints() const
  {
    return lef58AreaConstraints;
  }

  bool hasLef58AreaConstraint() const
  {
    return (!lef58AreaConstraints.empty());
  }

  void setLef58SameNetInterCutSpcTblConstraint(
      frLef58CutSpacingTableConstraint* con)
  {
    lef58SameNetInterCutSpacingTableConstraint = con;
  }

  bool hasLef58SameNetInterCutSpcTblConstraint() const
  {
    return lef58SameNetInterCutSpacingTableConstraint != nullptr;
  }

  frLef58CutSpacingTableConstraint* getLef58SameNetInterCutSpcTblConstraint()
      const
  {
    return lef58SameNetInterCutSpacingTableConstraint;
  }

  void setLef58SameMetalInterCutSpcTblConstraint(
      frLef58CutSpacingTableConstraint* con)
  {
    lef58SameMetalInterCutSpacingTableConstraint = con;
  }

  bool hasLef58SameMetalInterCutSpcTblConstraint() const
  {
    return lef58SameMetalInterCutSpacingTableConstraint != nullptr;
  }

  frLef58CutSpacingTableConstraint* getLef58SameMetalInterCutSpcTblConstraint()
      const
  {
    return lef58SameMetalInterCutSpacingTableConstraint;
  }

  void setLef58DefaultInterCutSpcTblConstraint(
      frLef58CutSpacingTableConstraint* con)
  {
    lef58DefaultInterCutSpacingTableConstraint = con;
  }

  bool hasLef58DefaultInterCutSpcTblConstraint() const
  {
    return lef58DefaultInterCutSpacingTableConstraint != nullptr;
  }

  frLef58CutSpacingTableConstraint* getLef58DefaultInterCutSpcTblConstraint()
      const
  {
    return lef58DefaultInterCutSpacingTableConstraint;
  }

  void setDrEolSpacingConstraint(frCoord width, frCoord space, frCoord within)
  {
    drEolCon.eolWidth = width;
    drEolCon.eolSpace = space;
    drEolCon.eolWithin = within;
  }

  const drEolSpacingConstraint& getDrEolSpacingConstraint() const
  {
    return drEolCon;
  }

  void printAllConstraints(utl::Logger* logger);

 protected:
  odb::dbTechLayer* db_layer_;
  bool fakeCut;
  bool fakeMasterslice;
  frLayerNum layerNum;
  frUInt4 width;
  frUInt4 wrongDirWidth;
  frUInt4 minWidth;
  frViaDef* defaultViaDef;
  bool hasMinStepViol;
  bool unidirectional;
  std::set<frViaDef*> viaDefs;
  std::vector<frLef58CutClass*> cutClasses;
  std::map<std::string, int> name2CutClassIdxMap;
  frCollection<frConstraint*> constraints;

  frCollection<frLef58SpacingEndOfLineConstraint*>
      lef58SpacingEndOfLineConstraints;

  frConstraint* minSpc;
  frSpacingSamenetConstraint* spacingSamenet;
  frSpacingTableInfluenceConstraint* spacingInfluence;
  std::vector<frSpacingEndOfLineConstraint*> eols;
  std::vector<frCutSpacingConstraint*> cutConstraints;
  std::vector<frCutSpacingConstraint*> cutSpacingSamenetConstraints;
  // limited one per layer, vector.size() == layers.size()
  std::vector<frCutSpacingConstraint*> interLayerCutSpacingConstraints;
  // limited one per layer and only effective when diff-net version exists,
  // vector.size() == layers.size()
  std::vector<frCutSpacingConstraint*> interLayerCutSpacingSamenetConstraints;
  // temp storage for inter-layer cut spacing before postProcess
  std::map<std::string, frCutSpacingConstraint*>
      interLayerCutSpacingConstraintsMap;
  std::map<std::string, frCutSpacingConstraint*>
      interLayerCutSpacingSamenetConstraintsMap;

  std::vector<frLef58CutSpacingConstraint*> lef58CutSpacingConstraints;
  std::vector<frLef58CutSpacingConstraint*> lef58CutSpacingSamenetConstraints;

  frRecheckConstraint* recheckConstraint;
  frShortConstraint* shortConstraint;
  frOffGridConstraint* offGridConstraint;
  std::vector<frMinEnclosedAreaConstraint*> minEnclosedAreaConstraints;
  frNonSufficientMetalConstraint* nonSufficientMetalConstraint;
  frAreaConstraint* areaConstraint;
  frMinStepConstraint* minStepConstraint;
  std::vector<frLef58MinStepConstraint*> lef58MinStepConstraints;
  std::vector<frLef58EolExtensionConstraint*> lef58EolExtConstraints;
  frMinWidthConstraint* minWidthConstraint;
  std::vector<frMinimumcutConstraint*> minimumcutConstraints;
  std::vector<frLef58MinimumcutConstraint*> lef58MinimumcutConstraints;
  frLef58RectOnlyConstraint* lef58RectOnlyConstraint;
  frLef58RightWayOnGridOnlyConstraint* lef58RightWayOnGridOnlyConstraint;

  frLef58CutSpacingTableConstraint* lef58CutSpacingTableSameNetMetalConstraint;
  frLef58CutSpacingTableConstraint* lef58CutSpacingTableDiffNetConstraint;
  frLef58CutSpacingTableConstraint* lef58SameNetInterCutSpacingTableConstraint;
  frLef58CutSpacingTableConstraint*
      lef58SameMetalInterCutSpacingTableConstraint;
  frLef58CutSpacingTableConstraint* lef58DefaultInterCutSpacingTableConstraint;

  std::vector<frLef58CornerSpacingConstraint*> lef58CornerSpacingConstraints;
  std::vector<frLef58EolKeepOutConstraint*> lef58EolKeepOutConstraints;
  std::vector<frMetalWidthViaConstraint*> metalWidthViaConstraints;
  std::vector<frLef58AreaConstraint*> lef58AreaConstraints;
  drEolSpacingConstraint drEolCon;
};
}  // namespace fr
