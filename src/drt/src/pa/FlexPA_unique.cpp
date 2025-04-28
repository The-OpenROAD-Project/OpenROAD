// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "FlexPA_unique.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <set>
#include <vector>

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

void UniqueInsts::computePrefTrackPatterns()
{
  for (const auto& track_pattern : design_->getTopBlock()->getTrackPatterns()) {
    const bool is_vertical_track = track_pattern->isHorizontal();
    const frLayerNum layer_num = track_pattern->getLayerNum();
    const frLayer* layer = getTech()->getLayer(layer_num);
    if (layer->getDir() == dbTechLayerDir::HORIZONTAL) {
      if (!is_vertical_track) {
        pref_track_patterns_.push_back(track_pattern);
      }
    } else {
      if (is_vertical_track) {
        pref_track_patterns_.push_back(track_pattern);
      }
    }
  }
}

void UniqueInsts::initMasterToPinLayerRange()
{
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
    master_to_pin_layer_range_[master] = {min_layer_num, max_layer_num};
  }
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

UniqueInsts::InstSet& UniqueInsts::computeUniqueClass(frInst* inst)
{
  const Point origin = inst->getOrigin();
  const Rect boundary_bbox = inst->getBoundaryBBox();
  const dbOrientType orient = inst->getOrient();
  auto it = master_to_pin_layer_range_.find(inst->getMaster());
  if (it == master_to_pin_layer_range_.end()) {
    logger_->error(DRT,
                   146,
                   "Master {} not found in master_to_pin_layer_range",
                   inst->getMaster()->getName());
  }
  const auto [min_layer_num, max_layer_num] = it->second;
  std::vector<frCoord> offset;
  for (auto& tp : pref_track_patterns_) {
    if (tp->getLayerNum() >= min_layer_num && tp->getLayerNum() <= max_layer_num
        && hasTrackPattern(tp, boundary_bbox)) {
      // vertical track
      if (tp->isHorizontal()) {
        offset.push_back(origin.x() % tp->getTrackSpacing());
      } else {
        offset.push_back(origin.y() % tp->getTrackSpacing());
      }
    } else {
      offset.push_back(tp->getTrackSpacing());
    }
  }

  // Fills data structure that relate a instance to its unique instance
  return master_orient_trackoffset_to_insts_[inst->getMaster()][orient][offset];
}

bool UniqueInsts::addInst(frInst* inst)
{
  if (!router_cfg_->AUTO_TAPER_NDR_NETS && isNDRInst(*inst)) {
    unique_to_idx_[inst] = unique_.size();
    unique_.push_back(inst);
    inst_to_unique_[inst] = inst;
    inst_to_class_[inst] = nullptr;
  }

  UniqueInsts::InstSet& unique_class = computeUniqueClass(inst);
  inst_to_class_[inst] = &unique_class;

  frInst* unique_inst = nullptr;
  bool new_unique_class = unique_class.empty();
  if (new_unique_class) {
    int i = unique_.size();
    unique_.push_back(inst);
    unique_to_idx_[inst] = i;
    unique_inst = inst;
  } else {
    // guarantees everyone on the class has the same unique_inst (the first
    // that came)
    unique_inst = inst_to_unique_[*unique_class.begin()];
  }
  unique_class.insert(inst);
  inst_to_unique_[inst] = unique_inst;
  return new_unique_class;
}

// must init all unique, including filler, macro, etc. to ensure frInst
// pin_access_idx is active
void UniqueInsts::computeUnique()
{
  computePrefTrackPatterns();
  initMasterToPinLayerRange();

  std::set<frInst*> target_frinsts;
  for (auto inst : target_insts_) {
    target_frinsts.insert(design_->getTopBlock()->findInst(inst));
  }

  for (auto& inst : design_->getTopBlock()->getInsts()) {
    if (!target_insts_.empty()
        && target_frinsts.find(inst.get()) == target_frinsts.end()) {
      continue;
    }
    addInst(inst.get());
  }
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

void UniqueInsts::initUniqueInstPinAccess(frInst* unique_inst)
{
  for (auto& inst_term : unique_inst->getInstTerms()) {
    for (auto& pin : inst_term->getTerm()->getPins()) {
      unique_inst->setPinAccessIdx(pin->getNumPinAccess());
      checkFigsOnGrid(pin.get());
      pin->addPinAccess(std::make_unique<frPinAccess>());
    }
  }
  for (frInst* inst : *inst_to_class_[unique_inst]) {
    inst->setPinAccessIdx(unique_inst->getPinAccessIdx());
  }
}

void UniqueInsts::initPinAccess()
{
  for (frInst* unique_inst : unique_) {
    initUniqueInstPinAccess(unique_inst);
  }

  // IO terms
  if (target_insts_.empty()) {
    for (auto& term : getDesign()->getTopBlock()->getTerms()) {
      for (auto& pin : term->getPins()) {
        pin->addPinAccess(std::make_unique<frPinAccess>());
      }
    }
  }
}

void UniqueInsts::init()
{
  computeUnique();
  initPinAccess();
}

void UniqueInsts::report() const
{
  logger_->report("#scanned instances     = {}", inst_to_unique_.size());
  logger_->report("#unique  instances     = {}", unique_.size());
}

UniqueInsts::InstSet* UniqueInsts::getClass(frInst* inst) const
{
  return inst_to_class_.at(inst);
}

bool UniqueInsts::hasUnique(frInst* inst) const
{
  return inst_to_unique_.find(inst) != inst_to_unique_.end();
}

// deleteInst has to be called both when an instance is deleted and might
// be needed when moved
frInst* UniqueInsts::deleteInst(frInst* inst)
{
  UniqueInsts::InstSet& unique_class = *inst_to_class_[inst];
  frInst* class_head = inst_to_unique_[inst];
  if (unique_class.size() == 1) {
    auto it = std::find(unique_.begin(), unique_.end(), inst);
    if (it == unique_.end()) {
      logger_->error(DRT,
                     25,
                     "{} not found on unique insts, although being the only "
                     "one of its unique class");
    }

    // readjusts unique_to_idx_ to compensate for posterior unique deletion
    for (int i = unique_to_idx_[inst]; i < unique_.size(); i++) {
      auto unique_inst = unique_[i];
      unique_to_idx_[unique_inst]--;
    }
    unique_.erase(it);
    unique_to_idx_.erase(inst);
    class_head = nullptr;

  } else if (inst == inst_to_unique_[inst]) {
    // the inst does not belong to the class anymore, but is the reference
    // unique_inst, so the reference has to be another inst
    auto class_begin = inst_to_class_[inst]->begin();
    // new class head
    class_head = *class_begin != inst ? *class_begin : *(++class_begin);
    unique_[unique_to_idx_[inst]] = class_head;
    for (frInst* other_inst : unique_class) {
      inst_to_unique_[other_inst] = class_head;
    }
    unique_to_idx_[class_head] = unique_to_idx_[inst];
    unique_to_idx_.erase(inst);
  }
  inst_to_class_[inst]->erase(inst);
  inst_to_class_.erase(inst);
  inst_to_unique_.erase(inst);
  inst->deletePinAccessIdx();
  return class_head;
}

int UniqueInsts::getIndex(frInst* inst)
{
  frInst* unique_inst = inst_to_unique_[inst];
  return unique_to_idx_[unique_inst];
}

const std::vector<frInst*>& UniqueInsts::getUnique() const
{
  return unique_;
}

frInst* UniqueInsts::getUnique(int idx) const
{
  return unique_[idx];
}

frInst* UniqueInsts::getUnique(frInst* inst) const
{
  return inst_to_unique_.at(inst);
}

}  // namespace drt
