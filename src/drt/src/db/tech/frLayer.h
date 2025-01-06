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
namespace drt {
namespace io {
class Parser;
}

class frLayer
{
 public:
  // setters
  void setDbLayer(odb::dbTechLayer* dbLayer) { db_layer_ = dbLayer; }
  void setFakeCut(bool fakeCutIn) { fakeCut_ = fakeCutIn; }
  void setFakeMasterslice(bool fakeMastersliceIn)
  {
    fakeMasterslice_ = fakeMastersliceIn;
  }
  void setLayerNum(frLayerNum layerNumIn) { layerNum_ = layerNumIn; }
  void setWidth(frUInt4 widthIn) { width_ = widthIn; }
  void setMinWidth(frUInt4 minWidthIn) { minWidth_ = minWidthIn; }
  void setDefaultViaDef(const frViaDef* in) { defaultViaDef_ = in; }
  void addSecondaryViaDef(const frViaDef* in)
  {
    secondaryViaDefs_.emplace_back(in);
  }
  const std::vector<const frViaDef*>& getSecondaryViaDefs() const
  {
    return secondaryViaDefs_;
  }
  void addConstraint(frConstraint* consIn) { constraints_.push_back(consIn); }
  void addViaDef(const frViaDef* viaDefIn) { viaDefs_.insert(viaDefIn); }
  void setHasVia2ViaMinStepViol(bool in) { hasMinStepViol_ = in; }
  void setUnidirectional(bool in) { unidirectional_ = in; }
  // getters
  odb::dbTechLayer* getDbLayer() const { return db_layer_; }
  bool isFakeCut() const { return fakeCut_; }
  bool isFakeMasterslice() const { return fakeMasterslice_; }
  frUInt4 getNumMasks() const
  {
    return (fakeCut_ || fakeMasterslice_) ? 1 : db_layer_->getNumMasks();
  }
  frLayerNum getLayerNum() const { return layerNum_; }
  void getName(frString& nameIn) const
  {
    nameIn = (fakeCut_)           ? "FR_VIA"
             : (fakeMasterslice_) ? "FR_MASTERSLICE"
                                  : db_layer_->getName();
  }
  frString getName() const
  {
    return (fakeCut_)           ? "Fr_VIA"
           : (fakeMasterslice_) ? "FR_MASTERSLICE"
                                : db_layer_->getName();
  }
  frUInt4 getPitch() const
  {
    return (fakeCut_ || fakeMasterslice_) ? 0 : db_layer_->getPitch();
  }
  frUInt4 getWidth() const { return width_; }
  frUInt4 getWrongDirWidth() const { return db_layer_->getWrongWayWidth(); }
  frUInt4 getMinWidth() const { return minWidth_; }
  dbTechLayerDir getDir() const
  {
    if (fakeCut_ || fakeMasterslice_) {
      return dbTechLayerDir::NONE;
    }
    return db_layer_->getDirection();
  }
  bool isVertical() const
  {
    return (fakeCut_ || fakeMasterslice_)
               ? false
               : db_layer_->getDirection() == dbTechLayerDir::VERTICAL;
  }
  bool isHorizontal() const
  {
    return (fakeCut_ || fakeMasterslice_)
               ? false
               : db_layer_->getDirection() == dbTechLayerDir::HORIZONTAL;
  }
  bool isUnidirectional() const
  {
    // We don't handle coloring so any multiple patterned
    // layer is treated as unidirectional.
    // RectOnly could allow for a purely wrong-way rect but
    // we ignore that rare case and treat it as unidirectional.
    return getNumMasks() > 1 || getLef58RectOnlyConstraint() || unidirectional_;
  }
  frSegStyle getDefaultSegStyle() const
  {
    frSegStyle style;
    style.setWidth(width_);
    style.setBeginStyle(frcExtendEndStyle, width_ / 2);
    style.setEndStyle(frcExtendEndStyle, width_ / 2);
    return style;
  }
  const frViaDef* getDefaultViaDef() const { return defaultViaDef_; }
  const frViaDef* getSecondaryViaDef(int idx) const
  {
    return secondaryViaDefs_.at(idx);
  }
  bool hasVia2ViaMinStepViol() { return hasMinStepViol_; }
  std::set<const frViaDef*> getViaDefs() const { return viaDefs_; }
  dbTechLayerType getType() const
  {
    if (fakeCut_) {
      return dbTechLayerType::CUT;
    }

    if (fakeMasterslice_) {
      return dbTechLayerType::MASTERSLICE;
    }

    return db_layer_->getType();
  }

