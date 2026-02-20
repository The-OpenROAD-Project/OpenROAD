// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "gr/FlexGRCMap.h"

#include <algorithm>
#include <climits>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <vector>

#include "db/obj/frBTerm.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frRPin.h"
#include "frBaseTypes.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

using odb::dbTechLayerType;

namespace drt {

void FlexGRCMap::initFrom3D(FlexGRCMap* cmap3D)
{
  // fake zMap
  zMap_[0] = odb::dbTechLayerDir::NONE;

  // resize cmap
  unsigned size = xgp_->getCount() * ygp_->getCount();
  bits_.resize(size, 0);

  // init supply / demand (from 3D cmap)
  unsigned zIdx = 0;
  for (auto& [layerIdx, dir] : cmap3D->getZMap()) {
    if (dir == odb::dbTechLayerDir::HORIZONTAL) {
      for (unsigned yIdx = 0; yIdx < ygp_->getCount(); yIdx++) {
        // non-transition via layer
        for (unsigned xIdx = 0; xIdx < xgp_->getCount(); xIdx++) {
          // supply
          if (!cmap3D->hasBlock(xIdx, yIdx, zIdx, frDirEnum::E)) {
            auto supply2D = getSupply(xIdx, yIdx, 0, frDirEnum::E);
            auto supply3D = cmap3D->getSupply(xIdx, yIdx, zIdx, frDirEnum::E);
            setSupply(xIdx, yIdx, 0, frDirEnum::E, supply2D + supply3D);
            // demand
            auto rawDemand2D = getRawDemand(xIdx, yIdx, 0, frDirEnum::E);
            auto rawDemand3D
                = cmap3D->getRawDemand(xIdx, yIdx, zIdx, frDirEnum::E);
            setRawDemand(
                xIdx, yIdx, 0, frDirEnum::E, rawDemand2D + rawDemand3D);
          }
        }
      }
    } else if (dir == odb::dbTechLayerDir::VERTICAL) {
      for (unsigned xIdx = 0; xIdx < xgp_->getCount(); xIdx++) {
        for (unsigned yIdx = 0; yIdx < ygp_->getCount(); yIdx++) {
          // supply
          if (!cmap3D->hasBlock(xIdx, yIdx, zIdx, frDirEnum::N)) {
            auto supply2D = getSupply(xIdx, yIdx, 0, frDirEnum::N);
            auto supply3D = cmap3D->getSupply(xIdx, yIdx, zIdx, frDirEnum::N);
            setSupply(xIdx, yIdx, 0, frDirEnum::N, supply2D + supply3D);
            // demand
            auto rawDemand2D = getRawDemand(xIdx, yIdx, 0, frDirEnum::N);
            auto rawDemand3D
                = cmap3D->getRawDemand(xIdx, yIdx, zIdx, frDirEnum::N);
            setRawDemand(
                xIdx, yIdx, 0, frDirEnum::N, rawDemand2D + rawDemand3D);
          }
        }
      }
    } else {
      std::cout << "Warning: unsupported routing direction for layerIdx = "
                << layerIdx << "\n";
    }
    zIdx++;
  }

  for (unsigned xIdx = 0; xIdx < xgp_->getCount(); xIdx++) {
    for (unsigned yIdx = 0; yIdx < ygp_->getCount(); yIdx++) {
      zIdx = 0;

      // block
      if (getRawDemand(xIdx, yIdx, 0, frDirEnum::E)
          >= getRawSupply(xIdx, yIdx, zIdx, frDirEnum::E)) {
        setBlock(xIdx, yIdx, 0, frDirEnum::E, true);
      }
      if (getRawDemand(xIdx, yIdx, 0, frDirEnum::N)
          >= getRawSupply(xIdx, yIdx, zIdx, frDirEnum::N)) {
        setBlock(xIdx, yIdx, 0, frDirEnum::N, true);
      }
    }
  }
}

void FlexGRCMap::init()
{
  // get cmap layer size
  unsigned layerSize = 0;
  for (unsigned layerIdx = 0; layerIdx < design_->getTech()->getLayers().size();
       layerIdx++) {
    auto layer = design_->getTech()->getLayer(layerIdx);
    if (layer->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    layerSize++;
    zMap_[layerIdx] = layer->getDir();
  }
  // resize cmap
  unsigned size = xgp_->getCount() * ygp_->getCount() * layerSize;
  numLayers_ = layerSize;
  bits_.resize(size, 0);

  // init supply (only for pref routing direction)
  unsigned cmapLayerIdx = 0;
  for (auto& [layerIdx, dir] : zMap_) {
    if (dir == odb::dbTechLayerDir::HORIZONTAL) {
      for (unsigned yIdx = 0; yIdx < ygp_->getCount(); yIdx++) {
        odb::Rect startGCellBox
            = design_->getTopBlock()->getGCellBox(odb::Point(0, yIdx));
        frCoord low = startGCellBox.yMin();
        frCoord high = startGCellBox.yMax();
        // non-transition via layer
        if (layerTrackPitches_[cmapLayerIdx] == layerPitches_[cmapLayerIdx]) {
          unsigned numTrack
              = getNumTracks(design_->getTopBlock()->getTrackPatterns(layerIdx),
                             true,
                             low,
                             high);
          for (unsigned xIdx = 0; xIdx < xgp_->getCount(); xIdx++) {
            setSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::E, numTrack);
          }
        } else {
          unsigned numTrack
              = getNumTracks(design_->getTopBlock()->getTrackPatterns(layerIdx),
                             true,
                             low,
                             high,
                             layerPitches_[cmapLayerIdx]);
          for (unsigned xIdx = 0; xIdx < xgp_->getCount(); xIdx++) {
            setSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::E, numTrack);
          }
        }
      }
    } else if (dir == odb::dbTechLayerDir::VERTICAL) {
      for (unsigned xIdx = 0; xIdx < xgp_->getCount(); xIdx++) {
        odb::Rect startGCellBox
            = design_->getTopBlock()->getGCellBox(odb::Point(xIdx, 0));
        frCoord low = startGCellBox.xMin();
        frCoord high = startGCellBox.xMax();
        if (layerTrackPitches_[cmapLayerIdx] == layerPitches_[cmapLayerIdx]) {
          unsigned numTrack
              = getNumTracks(design_->getTopBlock()->getTrackPatterns(layerIdx),
                             false,
                             low,
                             high);
          for (unsigned yIdx = 0; yIdx < ygp_->getCount(); yIdx++) {
            setSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::N, numTrack);
          }
        } else {
          unsigned numTrack
              = getNumTracks(design_->getTopBlock()->getTrackPatterns(layerIdx),
                             false,
                             low,
                             high,
                             layerPitches_[cmapLayerIdx]);
          for (unsigned yIdx = 0; yIdx < ygp_->getCount(); yIdx++) {
            setSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::N, numTrack);
          }
        }
      }
    } else {
      std::cout << "Warning: unsupported routing direction for layerIdx = "
                << layerIdx << "\n";
    }
    cmapLayerIdx++;
  }

  // update demand for fixed objects (only for pref routing direction)
  cmapLayerIdx = 0;
  std::set<frCoord> trackLocs;
  std::vector<rq_box_value_t<frBlockObject*>> queryResult;
  auto regionQuery = design_->getRegionQuery();
  unsigned numBlkTracks = 0;
  // layerIdx == tech layer num
  for (auto& [layerIdx, dir] : zMap_) {
    frCoord width = design_->getTech()->getLayer(layerIdx)->getWidth();
    if (dir == odb::dbTechLayerDir::HORIZONTAL) {
      for (unsigned yIdx = 0; yIdx < ygp_->getCount(); yIdx++) {
        trackLocs.clear();
        odb::Rect startGCellBox
            = design_->getTopBlock()->getGCellBox(odb::Point(0, yIdx));
        frCoord low = startGCellBox.yMin();
        frCoord high = startGCellBox.yMax();
        getTrackLocs(design_->getTopBlock()->getTrackPatterns(layerIdx),
                     true,
                     low,
                     high,
                     trackLocs);

        for (unsigned xIdx = 0; xIdx < xgp_->getCount(); xIdx++) {
          // add initial demand
          // addRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::E, 1);
          // add blocked track demand
          odb::Rect currGCellBox
              = design_->getTopBlock()->getGCellBox(odb::Point(xIdx, yIdx));
          queryResult.clear();
          regionQuery->query(currGCellBox, layerIdx, queryResult);
          numBlkTracks
              = getNumBlkTracks(true, layerIdx, trackLocs, queryResult, width);

          addDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::E, numBlkTracks);
        }
      }
    } else if (dir == odb::dbTechLayerDir::VERTICAL) {
      for (unsigned xIdx = 0; xIdx < xgp_->getCount(); xIdx++) {
        trackLocs.clear();
        odb::Rect startGCellBox
            = design_->getTopBlock()->getGCellBox(odb::Point(xIdx, 0));
        frCoord low = startGCellBox.xMin();
        frCoord high = startGCellBox.xMax();
        getTrackLocs(design_->getTopBlock()->getTrackPatterns(layerIdx),
                     false,
                     low,
                     high,
                     trackLocs);

        for (unsigned yIdx = 0; yIdx < ygp_->getCount(); yIdx++) {
          // add initial demand
          // addRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::N, 1);
          // add blocked track demand
          odb::Rect currGCellBox
              = design_->getTopBlock()->getGCellBox(odb::Point(xIdx, yIdx));
          queryResult.clear();
          regionQuery->query(currGCellBox, layerIdx, queryResult);
          numBlkTracks
              = getNumBlkTracks(false, layerIdx, trackLocs, queryResult, width);

          addDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::N, numBlkTracks);
        }
      }
    }
    cmapLayerIdx++;
  }

  // update demand for rpins (only for pref routing direction)
  cmapLayerIdx = 0;
  std::vector<rq_box_value_t<frRPin*>> rpinQueryResult;
  // layerIdx == tech layer num
  for (auto& [layerIdx, dir] : zMap_) {
    if (dir == odb::dbTechLayerDir::HORIZONTAL) {
      for (unsigned yIdx = 0; yIdx < ygp_->getCount(); yIdx++) {
        for (unsigned xIdx = 0; xIdx < xgp_->getCount(); xIdx++) {
          odb::Rect currGCellBox
              = design_->getTopBlock()->getGCellBox(odb::Point(xIdx, yIdx));
          rpinQueryResult.clear();
          regionQuery->queryRPin(currGCellBox, layerIdx, rpinQueryResult);

          unsigned numRPins = rpinQueryResult.size();

          if (layerIdx > router_cfg_->VIA_ACCESS_LAYERNUM) {
            addRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::E, numRPins);
          } else {
            addRawDemand(xIdx, yIdx, cmapLayerIdx + 1, frDirEnum::N, numRPins);
          }
        }
      }
    } else if (dir == odb::dbTechLayerDir::VERTICAL) {
      for (unsigned xIdx = 0; xIdx < xgp_->getCount(); xIdx++) {
        for (unsigned yIdx = 0; yIdx < ygp_->getCount(); yIdx++) {
          odb::Rect currGCellBox
              = design_->getTopBlock()->getGCellBox(odb::Point(xIdx, yIdx));
          rpinQueryResult.clear();
          regionQuery->queryRPin(currGCellBox, layerIdx, rpinQueryResult);

          unsigned numRPins = rpinQueryResult.size();

          if (layerIdx > router_cfg_->VIA_ACCESS_LAYERNUM) {
            addRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::N, numRPins);
          } else {
            addRawDemand(xIdx, yIdx, cmapLayerIdx + 1, frDirEnum::E, numRPins);
          }
        }
      }
    }
    cmapLayerIdx++;
  }

  // update blocked track
  cmapLayerIdx = 0;
  for (auto& [layerIdx, dir] : zMap_) {
    if (dir == odb::dbTechLayerDir::HORIZONTAL) {
      for (unsigned yIdx = 0; yIdx < ygp_->getCount(); yIdx++) {
        for (unsigned xIdx = 0; xIdx < xgp_->getCount(); xIdx++) {
          if (getRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::E)
              >= getRawSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::E)) {
            setBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::E, true);
          }
        }
      }
    } else if (dir == odb::dbTechLayerDir::VERTICAL) {
      for (unsigned xIdx = 0; xIdx < xgp_->getCount(); xIdx++) {
        for (unsigned yIdx = 0; yIdx < ygp_->getCount(); yIdx++) {
          if (getRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::N)
              >= getRawSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::N)) {
            setBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::N, true);
          }
        }
      }
    }
    cmapLayerIdx++;
  }
}

