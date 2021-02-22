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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FR_LAYER_H_
#define _FR_LAYER_H_

#include "frBaseTypes.h"
#include "db/infra/frPrefRoutingDir.h"
#include "db/infra/frSegStyle.h"
#include "db/obj/frVia.h"
#include "db/tech/frConstraint.h"
#include <set>

namespace fr {
  namespace io {
    class Parser;
  }
  class frLayer {
  public:
    friend class io::Parser;
    // constructor
    frLayer(): type(frLayerTypeEnum::IMPLANT), layerNum(0), pitch(0), width(0), minWidth(0), defaultViaDef(nullptr), minSpc(nullptr), spacingSamenet(nullptr),
               eols(), cutConstraints(), cutSpacingSamenetConstraints(),
               interLayerCutSpacingConstraints(), interLayerCutSpacingSamenetConstraints(),
               interLayerCutSpacingConstraintsMap(), interLayerCutSpacingSamenetConstraintsMap(),
               lef58CutSpacingConstraints(), lef58CutSpacingSamenetConstraints(),
               recheckConstraint(nullptr), shortConstraint(nullptr), offGridConstraint(nullptr), minEnclosedAreaConstraints(),
               nonSufficientMetalConstraint(nullptr), areaConstraint(nullptr), 
               minStepConstraint(nullptr), lef58MinStepConstraints(), minWidthConstraint(nullptr),
               minimumcutConstraints(), lef58RectOnlyConstraint(nullptr), lef58RightWayOnGridOnlyConstraint(nullptr) {}
    frLayer(frLayerNum layerNumIn, const frString &nameIn): type(frLayerTypeEnum::IMPLANT), layerNum(layerNumIn), name(nameIn), pitch(0), width(0), minWidth(-1), defaultViaDef(nullptr),
                                                            minSpc(nullptr), spacingSamenet(nullptr), eols(),
                                                            cutConstraints(), cutSpacingSamenetConstraints(),
                                                            interLayerCutSpacingConstraints(), interLayerCutSpacingSamenetConstraints(),
                                                            interLayerCutSpacingConstraintsMap(), interLayerCutSpacingSamenetConstraintsMap(),
                                                            lef58CutSpacingConstraints(), lef58CutSpacingSamenetConstraints(),
                                                            recheckConstraint(nullptr), shortConstraint(nullptr), offGridConstraint(nullptr), nonSufficientMetalConstraint(nullptr),
                                                            areaConstraint(nullptr), minStepConstraint(nullptr), lef58MinStepConstraints(),
                                                            minWidthConstraint(nullptr), minimumcutConstraints(),
                                                            lef58RectOnlyConstraint(nullptr), lef58RightWayOnGridOnlyConstraint(nullptr) {}
    // setters
    void setLayerNum(frLayerNum layerNumIn) {
      layerNum = layerNumIn;
    }
    void setName(const frString &nameIn) {
      name = nameIn;
    }
    void setPitch(frUInt4 in) {
      pitch = in;
    }
    void setWidth(frUInt4 widthIn) {
      width = widthIn;
    }
    void setMinWidth(frUInt4 minWidthIn) {
      minWidth = minWidthIn;
    }
    void setDir(frPrefRoutingDirEnum dirIn) {
      dir.set(dirIn);
    }
    void setDefaultViaDef(frViaDef* in) {
      defaultViaDef = in;
    }
    void addConstraint(const std::shared_ptr<frConstraint> &consIn) {
      constraints.push_back(consIn);
    }
    void setType(frLayerTypeEnum typeIn) {
      type = typeIn;
    }
    void addViaDef(frViaDef* viaDefIn) {
      viaDefs.insert(viaDefIn);
    }