  // cut class (new)
  void addCutClass(frLef58CutClass* in)
  {
    name2CutClassIdxMap_[in->getName()] = cutClasses_.size();
    cutClasses_.push_back(in);
  }

  int getCutClassIdx(frCoord width, frCoord length) const
  {
    int cutClassIdx = -1;
    for (int i = 0; i < (int) cutClasses_.size(); i++) {
      auto cutClass = cutClasses_[i];
      if (cutClass->getViaWidth() == width
          && cutClass->getViaLength() == length) {
        cutClassIdx = i;
      }
    }
    return cutClassIdx;
  }

  int getCutClassIdx(const std::string& name)
  {
    if (name2CutClassIdxMap_.find(name) != name2CutClassIdxMap_.end()) {
      return name2CutClassIdxMap_[name];
    }
    return -1;
  }

  frLef58CutClass* getCutClass(frCoord width, frCoord length) const
  {
    int cutClassIdx = getCutClassIdx(width, length);
    if (cutClassIdx != -1) {
      return cutClasses_[cutClassIdx];
    }
    return nullptr;
  }

  frLef58CutClass* getCutClass(int cutClassIdx)
  {
    return cutClasses_[cutClassIdx];
  }

  void printCutClasses()
  {
    for (auto& [name, idx] : name2CutClassIdxMap_) {
      std::cout << "cutClass name: " << name << ", idx: " << idx << "\n";
    }
  }

  // cut spacing table

  void setLef58SameNetCutSpcTblConstraint(frLef58CutSpacingTableConstraint* con)
  {
    lef58CutSpacingTableSameNetMetalConstraint_ = con;
  }

  bool hasLef58SameNetCutSpcTblConstraint() const
  {
    return lef58CutSpacingTableSameNetMetalConstraint_ != nullptr
           && lef58CutSpacingTableSameNetMetalConstraint_->getODBRule()
                  ->isSameNet();
  }

  frLef58CutSpacingTableConstraint* getLef58SameNetCutSpcTblConstraint() const
  {
    if (hasLef58SameNetCutSpcTblConstraint()) {
      return lef58CutSpacingTableSameNetMetalConstraint_;
    }
    return nullptr;
  }

  void setLef58SameMetalCutSpcTblConstraint(
      frLef58CutSpacingTableConstraint* con)
  {
    lef58CutSpacingTableSameNetMetalConstraint_ = con;
  }

  bool hasLef58SameMetalCutSpcTblConstraint() const
  {
    return lef58CutSpacingTableSameNetMetalConstraint_ != nullptr
           && lef58CutSpacingTableSameNetMetalConstraint_->getODBRule()
                  ->isSameMetal();
  }

  frLef58CutSpacingTableConstraint* getLef58SameMetalCutSpcTblConstraint() const
  {
    if (hasLef58SameMetalCutSpcTblConstraint()) {
      return lef58CutSpacingTableSameNetMetalConstraint_;
    }
    return nullptr;
  }

  void setLef58DiffNetCutSpcTblConstraint(frLef58CutSpacingTableConstraint* con)
  {
    lef58CutSpacingTableDiffNetConstraint_ = con;
  }

  bool hasLef58DiffNetCutSpcTblConstraint() const
  {
    return lef58CutSpacingTableDiffNetConstraint_ != nullptr;
  }

  frLef58CutSpacingTableConstraint* getLef58DiffNetCutSpcTblConstraint() const
  {
    return lef58CutSpacingTableDiffNetConstraint_;
  }

  // spacing end of line
  bool hasLef58SpacingEndOfLineConstraints() const
  {
    return !lef58SpacingEndOfLineConstraints_.empty();
  }
  const frCollection<frLef58SpacingEndOfLineConstraint*>&
  getLef58SpacingEndOfLineConstraints() const
  {
    return lef58SpacingEndOfLineConstraints_;
  }

