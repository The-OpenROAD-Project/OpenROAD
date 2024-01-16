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

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include "db/obj/frVia.h"
#include "db/tech/frLayer.h"
#include "db/tech/frViaRuleGenerate.h"
#include "frBaseTypes.h"
#include "global.h"
#include "utl/Logger.h"

namespace odb {
class dbTechLayer;
}
namespace fr {
namespace io {
class Parser;
}

class frTechObject
{
 public:
  // constructors
  frTechObject() : dbUnit(0), manufacturingGrid(0) {}
  // getters
  frUInt4 getDBUPerUU() const { return dbUnit; }
  frUInt4 getManufacturingGrid() const { return manufacturingGrid; }
  frLayer* getLayer(const frString& name) const
  {
    if (name2layer.find(name) == name2layer.end()) {
      // std::cout <<"Error: cannot find layer" <<std::endl;
      // exit(1);
      return nullptr;
    } else {
      return name2layer.at(name);
    }
  }
  frLayer* getLayer(frLayerNum in) const
  {
    if ((int) in < 0 || in >= (int) layers.size()) {
      std::cout << "Error: cannot find layer" << std::endl;
      exit(1);
    } else {
      return layers.at(in).get();
    }
  }
  frLayerNum getBottomLayerNum() const { return 0; }
  frLayerNum getTopLayerNum() const
  {
    return (frLayerNum) ((int) layers.size() - 1);
  }
  const std::vector<std::unique_ptr<frLayer>>& getLayers() const
  {
    return layers;
  }

  const std::vector<std::unique_ptr<frViaDef>>& getVias() const { return vias; }
  const std::vector<std::unique_ptr<frViaRuleGenerate>>& getViaRuleGenerates()
      const
  {
    return viaRuleGenerates;
  }
  bool hasUnidirectionalLayer(odb::dbTechLayer* dbLayer) const
  {
    return unidirectional_layers_.find(dbLayer) != unidirectional_layers_.end();
  }

  // setters
  void setDBUPerUU(frUInt4 uIn) { dbUnit = uIn; }
  void setManufacturingGrid(frUInt4 in) { manufacturingGrid = in; }
  void addLayer(std::unique_ptr<frLayer> in)
  {
    name2layer[in->getName()] = in.get();
    layers.push_back(std::move(in));
    layer2Name2CutClass.push_back(std::map<std::string, frLef58CutClass*>());
    layerCutClass.push_back(std::vector<std::unique_ptr<frLef58CutClass>>());
  }
  void addVia(std::unique_ptr<frViaDef> in)
  {
    in->setId(vias.size());
    if (name2via.find(in->getName()) != name2via.end()) {
      std::cout << "Error: duplicated via definition for " << in->getName()
                << "\n";
    }
    name2via[in->getName()] = in.get();
    vias.push_back(std::move(in));
  }
  void addCutClass(frLayerNum lNum, std::unique_ptr<frLef58CutClass> in)
  {
    auto rptr = in.get();
    layer2Name2CutClass[lNum][in->getName()] = rptr;
    layerCutClass[lNum].push_back(std::move(in));
    layers[lNum]->addCutClass(rptr);
  }
  void addViaRuleGenerate(std::unique_ptr<frViaRuleGenerate> in)
  {
    name2viaRuleGenerate[in->getName()] = in.get();
    viaRuleGenerates.push_back(std::move(in));
  }
  void addUConstraint(std::unique_ptr<frConstraint> in)
  {
    in->setId(uConstraints.size());
    auto type = in->typeId();
    if (type == frConstraintTypeEnum::frcMinStepConstraint
        || type == frConstraintTypeEnum::frcLef58MinStepConstraint) {
      hasCornerSpacingConstraint_ = true;
    }
    uConstraints.push_back(std::move(in));
  }
  frConstraint* getConstraint(int idx)
  {
    if (idx < uConstraints.size())
      return uConstraints[idx].get();
    return nullptr;
  }
  void setUnidirectionalLayer(odb::dbTechLayer* dbLayer)
  {
    unidirectional_layers_.insert(dbLayer);
  }

  // forbidden length table related
  bool isVia2ViaForbiddenLen(int tableLayerIdx,
                             bool isPrevDown,
                             bool isCurrDown,
                             bool isCurrDirX,
                             frCoord len,
                             frNonDefaultRule* ndr = nullptr)
  {
    int tableEntryIdx = getTableEntryIdx(!isPrevDown, !isCurrDown, !isCurrDirX);
    return isIncluded((ndr ? ndr->via2ViaForbiddenLen
                           : via2ViaForbiddenLen)[tableLayerIdx][tableEntryIdx],
                      len);
  }