// TODO: separately deal with OBS to match TA behavior
unsigned FlexGRCMap::getNumBlkTracks(
    bool isHorz,
    frLayerNum lNum,
    const std::set<frCoord>& trackLocs,
    const std::vector<rq_box_value_t<frBlockObject*>>& results,
    const frCoord bloatDist = 0)
{
  std::set<frCoord> openTrackLocs = trackLocs;
  frCoord low, high;
  frCoord actBloatDist;
  auto layer = getDesign()->getTech()->getLayer(lNum);
  for (auto& [box, obj] : results) {
    actBloatDist = bloatDist;
    if (obj->typeId() == frcInstTerm) {
      odb::dbSigType sigType
          = static_cast<frInstTerm*>(obj)->getTerm()->getType();
      if (sigType.isSupply()) {
        actBloatDist = calcBloatDist(obj, lNum, box, false);
      }
    }
    if (obj->typeId() == frcBlockage || obj->typeId() == frcInstBlockage) {
      auto inst = (static_cast<frInstBlockage*>(obj))->getInst();
      if (inst->getMaster()->getMasterType() == odb::dbMasterType::BLOCK) {
        // actBloatDist = calcBloatDist(obj, lNum, boostB);
        // currently hack to prevent via EOL violation from above / below layer
        // (see TA prevention for prl)
        actBloatDist = 1.5 * layer->getPitch();
      }
    }

    if (openTrackLocs.empty()) {
      break;
    }
    if (isHorz) {
      low = box.yMin() - actBloatDist;
      high = box.yMax() + actBloatDist;
    } else {
      low = box.xMin() - actBloatDist;
      high = box.xMax() + actBloatDist;
    }
    auto iterLow = openTrackLocs.lower_bound(low);
    auto iterHigh = openTrackLocs.upper_bound(high);

    openTrackLocs.erase(iterLow, iterHigh);
  }
  unsigned numBlkTracks = trackLocs.size() - openTrackLocs.size();
  return numBlkTracks;
}