    // getters
    frLayerNum getLayerNum() const {
      return layerNum;
    }
    void getName(frString &nameIn) const {
      nameIn = name;
    }
    frString getName() const {
      return name;
    }
    frUInt4 getPitch() const {
      return pitch;
    }
    frUInt4 getWidth() const {
      return width;
    }
    frUInt4 getMinWidth() const {
      return minWidth;
    }
    frPrefRoutingDir getDir() const {
      return dir;
    }
    frSegStyle getDefaultSegStyle() const {
      frSegStyle style;
      style.setWidth(width);
      style.setBeginStyle(frcExtendEndStyle, width/2);
      style.setEndStyle(frcExtendEndStyle, width/2);
      return style;
    }
    frViaDef* getDefaultViaDef() const {
      return defaultViaDef;
    }
    std::set<frViaDef*> getViaDefs() const {
      return viaDefs;
    }
    frCollection<std::shared_ptr<frConstraint> > getConstraints() const {
      frCollection<std::shared_ptr<frConstraint> > constraintsOut;
      for (auto constraint: constraints) {
        constraintsOut.push_back(constraint.lock());
      }
      return constraintsOut;
    }
    frLayerTypeEnum getType() const {
      return type;
    }

    // cut class (new)
    void addCutClass(frLef58CutClass *in) {
      name2CutClassIdxMap[in->getName()] = cutClasses.size();
      cutClasses.push_back(in);
    }

    int getCutClassIdx(frCoord width, frCoord length) const {
      int cutClassIdx = -1;
      for (int i = 0; i < (int)cutClasses.size(); i++) {
        auto cutClass = cutClasses[i];
        if (cutClass->getViaWidth() == width && cutClass->getViaLength() == length) {
          cutClassIdx = i;
        }
      }
      return cutClassIdx;
    }

    int getCutClassIdx(std::string name) {
      if (name2CutClassIdxMap.find(name) != name2CutClassIdxMap.end()) {
        return name2CutClassIdxMap[name];
      } else {
        return -1;
      }
    }

    frLef58CutClass* getCutClass(frCoord width, frCoord length) const {
      int cutClassIdx = getCutClassIdx(width, length);
      if (cutClassIdx != -1) {
        return cutClasses[cutClassIdx];
      } else {
        return nullptr;
      }
    }

    frLef58CutClass* getCutClass(int cutClassIdx) {
      return cutClasses[cutClassIdx];
    }

    void printCutClasses() {
      for (auto &[name, idx]: name2CutClassIdxMap) {
        std::cout << "cutClass name: " << name << ", idx: " << idx << "\n";
      }
    }

    // cut spacing table
    bool hasLef58CutSpacingTableConstraints() const {
      return (lef58CutSpacingTableConstraints.size()) ? true : false;
    }
    frCollection<std::shared_ptr<frLef58CutSpacingTableConstraint> > getLef58CutSpacingTableConstraints() const {
      frCollection<std::shared_ptr<frLef58CutSpacingTableConstraint> > sol;
      std::transform(lef58CutSpacingTableConstraints.begin(), lef58CutSpacingTableConstraints.end(), 
                     std::back_inserter(sol), 
                     [](auto &kv) {return kv.lock();});
      return sol;
    }
    // spacing end of line
    bool hasLef58SpacingEndOfLineConstraints() const {
      return (lef58SpacingEndOfLineConstraints.size()) ? true : false;
    }
    frCollection<std::shared_ptr<frLef58SpacingEndOfLineConstraint> > getLef58SpacingEndOfLineConstraints() const {
      frCollection<std::shared_ptr<frLef58SpacingEndOfLineConstraint> > sol;
      std::transform(lef58SpacingEndOfLineConstraints.begin(), lef58SpacingEndOfLineConstraints.end(), 
                     std::back_inserter(sol), 
                     [](auto &kv) {return kv.lock();});
      return sol;
    }