  // PRL table related
  bool isVia2ViaPRL(int tableLayerIdx,
                    bool isPrevDown,
                    bool isCurrDown,
                    bool isCurrDirX,
                    frCoord len)
  {
    int tableEntryIdx = getTableEntryIdx(!isPrevDown, !isCurrDown, !isCurrDirX);
    return len <= via2ViaPrlLen[tableLayerIdx][tableEntryIdx];
  }

  bool isViaForbiddenTurnLen(int tableLayerIdx,
                             bool isDown,
                             bool isCurrDirX,
                             frCoord len,
                             frNonDefaultRule* ndr = nullptr)
  {
    int tableEntryIdx = getTableEntryIdx(!isDown, !isCurrDirX);
    return isIncluded((ndr ? ndr->viaForbiddenTurnLen
                           : viaForbiddenTurnLen)[tableLayerIdx][tableEntryIdx],
                      len);
  }

  bool isLine2LineForbiddenLen(int tableLayerIdx,
                               bool isZShape,
                               bool isCurrDirX,
                               frCoord len)
  {
    int tableEntryIdx = getTableEntryIdx(!isZShape, !isCurrDirX);
    return isIncluded(line2LineForbiddenLen[tableLayerIdx][tableEntryIdx], len);
  }

  bool isViaForbiddenThrough(int tableLayerIdx, bool isDown, bool isCurrDirX)
  {
    int tableEntryIdx = getTableEntryIdx(!isDown, !isCurrDirX);
    return viaForbiddenThrough[tableLayerIdx][tableEntryIdx];
  }

  frViaDef* getVia(frString name) const { return name2via.at(name); }

  frViaRuleGenerate* getViaRule(frString name) const
  {
    return name2viaRuleGenerate.at(name);
  }

  void addNDR(std::unique_ptr<frNonDefaultRule> n)
  {
    nonDefaultRules.push_back(std::move(n));
  }

  const std::vector<std::unique_ptr<frNonDefaultRule>>& getNondefaultRules()
      const
  {
    return nonDefaultRules;
  }

  frNonDefaultRule* getNondefaultRule(std::string name)
  {
    for (std::unique_ptr<frNonDefaultRule>& nd : nonDefaultRules) {
      if (nd->getName() == name)
        return nd.get();
    }
    return nullptr;
  }

  frCoord getMaxNondefaultSpacing(int z)
  {
    frCoord spc = 0;
    for (std::unique_ptr<frNonDefaultRule>& nd : nonDefaultRules) {
      if (nd->getSpacing(z) > spc)
        spc = nd->getSpacing(z);
    }
    return spc;
  }

  frCoord getMaxNondefaultWidth(int z)
  {
    frCoord spc = 0;
    for (std::unique_ptr<frNonDefaultRule>& nd : nonDefaultRules) {
      if (nd->getWidth(z) > spc)
        spc = nd->getWidth(z);
    }
    return spc;
  }

  bool hasNondefaultRules() { return !nonDefaultRules.empty(); }

  // debug
  void printAllConstraints(utl::Logger* logger)
  {
    logger->report("Reporting layer properties.");
    for (auto& layer : layers) {
      auto type = layer->getType();
      if (type == dbTechLayerType::CUT)
        logger->report("Cut layer {}.", layer->getName());
      else if (type == dbTechLayerType::ROUTING)
        logger->report("Routing layer {}.", layer->getName());
      layer->printAllConstraints(logger);
    }
  }

  void printDefaultVias(Logger* logger)
  {
    logger->info(DRT, 167, "List of default vias:");
    for (auto& layer : layers) {
      if (layer->getType() == dbTechLayerType::CUT
          && layer->getLayerNum() >= BOTTOM_ROUTING_LAYER) {
        logger->report("  Layer {}", layer->getName());
        if (layer->getDefaultViaDef() != nullptr) {
          logger->report("    default via: {}",
                         layer->getDefaultViaDef()->getName());
        } else {
          logger->report("    default via: none");
        }
      }
    }
  }

  friend class io::Parser;
  void setVia2ViaMinStep(bool in) { hasVia2viaMinStep_ = in; }
  bool hasVia2ViaMinStep() const { return hasVia2viaMinStep_; }
  bool hasCornerSpacingConstraint() const
  {
    return hasCornerSpacingConstraint_;
  }

  bool isHorizontalLayer(frLayerNum l)
  {
    return getLayer(l)->getDir() == dbTechLayerDir::HORIZONTAL;
  }

  bool isVerticalLayer(frLayerNum l)
  {
    return getLayer(l)->getDir() == dbTechLayerDir::VERTICAL;
  }

 private:
  frUInt4 dbUnit;
  frUInt4 manufacturingGrid;

  std::map<frString, frLayer*> name2layer;
  std::vector<std::unique_ptr<frLayer>> layers;

  std::map<frString, frViaDef*> name2via;
  std::vector<std::unique_ptr<frViaDef>> vias;