  void addLef58SpacingEndOfLineConstraint(
      frLef58SpacingEndOfLineConstraint* constraintIn)
  {
    lef58SpacingEndOfLineConstraints_.push_back(constraintIn);
  }

  // spacing wrong direction
  bool hasLef58SpacingWrongDirConstraints() const
  {
    return !lef58SpacingWrongDirConstraints_.empty();
  }
  const frCollection<frLef58SpacingWrongDirConstraint*>&
  getLef58SpacingWrongDirConstraints() const
  {
    return lef58SpacingWrongDirConstraints_;
  }

  void addLef58SpacingWrongDirConstraint(
      frLef58SpacingWrongDirConstraint* constraintIn)
  {
    lef58SpacingWrongDirConstraints_.push_back(constraintIn);
  }

  // new functions
  bool hasMinSpacing() const { return minSpc_; }
  frConstraint* getMinSpacing() const { return minSpc_; }
  frCoord getMinSpacingValue(frCoord width1,
                             frCoord width2,
                             frCoord prl,
                             bool use_min_spacing);
  void setMinSpacing(frConstraint* in) { minSpc_ = in; }
  bool hasSpacingSamenet() const { return spacingSamenet_; }
  frSpacingSamenetConstraint* getSpacingSamenet() const
  {
    return spacingSamenet_;
  }
  void setSpacingSamenet(frSpacingSamenetConstraint* in)
  {
    spacingSamenet_ = in;
  }
  bool hasSpacingTableInfluence() const { return spacingInfluence_; }
  frSpacingTableInfluenceConstraint* getSpacingTableInfluence() const
  {
    return spacingInfluence_;
  }
  void setSpacingTableInfluence(frSpacingTableInfluenceConstraint* in)
  {
    spacingInfluence_ = in;
  }
  bool hasEolSpacing() const { return (eols_.empty() ? false : true); }
  void addEolSpacing(frSpacingEndOfLineConstraint* in) { eols_.push_back(in); }
  const std::vector<frSpacingEndOfLineConstraint*>& getEolSpacing() const
  {
    return eols_;
  }
  // lef58
  void addLef58CutSpacingConstraint(frLef58CutSpacingConstraint* in)
  {
    if (in->isSameNet()) {
      lef58CutSpacingSamenetConstraints_.push_back(in);
    } else {
      lef58CutSpacingConstraints_.push_back(in);
    }
  }
  const std::vector<frLef58CutSpacingConstraint*>&
  getLef58CutSpacingConstraints(bool samenet = false) const
  {
    if (samenet) {
      return lef58CutSpacingSamenetConstraints_;
    }
    return lef58CutSpacingConstraints_;
  }
  void addLef58MinStepConstraint(frLef58MinStepConstraint* in)
  {
    lef58MinStepConstraints_.push_back(in);
  }
  const std::vector<frLef58MinStepConstraint*>& getLef58MinStepConstraints()
      const
  {
    return lef58MinStepConstraints_;
  }

  void addLef58EolExtConstraint(frLef58EolExtensionConstraint* in)
  {
    lef58EolExtConstraints_.push_back(in);
  }
  const std::vector<frLef58EolExtensionConstraint*>& getLef58EolExtConstraints()
      const
  {
    return lef58EolExtConstraints_;
  }

