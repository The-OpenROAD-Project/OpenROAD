// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "db/obj/frVia.h"
#include "db/tech/frConstraint.h"
#include "db/tech/frLayer.h"
#include "db/tech/frViaDef.h"
#include "db/tech/frViaRuleGenerate.h"
#include "frBaseTypes.h"
#include "global.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace odb {
class dbTechLayer;
}
namespace drt {
namespace io {
class Parser;
}

class frTechObject
{
 public:
  // getters
  frUInt4 getDBUPerUU() const { return dbUnit_; }
  frUInt4 getManufacturingGrid() const { return manufacturingGrid_; }
  frLayer* getLayer(const frString& name) const
  {
    auto it = name2layer_.find(name);
    if (it == name2layer_.end()) {
      return nullptr;
    }
    return it->second;
  }
  frLayer* getLayer(frLayerNum in) const { return layers_.at(in).get(); }
  frLayerNum getBottomLayerNum() const { return 0; }
  frLayerNum getTopLayerNum() const { return layers_.size() - 1; }
  const std::vector<std::unique_ptr<frLayer>>& getLayers() const
  {
    return layers_;
  }

  const std::vector<std::unique_ptr<frViaDef>>& getVias() const
  {
    return vias_;
  }
  const std::vector<std::unique_ptr<frViaRuleGenerate>>& getViaRuleGenerates()
      const
  {
    return viaRuleGenerates_;
  }
  bool hasMaxSpacingConstraints() const
  {
    for (const auto& layer : layers_) {
      if (layer->hasLef58MaxSpacingConstraints()) {
        return true;
      }
    }
    return false;
  }

