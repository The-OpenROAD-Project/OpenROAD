// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "powerBase.h"

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <utility>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "nesterovBase.h"
#include "odb/db.h"
#include "placerBase.h"
#include "power/Power.hh"
#include "sta/Liberty.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/PowerClass.hh"
#include "sta/Scene.hh"
#include "utl/Logger.h"

namespace gpl {

using utl::GPL;

static const sta::LibertyCell* bufferOrInverter(sta::dbNetwork* network,
                                                const sta::Instance* inst)
{
  const sta::LibertyCell* cell = network->libertyCell(inst);
  if (cell == nullptr || !(cell->isBuffer() || cell->isInverter())) {
    return nullptr;
  }
  return cell;
}

PowerBase::PowerBase(std::shared_ptr<NesterovBaseCommon> nbc,
                     sta::dbSta* sta,
                     float net_weight,
                     utl::Logger* log)
    : nbc_(std::move(nbc)),
      sta_(sta),
      network_(sta->getDbNetwork()),
      scene_(sta->cmdScene()),
      log_(log),
      net_weight_(net_weight)
{
  if (scene_ != nullptr) {
    sta_->updateGeneratedClks();
    min_period_ = sta_->power()->clockMinPeriod(scene_->mode()->sdc());
  }
  if (min_period_ <= 0) {
    log_->warn(
        GPL, 431, "Power-driven: no clocks. Power-driven mode disabled.");
    return;
  }
  collectUserActivityPins();
  addOwner(network_->block());
}

// Buffer and inverter outputs with user-given activity from
//  - set_power_activity -pins
//  - read_vcd
//  - read_saif
void PowerBase::collectUserActivityPins()
{
  std::unique_ptr<sta::LeafInstanceIterator> inst_iter(
      network_->leafInstanceIterator());
  while (inst_iter->hasNext()) {
    const sta::Instance* inst = inst_iter->next();
    const sta::LibertyCell* cell = bufferOrInverter(network_, inst);
    if (cell == nullptr) {
      continue;
    }
    sta::LibertyPort* input;
    sta::LibertyPort* output;
    cell->bufferPorts(input, output);
    const sta::Pin* pin = network_->findPin(inst, output);
    if (pin == nullptr) {
      continue;
    }
    const sta::PwrActivityOrigin origin = sta_->activity(pin, scene_).origin();
    if (origin == sta::PwrActivityOrigin::user
        || origin == sta::PwrActivityOrigin::vcd
        || origin == sta::PwrActivityOrigin::saif) {
      user_activity_pins_.insert(pin);
    }
  }
}

void PowerBase::setPowerNetWeights()
{
  if (min_period_ <= 0) {
    return;
  }
  for (auto& gNet : nbc_->getGNets()) {
    float toggles = 0;
    if (gNet->getGPins().size() > 1) {
      for (const Net* pb_net : gNet->getPbNets()) {
        toggles = std::max(toggles,
                           netToggles(network_->dbToSta(pb_net->getDbNet())));
      }
    }
    const float power_weight = net_weight_ * toggles;
    gNet->setPowerWeight(power_weight);
    debugPrint(log_,
               GPL,
               "power",
               1,
               "net:{} toggles:{} power weight:{}",
               gNet->getPbNet()->getDbNet()->getConstName(),
               toggles,
               power_weight);
  }
}

// Cache source toggles per cycle, capped at one toggle per cycle.
// Assuming resizer operations preserve logic throughout the run.
float PowerBase::netToggles(const sta::Net* net)
{
  sta::PinSet* drivers = network_->drivers(net);
  if (drivers == nullptr || drivers->empty()
      || sta_->isClock(*drivers->begin(), scene_->mode())) {
    return 0;
  }
  const sta::Pin* source = activitySource(*drivers->begin());
  auto [it, inserted] = source_toggles_.try_emplace(source, 0.0f);
  if (inserted) {
    it->second = std::min(
        sta_->activity(source, scene_).density() * min_period_, 1.0f);
  }
  return it->second;
}

// Buffers and inverters inherit toggle activity from their upstream driver.
const sta::Pin* PowerBase::activitySource(const sta::Pin* drvr) const
{
  constexpr int kMaxChain = 64;
  for (int depth = 0; depth < kMaxChain; depth++) {
    if (user_activity_pins_.contains(drvr)) {
      return drvr;
    }
    const sta::Instance* inst = network_->instance(drvr);
    const sta::LibertyCell* cell = bufferOrInverter(network_, inst);
    if (cell == nullptr) {
      return drvr;
    }
    sta::LibertyPort* input;
    sta::LibertyPort* output;
    cell->bufferPorts(input, output);
    const sta::Pin* in_pin = network_->findPin(inst, input);
    sta::PinSet* drivers = in_pin ? network_->drivers(in_pin) : nullptr;
    if (drivers == nullptr || drivers->empty()) {
      return drvr;
    }
    drvr = *drivers->begin();
  }
  return drvr;
}

// Remove destroyed pins to prevent stale entries when ODB reuses pin address.
void PowerBase::inDbITermDestroy(odb::dbITerm* iterm)
{
  const sta::Pin* pin = network_->dbToSta(iterm);
  source_toggles_.erase(pin);
  user_activity_pins_.erase(pin);
}

}  // namespace gpl
