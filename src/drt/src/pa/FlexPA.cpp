// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "pa/FlexPA.h"

#include <cstdint>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "boost/archive/text_iarchive.hpp"
#include "boost/archive/text_oarchive.hpp"
#include "boost/io/ios_state.hpp"
#include "boost/serialization/export.hpp"
#include "db/infra/frTime.h"
#include "db/obj/frAccess.h"
#include "db/obj/frBlockObject.h"
#include "distributed/PinAccessJobDescription.h"
#include "distributed/frArchive.h"
#include "distributed/paUpdate.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "frBaseTypes.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "global.h"
#include "odb/db.h"
#include "omp.h"
#include "pa/AbstractPAGraphics.h"
#include "pa/FlexPA_unique.h"
#include "serialization.h"
#include "utl/Logger.h"
#include "utl/exception.h"

BOOST_CLASS_EXPORT(drt::PinAccessJobDescription)

namespace drt {

using utl::ThreadException;

static inline void serializePatterns(
    const std::unordered_map<
        UniqueClass*,
        std::vector<std::unique_ptr<FlexPinAccessPattern>>>& patterns,
    const std::string& file_name)
{
  std::ofstream file(file_name.c_str());
  frOArchive ar(file);
  registerTypes(ar);
  int sz = patterns.size();
  ar << sz;
  for (auto& [unique_class, pattern] : patterns) {
    frBlockObject* obj = (frBlockObject*) unique_class->getFirstInst();
    serializeBlockObject(ar, obj);
    ar << pattern;
  }
  file.close();
}

FlexPA::FlexPA(frDesign* in,
               utl::Logger* logger,
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

void FlexPA::addDirtyInst(frInst* inst)
{
  dirty_insts_.insert(inst);
}

void FlexPA::addAvoidViaDef(const frViaDef* via_def)
{
  avoid_via_defs_.insert(via_def);
}

void FlexPA::removeDirtyInst(frInst* inst)
{
  dirty_insts_.erase(inst);
}

void FlexPA::updateDirtyInsts()
{
  std::set<UniqueClass*> dirty_unique_classes;
  frOrderedIdSet<frInst*>
      pattern_insts;  // list of insts that need row pattern generation
  for (const auto& inst : dirty_insts_) {
    removeFromInstsSet(inst);
    const auto& old_unique_class = unique_insts_.getUniqueClass(inst);
    const auto& new_unique_class = unique_insts_.computeUniqueClass(inst);
    if (old_unique_class == new_unique_class) {  // same unique class
      if (updateSkipInstTerm(inst)) {            // a new connection added
        dirty_unique_classes.insert(old_unique_class);
      } else if (inst->getLatestPATransform()
                 != inst->getTransform()) {  // cell has been moved
        pattern_insts.insert(inst);
      }
    } else {  // cell changed unique class
      if (old_unique_class != nullptr) {
        unique_insts_.deleteInst(inst);
      }
      const bool is_new_unique = unique_insts_.addInst(inst);
      if (is_new_unique || updateSkipInstTerm(inst)) {
        dirty_unique_classes.insert(new_unique_class);
      } else {
        pattern_insts.insert(inst);
      }
    }
  }
  for (auto& unique_class : dirty_unique_classes) {
    if (!unique_class->isInitialized()) {
      unique_insts_.initUniqueInstPinAccess(unique_class);
    }
    unique_class->getMaster()->setHasPinAccessUpdate(
        unique_class->getPinAccessIdx());
    for (auto inst : unique_class->getInsts()) {
      pattern_insts.insert(inst);
    }
  }
  std::vector<UniqueClass*> dirty_unique_classes_vec(
      dirty_unique_classes.begin(), dirty_unique_classes.end());
  for (auto& unique_class : dirty_unique_classes_vec) {
    unique_inst_patterns_[unique_class]
        = std::vector<std::unique_ptr<FlexPinAccessPattern>>();
  }
#pragma omp parallel for schedule(dynamic)
  for (auto& unique_class : dirty_unique_classes_vec) {
    initSkipInstTerm(unique_class);
    auto candidate_inst = unique_class->getFirstInst();
    genInstAccessPoints(candidate_inst);
    if (isStdCell(candidate_inst)) {
      prepPatternInst(candidate_inst);
    }
    revertAccessPoints(candidate_inst);
  }
  for (auto& inst : dirty_insts_) {
    addToInstsSet(inst);
  }
  for (auto& unique_class : dirty_unique_classes) {
    // In case of unique class that was previously skipped
    for (auto& inst : unique_class->getInsts()) {
      addToInstsSet(inst);
    }
  }
  frOrderedIdSet<frInst*> processed_insts;
  std::vector<std::vector<frInst*>> inst_rows;
  for (auto& inst : pattern_insts) {
    if (processed_insts.find(inst) != processed_insts.end() || isSkipInst(inst)
        || !isStdCell(inst)) {
      continue;
    }
    std::vector<frInst*> inst_row = getAdjacentInstancesCluster(inst);
    processed_insts.insert(inst_row.begin(), inst_row.end());
    inst_rows.push_back(inst_row);
  }
#pragma omp parallel for schedule(dynamic)
  for (auto& inst_row : inst_rows) {
    genInstRowPattern(inst_row);
  }
  dirty_insts_.clear();
}

void FlexPA::deleteInst(frInst* inst)
{
  removeDirtyInst(inst);
  auto unique_class = unique_insts_.getUniqueClass(inst);
  if (unique_class == nullptr) {
    return;
  }
  unique_insts_.deleteInst(inst);
}

void FlexPA::applyPatternsFile(const char* file_path)
{
  unique_inst_patterns_.clear();
  std::ifstream file(file_path);
  frIArchive ar(file);
  ar.setDesign(design_);
  registerTypes(ar);
  int sz = 0;
  ar >> sz;
  while (sz--) {
    frBlockObject* obj;
    serializeBlockObject(ar, obj);
    auto unique_class = unique_insts_.getUniqueClass(static_cast<frInst*>(obj));
    auto& pattern = unique_inst_patterns_[unique_class];
    ar >> pattern;
  }
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

    dst::JobMessage msg(dst::JobMessage::kPinAccess,
                        dst::JobMessage::kBroadcast),
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

  const auto& unique = unique_insts_.getUniqueClasses();

  // reserve space for unique_inst_patterns_
  unique_inst_patterns_.reserve(unique.size());
  for (auto& unique_class : unique) {
    unique_inst_patterns_[unique_class.get()]
        = std::vector<std::unique_ptr<FlexPinAccessPattern>>();
  }

  int cnt = 0;

  omp_set_num_threads(router_cfg_->MAX_THREADS);
  ThreadException exception;
#pragma omp parallel for schedule(dynamic)
  for (auto& unique_class : unique) {
    try {
      // only do for core and block cells
      // TODO the above comment says "block cells" but that's not what the code
      // does?
      if (unique_class->getInsts().empty()) {
        continue;
      }
      auto candidate_inst = *unique_class->getInsts().begin();
      if (!isStdCell(candidate_inst)) {
        continue;
      }
      prepPatternInst(candidate_inst);
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
    dst::JobMessage msg(dst::JobMessage::kPinAccess,
                        dst::JobMessage::kBroadcast),
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

// Skip power pins, pins connected to special nets, dangling pins and pins
// connected by abuttment (since we won't route these).
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
  if (in_net && in_net->isConnectedByAbutment()) {
    return true;
  }
  if (in_net && !in_net->isSpecial()) {
    return false;
  }
  return true;
}

bool FlexPA::isSkipInstTerm(frInstTerm* in)
{
  auto unique_class = unique_insts_.getUniqueClass(in->getInst());
  if (unique_class == nullptr) {
    return isSkipInstTermLocal(in);
  }

  // This should be already computed in initSkipInstTerm()
  return unique_class->isSkipTerm(in->getTerm());
}

bool FlexPA::isSkipInst(frInst* inst)
{
  for (auto& inst_term : inst->getInstTerms()) {
    if (!isSkipInstTerm(inst_term.get())) {
      return false;
    }
  }
  return true;
}

// TODO there should be a better way to get this info by getting the master
// terms from OpenDB
bool FlexPA::isStdCell(frInst* inst)
{
  return inst->getMaster()->getMasterType().isCore();
}

bool FlexPA::isMacroCell(frInst* inst)
{
  odb::dbMasterType masterType = inst->getMaster()->getMasterType();
  return (masterType.isBlock() || masterType.isPad()
          || masterType == odb::dbMasterType::RING);
}

bool FlexPA::isStdCellTerm(frInstTerm* inst_term)
{
  return inst_term && isStdCell(inst_term->getInst());
}

bool FlexPA::isMacroCellTerm(frInstTerm* inst_term)
{
  return inst_term && isMacroCell(inst_term->getInst());
}

// It is sometimes important to understand that when PA is checking for nullptr
// it means its checking for an io term, not a corner case with an invalid
// inst_term. This function is made to avoid confusion
bool FlexPA::isIOTerm(frInstTerm* inst_term)
{
  return inst_term == nullptr;
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
    if (inst->getMaster()->getMasterType() != odb::dbMasterType::CORE) {
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
