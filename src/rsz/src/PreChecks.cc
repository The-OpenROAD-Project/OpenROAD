// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "PreChecks.hh"

#include <algorithm>
#include <memory>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Transition.hh"
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

void PreChecks::checkCapLimit(const sta::Pin* drvr_pin)
{
  if (!min_cap_load_computed_) {
    min_cap_load_computed_ = true;
    // Find the smallest buffer/inverter input cap
    min_cap_load_ = sta::INF;
    sta::dbNetwork* network = resizer_->getDbNetwork();
    std::unique_ptr<sta::LibertyLibraryIterator> lib_iter{
        network->libertyLibraryIterator()};

    auto update_min_cap = [&](const auto& cells) {
      for (sta::LibertyCell* cell : cells) {
        sta::LibertyPort* input;
        sta::LibertyPort* output;
        cell->bufferPorts(input, output);
        min_cap_load_ = std::min(min_cap_load_, input->capacitance());
      }
    };

    while (lib_iter->hasNext()) {
      sta::LibertyLibrary* lib = lib_iter->next();
      update_min_cap(*lib->buffers());
      update_min_cap(*lib->inverters());
    }
  }

  float cap1, max_cap1, cap_slack1;
  const sta::Corner* corner1;
  const sta::RiseFall* tr1;
  sta_->checkCapacitance(drvr_pin,
                         nullptr,
                         sta::MinMax::max(),
                         corner1,
                         tr1,
                         cap1,
                         max_cap1,
                         cap_slack1);
  if (max_cap1 > 0 && max_cap1 < min_cap_load_) {
    sta::dbNetwork* network = resizer_->getDbNetwork();
    const sta::Unit* cap_unit = sta_->units()->capacitanceUnit();
    std::string master_name = "-";
    if (sta::Instance* inst = network->instance(drvr_pin)) {
      sta::Cell* cell = network->cell(inst);
      if (cell) {
        master_name = network->name(cell);
      }
    }
    logger_->error(
        RSZ,
        169,
        "Max cap for driver {} of type {} is unreasonably small {}{}F. "
        "Min buffer or inverter input cap is {}{}F",
        network->name(drvr_pin),
        master_name,
        cap_unit->asString(max_cap1),
        cap_unit->scaleAbbreviation(),
        cap_unit->asString(min_cap_load_),
        cap_unit->scaleAbbreviation());
  }
}

}  // namespace rsz