  std::vector<std::map<frString, frLef58CutClass*>> layer2Name2CutClass;
  std::vector<std::vector<std::unique_ptr<frLef58CutClass>>> layerCutClass;

  std::map<frString, frViaRuleGenerate*> name2viaRuleGenerate;
  std::vector<std::unique_ptr<frViaRuleGenerate>> viaRuleGenerates;

  std::vector<std::unique_ptr<frConstraint>> uConstraints;
  std::vector<std::unique_ptr<frNonDefaultRule>> nonDefaultRules;

  template <typename T>
  using ByLayer = std::vector<T>;

  // via2ViaForbiddenLen[z][0], prev via is down, curr via is down, forbidden x
  // dist range (for non-shape-based rule)
  // via2ViaForbiddenLen[z][1], prev via is down, curr via is down, forbidden y
  // dist range (for non-shape-based rule)
  // via2ViaForbiddenLen[z][2], prev via is down, curr via is up,   forbidden x
  // dist range (for non-shape-based rule)
  // via2ViaForbiddenLen[z][3], prev via is down, curr via is up,   forbidden y
  // dist range (for non-shape-based rule)
  // via2ViaForbiddenLen[z][4], prev via is up,   curr via is down, forbidden x
  // dist range (for non-shape-based rule)
  // via2ViaForbiddenLen[z][5], prev via is up,   curr via is down, forbidden y
  // dist range (for non-shape-based rule)
  // via2ViaForbiddenLen[z][6], prev via is up,   curr via is up,   forbidden x
  // dist range (for non-shape-based rule)
  // via2ViaForbiddenLen[z][7], prev via is up,   curr via is up,   forbidden y
  // dist range (for non-shape-based rule)
  ByLayer<std::array<ForbiddenRanges, 8>> via2ViaForbiddenLen;

  ByLayer<std::array<frCoord, 8>> via2ViaPrlLen;

  // viaForbiddenTurnLen[z][0], last via is down, forbidden x dist range before
  // turn
  // viaForbiddenTurnLen[z][1], last via is down, forbidden y dist range before
  // turn
  // viaForbiddenTurnLen[z][2], last via is up,   forbidden x dist range before
  // turn
  // viaForbiddenTurnLen[z][3], last via is up,   forbidden y dist range before
  // turn
  ByLayer<std::array<ForbiddenRanges, 4>> viaForbiddenTurnLen;

  // viaForbiddenPlanarLen[z][0], last via is down, forbidden x dist range
  // viaForbiddenPlanarLen[z][1], last via is down, forbidden y dist range
  // viaForbiddenPlanarLen[z][2], last via is up,   forbidden x dist range
  // viaForbiddenPlanarLen[z][3], last via is up,   forbidden y dist range
  ByLayer<std::array<ForbiddenRanges, 4>> viaForbiddenPlanarLen;

  // line2LineForbiddenForbiddenLen[z][0], z shape, forbidden x dist range
  // line2LineForbiddenForbiddenLen[z][1], z shape, forbidden y dist range
  // line2LineForbiddenForbiddenLen[z][2], u shape, forbidden x dist range
  // line2LineForbiddenForbiddenLen[z][3], u shape, forbidden y dist range
  ByLayer<std::array<ForbiddenRanges, 4>> line2LineForbiddenLen;

  // viaForbiddenPlanarThrough[z][0], forbidden planar through along x direction
  // for down via
  // viaForbiddenPlanarThrough[z][1], forbidden planar through along y direction
  // for down via
  // viaForbiddenPlanarThrough[z][2], forbidden planar through along x direction
  // for up via
  // viaForbiddenPlanarThrough[z][3], forbidden planar through along y direction
  // for up via
  ByLayer<std::array<bool, 4>> viaForbiddenThrough;
  bool hasVia2viaMinStep_ = false;
  bool hasCornerSpacingConstraint_ = false;
  // unidirectional layers
  std::set<odb::dbTechLayer*> unidirectional_layers_;

  // forbidden length table related utilities
  int getTableEntryIdx(bool in1, bool in2)
  {
    int retIdx = 0;
    if (in1) {
      retIdx += 1;
    }
    retIdx <<= 1;
    if (in2) {
      retIdx += 1;
    }
    return retIdx;
  }

  int getTableEntryIdx(bool in1, bool in2, bool in3)
  {
    int retIdx = 0;
    if (in1) {
      retIdx += 1;
    }
    retIdx <<= 1;
    if (in2) {
      retIdx += 1;
    }
    retIdx <<= 1;
    if (in3) {
      retIdx += 1;
    }
    return retIdx;
  }

  bool isIncluded(const ForbiddenRanges& intervals, const frCoord len)
  {
    bool included = false;
    for (auto& interval : intervals) {
      if (interval.first <= len && interval.second >= len) {
        included = true;
        break;
      }
    }
    return included;
  }

  friend class FlexRP;
};
}  // namespace fr
