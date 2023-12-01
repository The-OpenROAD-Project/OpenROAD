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

#include <chrono>
#include <iostream>
#include <sstream>

#include "FlexPA.h"
#include "db/infra/frTime.h"
#include "gc/FlexGC.h"

using namespace fr;

void FlexPA::initViaRawPriority()
{
  for (auto layerNum = design_->getTech()->getBottomLayerNum();
       layerNum <= design_->getTech()->getTopLayerNum();
       ++layerNum) {
    if (design_->getTech()->getLayer(layerNum)->getType()
        != dbTechLayerType::CUT) {
      continue;
    }
    for (auto& viaDef : design_->getTech()->getLayer(layerNum)->getViaDefs()) {
      const int cutNum = int(viaDef->getCutFigs().size());
      ViaRawPriorityTuple priority;
      getViaRawPriority(viaDef, priority);
      layerNum2ViaDefs_[layerNum][cutNum][priority] = viaDef;
    }
  }
}

void FlexPA::getViaRawPriority(frViaDef* viaDef, ViaRawPriorityTuple& priority)
{
  const bool isNotDefaultVia = !(viaDef->getDefault());
  gtl::polygon_90_set_data<frCoord> viaLayerPS1;

  for (auto& fig : viaDef->getLayer1Figs()) {
    const Rect bbox = fig->getBBox();
    gtl::rectangle_data<frCoord> bboxRect(
        bbox.xMin(), bbox.yMin(), bbox.xMax(), bbox.yMax());
    using namespace boost::polygon::operators;
    viaLayerPS1 += bboxRect;
  }
  gtl::rectangle_data<frCoord> layer1Rect;
  gtl::extents(layer1Rect, viaLayerPS1);
  const bool isLayer1Horz = (gtl::xh(layer1Rect) - gtl::xl(layer1Rect))
                            > (gtl::yh(layer1Rect) - gtl::yl(layer1Rect));
  const frCoord layer1Width
      = std::min((gtl::xh(layer1Rect) - gtl::xl(layer1Rect)),
                 (gtl::yh(layer1Rect) - gtl::yl(layer1Rect)));

  const auto layer1Num = viaDef->getLayer1Num();
  const auto dir1 = getDesign()->getTech()->getLayer(layer1Num)->getDir();

  const bool isNotLowerAlign
      = (isLayer1Horz && (dir1 == dbTechLayerDir::VERTICAL))
        || (!isLayer1Horz && (dir1 == dbTechLayerDir::HORIZONTAL));

  gtl::polygon_90_set_data<frCoord> viaLayerPS2;
  for (auto& fig : viaDef->getLayer2Figs()) {
    const Rect bbox = fig->getBBox();
    const gtl::rectangle_data<frCoord> bboxRect(
        bbox.xMin(), bbox.yMin(), bbox.xMax(), bbox.yMax());
    using namespace boost::polygon::operators;
    viaLayerPS2 += bboxRect;
  }
  gtl::rectangle_data<frCoord> layer2Rect;
  gtl::extents(layer2Rect, viaLayerPS2);
  const bool isLayer2Horz = (gtl::xh(layer2Rect) - gtl::xl(layer2Rect))
                            > (gtl::yh(layer2Rect) - gtl::yl(layer2Rect));
  const frCoord layer2Width
      = std::min((gtl::xh(layer2Rect) - gtl::xl(layer2Rect)),
                 (gtl::yh(layer2Rect) - gtl::yl(layer2Rect)));

  const auto layer2Num = viaDef->getLayer2Num();
  const auto dir2 = getDesign()->getTech()->getLayer(layer2Num)->getDir();

  const bool isNotUpperAlign
      = (isLayer2Horz && (dir2 == dbTechLayerDir::VERTICAL))
        || (!isLayer2Horz && (dir2 == dbTechLayerDir::HORIZONTAL));

  const frCoord layer1Area = gtl::area(viaLayerPS1);
  const frCoord layer2Area = gtl::area(viaLayerPS2);

  priority = std::make_tuple(isNotDefaultVia,
                             layer1Width,
                             layer2Width,
                             isNotUpperAlign,
                             layer2Area,
                             layer1Area,
                             isNotLowerAlign);
}

void FlexPA::initTrackCoords()
{
  const int numLayers = getDesign()->getTech()->getLayers().size();
  const frCoord manuGrid = getDesign()->getTech()->getManufacturingGrid();

  // full coords
  trackCoords_.clear();
  trackCoords_.resize(numLayers);
  for (auto& trackPattern : design_->getTopBlock()->getTrackPatterns()) {
    const auto layerNum = trackPattern->getLayerNum();
    const auto isVLayer = (design_->getTech()->getLayer(layerNum)->getDir()
                           == dbTechLayerDir::VERTICAL);
    const auto isVTrack = trackPattern->isHorizontal();  // yes = vertical track
    if ((!isVLayer && !isVTrack) || (isVLayer && isVTrack)) {
      frCoord currCoord = trackPattern->getStartCoord();
      for (int i = 0; i < (int) trackPattern->getNumTracks(); i++) {
        trackCoords_[layerNum][currCoord] = frAccessPointEnum::OnGrid;
        currCoord += trackPattern->getTrackSpacing();
      }
    }
  }

  // half coords
  std::vector<std::vector<frCoord>> halfTrackCoords(numLayers);
  for (int i = 0; i < numLayers; i++) {
    frCoord prevFullCoord = std::numeric_limits<frCoord>::max();

    for (auto& [currFullCoord, cost] : trackCoords_[i]) {
      if (currFullCoord > prevFullCoord) {
        const frCoord currHalfGrid
            = (currFullCoord + prevFullCoord) / 2 / manuGrid * manuGrid;
        if (currHalfGrid != currFullCoord && currHalfGrid != prevFullCoord) {
          halfTrackCoords[i].push_back(currHalfGrid);
        }
      }
      prevFullCoord = currFullCoord;
    }
    for (auto halfCoord : halfTrackCoords[i]) {
      trackCoords_[i][halfCoord] = frAccessPointEnum::HalfGrid;
    }
  }
}