  void addCutSpacingConstraint(frCutSpacingConstraint* in)
  {
    if (!(in->isLayer())) {
      if (in->hasSameNet()) {
        cutSpacingSamenetConstraints_.push_back(in);
      } else {
        cutConstraints_.push_back(in);
      }
    } else {
      if (!(in->hasSameNet())) {
        if (interLayerCutSpacingConstraintsMap_.find(in->getSecondLayerName())
            != interLayerCutSpacingConstraintsMap_.end()) {
          std::cout << "Error: Up to one diff-net inter-layer cut spacing rule "
                       "can be specified for one layer pair. Rule ignored\n";
        } else {
          interLayerCutSpacingConstraintsMap_[in->getSecondLayerName()] = in;
        }
      } else {
        if (interLayerCutSpacingSamenetConstraintsMap_.find(
                in->getSecondLayerName())
            != interLayerCutSpacingSamenetConstraintsMap_.end()) {
          std::cout << "Error: Up to one same-net inter-layer cut spacing rule "
                       "can be specified for one layer pair. Rule ignored\n";
        } else {
          interLayerCutSpacingSamenetConstraintsMap_[in->getSecondLayerName()]
              = in;
        }
      }
    }
  }
  frCutSpacingConstraint* getInterLayerCutSpacing(frLayerNum layerNum,
                                                  bool samenet = false) const
  {
    if (!samenet) {
      return interLayerCutSpacingConstraints_[layerNum];
    }
    return interLayerCutSpacingSamenetConstraints_[layerNum];
  }
  const std::vector<frCutSpacingConstraint*>& getInterLayerCutSpacingConstraint(
      bool samenet = false) const
  {
    if (!samenet) {
      return interLayerCutSpacingConstraints_;
    }
    return interLayerCutSpacingSamenetConstraints_;
  }
  // do not use this after initialization
  std::vector<frCutSpacingConstraint*>& getInterLayerCutSpacingConstraintRef(
      bool samenet = false)
  {
    if (!samenet) {
      return interLayerCutSpacingConstraints_;
    }
    return interLayerCutSpacingSamenetConstraints_;
  }
  const std::map<std::string, frCutSpacingConstraint*>&
  getInterLayerCutSpacingConstraintMap(bool samenet = false) const
  {
    if (!samenet) {
      return interLayerCutSpacingConstraintsMap_;
    }
    return interLayerCutSpacingSamenetConstraintsMap_;
  }
  const std::vector<frCutSpacingConstraint*>& getCutConstraint(bool samenet
                                                               = false) const
  {
    if (samenet) {
      return cutSpacingSamenetConstraints_;
    }
    return cutConstraints_;
  }
  const std::vector<frCutSpacingConstraint*>& getCutSpacing(bool samenet
                                                            = false) const
  {
    if (samenet) {
      return cutSpacingSamenetConstraints_;
    }
    return cutConstraints_;
  }
  frCoord getCutSpacingValue() const
  {
    frCoord s = 0;
    for (auto con : getCutSpacing()) {
      s = std::max(s, con->getCutSpacing());
    }
    return s;
  }
  bool hasCutSpacing(bool samenet = false) const
  {
    if (samenet) {
      return !cutSpacingSamenetConstraints_.empty();
    }
    return !cutConstraints_.empty();
  }
  bool haslef58CutSpacing(bool samenet = false) const
  {
    if (samenet) {
      return !lef58CutSpacingSamenetConstraints_.empty();
    }
    return !lef58CutSpacingConstraints_.empty();
  }
  bool hasInterLayerCutSpacing(frLayerNum layerNum, bool samenet = false) const
  {
    if (samenet) {
      return interLayerCutSpacingSamenetConstraints_[layerNum] != nullptr;
    }
    return interLayerCutSpacingConstraints_[layerNum] != nullptr;
  }
  void setRecheckConstraint(frRecheckConstraint* in)
  {
    recheckConstraint_ = in;
  }
  frRecheckConstraint* getRecheckConstraint() { return recheckConstraint_; }
  void setShortConstraint(frShortConstraint* in) { shortConstraint_ = in; }
  frShortConstraint* getShortConstraint() { return shortConstraint_; }
  void setOffGridConstraint(frOffGridConstraint* in)
  {
    offGridConstraint_ = in;
  }
  frOffGridConstraint* getOffGridConstraint() { return offGridConstraint_; }
  void setNonSufficientMetalConstraint(frNonSufficientMetalConstraint* in)
  {
    nonSufficientMetalConstraint_ = in;
  }
  frNonSufficientMetalConstraint* getNonSufficientMetalConstraint()
  {
    return nonSufficientMetalConstraint_;
  }
  void setAreaConstraint(frAreaConstraint* in) { areaConstraint_ = in; }
  frAreaConstraint* getAreaConstraint() { return areaConstraint_; }
  void setMinStepConstraint(frMinStepConstraint* in)
  {
    minStepConstraint_ = in;
  }
  frMinStepConstraint* getMinStepConstraint() { return minStepConstraint_; }
  void setMinWidthConstraint(frMinWidthConstraint* in)
  {
    minWidthConstraint_ = in;
  }
  frMinWidthConstraint* getMinWidthConstraint() { return minWidthConstraint_; }

