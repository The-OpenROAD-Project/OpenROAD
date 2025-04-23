// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "FlexPA.h"

#include <omp.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/serialization/export.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "AbstractPAGraphics.h"
#include "db/infra/frTime.h"
#include "distributed/PinAccessJobDescription.h"
#include "distributed/frArchive.h"
#include "distributed/paUpdate.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "serialization.h"
#include "utl/exception.h"

BOOST_CLASS_EXPORT(drt::PinAccessJobDescription)

namespace drt {

using utl::ThreadException;

static inline void serializePatterns(
    const std::unordered_map<
        frInst*,
        std::vector<std::unique_ptr<FlexPinAccessPattern>>>& patterns,
    const std::string& file_name)
{
  std::ofstream file(file_name.c_str());
  frOArchive ar(file);
  registerTypes(ar);
  ar << patterns;
  file.close();
}

FlexPA::FlexPA(frDesign* in,
               Logger* logger,
               dst::Distributed* dist,
               RouterConfiguration* router_cfg)
    : design_(in),
      logger_(logger),
      dist_(dist),
      router_cfg_(router_cfg),
      unique_insts_(design_, target_insts_, logger_, router_cfg)
{
}

// must be out-of-line due to the unique_ptr
FlexPA::~FlexPA() = default;

void FlexPA::setDebug(std::unique_ptr<AbstractPAGraphics> pa_graphics)
{
  graphics_ = std::move(pa_graphics);
}

void FlexPA::init()
{
  ProfileTask profile("PA:init");
  for (auto& master : design_->getMasters()) {
    for (auto& term : master->getTerms()) {
      for (auto& pin : term->getPins()) {
        pin->clearPinAccess();
      }
    }
  }

  for (auto& term : design_->getTopBlock()->getTerms()) {
    for (auto& pin : term->getPins()) {
      pin->clearPinAccess();
    }
  }
  initViaRawPriority();
  initTrackCoords();

  unique_insts_.init();
  initAllSkipInstTerm();
}

void FlexPA::deleteInst(frInst* inst)
{
  const bool is_class_head = (inst == unique_insts_.getUnique(inst));
  // if inst is the class head the new head will be returned by deleteInst()
  frInst* class_head = unique_insts_.deleteInst(inst);
  UniqueInsts::InstSet* unique_class = unique_insts_.getClass(inst);
  // whole class has to be deleted
  if (!class_head) {
    unique_inst_patterns_.erase(inst);
    for (auto& inst_term : inst->getInstTerms()) {
      skip_unique_inst_term_.erase({unique_class, inst_term->getTerm()});
    }
  }
  // new class representative has to be chosen
  else if (is_class_head) {
    unique_inst_patterns_[class_head] = std::move(unique_inst_patterns_[inst]);
    unique_inst_patterns_.erase(inst);
  }
}

void FlexPA::applyPatternsFile(const char* file_path)
{
  unique_inst_patterns_.clear();
  std::ifstream file(file_path);
  frIArchive ar(file);
  ar.setDesign(design_);
  registerTypes(ar);
  ar >> unique_inst_patterns_;
  file.close();
}

void FlexPA::prep()
{
  ProfileTask profile("PA:prep");
  genAllAccessPoints();
  revertAccessPoints();
  if (isDistributed()) {
    std::vector<paUpdate> updates;
    paUpdate update;
    for (const auto& master : design_->getMasters()) {
      for (const auto& term : master->getTerms()) {
        for (const auto& pin : term->getPins()) {
          std::vector<std::unique_ptr<frPinAccess>> pa;
          pa.reserve(pin->getNumPinAccess());
          for (int i = 0; i < pin->getNumPinAccess(); i++) {
            pa.push_back(std::make_unique<frPinAccess>(*pin->getPinAccess(i)));
          }
          update.addPinAccess(pin.get(), std::move(pa));
        }
      }
    }
    for (const auto& term : design_->getTopBlock()->getTerms()) {
      for (const auto& pin : term->getPins()) {
        std::vector<std::unique_ptr<frPinAccess>> pa;
        pa.reserve(pin->getNumPinAccess());
        for (int i = 0; i < pin->getNumPinAccess(); i++) {
          pa.push_back(std::make_unique<frPinAccess>(*pin->getPinAccess(i)));
        }
        update.addPinAccess(pin.get(), std::move(pa));
      }
    }

    dst::JobMessage msg(dst::JobMessage::PIN_ACCESS,
                        dst::JobMessage::BROADCAST),
        result;
    std::unique_ptr<PinAccessJobDescription> uDesc
        = std::make_unique<PinAccessJobDescription>();
    std::string updates_file = fmt::format("{}/updates.bin", shared_vol_);
    paUpdate::serialize(update, updates_file);
    uDesc->setPath(updates_file);
    uDesc->setType(PinAccessJobDescription::UPDATE_PA);
    msg.setJobDescription(std::move(uDesc));
    const bool ok
        = dist_->sendJob(msg, remote_host_.c_str(), remote_port_, result);
    if (!ok) {
      logger_->error(utl::DRT, 331, "Error sending UPDATE_PA Job to cloud");
    }
  }
  prepPattern();
}

void FlexPA::prepPattern()
{
  ProfileTask profile("PA:pattern");

  const auto& unique = unique_insts_.getUnique();

  // revert access points to origin
  unique_inst_patterns_.reserve(unique.size());

  int cnt = 0;

  omp_set_num_threads(router_cfg_->MAX_THREADS);
  ThreadException exception;
#pragma omp parallel for schedule(dynamic)
  for (frInst* unique_inst : unique) {
    try {
      // only do for core and block cells
      // TODO the above comment says "block cells" but that's not what the code
      // does?
      if (!isStdCell(unique_inst)) {
        continue;
      }
      prepPatternInst(unique_inst);
#pragma omp critical
      {
        cnt++;
        if (router_cfg_->VERBOSE > 0) {
          if (cnt % (cnt > 1000 ? 1000 : 100) == 0) {
            logger_->info(DRT, 79, "  Complete {} unique inst patterns.", cnt);
          }
        }
      }
    } catch (...) {
      exception.capture();
    }
  }
  exception.rethrow();
  if (router_cfg_->VERBOSE > 0) {
    logger_->info(DRT, 81, "  Complete {} unique inst patterns.", cnt);
  }
  if (isDistributed()) {
    dst::JobMessage msg(dst::JobMessage::PIN_ACCESS,
                        dst::JobMessage::BROADCAST),
        result;
    std::unique_ptr<PinAccessJobDescription> uDesc
        = std::make_unique<PinAccessJobDescription>();
    std::string patterns_file = fmt::format("{}/patterns.bin", shared_vol_);
    serializePatterns(unique_inst_patterns_, patterns_file);
    uDesc->setPath(patterns_file);
    uDesc->setType(PinAccessJobDescription::UPDATE_PATTERNS);
    msg.setJobDescription(std::move(uDesc));
    const bool ok
        = dist_->sendJob(msg, remote_host_.c_str(), remote_port_, result);
    if (!ok) {
      logger_->error(
          utl::DRT, 330, "Error sending UPDATE_PATTERNS Job to cloud");
    }
  }

  std::vector<std::vector<frInst*>> inst_rows = computeInstRows();
  prepPatternInstRows(inst_rows);
}

void FlexPA::setTargetInstances(const frCollection<odb::dbInst*>& insts)
{
  target_insts_ = insts;
}

void FlexPA::setDistributed(const std::string& rhost,
                            const uint16_t rport,
                            const std::string& shared_vol,
                            const int cloud_sz)
{
  remote_host_ = rhost;
  remote_port_ = rport;
  shared_vol_ = shared_vol;
  cloud_sz_ = cloud_sz;
}

// Skip power pins, pins connected to special nets, and dangling pins
// (since we won't route these).
//
// Checks only this inst_term and not an equivalent ones.  This
// is a helper to isSkipInstTerm and initSkipInstTerm.
bool FlexPA::isSkipInstTermLocal(frInstTerm* in)
{
  auto term = in->getTerm();
  if (term->getType().isSupply()) {
    return true;
  }
  auto in_net = in->getNet();
  if (in_net && !in_net->isSpecial()) {
    return false;
  }
  return true;
}

bool FlexPA::isSkipInstTerm(frInstTerm* in)
{
  auto inst_class = unique_insts_.getClass(in->getInst());
  if (inst_class == nullptr) {
    return isSkipInstTermLocal(in);
  }

  // This should be already computed in initSkipInstTerm()
  return skip_unique_inst_term_.at({inst_class, in->getTerm()});
}

// TODO there should be a better way to get this info by getting the master
// terms from OpenDB
bool FlexPA::isStdCell(frInst* inst)
{
  return inst->getMaster()->getMasterType().isCore();
}

bool FlexPA::isMacroCell(frInst* inst)
{
  dbMasterType masterType = inst->getMaster()->getMasterType();
  return (masterType.isBlock() || masterType.isPad()
          || masterType == dbMasterType::RING);
}

int FlexPA::main()
{
  ProfileTask profile("PA:main");

  frTime t;
  if (router_cfg_->VERBOSE > 0) {
    logger_->info(DRT, 165, "Start pin access.");
  }

  init();
  prep();

  int std_cell_pin_cnt = 0;
  for (auto& inst : getDesign()->getTopBlock()->getInsts()) {
    if (inst->getMaster()->getMasterType() != dbMasterType::CORE) {
      continue;
    }
    for (auto& inst_term : inst->getInstTerms()) {
      if (isSkipInstTerm(inst_term.get())) {
        continue;
      }
      if (inst_term->hasNet()) {
        std_cell_pin_cnt++;
      }
    }
  }

  if (router_cfg_->VERBOSE > 0) {
    unique_insts_.report();
    //clang-format off
    logger_->report("#stdCellGenAp          = {}", std_cell_pin_gen_ap_cnt_);
    logger_->report("#stdCellValidPlanarAp  = {}",
                    std_cell_pin_valid_planar_ap_cnt_);
    logger_->report("#stdCellValidViaAp     = {}",
                    std_cell_pin_valid_via_ap_cnt_);
    logger_->report("#stdCellPinNoAp        = {}", std_cell_pin_no_ap_cnt_);
    logger_->report("#stdCellPinCnt         = {}", std_cell_pin_cnt);
    logger_->report("#instTermValidViaApCnt = {}", inst_term_valid_via_ap_cnt_);
    logger_->report("#macroGenAp            = {}", macro_cell_pin_gen_ap_cnt_);
    logger_->report("#macroValidPlanarAp    = {}",
                    macro_cell_pin_valid_planar_ap_cnt_);
    logger_->report("#macroValidViaAp       = {}",
                    macro_cell_pin_valid_via_ap_cnt_);
    logger_->report("#macroNoAp             = {}", macro_cell_pin_no_ap_cnt_);
    //clang-format on
  }

  if (router_cfg_->VERBOSE > 0) {
    logger_->info(DRT, 166, "Complete pin access.");
    t.print(logger_);
  }
  return 0;
}

template <class Archive>
void FlexPinAccessPattern::serialize(Archive& ar, const unsigned int version)
{
  if (is_loading(ar)) {
    int sz = 0;
    (ar) & sz;
    while (sz--) {
      frBlockObject* obj;
      serializeBlockObject(ar, obj);
      pattern_.push_back((frAccessPoint*) obj);
    }
  } else {
    int sz = pattern_.size();
    (ar) & sz;
    for (auto ap : pattern_) {
      frBlockObject* obj = (frBlockObject*) ap;
      serializeBlockObject(ar, obj);
    }
  }
  frBlockObject* obj = (frBlockObject*) right_;
  serializeBlockObject(ar, obj);
  right_ = (frAccessPoint*) obj;
  obj = (frBlockObject*) left_;
  serializeBlockObject(ar, obj);
  left_ = (frAccessPoint*) obj;
  (ar) & cost_;
}

template void FlexPinAccessPattern::serialize<frIArchive>(
    frIArchive& ar,
    const unsigned int file_version);

template void FlexPinAccessPattern::serialize<frOArchive>(
    frOArchive& ar,
    const unsigned int file_version);

}  // namespace drt
