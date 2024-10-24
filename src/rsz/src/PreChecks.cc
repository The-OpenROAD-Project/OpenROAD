/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2023, Precision Innovations Inc.
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "PreChecks.hh"

#include "db_sta/dbNetwork.hh"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/LibertyClass.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace rsz {

using sta::LibertyCell;
using sta::LibertyCellSeq;
using utl::RSZ;

PreChecks::PreChecks(Resizer* resizer) : resizer_(resizer)
{
  logger_ = resizer_->logger_;
  sta_ = resizer_->sta_;
}

void PreChecks::checkSlewLimit(float ref_cap, float max_load_slew)
{
  // Ensure the max slew value specified is something the library can
  // potentially handle
  if (!best_case_slew_computed_ || ref_cap < best_case_slew_load_) {
    LibertyCellSeq swappable_cells
        = resizer_->getSwappableCells(resizer_->buffer_lowest_drive_);
    float slew = resizer_->bufferSlew(
        resizer_->buffer_lowest_drive_, ref_cap, resizer_->tgt_slew_dcalc_ap_);
    for (LibertyCell* buffer : swappable_cells) {
      slew = std::min(
          slew,
          resizer_->bufferSlew(buffer, ref_cap, resizer_->tgt_slew_dcalc_ap_));
    }
    best_case_slew_computed_ = true;
    best_case_slew_load_ = ref_cap;
    best_case_slew_ = slew;
  }

  if (max_load_slew < best_case_slew_) {
    const sta::Unit* time_unit = sta_->units()->timeUnit();
    const sta::Unit* cap_unit = sta_->units()->capacitanceUnit();

    logger_->error(RSZ,
                   90,
                   "Max transition time from SDC is {}{}s. Best achievable "
                   "transition time is {}{}s with a load of {}{}F",
                   time_unit->asString(max_load_slew),
                   time_unit->scaleAbbreviation(),
                   time_unit->asString(best_case_slew_),
                   time_unit->scaleAbbreviation(),
                   cap_unit->asString(best_case_slew_load_, 2),
                   cap_unit->scaleAbbreviation());
  }
}
}  // namespace rsz