  void addMinimumcutConstraint(frMinimumcutConstraint* in)
  {
    minimumcutConstraints_.push_back(in);
  }
  const std::vector<frMinimumcutConstraint*>& getMinimumcutConstraints() const
  {
    return minimumcutConstraints_;
  }
  bool hasMinimumcut() const { return !minimumcutConstraints_.empty(); }

  void addLef58MinimumcutConstraint(frLef58MinimumcutConstraint* in)
  {
    lef58MinimumcutConstraints_.push_back(in);
  }
  const std::vector<frLef58MinimumcutConstraint*>&
  getLef58MinimumcutConstraints() const
  {
    return lef58MinimumcutConstraints_;
  }
  bool hasLef58Minimumcut() const
  {
    return !lef58MinimumcutConstraints_.empty();
  }

  void addMinEnclosedAreaConstraint(frMinEnclosedAreaConstraint* in)
  {
    minEnclosedAreaConstraints_.push_back(in);
  }

  const std::vector<frMinEnclosedAreaConstraint*>&
  getMinEnclosedAreaConstraints() const
  {
    return minEnclosedAreaConstraints_;
  }

  bool hasMinEnclosedArea() const
  {
    return !minEnclosedAreaConstraints_.empty();
  }

  void setLef58RectOnlyConstraint(frLef58RectOnlyConstraint* in)
  {
    lef58RectOnlyConstraint_ = in;
  }
  frLef58RectOnlyConstraint* getLef58RectOnlyConstraint() const
  {
    return lef58RectOnlyConstraint_;
  }

  void setLef58RightWayOnGridOnlyConstraint(
      frLef58RightWayOnGridOnlyConstraint* in)
  {
    lef58RightWayOnGridOnlyConstraint_ = in;
  }
  frLef58RightWayOnGridOnlyConstraint* getLef58RightWayOnGridOnlyConstraint()
  {
    return lef58RightWayOnGridOnlyConstraint_;
  }

  void addLef58CornerSpacingConstraint(frLef58CornerSpacingConstraint* in)
  {
    lef58CornerSpacingConstraints_.push_back(in);
  }

  const std::vector<frLef58CornerSpacingConstraint*>&
  getLef58CornerSpacingConstraints() const
  {
    return lef58CornerSpacingConstraints_;
  }

  bool hasLef58CornerSpacingConstraint() const
  {
    return !lef58CornerSpacingConstraints_.empty();
  }

  void addLef58EolKeepOutConstraint(frLef58EolKeepOutConstraint* in)
  {
    lef58EolKeepOutConstraints_.push_back(in);
  }

  const std::vector<frLef58EolKeepOutConstraint*>&
  getLef58EolKeepOutConstraints() const
  {
    return lef58EolKeepOutConstraints_;
  }

  bool hasLef58EolKeepOutConstraint() const
  {
    return !lef58EolKeepOutConstraints_.empty();
  }

  void addLef58MaxSpacingConstraint(frLef58MaxSpacingConstraint* in)
  {
    maxSpacingConstraints_.emplace_back(in);
  }

  const std::vector<frLef58MaxSpacingConstraint*>&
  getLef58MaxSpacingConstraints() const
  {
    return maxSpacingConstraints_;
  }

  bool hasLef58MaxSpacingConstraints() const
  {
    return !maxSpacingConstraints_.empty();
  }

  void addMetalWidthViaConstraint(frMetalWidthViaConstraint* in)
  {
    metalWidthViaConstraints_.push_back(in);
  }

  const std::vector<frMetalWidthViaConstraint*>& getMetalWidthViaConstraints()
      const
  {
    return metalWidthViaConstraints_;
  }
  void addLef58AreaConstraint(frLef58AreaConstraint* in)
  {
    lef58AreaConstraints_.push_back(in);
  }

  const std::vector<frLef58AreaConstraint*>& getLef58AreaConstraints() const
  {
    return lef58AreaConstraints_;
  }

  bool hasLef58AreaConstraint() const { return !lef58AreaConstraints_.empty(); }

  void addKeepOutZoneConstraint(frLef58KeepOutZoneConstraint* in)
  {
    keepOutZoneConstraints_.push_back(in);
  }

