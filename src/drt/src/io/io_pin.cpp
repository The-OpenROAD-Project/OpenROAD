// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <tuple>
#include <vector>

#include "db/obj/frMaster.h"
#include "frBaseTypes.h"
#include "io/io.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

using odb::dbTechLayerDir;

namespace drt {

void io::Parser::instAnalysis()
{
  if (router_cfg_->VERBOSE > 0) {
    logger_->info(DRT, 162, "Library cell analysis.");
  }
  trackOffsetMap_.clear();
  prefTrackPatterns_.clear();
  for (auto& trackPattern : getBlock()->getTrackPatterns()) {
    auto isVerticalTrack
        = trackPattern->isHorizontal();  // yes = vertical track
    if (getTech()->getLayer(trackPattern->getLayerNum())->getDir()
        == dbTechLayerDir::HORIZONTAL) {
      if (!isVerticalTrack) {
        prefTrackPatterns_.push_back(trackPattern);
      }
    } else {
      if (isVerticalTrack) {
        prefTrackPatterns_.push_back(trackPattern);
      }
    }
  }

  int numLayers = getTech()->getLayers().size();
  frOrderedIdMap<frMaster*, std::tuple<frLayerNum, frLayerNum>>
      masterPinLayerRange;
  for (auto& uMaster : getDesign()->getMasters()) {
    auto master = uMaster.get();
    frLayerNum minLayerNum = numLayers;
    frLayerNum maxLayerNum = 0;
    for (auto& uTerm : master->getTerms()) {
      for (auto& uPin : uTerm->getPins()) {
        for (auto& uPinFig : uPin->getFigs()) {
          auto pinFig = uPinFig.get();
          if (pinFig->typeId() == frcRect) {
            auto lNum = static_cast<frRect*>(pinFig)->getLayerNum();
            minLayerNum = std::min(minLayerNum, lNum);
            maxLayerNum = std::max(maxLayerNum, lNum);
          } else {
            logger_->warn(DRT, 248, "instAnalysis unsupported pinFig.");
          }
        }
      }
    }
    maxLayerNum = std::min(maxLayerNum + 2, numLayers);
    masterPinLayerRange[master] = std::make_tuple(minLayerNum, maxLayerNum);
  }
  // std::cout <<"  master pin layer range done" <<std::endl;

  if (router_cfg_->VERBOSE > 0) {
    logger_->info(DRT, 163, "Instance analysis.");
  }

  std::vector<frCoord> offset;
  int cnt = 0;
  for (auto& inst : getBlock()->getInsts()) {
    odb::Point origin = inst->getOrigin();
    auto orient = inst->getOrient();
    auto [minLayerNum, maxLayerNum] = masterPinLayerRange[inst->getMaster()];
    offset.clear();
    for (auto& tp : prefTrackPatterns_) {
      if (tp->getLayerNum() >= minLayerNum
          && tp->getLayerNum() <= maxLayerNum) {
        // vertical track
        if (tp->isHorizontal()) {
          offset.push_back(origin.x() % tp->getTrackSpacing());
          // std::cout <<"inst/offset/layer " <<inst->getName() <<" "
          // <<origin.y() % tp->getTrackSpacing()
          //      <<" "
          //      <<design->getTech()->getLayer(tp->getLayerNum())->getName()
          //      <<std::endl;
        } else {
          offset.push_back(origin.y() % tp->getTrackSpacing());
          // std::cout <<"inst/offset/layer " <<inst->getName() <<" "
          // <<origin.x() % tp->getTrackSpacing()
          //      <<" "
          //      <<design->getTech()->getLayer(tp->getLayerNum())->getName()
          //      <<std::endl;
        }
      }
    }
    trackOffsetMap_[inst->getMaster()][orient][offset].insert(inst.get());
    cnt++;
    if (router_cfg_->VERBOSE > 0) {
      if (cnt < 1000000) {
        if (cnt % 100000 == 0) {
          logger_->report("  Complete {} instances.", cnt);
        }
      } else {
        if (cnt % 1000000 == 0) {
          logger_->report("  Complete {} instances.", cnt);
        }
      }
    }
  }

  cnt = 0;
  for (auto& [master, orientMap] : trackOffsetMap_) {
    for (auto& [orient, offsetMap] : orientMap) {
      cnt += offsetMap.size();
    }
  }
  if (router_cfg_->VERBOSE > 0) {
    logger_->info(DRT, 164, "Number of unique instances = {}.", cnt);
  }
}

}  // namespace drt