  // setters
  void setDBUPerUU(frUInt4 uIn) { dbUnit_ = uIn; }
  void setManufacturingGrid(frUInt4 in) { manufacturingGrid_ = in; }
  void addLayer(std::unique_ptr<frLayer> in)
  {
    name2layer_[in->getName()] = in.get();
    layers_.push_back(std::move(in));
    layer2Name2CutClass_.emplace_back();
    layerCutClass_.emplace_back();
  }
  frViaDef* addVia(std::unique_ptr<frViaDef> in)
  {
    in->setId(vias_.size());
    if (name2via_.find(in->getName()) != name2via_.end()) {
      if (*(name2via_[in->getName()]) == *in) {
        return name2via_[in->getName()];
      }
      return nullptr;
    }
    frViaDef* rptr = in.get();
    name2via_[in->getName()] = in.get();
    vias_.push_back(std::move(in));
    return rptr;
  }
  void addCutClass(frLayerNum lNum, std::unique_ptr<frLef58CutClass> in)
  {
    auto rptr = in.get();
    layer2Name2CutClass_[lNum][in->getName()] = rptr;
    layerCutClass_[lNum].push_back(std::move(in));
    layers_[lNum]->addCutClass(rptr);
  }
  void addViaRuleGenerate(std::unique_ptr<frViaRuleGenerate> in)
  {
    name2viaRuleGenerate_[in->getName()] = in.get();
    viaRuleGenerates_.push_back(std::move(in));
  }
  void addUConstraint(std::unique_ptr<frConstraint> in)
  {
    in->setId(uConstraints_.size());
    auto type = in->typeId();
    if (type == frConstraintTypeEnum::frcMinStepConstraint
        || type == frConstraintTypeEnum::frcLef58MinStepConstraint) {
      hasCornerSpacingConstraint_ = true;
    }
    uConstraints_.push_back(std::move(in));
  }
  frConstraint* getConstraint(int idx)
  {
    if (idx < uConstraints_.size()) {
      return uConstraints_[idx].get();
    }
    return nullptr;
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
    return isIncluded(
        (ndr ? ndr->via2ViaForbiddenLen_
             : via2ViaForbiddenLen_)[tableLayerIdx][tableEntryIdx],
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
    return len <= via2ViaPrlLen_[tableLayerIdx][tableEntryIdx];
  }

  bool isViaForbiddenTurnLen(int tableLayerIdx,
                             bool isDown,
                             bool isCurrDirX,
                             frCoord len,
                             frNonDefaultRule* ndr = nullptr)
  {
    int tableEntryIdx = getTableEntryIdx(!isDown, !isCurrDirX);
    return isIncluded(
        (ndr ? ndr->viaForbiddenTurnLen_
             : viaForbiddenTurnLen_)[tableLayerIdx][tableEntryIdx],
        len);
  }

  bool isLine2LineForbiddenLen(int tableLayerIdx,
                               bool isZShape,
                               bool isCurrDirX,
                               frCoord len)
  {
    int tableEntryIdx = getTableEntryIdx(!isZShape, !isCurrDirX);
    return isIncluded(line2LineForbiddenLen_[tableLayerIdx][tableEntryIdx],
                      len);
  }

  bool isViaForbiddenThrough(int tableLayerIdx, bool isDown, bool isCurrDirX)
  {
    int tableEntryIdx = getTableEntryIdx(!isDown, !isCurrDirX);
    return viaForbiddenThrough_[tableLayerIdx][tableEntryIdx];
  }

  frViaDef* getVia(const frString& name) const { return name2via_.at(name); }

  frViaRuleGenerate* getViaRule(const frString& name) const
  {
    return name2viaRuleGenerate_.at(name);
  }

  void addNDR(std::unique_ptr<frNonDefaultRule> n)
  {
    nonDefaultRules_.push_back(std::move(n));
  }

  const std::vector<std::unique_ptr<frNonDefaultRule>>& getNondefaultRules()
      const
  {
    return nonDefaultRules_;
  }

  frNonDefaultRule* getNondefaultRule(const std::string& name)
  {
    for (std::unique_ptr<frNonDefaultRule>& nd : nonDefaultRules_) {
      if (nd->getName() == name) {
        return nd.get();
      }
    }
    return nullptr;
  }

  frCoord getMaxNondefaultSpacing(int z)
  {
    frCoord spc = 0;
    for (std::unique_ptr<frNonDefaultRule>& nd : nonDefaultRules_) {
      spc = std::max(nd->getSpacing(z), spc);
    }
    return spc;
  }

  frCoord getMaxNondefaultWidth(int z)
  {
    frCoord spc = 0;
    for (std::unique_ptr<frNonDefaultRule>& nd : nonDefaultRules_) {
      spc = std::max(nd->getWidth(z), spc);
    }
    return spc;
  }

  bool hasNondefaultRules() { return !nonDefaultRules_.empty(); }

  // debug
  void printAllConstraints(utl::Logger* logger)
  {
    logger->report("Reporting layer properties.");
    for (auto& layer : layers_) {
      auto type = layer->getType();
      if (type == odb::dbTechLayerType::CUT) {
        logger->report("Cut layer {}.", layer->getName());
      } else if (type == odb::dbTechLayerType::ROUTING) {
        logger->report("Routing layer {}.", layer->getName());
      }
      layer->printAllConstraints(logger);
    }
  }

  void printDefaultVias(utl::Logger* logger, RouterConfiguration* router_cfg)
  {
    logger->info(DRT, 167, "List of default vias:");
    for (auto& layer : layers_) {
      if (layer->getType() == odb::dbTechLayerType::CUT
          && layer->getLayerNum() >= router_cfg->BOTTOM_ROUTING_LAYER) {
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
    return getLayer(l)->getDir() == odb::dbTechLayerDir::HORIZONTAL;
  }

  bool isVerticalLayer(frLayerNum l)
  {
    return getLayer(l)->getDir() == odb::dbTechLayerDir::VERTICAL;
  }

 private:
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

  frUInt4 dbUnit_{0};
  frUInt4 manufacturingGrid_{0};

  std::map<frString, frLayer*> name2layer_;
  std::vector<std::unique_ptr<frLayer>> layers_;

  std::map<frString, frViaDef*> name2via_;
  std::vector<std::unique_ptr<frViaDef>> vias_;

  std::vector<std::map<frString, frLef58CutClass*>> layer2Name2CutClass_;
  std::vector<std::vector<std::unique_ptr<frLef58CutClass>>> layerCutClass_;

  std::map<frString, frViaRuleGenerate*> name2viaRuleGenerate_;
  std::vector<std::unique_ptr<frViaRuleGenerate>> viaRuleGenerates_;

  std::vector<std::unique_ptr<frConstraint>> uConstraints_;
  std::vector<std::unique_ptr<frNonDefaultRule>> nonDefaultRules_;

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
  ByLayer<std::array<ForbiddenRanges, 8>> via2ViaForbiddenLen_;

  ByLayer<std::array<frCoord, 8>> via2ViaPrlLen_;

  // viaForbiddenTurnLen[z][0], last via is down, forbidden x dist range before
  // turn
  // viaForbiddenTurnLen[z][1], last via is down, forbidden y dist range before
  // turn
  // viaForbiddenTurnLen[z][2], last via is up,   forbidden x dist range before
  // turn
  // viaForbiddenTurnLen[z][3], last via is up,   forbidden y dist range before
  // turn
  ByLayer<std::array<ForbiddenRanges, 4>> viaForbiddenTurnLen_;

  // viaForbiddenPlanarLen[z][0], last via is down, forbidden x dist range
  // viaForbiddenPlanarLen[z][1], last via is down, forbidden y dist range
  // viaForbiddenPlanarLen[z][2], last via is up,   forbidden x dist range
  // viaForbiddenPlanarLen[z][3], last via is up,   forbidden y dist range
  ByLayer<std::array<ForbiddenRanges, 4>> viaForbiddenPlanarLen_;

  // line2LineForbiddenForbiddenLen[z][0], z shape, forbidden x dist range
  // line2LineForbiddenForbiddenLen[z][1], z shape, forbidden y dist range
  // line2LineForbiddenForbiddenLen[z][2], u shape, forbidden x dist range
  // line2LineForbiddenForbiddenLen[z][3], u shape, forbidden y dist range
  ByLayer<std::array<ForbiddenRanges, 4>> line2LineForbiddenLen_;

  // viaForbiddenPlanarThrough[z][0], forbidden planar through along x direction
  // for down via
  // viaForbiddenPlanarThrough[z][1], forbidden planar through along y direction
  // for down via
  // viaForbiddenPlanarThrough[z][2], forbidden planar through along x direction
  // for up via
  // viaForbiddenPlanarThrough[z][3], forbidden planar through along y direction
  // for up via
  ByLayer<std::array<bool, 4>> viaForbiddenThrough_;
  bool hasVia2viaMinStep_ = false;
  bool hasCornerSpacingConstraint_ = false;

  friend class FlexRP;
};
}  // namespace drt
