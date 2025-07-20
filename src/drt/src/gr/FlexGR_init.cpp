/* Authors: Lutong Wang and Bangqi Xu */
/* Updated version:  Zhiang Wang*/
/*
 * Copyright (c) 2025, The Regents of the University of California
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

#include <deque>
#include <iostream>
#include <vector>

#include "FlexGR.h"
#include "FlexGRCMap.h"

namespace drt {

void FlexGR::init()
{
  initGCell();
  initLayerPitch();
  initCMap();
}


void FlexGR::initGCell()
{
  auto& gcellPatterns = block_->getGCellPatterns();
  if (!gcellPatterns.empty()) { return; } // already initialized
  // Originallly, we use M1 as the default layer for GCell
  // We use M2 as the default layer for GCell 
  const auto layer = design_->getTech()->getLayer(4); // 4 for M2, 2 for M1
  const auto pitch = layer->getPitch();
  logger_->info(utl::DRT, 83,
      "\nGenerating GCell with size = 15 tracks, using layer {} pitch = {}",
      layer->getName(),
      pitch / static_cast<double>(block_->getDBUPerUU()));
  
  const Rect dieBox = block_->getDieBox();
  frCoord startCoordX = dieBox.xMin() - tech_->getManufacturingGrid();
  frCoord startCoordY = dieBox.yMin() - tech_->getManufacturingGrid();
  frCoord gcellGridSize = pitch * 15; // 15 tracks per GCell
  if (gcellGridSize == 0) {
    logger_->error(utl::DRT, 224,
        "Error: GCell grid size is zero for layer {} with pitch {}",
        layer->getName(),
        pitch / static_cast<double>(block_->getDBUPerUU()));
  }

  frGCellPattern xgp, ygp;
  // set xgp
  xgp.setHorizontal(false);
  xgp.setStartCoord(startCoordX);
  xgp.setSpacing(gcellGridSize);
  xgp.setCount((dieBox.xMax() - startCoordX) / gcellGridSize);
  // set ygp
  ygp.setHorizontal(true);
  ygp.setStartCoord(startCoordY);
  ygp.setSpacing(gcellGridSize);
  ygp.setCount((dieBox.yMax() - startCoordY) / gcellGridSize);
  block_->setGCellPatterns({xgp, ygp});
}


void FlexGR::initLayerPitch()
{
  int numRoutingLayer = 0;
  auto& layers = tech_->getLayers();
  for (auto& layer : layers) {
    if (layer->getType() == dbTechLayerType::ROUTING) {
      numRoutingLayer++;
    }
  }

  // init pitches
  trackPitches_.resize(numRoutingLayer, -1);
  line2ViaPitches_.resize(numRoutingLayer, -1);
  layerPitches_.resize(numRoutingLayer, -1);
  zHeights_.resize(numRoutingLayer, 0);

  int bottomRoutingLayerNum = tech_->getBottomLayerNum();
  int topRoutingLayerNum = tech_->getTopLayerNum();
  // Layer 0: active layer,  Layer 1: Fr_VIA layer
  // M1 is layer 2, M2 is layer 4, etc.
  int zIdx = 0;
  for (auto& layer : layers) {
    if (layer->getType() != dbTechLayerType::ROUTING) {
      continue;
    }

    const int lNum = (zIdx + 1) * 2;
    bool isLayerHorz = (layer->getDir() == dbTechLayerDir::HORIZONTAL);
    // get track pitch (pick the smallest one from all the track patterns at current layer)
    for (auto& tp : block_->getTrackPatterns(lNum)) {
      if ((isLayerHorz && !tp->isHorizontal())
          || (!isLayerHorz && tp->isHorizontal())) {
        if (trackPitches_[zIdx] == -1
            || (int) tp->getTrackSpacing() < trackPitches_[zIdx]) {
          trackPitches_[zIdx] = tp->getTrackSpacing();
        }
      }
    }

    // calculate line-2-via pitch
    frCoord defaultWidth = layer->getWidth();
    frCoord minLine2ViaPitch = -1;

    // Make sure that you are using C++17 or later
    auto processVia = [&](const frViaDef* viaDef, bool isUpVia) -> void{
      if (viaDef == nullptr) { return; }
      frVia via(viaDef);
      Rect viaBox = isUpVia ? via.getLayer1BBox() : via.getLayer2BBox();
      frCoord enclosureWidth = viaBox.minDXDY();
      frCoord prl = isLayerHorz ? (viaBox.xMax() - viaBox.xMin())
                                : (viaBox.yMax() - viaBox.yMin());
      // calculate minNonOvlpDist
      frCoord minNonOvlpDist = defaultWidth / 2;
      minNonOvlpDist += isLayerHorz ? (viaBox.yMax() - viaBox.yMin()) / 2
                                    : (viaBox.xMax() - viaBox.xMin()) / 2;
      
      // calculate minSpc required val
      frCoord minReqDist = std::numeric_limits<int>::min();
      auto con = layer->getMinSpacing();
      if (con) {
        switch (con->typeId()) {
          case frConstraintTypeEnum::frcSpacingConstraint:
            minReqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
            break;
          case frConstraintTypeEnum::frcSpacingTablePrlConstraint:
            minReqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
                std::max(enclosureWidth, defaultWidth), prl);
            break;
          case frConstraintTypeEnum::frcSpacingTableTwConstraint:
            minReqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
                enclosureWidth, defaultWidth, prl);
            break;
          default:
            break;
        }
      }

      if (minReqDist != std::numeric_limits<int>::min()) {
        minReqDist += minNonOvlpDist;
        if (line2ViaPitches_[zIdx] == -1 || minReqDist > line2ViaPitches_[zIdx]) {
          line2ViaPitches_[zIdx] = minReqDist;
        }

        if (minLine2ViaPitch == -1 || minReqDist < minLine2ViaPitch) {
          minLine2ViaPitch = minReqDist;
        }
      }
    };

    // calculate line-2-via pitch
    if (bottomRoutingLayerNum <= lNum - 1) {
      processVia(tech_->getLayer(lNum - 1)->getDefaultViaDef(), false); // down via
    }
    
    if (topRoutingLayerNum >= lNum + 1) {
      processVia(tech_->getLayer(lNum + 1)->getDefaultViaDef(), true); // up via
    }

    // Zhiang: I do not understand the logic here
    // Should we use line2ViaPitches_[zIdx] instead of minLine2ViaPitch
    // I think we should use the maximum of line2ViaPitches_[zIdx] and trackPitches_[zIdx]
    // Original implementation:
    // if (minLine2ViaPitch > trackPitches_[zIdx]) {
    //  layerPitches_[zIdx] = line2ViaPitches_[zIdx];
    // } else {
    //  layerPitches_[zIdx] = trackPitches_[zIdx];
    //}
    // For the NG45, I notice that the line2ViaPitches_[zIdx] is smaller than trackPitches_[zIdx]
    // Updated by Zhiang:
    layerPitches_[zIdx] = std::max(line2ViaPitches_[zIdx], trackPitches_[zIdx]);
    // Zhiang: we need to understand how the zHeights_ are used
    zHeights_[zIdx] = zIdx == 0 ? layerPitches_[zIdx]
                                : zHeights_[zIdx - 1] + layerPitches_[zIdx];
    
    // print the layer information
    std::string orientation = isLayerHorz ? "H" : "V";
    double layerPitchUU = layer->getPitch() / static_cast<double>(block_->getDBUPerUU());
    double trackPitchUU = trackPitches_[zIdx] / static_cast<double>(block_->getDBUPerUU());
    double line2ViaPitchUU = line2ViaPitches_[zIdx] / static_cast<double>(block_->getDBUPerUU());
    std::string warningMsg = "";
    if (trackPitches_[zIdx] < line2ViaPitches_[zIdx]) {
      warningMsg = "(Warning: Track pitch is smaller than line-2-via pitch!)";
    } 
    logger_->info(utl::DRT, 77,
                  "Layer {}: layerNum = {}, zIdx = {}, Preferred dirction = {}, "
                  "Layer Pitch = {:0.5f}, Track Pitch = {:0.5f}, Line-2-Via Pitch = {:0.5f} "
                  "{}",
                  layer->getName(), lNum, zIdx, orientation, 
                  layerPitchUU, trackPitchUU, line2ViaPitchUU, warningMsg);
    // Move to the next layer
    zIdx++;
  }
}

void FlexGR::initCMap()
{
  logger_->info(utl::DRT, 225, "Initializing congestion map...");
  auto cmap = std::make_unique<FlexGRCMap>(design_, router_cfg_, logger_);
  cmap->setLayerTrackPitches(trackPitches_);
  cmap->setLayerLine2ViaPitches(line2ViaPitches_);
  cmap->setLayerPitches(layerPitches_);
  cmap->init();
  auto cmap2D = std::make_unique<FlexGRCMap>(design_, router_cfg_, logger_);
  cmap2D->initFrom3D(cmap.get());
  setCMap(cmap);
  setCMap2D(cmap2D);
}




// Todo: 
// The following functions are just for debugging and printing purposes
// We will delete them in the release version

void FlexGRWorker::initNets_printNets()
{
  std::cout << std::endl << "printing grNets\n";
  for (auto& net : nets_) {
    initNets_printNet(net.get());
  }
}

void FlexGRWorker::initNets_printNet(grNet* net)
{
  auto root = net->getRoot();
  std::deque<grNode*> nodeQ;
  nodeQ.push_back(root);

  std::cout << "start traversing subnet of " << net->getFrNet()->getName()
            << std::endl;

  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();

    std::cout << "  ";
    if (node->getType() == frNodeTypeEnum::frcSteiner) {
      std::cout << "steiner node ";
    } else if (node->getType() == frNodeTypeEnum::frcPin) {
      std::cout << "pin node ";
    } else if (node->getType() == frNodeTypeEnum::frcBoundaryPin) {
      std::cout << "boundary pin node ";
    }
    std::cout << "at ";
    Point loc = node->getLoc();
    frLayerNum lNum = node->getLayerNum();
    std::cout << "(" << loc.x() << ", " << loc.y() << ") on layerNum " << lNum
              << std::endl;

    for (auto child : node->getChildren()) {
      nodeQ.push_back(child);
    }
  }
}

void FlexGRWorker::initNets_printFNets(
    std::map<frNet*, std::vector<frNode*>, frBlockObjectComp>& netRoots)
{
  std::cout << std::endl << "printing frNets\n";
  for (auto& [net, roots] : netRoots) {
    for (auto root : roots) {
      initNets_printFNet(root);
    }
  }
}

void FlexGRWorker::initNets_printFNet(frNode* root)
{
  std::deque<frNode*> nodeQ;
  nodeQ.push_back(root);

  std::cout << "start traversing subnet of " << root->getNet()->getName()
            << std::endl;

  bool isRoot = true;

  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();

    std::cout << "  ";
    if (node->getType() == frNodeTypeEnum::frcSteiner) {
      std::cout << "steiner node ";
    } else if (node->getType() == frNodeTypeEnum::frcPin) {
      std::cout << "pin node ";
    } else if (node->getType() == frNodeTypeEnum::frcBoundaryPin) {
      std::cout << "boundary pin node ";
    }
    std::cout << "at ";
    Point loc = node->getLoc();
    frLayerNum lNum = node->getLayerNum();
    std::cout << "(" << loc.x() << ", " << loc.y() << ") on layerNum " << lNum
              << std::endl;

    if (isRoot || node->getType() == frNodeTypeEnum::frcSteiner) {
      for (auto child : node->getChildren()) {
        nodeQ.push_back(child);
      }
    }

    isRoot = false;
  }
}


}  // namespace drt