frCoord FlexGRCMap::calcBloatDist(frBlockObject* obj,
                                  const frLayerNum lNum,
                                  const odb::Rect& box,
                                  bool isOBS)
{
  auto layer = getDesign()->getTech()->getLayer(lNum);
  frCoord width = layer->getWidth();
  // use width if minSpc does not exist
  frCoord bloatDist = width;
  frCoord objWidth = std::min(box.xMax() - box.xMin(), box.yMax() - box.yMin());
  frCoord prl = (layer->getDir() == odb::dbTechLayerDir::HORIZONTAL)
                    ? (box.xMax() - box.xMin())
                    : (box.yMax() - box.yMin());
  if (obj->typeId() == frcBlockage || obj->typeId() == frcInstBlockage) {
    if (isOBS && router_cfg_->USEMINSPACING_OBS) {
      objWidth = width;
    }
  }
  auto con = getDesign()->getTech()->getLayer(lNum)->getMinSpacing();
  if (con) {
    if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
      bloatDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
    } else if (con->typeId()
               == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
      bloatDist
          = static_cast<frSpacingTablePrlConstraint*>(con)->find(objWidth, prl);
    } else if (con->typeId()
               == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
      bloatDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
          objWidth, width, prl);
    }
  }
  // assuming the wire width is width
  bloatDist += width / 2;
  return bloatDist;
}

