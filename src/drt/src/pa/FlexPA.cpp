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

#include "FlexPA.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/serialization/export.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

#include "FlexPA_graphics.h"
#include "db/infra/frTime.h"
#include "distributed/PinAccessJobDescription.h"
#include "distributed/frArchive.h"
#include "distributed/paUpdate.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "serialization.h"

using namespace std;
using namespace fr;
BOOST_CLASS_EXPORT(PinAccessJobDescription)

FlexPA::FlexPA(frDesign* in, Logger* logger, dst::Distributed* dist)
    : design_(in),
      logger_(logger),
      dist_(dist),
      unique_insts_(design_, target_insts_, logger_)
{
}

// must be out-of-line due to the unique_ptr
FlexPA::~FlexPA() = default;

void FlexPA::setDebug(frDebugSettings* settings, odb::dbDatabase* db)
{
  const bool on = settings->debugPA;
  graphics_
      = on && FlexPAGraphics::guiActive()
            ? std::make_unique<FlexPAGraphics>(settings, design_, db, logger_)
            : nullptr;
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
}

void FlexPA::applyPatternsFile(const char* file_path)
{
  uniqueInstPatterns_.clear();
  std::ifstream file(file_path);
  frIArchive ar(file);
  ar.setDesign(design_);
  registerTypes(ar);
  ar >> uniqueInstPatterns_;
  file.close();
}

void FlexPA::prep()
{
  ProfileTask profile("PA:prep");
  prepPoint();
  revertAccessPoints();
  if (isDistributed()) {
    std::vector<paUpdate> updates;
    paUpdate update;
    for (const auto& master : design_->getMasters()) {
      for (const auto& term : master->getTerms()) {
        for (const auto& pin : term->getPins()) {
          std::vector<std::unique_ptr<frPinAccess>> pa;
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
    if (!ok)
      logger_->error(utl::DRT, 331, "Error sending UPDATE_PA Job to cloud");
  }
  prepPattern();
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

int FlexPA::main()
{
  ProfileTask profile("PA:main");

  frTime t;
  if (VERBOSE > 0) {
    logger_->info(DRT, 165, "Start pin access.");
  }

  init();
  prep();

  int stdCellPinCnt = 0;
  for (auto& inst : getDesign()->getTopBlock()->getInsts()) {
    if (inst->getMaster()->getMasterType() != dbMasterType::CORE) {
      continue;
    }
    for (auto& instTerm : inst->getInstTerms()) {
      if (isSkipInstTerm(instTerm.get())) {
        continue;
      }
      if (instTerm->hasNet()) {
        stdCellPinCnt++;
      }
    }
  }

  if (VERBOSE > 0) {
    unique_insts_.report();
    logger_->report("#stdCellGenAp          = {}", stdCellPinGenApCnt_);
    logger_->report("#stdCellValidPlanarAp  = {}", stdCellPinValidPlanarApCnt_);
    logger_->report("#stdCellValidViaAp     = {}", stdCellPinValidViaApCnt_);
    logger_->report("#stdCellPinNoAp        = {}", stdCellPinNoApCnt_);
    logger_->report("#stdCellPinCnt         = {}", stdCellPinCnt);
    logger_->report("#instTermValidViaApCnt = {}", instTermValidViaApCnt_);
    logger_->report("#macroGenAp            = {}", macroCellPinGenApCnt_);
    logger_->report("#macroValidPlanarAp    = {}",
                    macroCellPinValidPlanarApCnt_);
    logger_->report("#macroValidViaAp       = {}", macroCellPinValidViaApCnt_);
    logger_->report("#macroNoAp             = {}", macroCellPinNoApCnt_);
  }

  if (VERBOSE > 0) {
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