    // new functions
    bool hasMinSpacing() const {
      return (minSpc);
    }
    frConstraint* getMinSpacing() const {
      return minSpc;
    }
    void setMinSpacing(frConstraint* in, bool verbose = false) {
      if (verbose && minSpc != nullptr) {
        std::cout <<"Warning: override minspacing rule, ";
        if (minSpc->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
          std::cout <<"original type is SPACING, ";
        } else if (minSpc->typeId() == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
          std::cout <<"original type is SPACINGTABLE PARALLELRUNLENGTH, ";
        } else if (minSpc->typeId() == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
          std::cout <<"original type is SPACINGTABLE TWOWIDTHS, ";
        } else {
          std::cout <<"original type is UNKNWON, ";
        }
        if (in->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
          std::cout <<"new type is SPACING";
        } else if (in->typeId() == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
          std::cout <<"new type is SPACINGTABLE PARALLELRUNLENGTH";
        } else if (in->typeId() == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
          std::cout <<"new type is SPACINGTABLE TWOWIDTHS";
        } else {
          std::cout <<"new type is UNKNWON";
        }
        std::cout <<std::endl;
      }
      minSpc = in;
    }
    bool hasSpacingSamenet() const {
      return (spacingSamenet);
    }
    frSpacingSamenetConstraint* getSpacingSamenet() const {
      return spacingSamenet;
    }
    void setSpacingSamenet(frSpacingSamenetConstraint* in) {
      spacingSamenet = in;
    }
    bool hasEolSpacing() const {
      return (eols.empty() ? false : true);
    }
    void addEolSpacing(frSpacingEndOfLineConstraint* in) {
      eols.push_back(in);
    }
    const std::vector<frSpacingEndOfLineConstraint*>& getEolSpacing() const {
      return eols;
    }
    // lef58
    void addLef58CutSpacingConstraint(frLef58CutSpacingConstraint* in) {
      if (in->isSameNet()) {
        lef58CutSpacingSamenetConstraints.push_back(in);
      } else {
        lef58CutSpacingConstraints.push_back(in);
      }
    }
    const std::vector<frLef58CutSpacingConstraint*>& getLef58CutSpacingConstraints(bool samenet = false) const {
      if (samenet) {
        return lef58CutSpacingSamenetConstraints;
      } else {
        return lef58CutSpacingConstraints;
      }
    }
    void addLef58MinStepConstraint(frLef58MinStepConstraint* in) {
      lef58MinStepConstraints.push_back(in);
    }
    const std::vector<frLef58MinStepConstraint*>& getLef58MinStepConstraints() const {
      return lef58MinStepConstraints;
    }

