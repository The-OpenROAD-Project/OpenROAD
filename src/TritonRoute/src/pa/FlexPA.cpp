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

#include <chrono>
#include <iostream>
#include <sstream>

#include "FlexPA_graphics.h"
#include "db/infra/frTime.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"

using namespace std;
using namespace fr;

FlexPA::FlexPA(frDesign* in, Logger* logger)
    : design_(in),
      logger_(logger),
      stdCellPinGenApCnt_(0),
      stdCellPinValidPlanarApCnt_(0),
      stdCellPinValidViaApCnt_(0),
      stdCellPinNoApCnt_(0),
      macroCellPinGenApCnt_(0),
      macroCellPinValidPlanarApCnt_(0),
      macroCellPinValidViaApCnt_(0),
      macroCellPinNoApCnt_(0),
      maxAccessPatternSize_(0)
{
}

FlexPA::~FlexPA()
{
  // must be out-of-line due to unique_ptr
}

void FlexPA::setDebug(frDebugSettings* settings, odb::dbDatabase* db)
{
  bool on = settings->debugPA;
  graphics_
      = on && FlexPAGraphics::guiActive()
            ? std::make_unique<FlexPAGraphics>(settings, design_, db, logger_)
            : nullptr;
}

void FlexPA::init()
{
  ProfileTask profile("PA:init");
  initViaRawPriority();
  initTrackCoords();

  initUniqueInstance();
  initPinAccess();
}

void FlexPA::prep()
{
  ProfileTask profile("PA:prep");
  prepPoint();
  revertAccessPoints();
  prepPattern();
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
    if (inst->getRefBlock()->getMacroClass() != MacroClassEnum::CORE) {
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
    logger_->report("#scanned instances     = {}", inst2unique_.size());
    logger_->report("#unique  instances     = {}", uniqueInstances_.size());
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