unsigned FlexGRCMap::getNumPins(
    const std::vector<rq_box_value_t<frBlockObject*>>& results)
{
  unsigned numPins = 0;
  std::set<frBlockObject*> pins;
  for (auto& [box, obj] : results) {
    if (obj->typeId() == frcInstTerm) {
      auto instTerm = static_cast<frInstTerm*>(obj);
      if (instTerm->hasNet()) {
        pins.insert(obj);
      }
    } else if (obj->typeId() == frcBTerm) {
      auto term = static_cast<frBTerm*>(obj);
      if (term->hasNet()) {
        pins.insert(obj);
      }
    }
  }
  numPins = pins.size();
  return numPins;
}

void FlexGRCMap::getTrackLocs(
    const std::vector<std::unique_ptr<frTrackPattern>>& tps,
    bool isHorz,
    frCoord low,
    frCoord high,
    std::set<frCoord>& trackLocs)
{
  for (auto& tp : tps) {
    bool skip = true;
    if ((!isHorz && tp->isHorizontal()) || (isHorz && !tp->isHorizontal())) {
      skip = false;
    } else {
      skip = true;
    }

    if (skip) {
      continue;
    }

    int trackNum = (low - tp->getStartCoord()) / (int) tp->getTrackSpacing();
    trackNum = std::max(trackNum, 0);
    if (trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord() < low) {
      ++trackNum;
    }
    for (; trackNum < (int) tp->getNumTracks()
           && trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord()
                  <= high;
         ++trackNum) {
      frCoord trackLoc = trackNum * tp->getTrackSpacing() + tp->getStartCoord();
      trackLocs.insert(trackLoc);
    }
  }
}

