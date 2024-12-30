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

namespace drt {

UniqueInsts::UniqueInsts(frDesign* design,
                         const frCollection<odb::dbInst*>& target_insts,
                         Logger* logger,
                         RouterConfiguration* router_cfg)
    : design_(design),
      target_insts_(target_insts),
      logger_(logger),
      router_cfg_(router_cfg)
{
}

std::vector<frTrackPattern*> UniqueInsts::getPrefTrackPatterns()
{
  std::vector<frTrackPattern*> pref_track_patterns;
  for (const auto& track_pattern : design_->getTopBlock()->getTrackPatterns()) {
    const bool is_vertical_track = track_pattern->isHorizontal();
    const frLayerNum layer_num = track_pattern->getLayerNum();
    const frLayer* layer = getTech()->getLayer(layer_num);
    if (layer->getDir() == dbTechLayerDir::HORIZONTAL) {
      if (!is_vertical_track) {
        pref_track_patterns.push_back(track_pattern);
      }
    } else {
      if (is_vertical_track) {
        pref_track_patterns.push_back(track_pattern);
      }
    }
  }
  return pref_track_patterns;
}

UniqueInsts::MasterLayerRange UniqueInsts::initMasterToPinLayerRange()
{
  MasterLayerRange master_to_pin_layer_range;
  std::set<frString> masters;
  for (odb::dbInst* inst : target_insts_) {
    masters.insert(inst->getMaster()->getName());
  }
  const int num_layers = getTech()->getLayers().size();
  const frLayerNum bottom_layer_num = getTech()->getBottomLayerNum();
  for (auto& uMaster : design_->getMasters()) {
    auto master = uMaster.get();
    if (!masters.empty() && masters.find(master->getName()) == masters.end()) {
      continue;
    }
    frLayerNum min_layer_num = std::numeric_limits<frLayerNum>::max();
    frLayerNum max_layer_num = std::numeric_limits<frLayerNum>::min();
    for (auto& u_term : master->getTerms()) {
      if (u_term->getType().isSupply()) {
        continue;
      }
      for (auto& u_pin : u_term->getPins()) {
        for (auto& u_pin_fig : u_pin->getFigs()) {
          auto pin_fig = u_pin_fig.get();
          frLayerNum lNum;
          if (pin_fig->typeId() == frcRect) {
            lNum = static_cast<frRect*>(pin_fig)->getLayerNum();
          } else if (pin_fig->typeId() == frcPolygon) {
            lNum = static_cast<frPolygon*>(pin_fig)->getLayerNum();
          } else {
            logger_->error(DRT, 65, "instAnalysis unsupported pin_fig.");
          }
          min_layer_num
              = std::min(min_layer_num, std::max(bottom_layer_num, lNum));
          max_layer_num = std::max(max_layer_num, lNum);
        }
      }
    }
    if (min_layer_num < bottom_layer_num
        || max_layer_num > getTech()->getTopLayerNum()) {
      logger_->warn(DRT,
                    66,
                    "instAnalysis skips {} due to no pin shapes.",
                    master->getName());
      continue;
    }
    max_layer_num = std::min(max_layer_num + 2, num_layers);
    master_to_pin_layer_range[master] = {min_layer_num, max_layer_num};
  }
  return master_to_pin_layer_range;
}

bool UniqueInsts::hasTrackPattern(frTrackPattern* tp, const Rect& box) const
{
  const bool is_vertical_track = tp->isHorizontal();
  const frCoord low = tp->getStartCoord();
  const frCoord high = low + tp->getTrackSpacing() * (tp->getNumTracks() - 1);
  if (is_vertical_track) {
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
// pin_access_idx is active
void UniqueInsts::computeUnique(
    const MasterLayerRange& master_to_pin_layer_range,
    const std::vector<frTrackPattern*>& pref_track_patterns)
{
  std::set<frInst*> target_frinsts;
  for (auto inst : target_insts_) {
    target_frinsts.insert(design_->getTopBlock()->findInst(inst->getName()));
  }

  std::vector<frInst*> ndr_insts;
  std::vector<frCoord> offset;
  for (auto& inst : design_->getTopBlock()->getInsts()) {
    if (!target_insts_.empty()
        && target_frinsts.find(inst.get()) == target_frinsts.end()) {
      continue;
    }
    if (!router_cfg_->AUTO_TAPER_NDR_NETS && isNDRInst(*inst)) {
      ndr_insts.push_back(inst.get());
      continue;
    }
    const Point origin = inst->getOrigin();
    const Rect boundary_bbox = inst->getBoundaryBBox();
    const dbOrientType orient = inst->getOrient();
    auto it = master_to_pin_layer_range.find(inst->getMaster());
    if (it == master_to_pin_layer_range.end()) {
      logger_->error(DRT,
                     146,
                     "Master {} not found in master_to_pin_layer_range",
                     inst->getMaster()->getName());
    }
    const auto [min_layer_num, max_layer_num] = it->second;
    offset.clear();
    for (auto& tp : pref_track_patterns) {
      if (tp->getLayerNum() >= min_layer_num
          && tp->getLayerNum() <= max_layer_num) {
        if (hasTrackPattern(tp, boundary_bbox)) {
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
    master_orient_trackoffset_to_insts_[inst->getMaster()][orient][offset]
        .insert(inst.get());
  }

  for (auto& [master, orientMap] : master_orient_trackoffset_to_insts_) {
    for (auto& [orient, offsetMap] : orientMap) {
      for (auto& [vec, insts] : offsetMap) {
        auto unique_inst = *(insts.begin());
        unique_.push_back(unique_inst);
        for (auto i : insts) {
          inst_to_unique_[i] = unique_inst;
          inst_to_class_[i] = &insts;
        }
      }
    }
  }
  for (frInst* inst : ndr_insts) {
    unique_.push_back(inst);
    inst_to_unique_[inst] = inst;
    inst_to_class_[inst] = nullptr;
  }

  // init unique2Idx
  for (int i = 0; i < (int) unique_.size(); i++) {
    unique_to_idx_[unique_[i]] = i;
  }
}

void UniqueInsts::initUniqueInstance()
{
  std::vector<frTrackPattern*> pref_track_patterns = getPrefTrackPatterns();

  MasterLayerRange master_to_pin_layer_range = initMasterToPinLayerRange();

  computeUnique(master_to_pin_layer_range, pref_track_patterns);
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
      logger_->error(DRT, 322, "checkFigsOnGrid unsupported pin_fig.");
    }
  }
}

void UniqueInsts::initPinAccess()
{
  for (auto& inst : unique_) {
    for (auto& inst_term : inst->getInstTerms()) {
      for (auto& pin : inst_term->getTerm()->getPins()) {
        if (unique_to_pa_idx_.find(inst) == unique_to_pa_idx_.end()) {
          unique_to_pa_idx_[inst] = pin->getNumPinAccess();
        } else if (unique_to_pa_idx_[inst] != pin->getNumPinAccess()) {
          logger_->error(DRT, 69, "initPinAccess error.");
        }
        checkFigsOnGrid(pin.get());
        auto pa = std::make_unique<frPinAccess>();
        pin->addPinAccess(std::move(pa));
      }
    }
    inst->setPinAccessIdx(unique_to_pa_idx_[inst]);
  }
  for (auto& [inst, unique_inst] : inst_to_unique_) {
    inst->setPinAccessIdx(unique_inst->getPinAccessIdx());
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
  logger_->report("#scanned instances     = {}", inst_to_unique_.size());
  logger_->report("#unique  instances     = {}", unique_.size());
}

std::set<frInst*, frBlockObjectComp>* UniqueInsts::getClass(frInst* inst) const
{
  return inst_to_class_.at(inst);
}

bool UniqueInsts::hasUnique(frInst* inst) const
{
  return inst_to_unique_.find(inst) != inst_to_unique_.end();
}

int UniqueInsts::getIndex(frInst* inst)
{
  frInst* unique_inst = inst_to_unique_[inst];
  return unique_to_idx_[unique_inst];
}

int UniqueInsts::getPAIndex(frInst* inst) const
{
  return unique_to_pa_idx_.at(inst);
}

const std::vector<frInst*>& UniqueInsts::getUnique() const
{
  return unique_;
}

frInst* UniqueInsts::getUnique(int idx) const
{
  return unique_[idx];
}

}  // namespace drt