  const std::vector<frLef58KeepOutZoneConstraint*>& getKeepOutZoneConstraints()
      const
  {
    return keepOutZoneConstraints_;
  }

  bool hasKeepOutZoneConstraints() const
  {
    return !keepOutZoneConstraints_.empty();
  }

  void addSpacingRangeConstraint(frSpacingRangeConstraint* in)
  {
    spacingRangeConstraints_.push_back(in);
  }

  const std::vector<frSpacingRangeConstraint*>& getSpacingRangeConstraints()
      const
  {
    return spacingRangeConstraints_;
  }

  bool hasSpacingRangeConstraints() const
  {
    return !spacingRangeConstraints_.empty();
  }

  void addTwoWiresForbiddenSpacingConstraint(
      frLef58TwoWiresForbiddenSpcConstraint* in)
  {
    twForbiddenSpcConstraints_.push_back(in);
  }

  const std::vector<frLef58TwoWiresForbiddenSpcConstraint*>&
  getTwoWiresForbiddenSpacingConstraints() const
  {
    return twForbiddenSpcConstraints_;
  }

  bool hasTwoWiresForbiddenSpacingConstraints() const
  {
    return !twForbiddenSpcConstraints_.empty();
  }

  void addForbiddenSpacingConstraint(frLef58ForbiddenSpcConstraint* in)
  {
    forbiddenSpcConstraints_.push_back(in);
  }

  const std::vector<frLef58ForbiddenSpcConstraint*>&
  getForbiddenSpacingConstraints() const
  {
    return forbiddenSpcConstraints_;
  }

  bool hasForbiddenSpacingConstraints() const
  {
    return !forbiddenSpcConstraints_.empty();
  }

  void setLef58SameNetInterCutSpcTblConstraint(
      frLef58CutSpacingTableConstraint* con)
  {
    lef58SameNetInterCutSpacingTableConstraint_ = con;
  }

  bool hasLef58SameNetInterCutSpcTblConstraint() const
  {
    return lef58SameNetInterCutSpacingTableConstraint_ != nullptr;
  }

  frLef58CutSpacingTableConstraint* getLef58SameNetInterCutSpcTblConstraint()
      const
  {
    return lef58SameNetInterCutSpacingTableConstraint_;
  }

  void setLef58SameMetalInterCutSpcTblConstraint(
      frLef58CutSpacingTableConstraint* con)
  {
    lef58SameMetalInterCutSpacingTableConstraint_ = con;
  }

  bool hasLef58SameMetalInterCutSpcTblConstraint() const
  {
    return lef58SameMetalInterCutSpacingTableConstraint_ != nullptr;
  }

  frLef58CutSpacingTableConstraint* getLef58SameMetalInterCutSpcTblConstraint()
      const
  {
    return lef58SameMetalInterCutSpacingTableConstraint_;
  }

  void setLef58DefaultInterCutSpcTblConstraint(
      frLef58CutSpacingTableConstraint* con)
  {
    lef58DefaultInterCutSpacingTableConstraint_ = con;
  }

  bool hasLef58DefaultInterCutSpcTblConstraint() const
  {
    return lef58DefaultInterCutSpacingTableConstraint_ != nullptr;
  }

  frLef58CutSpacingTableConstraint* getLef58DefaultInterCutSpcTblConstraint()
      const
  {
    return lef58DefaultInterCutSpacingTableConstraint_;
  }

  void addLef58EnclosureConstraint(frLef58EnclosureConstraint* con)
  {
    auto addToLef58EncConstraints
        = [](std::vector<
                 std::map<frCoord, std::vector<frLef58EnclosureConstraint*>>>&
                 lef58EncConstraints,
             frLef58EnclosureConstraint* con) {
            int cutClassIdx = con->getCutClassIdx();
            if (lef58EncConstraints.size() <= cutClassIdx) {
              lef58EncConstraints.resize(cutClassIdx + 1);
            }
            lef58EncConstraints[cutClassIdx][con->getWidth()].push_back(con);
          };
    if (!con->isAboveOnly()) {
      addToLef58EncConstraints(con->isEol() ? belowLef58EncEolConstraints_
                                            : belowLef58EncConstraints_,
                               con);
    }
    if (!con->isBelowOnly()) {
      addToLef58EncConstraints(con->isEol() ? aboveLef58EncEolConstraints_
                                            : aboveLef58EncConstraints_,
                               con);
    }
  }

