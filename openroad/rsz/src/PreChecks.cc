// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "PreChecks.hh"

#include <algorithm>
#include <memory>
#include <string>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace rsz {

using sta::LibertyCell;
using sta::LibertyCellSeq;
using sta::LibertyLibraryIterator;
using sta::RiseFall;
using sta::Scene;
using utl::RSZ;

PreChecks::PreChecks(Resizer* resizer) : resizer_(resizer)
{
  logger_ = resizer_->logger_;
  sta_ = resizer_->sta_;
}

void PreChecks::checkSlewLimit(const sta::Pin* load_pin,
                               float ref_cap,
                               float max_load_slew,
                               const sta::Scene* corner)
{
  SlewCheckCache& cache = slew_check_by_corner_[corner];

  // Ensure the max slew value specified is something the library can
  // potentially handle
  if (!cache.best_case_slew_computed || ref_cap < cache.best_case_slew_load) {
    // The global target slews come from the slowest corner. Use this
    // corner's target input slews when judging its transition limit.
    float target_slews[sta::RiseFall::index_count]{};
    int counts[sta::RiseFall::index_count]{};
    const int lib_index = corner->libertyIndex(sta::MinMax::max());
    const sta::Pvt* pvt
        = sta_->cmdMode()->sdc()->operatingConditions(sta::MinMax::max());
    for (LibertyCell* buffer : resizer_->buffer_cells_) {
      resizer_->findBufferTargetSlews(
          buffer->sceneCell(lib_index), pvt, target_slews, counts);
    }
    sta::Slew input_slews[sta::RiseFall::index_count];
    for (int rf_index : sta::RiseFall::rangeIndex()) {
      if (counts[rf_index] == 0) {
        logger_->error(RSZ,
                       3247,
                       "Cannot check the slew limit for pin {} at corner {}: "
                       "no buffer timing models for {} transitions.",
                       resizer_->dbNetwork()->pathName(load_pin),
                       corner->name(),
                       sta::RiseFall::find(rf_index)->name());
      }
      input_slews[rf_index] = target_slews[rf_index] / counts[rf_index];
    }
    auto buffer_slew = [&](LibertyCell* buffer) -> float {
      sta::LibertyPort *input, *output;
      buffer->bufferPorts(input, output);
      sta::ArcDelay delays[sta::RiseFall::index_count];
      sta::Slew slews[sta::RiseFall::index_count];
      resizer_->gateDelays(output,
                           ref_cap,
                           input_slews,
                           corner,
                           sta::MinMax::max(),
                           delays,
                           slews);
      return std::max(slews[sta::RiseFall::riseIndex()],
                      slews[sta::RiseFall::fallIndex()]);
    };
    LibertyCellSeq swappable_cells
        = resizer_->getSwappableCells(resizer_->buffer_lowest_drive_);
    float slew = buffer_slew(resizer_->buffer_lowest_drive_);
    for (LibertyCell* buffer : swappable_cells) {
      slew = std::min(slew, buffer_slew(buffer));
    }
    cache.best_case_slew_computed = true;
    cache.best_case_slew_load = ref_cap;
    cache.best_case_slew = slew;
  }

  if (max_load_slew < cache.best_case_slew) {
    const sta::Unit* time_unit = sta_->units()->timeUnit();
    const sta::Unit* cap_unit = sta_->units()->capacitanceUnit();
    logger_->error(RSZ,
                   90,
                   "Max transition time for pin {} at corner {} is {}{}s. Best "
                   "achievable transition time is {}{}s with a load of {}{}F",
                   resizer_->dbNetwork()->pathName(load_pin),
                   corner->name(),
                   time_unit->asString(max_load_slew),
                   time_unit->scaleAbbreviation(),
                   time_unit->asString(cache.best_case_slew),
                   time_unit->scaleAbbreviation(),
                   cap_unit->asString(cache.best_case_slew_load, 2),
                   cap_unit->scaleAbbreviation());
  }
}

void PreChecks::checkCapLimit(const sta::Pin* drvr_pin)
{
  if (!min_cap_load_computed_) {
    min_cap_load_computed_ = true;
    // Find the smallest buffer/inverter input cap
    min_cap_load_ = sta::INF;
    sta::dbNetwork* network = resizer_->dbNetwork();
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
  const Scene* corner1;
  const RiseFall* tr1;
  sta_->checkCapacitance(drvr_pin,
                         sta_->scenes(),
                         sta::MinMax::max(),
                         cap1,
                         max_cap1,
                         cap_slack1,
                         tr1,
                         corner1);
  if (max_cap1 > 0 && max_cap1 < min_cap_load_) {
    sta::dbNetwork* network = resizer_->dbNetwork();
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