    void addCutSpacingConstraint(frCutSpacingConstraint* in) {
      if (!(in->isLayer())) {
        if (in->hasSameNet()) {
          cutSpacingSamenetConstraints.push_back(in);
        } else {
          cutConstraints.push_back(in);
        }
      } else {
        if (!(in->hasSameNet())) {
          if (interLayerCutSpacingConstraintsMap.find(in->getSecondLayerName()) != interLayerCutSpacingConstraintsMap.end()) {
            std::cout << "Error: Up to one diff-net inter-layer cut spacing rule can be specified for one layer pair. Rule ignored\n";
          } else {
            interLayerCutSpacingConstraintsMap[in->getSecondLayerName()] = in;
          }
        } else {
          if (interLayerCutSpacingSamenetConstraintsMap.find(in->getSecondLayerName()) != interLayerCutSpacingSamenetConstraintsMap.end()) {
            std::cout << "Error: Up to one same-net inter-layer cut spacing rule can be specified for one layer pair. Rule ignored\n";
          } else {
            interLayerCutSpacingSamenetConstraintsMap[in->getSecondLayerName()] = in;
          }
        }
      }
    }
    frCutSpacingConstraint* getInterLayerCutSpacing(frLayerNum layerNum, bool samenet = false) const {
      if (!samenet) {
        return interLayerCutSpacingConstraints[layerNum];
      } else {
        return interLayerCutSpacingSamenetConstraints[layerNum];
      }
    }
    const std::vector<frCutSpacingConstraint*>& getInterLayerCutSpacingConstraint(bool samenet = false) const {
      if (!samenet) {
        return interLayerCutSpacingConstraints;
      } else {
        return interLayerCutSpacingSamenetConstraints;
      }
    }
    // do not use this after initialization
    std::vector<frCutSpacingConstraint*>& getInterLayerCutSpacingConstraintRef(bool samenet = false) {
      if (!samenet) {
        return interLayerCutSpacingConstraints;
      } else {
        return interLayerCutSpacingSamenetConstraints;
      }
    }
    const std::map<std::string, frCutSpacingConstraint*>& getInterLayerCutSpacingConstraintMap(bool samenet = false) const {
      if (!samenet) {
        return interLayerCutSpacingConstraintsMap;
      } else {
        return interLayerCutSpacingSamenetConstraintsMap;
      }
    }
    const std::vector<frCutSpacingConstraint*>& getCutConstraint(bool samenet = false) const {
      if (samenet) {
        return cutSpacingSamenetConstraints;
      } else {
        return cutConstraints;
      }
    }
    const std::vector<frCutSpacingConstraint*>& getCutSpacing(bool samenet = false) const {
      if (samenet) {
        return cutSpacingSamenetConstraints;
      } else {
        return cutConstraints;
      }
    }
    bool hasCutSpacing(bool samenet = false) const {
      if (samenet) {
        return (!cutSpacingSamenetConstraints.empty());
      } else {
        return (!cutConstraints.empty());
      }
    }
    bool hasInterLayerCutSpacing(frLayerNum layerNum, bool samenet = false) const {
      if (samenet) {
        return (!(interLayerCutSpacingSamenetConstraints[layerNum] == nullptr));
      } else {
        return (!(interLayerCutSpacingConstraints[layerNum] == nullptr));
      }
    }
    void setRecheckConstraint(frRecheckConstraint* in) {
      recheckConstraint = in;
    }
    frRecheckConstraint* getRecheckConstraint() {
      return recheckConstraint;
    }
    void setShortConstraint(frShortConstraint* in) {
      shortConstraint = in;
    }
    frShortConstraint* getShortConstraint() {
      return shortConstraint;
    }
    void setOffGridConstraint(frOffGridConstraint* in) {
      offGridConstraint = in;
    }
    frOffGridConstraint* getOffGridConstraint() {
      return offGridConstraint;
    }
    void setNonSufficientMetalConstraint(frNonSufficientMetalConstraint* in) {
      nonSufficientMetalConstraint = in;
    }
    frNonSufficientMetalConstraint* getNonSufficientMetalConstraint() {
      return nonSufficientMetalConstraint;
    }
    void setAreaConstraint(frAreaConstraint* in) {
      areaConstraint = in;
    }
    frAreaConstraint* getAreaConstraint() {
      return areaConstraint;
    }
    void setMinStepConstraint(frMinStepConstraint* in) {
      minStepConstraint = in;
    }
    frMinStepConstraint* getMinStepConstraint() {
      return minStepConstraint;
    }
    void setMinWidthConstraint(frMinWidthConstraint* in) {
      minWidthConstraint = in;
    }
    frMinWidthConstraint* getMinWidthConstraint() {
      return minWidthConstraint;
    }

    void addMinimumcutConstraint(frMinimumcutConstraint* in) {
      minimumcutConstraints.push_back(in);
    }
    const std::vector<frMinimumcutConstraint*>& getMinimumcutConstraints() const {
      return minimumcutConstraints;
    }
    bool hasMinimumcut() const {
      return (!minimumcutConstraints.empty());
    }

    void addMinEnclosedAreaConstraint(frMinEnclosedAreaConstraint* in) {
      minEnclosedAreaConstraints.push_back(in);
    }

    const std::vector<frMinEnclosedAreaConstraint*>& getMinEnclosedAreaConstraints() const {
      return minEnclosedAreaConstraints;
    }

    bool hasMinEnclosedArea() const {
      return (!minEnclosedAreaConstraints.empty());
    }

    void setLef58RectOnlyConstraint(frLef58RectOnlyConstraint* in) {
      lef58RectOnlyConstraint = in;
    }
    frLef58RectOnlyConstraint* getLef58RectOnlyConstraint() {
      return lef58RectOnlyConstraint;
    }