  bool hasLef58EnclosureConstraint(int cutClassIdx,
                                   bool above,
                                   bool eol = false) const
  {
    auto& lef58EncConstraints = above ? (eol ? aboveLef58EncEolConstraints_
                                             : aboveLef58EncConstraints_)
                                      : (eol ? belowLef58EncEolConstraints_
                                             : belowLef58EncConstraints_);
    if (cutClassIdx < 0 || lef58EncConstraints.size() <= cutClassIdx) {
      return false;
    }
    return !lef58EncConstraints.at(cutClassIdx).empty();
  }

  std::vector<frLef58EnclosureConstraint*> getLef58EnclosureConstraints(
      int cutClassIdx,
      frCoord width,
      bool above,
      bool eol = false) const
  {
    // initialize with empty vector
    std::vector<frLef58EnclosureConstraint*> result;
    // check class and size match first
    if (hasLef58EnclosureConstraint(cutClassIdx, above, eol)) {
      auto& lef58EncConstraints = above ? (eol ? aboveLef58EncEolConstraints_
                                               : aboveLef58EncConstraints_)
                                        : (eol ? belowLef58EncEolConstraints_
                                               : belowLef58EncConstraints_);
      const auto& mmap = lef58EncConstraints.at(cutClassIdx);
      auto it = mmap.upper_bound(width);
      if (it != mmap.begin()) {
        it--;
        result = it->second;
      }
    }
    return result;
  }

  bool hasOrthSpacingTableConstraint() const
  {
    return spc_tbl_orth_con_ != nullptr;
  }

  frOrthSpacingTableConstraint* getOrthSpacingTableConstraint() const
  {
    return spc_tbl_orth_con_;
  }

  void setOrthSpacingTableConstraint(frOrthSpacingTableConstraint* con)
  {
    spc_tbl_orth_con_ = con;
  }

  void setDrEolSpacingConstraint(frCoord width, frCoord space, frCoord within)
  {
    drEolCon_.eolWidth = width;
    drEolCon_.eolSpace = space;
    drEolCon_.eolWithin = within;
  }

  const drEolSpacingConstraint& getDrEolSpacingConstraint() const
  {
    return drEolCon_;
  }

  void printAllConstraints(utl::Logger* logger);

  void setWidthTblOrthCon(frLef58WidthTableOrthConstraint* con)
  {
    width_tbl_orth_con_ = con;
  }
  frLef58WidthTableOrthConstraint* getWidthTblOrthCon() const
  {
    return width_tbl_orth_con_;
  }

 protected:
  odb::dbTechLayer* db_layer_{nullptr};
  bool fakeCut_{false};
  bool fakeMasterslice_{false};
  frLayerNum layerNum_{0};
  frUInt4 width_{0};
  frUInt4 wrongDirWidth_{0};
  frUInt4 minWidth_{0};
  const frViaDef* defaultViaDef_{nullptr};
  std::vector<const frViaDef*> secondaryViaDefs_;
  bool hasMinStepViol_{false};
  bool unidirectional_{false};
  std::set<const frViaDef*> viaDefs_;
  std::vector<frLef58CutClass*> cutClasses_;
  std::map<std::string, int> name2CutClassIdxMap_;
  frCollection<frConstraint*> constraints_;

  frCollection<frLef58SpacingEndOfLineConstraint*>
      lef58SpacingEndOfLineConstraints_;

  frCollection<frLef58SpacingWrongDirConstraint*>
      lef58SpacingWrongDirConstraints_;

  frConstraint* minSpc_{nullptr};
  frSpacingSamenetConstraint* spacingSamenet_{nullptr};
  frSpacingTableInfluenceConstraint* spacingInfluence_{nullptr};
  std::vector<frSpacingEndOfLineConstraint*> eols_;
  std::vector<frCutSpacingConstraint*> cutConstraints_;
  std::vector<frCutSpacingConstraint*> cutSpacingSamenetConstraints_;
  // limited one per layer, vector.size() == layers.size()
  std::vector<frCutSpacingConstraint*> interLayerCutSpacingConstraints_;
  // limited one per layer and only effective when diff-net version exists,
  // vector.size() == layers.size()
  std::vector<frCutSpacingConstraint*> interLayerCutSpacingSamenetConstraints_;
  // temp storage for inter-layer cut spacing before postProcess
  std::map<std::string, frCutSpacingConstraint*>
      interLayerCutSpacingConstraintsMap_;
  std::map<std::string, frCutSpacingConstraint*>
      interLayerCutSpacingSamenetConstraintsMap_;

