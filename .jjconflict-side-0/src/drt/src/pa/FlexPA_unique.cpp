// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "pa/FlexPA_unique.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "db/obj/frAccess.h"
#include "db/obj/frInst.h"
#include "db/tech/frLayer.h"
#include "distributed/frArchive.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "global.h"
#include "odb/db.h"
#include "odb/dbTypes.h"

namespace drt {

UniqueClass::UniqueClass(const UniqueClassKey& key) : key_(key)
{
  for (const auto& term : key_.master->getTerms()) {
    skip_term_[term.get()] = false;
  }
}

void UniqueClass::addInst(frInst* inst)
{
  insts_.insert(inst);
}

void UniqueClass::removeInst(frInst* inst)
{
  insts_.erase(inst);
}

bool UniqueClass::hasInst(frInst* inst) const
{
  return insts_.find(inst) != insts_.end();
}

frInst* UniqueClass::getFirstInst() const
{
  return *insts_.begin();
}

bool UniqueClass::isSkipTerm(frMTerm* term) const
{
  return skip_term_.at(term);
}

void UniqueClass::setSkipTerm(frMTerm* term, bool skip)
{
  skip_term_[term] = skip;
}

UniqueInsts::UniqueInsts(frDesign* design,
                         const frCollection<odb::dbInst*>& target_insts,
                         utl::Logger* logger,
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
    if (layer->getDir() == odb::dbTechLayerDir::HORIZONTAL) {
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

bool UniqueInsts::hasTrackPattern(frTrackPattern* tp,
                                  const odb::Rect& box) const
{
  const bool is_vertical_track = tp->isHorizontal();
  const frCoord low = tp->getStartCoord();
  const frCoord high = low + tp->getTrackSpacing() * (tp->getNumTracks() - 1);
  if (is_vertical_track) {
    return !(low > box.xMax() || high < box.xMin());
  }
  return !(low > box.yMax() || high < box.yMin());
}

bool UniqueInsts::isNDRInst(frInst* inst) const
{
  for (const auto& a : inst->getInstTerms()) {
    if (a->getNet() && a->getNet()->getNondefaultRule()) {
      return true;
    }
  }
  return false;
}

UniqueClassKey UniqueInsts::computeUniqueClassKey(frInst* inst) const
{
  const odb::Point origin = inst->getOrigin();
  const odb::Rect boundary_bbox = inst->getBoundaryBBox();
  const odb::dbOrientType orient = inst->getOrient();
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
  // Special case for NDR instances, create a separate unique class for them
  frInst* ndr_inst = nullptr;
  if (!router_cfg_->AUTO_TAPER_NDR_NETS && isNDRInst(inst)) {
    ndr_inst = inst;
  }
  return UniqueClassKey(inst->getMaster(), orient, offset, ndr_inst);
}

UniqueClass* UniqueInsts::computeUniqueClass(frInst* inst)
{
  const auto key = computeUniqueClassKey(inst);
  if (unique_class_by_key_.find(key) == unique_class_by_key_.end()) {
    // new unique class
    auto unique_class = std::make_unique<UniqueClass>(key);
    auto unique_class_ptr = unique_class.get();
    unique_classes_.emplace_back(std::move(unique_class));
    unique_class_by_key_[key] = unique_class_ptr;
  }
  return unique_class_by_key_.at(key);
}

bool UniqueInsts::addInst(frInst* inst)
{
  auto unique_class = computeUniqueClass(inst);
  bool new_unique_class = unique_class->getInsts().empty();
  unique_class->addInst(inst);
  inst_to_unique_class_[inst] = unique_class;
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
      for (const odb::Point& pt : polygon->getPoints()) {
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

void UniqueInsts::initUniqueInstPinAccess(UniqueClass* unique_class)
{
  if (unique_class->getInsts().empty()) {
    return;
  }
  for (auto& term : unique_class->getMaster()->getTerms()) {
    for (auto& pin : term->getPins()) {
      unique_class->setPinAccessIdx(pin->getNumPinAccess());
      checkFigsOnGrid(pin.get());
      pin->addPinAccess(std::make_unique<frPinAccess>());
    }
  }
  for (frInst* inst : unique_class->getInsts()) {
    inst->setPinAccessIdx(unique_class->getPinAccessIdx());
  }
}

void UniqueInsts::initPinAccess()
{
  for (auto& unique_class : unique_classes_) {
    initUniqueInstPinAccess(unique_class.get());
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
  logger_->report("#scanned instances     = {}", inst_to_unique_class_.size());
  logger_->report("#unique  instances     = {}", unique_classes_.size());
}

UniqueClass* UniqueInsts::getUniqueClass(frInst* inst) const
{
  if (inst_to_unique_class_.find(inst) == inst_to_unique_class_.end()) {
    return nullptr;
  }
  return inst_to_unique_class_.at(inst);
}

bool UniqueInsts::hasUnique(frInst* inst) const
{
  return inst_to_unique_class_.find(inst) != inst_to_unique_class_.end();
}

// deleteInst has to be called both when an instance is deleted and might
// be needed when moved
void UniqueInsts::deleteInst(frInst* inst)
{
  auto unique_class = getUniqueClass(inst);
  if (!unique_class) {
    return;
  }
  unique_class->removeInst(inst);
  inst_to_unique_class_.erase(inst);
  inst->deletePinAccessIdx();
}

void UniqueInsts::deleteUniqueClass(UniqueClass* unique_class)
{
  unique_class_by_key_.erase(unique_class->key());
  unique_classes_.erase(std::find_if(
      unique_classes_.begin(),
      unique_classes_.end(),
      [unique_class](const auto& u) { return u.get() == unique_class; }));
}

const std::vector<std::unique_ptr<UniqueClass>>& UniqueInsts::getUniqueClasses()
    const
{
  return unique_classes_;
}

}  // namespace drt
