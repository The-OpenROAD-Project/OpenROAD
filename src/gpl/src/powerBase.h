// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "odb/dbBlockCallBackObj.h"

namespace sta {
class dbNetwork;
class dbSta;
class Net;
class Pin;
class Scene;
}  // namespace sta

namespace utl {
class Logger;
}

namespace gpl {

class NesterovBaseCommon;

// PowerBase weights nets by their switching activity during global placement,
// so that active nets are kept short.
class PowerBase : public odb::dbBlockCallBackObj
{
 public:
  PowerBase(std::shared_ptr<NesterovBaseCommon> nbc,
            sta::dbSta* sta,
            float net_weight,
            utl::Logger* log);

  // Rerun after every netlist change.
  void setPowerNetWeights();
  void inDbITermDestroy(odb::dbITerm* iterm) override;

 private:
  void collectUserActivityPins();
  float netToggles(const sta::Net* net);
  const sta::Pin* activitySource(const sta::Pin* drvr) const;

  std::shared_ptr<NesterovBaseCommon> nbc_;
  sta::dbSta* sta_ = nullptr;
  sta::dbNetwork* network_ = nullptr;
  const sta::Scene* scene_ = nullptr;
  utl::Logger* log_ = nullptr;
  float net_weight_ = 0;
  // Zero disables power-driven mode.
  float min_period_ = 0;
  std::unordered_map<const sta::Pin*, float> source_toggles_;
  // Buffer and inverter outputs annotated by set_power_activity, VCD or SAIF.
  std::unordered_set<const sta::Pin*> user_activity_pins_;
};

}  // namespace gpl