  std::vector<frLef58CutSpacingConstraint*> lef58CutSpacingConstraints_;
  std::vector<frLef58CutSpacingConstraint*> lef58CutSpacingSamenetConstraints_;

  frRecheckConstraint* recheckConstraint_{nullptr};
  frShortConstraint* shortConstraint_{nullptr};
  frOffGridConstraint* offGridConstraint_{nullptr};
  std::vector<frMinEnclosedAreaConstraint*> minEnclosedAreaConstraints_;
  frNonSufficientMetalConstraint* nonSufficientMetalConstraint_{nullptr};
  frAreaConstraint* areaConstraint_{nullptr};
  frMinStepConstraint* minStepConstraint_{nullptr};
  std::vector<frLef58MinStepConstraint*> lef58MinStepConstraints_;
  std::vector<frLef58EolExtensionConstraint*> lef58EolExtConstraints_;
  frMinWidthConstraint* minWidthConstraint_{nullptr};
  std::vector<frMinimumcutConstraint*> minimumcutConstraints_;
  std::vector<frLef58MinimumcutConstraint*> lef58MinimumcutConstraints_;
  frLef58RectOnlyConstraint* lef58RectOnlyConstraint_{nullptr};
  frLef58RightWayOnGridOnlyConstraint* lef58RightWayOnGridOnlyConstraint_{
      nullptr};

  frLef58CutSpacingTableConstraint* lef58CutSpacingTableSameNetMetalConstraint_{
      nullptr};
  frLef58CutSpacingTableConstraint* lef58CutSpacingTableDiffNetConstraint_{
      nullptr};
  frLef58CutSpacingTableConstraint* lef58SameNetInterCutSpacingTableConstraint_{
      nullptr};
  frLef58CutSpacingTableConstraint*
      lef58SameMetalInterCutSpacingTableConstraint_{nullptr};
  frLef58CutSpacingTableConstraint* lef58DefaultInterCutSpacingTableConstraint_{

      nullptr};

  std::vector<frLef58CornerSpacingConstraint*> lef58CornerSpacingConstraints_;
  std::vector<frLef58EolKeepOutConstraint*> lef58EolKeepOutConstraints_;
  std::vector<frMetalWidthViaConstraint*> metalWidthViaConstraints_;
  std::vector<frLef58AreaConstraint*> lef58AreaConstraints_;
  std::vector<frLef58KeepOutZoneConstraint*> keepOutZoneConstraints_;
  std::vector<frSpacingRangeConstraint*> spacingRangeConstraints_;
  std::vector<frLef58TwoWiresForbiddenSpcConstraint*>
      twForbiddenSpcConstraints_;
  std::vector<frLef58ForbiddenSpcConstraint*> forbiddenSpcConstraints_;
  std::vector<std::map<frCoord, std::vector<frLef58EnclosureConstraint*>>>
      aboveLef58EncConstraints_;
  std::vector<std::map<frCoord, std::vector<frLef58EnclosureConstraint*>>>
      belowLef58EncConstraints_;
  std::vector<std::map<frCoord, std::vector<frLef58EnclosureConstraint*>>>
      aboveLef58EncEolConstraints_;
  std::vector<std::map<frCoord, std::vector<frLef58EnclosureConstraint*>>>
      belowLef58EncEolConstraints_;
  // vector of maxspacing constraints
  std::vector<frLef58MaxSpacingConstraint*> maxSpacingConstraints_;

  frOrthSpacingTableConstraint* spc_tbl_orth_con_{nullptr};
  drEolSpacingConstraint drEolCon_;
  frLef58WidthTableOrthConstraint* width_tbl_orth_con_{nullptr};

  friend class io::Parser;
};
}  // namespace drt
