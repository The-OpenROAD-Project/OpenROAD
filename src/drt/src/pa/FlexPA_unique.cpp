/*
 * Copyright (c) 2023, The Regents of the University of California
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

#include "FlexPA_unique.h"

#include "distributed/frArchive.h"

namespace fr {

UniqueInsts::UniqueInsts(frDesign* design,
                         const frCollection<odb::dbInst*>& target_insts,
                         Logger* logger)
    : design_(design), target_insts_(target_insts), logger_(logger)
{
}

void UniqueInsts::getPrefTrackPatterns(
    std::vector<frTrackPattern*>& prefTrackPatterns)
{
  for (const auto& trackPattern : design_->getTopBlock()->getTrackPatterns()) {
    const bool isVerticalTrack = trackPattern->isHorizontal();
    const frLayerNum layer_num = trackPattern->getLayerNum();
    const frLayer* layer = getTech()->getLayer(layer_num);
    if (layer->getDir() == dbTechLayerDir::HORIZONTAL) {
      if (!isVerticalTrack) {
        prefTrackPatterns.push_back(trackPattern);
      }
    } else {
      if (isVerticalTrack) {
        prefTrackPatterns.push_back(trackPattern);
      }
    }
  }
}

void UniqueInsts::initMaster2PinLayerRange(
    MasterLayerRange& master2PinLayerRange)
{
  std::set<frString> masters;
  for (odb::dbInst* inst : target_insts_) {
    masters.insert(inst->getMaster()->getName());
  }
  const int numLayers = getTech()->getLayers().size();
  const frLayerNum bottom_layer_num = getTech()->getBottomLayerNum();
  for (auto& uMaster : design_->getMasters()) {
    auto master = uMaster.get();
    if (!masters.empty() && masters.find(master->getName()) == masters.end()) {
      continue;
    }
    frLayerNum minLayerNum = std::numeric_limits<frLayerNum>::max();
    frLayerNum maxLayerNum = std::numeric_limits<frLayerNum>::min();
    for (auto& uTerm : master->getTerms()) {
      if (uTerm->getType().isSupply()) {
        continue;
      }
      for (auto& uPin : uTerm->getPins()) {
        for (auto& uPinFig : uPin->getFigs()) {
          auto pinFig = uPinFig.get();
          frLayerNum lNum;
          if (pinFig->typeId() == frcRect) {
            lNum = static_cast<frRect*>(pinFig)->getLayerNum();
          } else if (pinFig->typeId() == frcPolygon) {
            lNum = static_cast<frPolygon*>(pinFig)->getLayerNum();
          } else {
            logger_->error(DRT, 65, "instAnalysis unsupported pinFig.");
          }
          minLayerNum = std::min(minLayerNum, std::max(bottom_layer_num, lNum));
          maxLayerNum = std::max(maxLayerNum, lNum);
        }
      }
    }
    if (minLayerNum < bottom_layer_num
        || maxLayerNum > getTech()->getTopLayerNum()) {
      logger_->warn(DRT,
                    66,
                    "instAnalysis skips {} due to no pin shapes.",
                    master->getName());
      continue;
    }
    maxLayerNum = std::min(maxLayerNum + 2, numLayers);
    master2PinLayerRange[master] = {minLayerNum, maxLayerNum};
  }
}

bool UniqueInsts::hasTrackPattern(frTrackPattern* tp, const Rect& box) const
{
  const bool isVerticalTrack = tp->isHorizontal();
  const frCoord low = tp->getStartCoord();
  const frCoord high = low + tp->getTrackSpacing() * (tp->getNumTracks() - 1);
  if (isVerticalTrack) {
    return !(low > box.xMax() || high < box.xMin());
  }
  return !(low > box.yMax() || high < box.yMin());
}

bool UniqueInsts::isNDRInst(frInst& inst)
{
  for (auto& a : inst.getInstTerms()) {
    if (a->getNet() && a->getNet()->getNondefaultRule()) {
      return true;
    }
  }
  return false;
}

// must init all unique, including filler, macro, etc. to ensure frInst
// pinAccessIdx is active
void UniqueInsts::computeUnique(
    const MasterLayerRange& master2PinLayerRange,
    const std::vector<frTrackPattern*>& prefTrackPatterns)
{
  std::set<frInst*> target_frinsts;
  for (auto inst : target_insts_) {
    target_frinsts.insert(design_->getTopBlock()->findInst(inst->getName()));
  }

  std::vector<frInst*> ndrInsts;
  std::vector<frCoord> offset;
  for (auto& inst : design_->getTopBlock()->getInsts()) {
    if (!target_insts_.empty()
        && target_frinsts.find(inst.get()) == target_frinsts.end()) {
      continue;
    }
    if (!AUTO_TAPER_NDR_NETS && isNDRInst(*inst)) {
      ndrInsts.push_back(inst.get());
      continue;
    }
    const Point origin = inst->getOrigin();
    const Rect boundaryBBox = inst->getBoundaryBBox();
    const dbOrientType orient = inst->getOrient();
    const auto [minLayerNum, maxLayerNum]
        = master2PinLayerRange.find(inst->getMaster())->second;
    offset.clear();
    for (auto& tp : prefTrackPatterns) {
      if (tp->getLayerNum() >= minLayerNum
          && tp->getLayerNum() <= maxLayerNum) {
        if (hasTrackPattern(tp, boundaryBBox)) {
          // vertical track
          if (tp->isHorizontal()) {
            offset.push_back(origin.x() % tp->getTrackSpacing());
          } else {
            offset.push_back(origin.y() % tp->getTrackSpacing());
          }
        } else {
          offset.push_back(tp->getTrackSpacing());
        }
      } else {
        offset.push_back(tp->getTrackSpacing());
      }
    }
    masterOT2Insts_[inst->getMaster()][orient][offset].insert(inst.get());
  }

  for (auto& [master, orientMap] : masterOT2Insts_) {
    for (auto& [orient, offsetMap] : orientMap) {
      for (auto& [vec, insts] : offsetMap) {
        auto uniqueInst = *(insts.begin());
        unique_.push_back(uniqueInst);
        for (auto i : insts) {
          inst2unique_[i] = uniqueInst;
          inst2Class_[i] = &insts;
        }
      }
    }
  }
  for (frInst* inst : ndrInsts) {
    unique_.push_back(inst);
    inst2unique_[inst] = inst;
    inst2Class_[inst] = nullptr;
  }

  // init unique2Idx
  for (int i = 0; i < (int) unique_.size(); i++) {
    unique2Idx_[unique_[i]] = i;
  }
}

void UniqueInsts::initUniqueInstance()
{
  std::vector<frTrackPattern*> prefTrackPatterns;
  getPrefTrackPatterns(prefTrackPatterns);

  MasterLayerRange master2PinLayerRange;
  initMaster2PinLayerRange(master2PinLayerRange);

  computeUnique(master2PinLayerRange, prefTrackPatterns);
}

void UniqueInsts::checkFigsOnGrid(const frMPin* pin)
{
  const frMTerm* term = pin->getTerm();
  const int grid = getTech()->getManufacturingGrid();
  for (const auto& fig : pin->getFigs()) {
    if (fig->typeId() == frcRect) {
      const auto box = static_cast<frRect*>(fig.get())->getBBox();
      if (box.xMin() % grid || box.yMin() % grid || box.xMax() % grid
          || box.yMax() % grid) {
        logger_->error(DRT,
                       320,
                       "Term {} of {} contains offgrid pin shape",
                       term->getName(),
                       term->getMaster()->getName());
      }
    } else if (fig->typeId() == frcPolygon) {
      const auto polygon = static_cast<frPolygon*>(fig.get());
      for (const Point& pt : polygon->getPoints()) {
        if (pt.getX() % grid || pt.getY() % grid) {
          logger_->error(DRT,
                         321,
                         "Term {} of {} contains offgrid pin shape",
                         term->getName(),
                         term->getMaster()->getName());
        }
      }
    } else {
      logger_->error(DRT, 322, "checkFigsOnGrid unsupported pinFig.");
    }
  }
}

void UniqueInsts::initPinAccess()
{
  for (auto& inst : unique_) {
    for (auto& instTerm : inst->getInstTerms()) {
      for (auto& pin : instTerm->getTerm()->getPins()) {
        if (unique2paidx_.find(inst) == unique2paidx_.end()) {
          unique2paidx_[inst] = pin->getNumPinAccess();
        } else if (unique2paidx_[inst] != pin->getNumPinAccess()) {
          logger_->error(DRT, 69, "initPinAccess error.");
        }
        checkFigsOnGrid(pin.get());
        auto pa = std::make_unique<frPinAccess>();
        pin->addPinAccess(std::move(pa));
      }
    }
    inst->setPinAccessIdx(unique2paidx_[inst]);
  }
  for (auto& [inst, uniqueInst] : inst2unique_) {
    inst->setPinAccessIdx(uniqueInst->getPinAccessIdx());
  }

  // IO terms
  if (target_insts_.empty()) {
    for (auto& term : getDesign()->getTopBlock()->getTerms()) {
      for (auto& pin : term->getPins()) {
        auto pa = std::make_unique<frPinAccess>();
        pin->addPinAccess(std::move(pa));
      }
    }
  }
}

void UniqueInsts::init()
{
  initUniqueInstance();
  initPinAccess();
}

void UniqueInsts::report() const
{
  logger_->report("#scanned instances     = {}", inst2unique_.size());
  logger_->report("#unique  instances     = {}", unique_.size());
}

std::set<frInst*, frBlockObjectComp>* UniqueInsts::getClass(frInst* inst) const
{
  return inst2Class_.at(inst);
}

bool UniqueInsts::hasUnique(frInst* inst) const
{
  return inst2unique_.find(inst) != inst2unique_.end();
}

int UniqueInsts::getIndex(frInst* inst)
{
  frInst* unique_inst = inst2unique_[inst];
  return unique2Idx_[unique_inst];
}

int UniqueInsts::getPAIndex(frInst* inst) const
{
  return unique2paidx_.at(inst);
}

const std::vector<frInst*>& UniqueInsts::getUnique() const
{
  return unique_;
}

frInst* UniqueInsts::getUnique(int idx) const
{
  return unique_[idx];
}

}  // namespace fr