// when line2ViaPitch != 0, the function is used to calculate estimated track
// numbers for transition layer
unsigned FlexGRCMap::getNumTracks(
    const std::vector<std::unique_ptr<frTrackPattern>>& tps,
    bool isHorz,
    frCoord low,
    frCoord high,
    frCoord line2ViaPitch)
{
  unsigned numTrack = 0;
  if (line2ViaPitch == 0) {
    for (auto& tp : tps) {
      bool skip = true;
      if ((!isHorz && tp->isHorizontal()) || (isHorz && !tp->isHorizontal())) {
        skip = false;
      } else {
        skip = true;
      }

      if (skip) {
        continue;
      }

      int trackNum = (low - tp->getStartCoord()) / (int) tp->getTrackSpacing();
      trackNum = std::max(trackNum, 0);
      if (trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord() < low) {
        ++trackNum;
      }
      for (; trackNum < (int) tp->getNumTracks()
             && trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord()
                    < high;
           trackNum++) {
        numTrack++;
      }
    }
  } else {
    frCoord startCoord = INT_MAX;
    for (auto& tp : tps) {
      bool skip = true;
      if ((!isHorz && tp->isHorizontal()) || (isHorz && !tp->isHorizontal())) {
        skip = false;
      } else {
        skip = true;
      }

      if (skip) {
        continue;
      }

      int offset = (tp->getStartCoord() - low) % (int) tp->getTrackSpacing();
      if (offset < 0) {
        offset += (int) tp->getTrackSpacing();
      }
      offset += low;

      startCoord = std::min(offset, startCoord);
    }
    if (startCoord != INT_MAX) {
      numTrack += (high - startCoord) / line2ViaPitch;
      if ((high - startCoord) % line2ViaPitch == 0) {
        numTrack--;
      }
    }
  }
  return numTrack;
}

void FlexGRCMap::printLayers()
{
  std::cout << "start printing layers in CMap\n";

  for (auto& [layerNum, dir] : zMap_) {
    std::cout << "  layerNum = " << layerNum << " dir = ";
    if (dir == odb::dbTechLayerDir::HORIZONTAL) {
      std::cout << "H";
    } else if (dir == odb::dbTechLayerDir::VERTICAL) {
      std::cout << "V";
    }
    std::cout << '\n';
  }
}

