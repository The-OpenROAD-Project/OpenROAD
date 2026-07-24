// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string_view>

#include "dcalc/DmpCeff.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Delay.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"

namespace sta {

struct CeffResult
{
  double ceff;
  double gate_delay;
  double drvr_slew;
};

// Declaring DmpCeffTwoPoleDelayCalc exactly as defined in OpenSTA.
// This allows us to inherit from it and delegate fallback solver calls.
class DmpCeffTwoPoleDelayCalc : public DmpCeffDelayCalc
{
 public:
  DmpCeffTwoPoleDelayCalc(StaState* sta);
  ArcDelayCalc* copy() override;
  std::string_view name() const override;
  Parasitic* findParasitic(const Pin* drvr_pin,
                           const RiseFall* rf,
                           const Scene* scene,
                           const MinMax* min_max) override;
  ArcDcalcResult inputPortDelay(const Pin* port_pin,
                                float in_slew,
                                const RiseFall* rf,
                                const Parasitic* parasitic,
                                const LoadPinIndexMap& load_pin_index_map,
                                const Scene* scene,
                                const MinMax* min_max) override;
  ArcDcalcResult gateDelay(const Pin* drvr_pin,
                           const TimingArc* arc,
                           const Slew& in_slew,
                           float load_cap,
                           const Parasitic* parasitic,
                           const LoadPinIndexMap& load_pin_index_map,
                           const Scene* scene,
                           const MinMax* min_max) override;

 protected:
  void loadDelaySlew(const Pin* load_pin,
                     double drvr_slew,
                     const RiseFall* rf,
                     const LibertyLibrary* drvr_library,
                     const Parasitic* parasitic,
                     // Return values.
                     double& wire_delay,
                     double& load_slew) override;

 protected:
  float loadDelay(double vth,
                  double p1,
                  double p2,
                  double k1,
                  double k2,
                  double B,
                  double k1_p1_2,
                  double k2_p2_2,
                  double tt,
                  double y_tt);

 private:
  void loadDelay(double drvr_slew,
                 Parasitic* pole_residue,
                 double p1,
                 double k1,
                 double& wire_delay,
                 double& load_slew);

  bool parasitic_is_pole_residue_{false};
  float vth_{0.0F};
  float vl_{0.0F};
  float vh_{0.0F};
  float slew_derate_{0.0F};
};

class DmpCeffLambertWDelayCalc : public DmpCeffTwoPoleDelayCalc
{
 public:
  DmpCeffLambertWDelayCalc(StaState* sta);
  ArcDelayCalc* copy() override;
  std::string_view name() const override { return "dmp_ceff_lambert_w"; }

  ArcDcalcResult gateDelay(const Pin* drvr_pin,
                           const TimingArc* arc,
                           const Slew& in_slew,
                           float load_cap,
                           const Parasitic* parasitic,
                           const LoadPinIndexMap& load_pin_index_map,
                           const Scene* scene,
                           const MinMax* min_max) override;

 protected:
  void loadDelaySlew(const Pin* load_pin,
                     double drvr_slew,
                     const RiseFall* rf,
                     const LibertyLibrary* drvr_library,
                     const Parasitic* parasitic,
                     // Return values.
                     double& wire_delay,
                     double& load_slew) override;

  CeffResult calculateCeff(const LibertyLibrary* library,
                           const LibertyCell* drvr_cell,
                           const Pvt* pvt,
                           const GateTableModel* gate_model,
                           const RiseFall* rf,
                           double rd,
                           double in_slew,
                           double c2,
                           double rpi,
                           double c1);

 protected:
  float loadDelay(double vth,
                  double p1,
                  double k1,
                  double k1_p1_2,
                  double B,
                  double tt,
                  double y_tt);

  float vth_lambert_{0.0F};
  float vl_lambert_{0.0F};
  float vh_lambert_{0.0F};
  float slew_derate_lambert_{0.0F};
};

ArcDelayCalc* makeDmpCeffLambertWDelayCalc(StaState* sta);

}  // namespace sta