    void setLef58RightWayOnGridOnlyConstraint(frLef58RightWayOnGridOnlyConstraint* in) {
      lef58RightWayOnGridOnlyConstraint = in;
    }
    frLef58RightWayOnGridOnlyConstraint* getLef58RightWayOnGridOnlyConstraint() {
      return lef58RightWayOnGridOnlyConstraint;
    }

    void addLef58CornerSpacingConstraint(frLef58CornerSpacingConstraint* in) {
      lef58CornerSpacingConstraints.push_back(in);
    }

    const std::vector<frLef58CornerSpacingConstraint*>& getLef58CornerSpacingConstraints() const {
      return lef58CornerSpacingConstraints;
    }

    bool hasLef58CornerSpacingConstraint() const {
      return (!lef58CornerSpacingConstraints.empty());
    }

    void printAllConstraints(utl::Logger* logger);
  protected:
    frLayerTypeEnum                                                 type;
    frLayerNum                                                      layerNum;
    frString                                                        name;
    frUInt4                                                         pitch;
    frUInt4                                                         width;
    frUInt4                                                         minWidth;
    frPrefRoutingDir                                                dir;
    frViaDef*                                                       defaultViaDef;
    std::set<frViaDef*>                                             viaDefs;
    std::vector<frLef58CutClass*>                                   cutClasses;
    std::map<std::string, int>                                      name2CutClassIdxMap;
    frCollection<std::weak_ptr<frConstraint> >                      constraints;
    frCollection<std::weak_ptr<frLef58CutSpacingTableConstraint> >  lef58CutSpacingTableConstraints;
    frCollection<std::weak_ptr<frLef58SpacingEndOfLineConstraint> > lef58SpacingEndOfLineConstraints;
    
    frConstraint*                                                   minSpc;
    frSpacingSamenetConstraint*                                     spacingSamenet;
    std::vector<frSpacingEndOfLineConstraint*>                      eols;
    std::vector<frCutSpacingConstraint*>                            cutConstraints;
    std::vector<frCutSpacingConstraint*>                            cutSpacingSamenetConstraints;
    // limited one per layer, vector.size() == layers.size()
    std::vector<frCutSpacingConstraint*>                            interLayerCutSpacingConstraints;
    // limited one per layer and only effective when diff-net version exists, vector.size() == layers.size()
    std::vector<frCutSpacingConstraint*>                            interLayerCutSpacingSamenetConstraints;
    // temp storage for inter-layer cut spacing before postProcess
    std::map<std::string, frCutSpacingConstraint*>                  interLayerCutSpacingConstraintsMap;
    std::map<std::string, frCutSpacingConstraint*>                  interLayerCutSpacingSamenetConstraintsMap;
    
    std::vector<frLef58CutSpacingConstraint*>                       lef58CutSpacingConstraints;
    std::vector<frLef58CutSpacingConstraint*>                       lef58CutSpacingSamenetConstraints;

    frRecheckConstraint*                                            recheckConstraint;
    frShortConstraint*                                              shortConstraint;
    frOffGridConstraint*                                            offGridConstraint;
    std::vector<frMinEnclosedAreaConstraint*>                       minEnclosedAreaConstraints;
    frNonSufficientMetalConstraint*                                 nonSufficientMetalConstraint;
    frAreaConstraint*                                               areaConstraint;
    frMinStepConstraint*                                            minStepConstraint;
    std::vector<frLef58MinStepConstraint*>                          lef58MinStepConstraints;
    frMinWidthConstraint*                                           minWidthConstraint;
    std::vector<frMinimumcutConstraint*>                            minimumcutConstraints;
    frLef58RectOnlyConstraint*                                      lef58RectOnlyConstraint;
    frLef58RightWayOnGridOnlyConstraint*                            lef58RightWayOnGridOnlyConstraint;
    std::vector<frLef58CornerSpacingConstraint*>                    lef58CornerSpacingConstraints;
  };
}

#endif