void FlexGRCMap::print(bool isAll)
{
  std::ofstream congMap;
  std::cout << "printing congestion map...\n";

  if (!router_cfg_->CMAP_FILE.empty()) {
    congMap.open(router_cfg_->CMAP_FILE.c_str());
  }

  if (congMap.is_open()) {
    congMap << "#     Area              demand/supply tracks\n";
  } else {
    std::cout << "#     Area              demand/supply tracks\n";
  }

  unsigned layerIdx = 0;
  for (auto& [layerNum, dir] : zMap_) {
    if (congMap.is_open()) {
      congMap << "----------------------"
              << design_->getTech()->getLayer(layerNum)->getName()
              << "----------------------\n";
    } else {
      std::cout << "----------------------"
                << design_->getTech()->getLayer(layerNum)->getName()
                << "----------------------\n";
    }
    for (unsigned yIdx = 0; yIdx < ygp_->getCount(); yIdx++) {
      for (unsigned xIdx = 0; xIdx < xgp_->getCount(); xIdx++) {
        odb::Rect gcellBox
            = design_->getTopBlock()->getGCellBox(odb::Point(xIdx, yIdx));
        unsigned demandV = getDemand(xIdx, yIdx, layerIdx, frDirEnum::N);
        unsigned demandH = getDemand(xIdx, yIdx, layerIdx, frDirEnum::E);
        unsigned supplyV = getSupply(xIdx, yIdx, layerIdx, frDirEnum::N);
        unsigned supplyH = getSupply(xIdx, yIdx, layerIdx, frDirEnum::E);

        if (isAll || (demandV > supplyV) || (demandH > supplyH)) {
          if (congMap.is_open()) {
            congMap << "(" << gcellBox.xMin() << ", " << gcellBox.yMin()
                    << ") (" << gcellBox.xMax() << ", " << gcellBox.yMax()
                    << ")" << " V: " << demandV << "/" << supplyV
                    << " H: " << demandH << "/" << supplyH << "\n";
          } else {
            std::cout << "(" << gcellBox.xMin() << ", " << gcellBox.yMin()
                      << ") (" << gcellBox.xMax() << ", " << gcellBox.yMax()
                      << ")" << " V: " << demandV << "/" << supplyV
                      << " H: " << demandH << "/" << supplyH << "\n";
          }
        }
      }
    }
    layerIdx++;
  }
  if (congMap.is_open()) {
    congMap.close();
  }
}

void FlexGRCMap::print2D(bool isAll)
{
  std::cout << "printing 2D congestion map...\n";
  std::ofstream congMap;
  if (!router_cfg_->CMAP_FILE.empty()) {
    congMap.open(router_cfg_->CMAP_FILE.c_str());
  }

  if (congMap.is_open()) {
    congMap << "#     Area              demand/supply tracks\n";
  } else {
    std::cout << "#     Area              demand/supply tracks\n";
  }

  for (unsigned yIdx = 0; yIdx < ygp_->getCount(); yIdx++) {
    for (unsigned xIdx = 0; xIdx < xgp_->getCount(); xIdx++) {
      unsigned demandH = 0;
      unsigned demandV = 0;
      unsigned supplyH = 0;
      unsigned supplyV = 0;

      for (unsigned layerIdx = 0; layerIdx < zMap_.size(); ++layerIdx) {
        demandH += getDemand(xIdx, yIdx, layerIdx, frDirEnum::E);
        demandV += getDemand(xIdx, yIdx, layerIdx, frDirEnum::N);
        supplyH += getSupply(xIdx, yIdx, layerIdx, frDirEnum::E);
        supplyV += getSupply(xIdx, yIdx, layerIdx, frDirEnum::N);
      }

      odb::Rect gcellBox
          = design_->getTopBlock()->getGCellBox(odb::Point(xIdx, yIdx));
      if (isAll || (demandV > supplyV) || (demandH > supplyH)) {
        if (congMap.is_open()) {
          congMap << "(" << gcellBox.xMin() << ", " << gcellBox.yMin() << ") ("
                  << gcellBox.xMax() << ", " << gcellBox.yMax() << ")"
                  << " V: " << demandV << "/" << supplyV << " H: " << demandH
                  << "/" << supplyH << "\n";
        } else {
          std::cout << "(" << gcellBox.xMin() << ", " << gcellBox.yMin()
                    << ") (" << gcellBox.xMax() << ", " << gcellBox.yMax()
                    << ")" << " V: " << demandV << "/" << supplyV
                    << " H: " << demandH << "/" << supplyH << "\n";
        }
      }
    }
  }
  if (congMap.is_open()) {
    congMap.close();
  }
}

}  // namespace drt
