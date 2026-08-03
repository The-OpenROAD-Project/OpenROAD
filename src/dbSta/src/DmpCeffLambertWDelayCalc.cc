// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "DmpCeffLambertWDelayCalc.hh"

#include <algorithm>
#include <boost/math/special_functions/lambert_w.hpp>
#include <cmath>
#include <numbers>

#include "sta/ArcDelayCalc.hh"
#include "sta/Delay.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/Parasitics.hh"
#include "sta/Sdc.hh"
#include "sta/TableModel.hh"
#include "sta/TimingArc.hh"
#include "sta/Transition.hh"

namespace sta {

DmpCeffLambertWDelayCalc::DmpCeffLambertWDelayCalc(StaState* sta)
    : DmpCeffTwoPoleDelayCalc(sta)
{
}

ArcDelayCalc* DmpCeffLambertWDelayCalc::copy()
{
  return new DmpCeffLambertWDelayCalc(*this);
}

static double gateModelRd(const LibertyCell* cell,
                          const GateTableModel* gate_model,
                          const RiseFall* rf,
                          double in_slew,
                          double c2,
                          double c1,
                          const Pvt* pvt)
{
  float cap1 = c1 + c2;
  float cap2 = cap1 + 1e-15;
  float d1, d2, s1, s2;
  gate_model->gateDelay(pvt, in_slew, cap1, d1, s1);
  gate_model->gateDelay(pvt, in_slew, cap2, d2, s2);
  double vth = cell->libertyLibrary()->outputThreshold(rf);
  float rd = -std::log(vth) * std::abs(d1 - d2) / (cap2 - cap1);
  return rd;
}

ArcDcalcResult DmpCeffLambertWDelayCalc::gateDelay(
    const Pin* drvr_pin,
    const TimingArc* arc,
    const Slew& in_slew,
    float load_cap,
    const Parasitic* parasitic,
    const LoadPinIndexMap& load_pin_index_map,
    const Scene* scene,
    const MinMax* min_max)
{
  parasitics_ = scene->parasitics(min_max);

  const RiseFall* rf = arc->toEdge()->asRiseFall();
  const LibertyCell* drvr_cell = arc->from()->libertyCell();
  const LibertyLibrary* drvr_library = drvr_cell->libertyLibrary();

  vth_lambert_ = drvr_library->outputThreshold(rf);
  vl_lambert_ = drvr_library->slewLowerThreshold(rf);
  vh_lambert_ = drvr_library->slewUpperThreshold(rf);
  slew_derate_lambert_ = drvr_library->slewDerateFromLibrary();

  GateTableModel* table_model = arc->gateTableModel(scene, min_max);
  if (!table_model || !parasitic) {
    if (parasitic) {
      report_->warn(1041,
                    "cell {} delay model not supported on SPF parasitics "
                    "by DMP delay calculator",
                    drvr_cell->name());
    }
    return DmpCeffTwoPoleDelayCalc::gateDelay(drvr_pin,
                                              arc,
                                              in_slew,
                                              load_cap,
                                              parasitic,
                                              load_pin_index_map,
                                              scene,
                                              min_max);
  }

  float c2, rpi, c1;
  parasitics_->piModel(parasitic, c2, rpi, c1);
  if (std::isnan(c2) || std::isnan(c1) || std::isnan(rpi)) {
    report_->error(1040, "parasitic Pi model has NaNs.");
  }

  float in_slew1 = delayAsFloat(in_slew);

  const Pvt* pvt = pinPvt(drvr_pin, scene, min_max);
  double rd = gateModelRd(drvr_cell, table_model, rf, in_slew1, c2, c1, pvt);
  CeffResult ceff_res = calculateCeff(
      drvr_library, drvr_cell, pvt, table_model, rf, rd, in_slew1, c2, rpi, c1);

  double ceff = ceff_res.ceff;
  double gate_delay = ceff_res.gate_delay;
  double drvr_slew = ceff_res.drvr_slew;

  // Copy the drive and delay slew and modify them if pocv is enabled!
  ArcDelay gate_delay2(gate_delay);
  Slew drvr_slew2(drvr_slew);

  if (variables_->pocvEnabled()) {
    table_model->gateDelayPocv(pvt,
                               in_slew1,
                               ceff,
                               min_max,
                               variables_->pocvMode(),
                               gate_delay2,
                               drvr_slew2);
  }

  ArcDcalcResult dcalc_result(load_pin_index_map.size());
  dcalc_result.setGateDelay(gate_delay2);
  dcalc_result.setDrvrSlew(drvr_slew2);

  for (const auto& [load_pin, load_idx] : load_pin_index_map) {
    double wire_delay;
    double load_slew;
    loadDelaySlew(load_pin,
                  drvr_slew,
                  rf,
                  drvr_library,
                  parasitic,
                  wire_delay,
                  load_slew);
    // Copy pocv params from driver.
    ArcDelay wire_delay2(gate_delay2);
    Slew load_slew2(drvr_slew2);
    delaySetMean(wire_delay2, wire_delay);
    delaySetMean(load_slew2, load_slew);
    dcalc_result.setWireDelay(load_idx, wire_delay2);
    dcalc_result.setLoadSlew(load_idx, load_slew2);
  }
  return dcalc_result;
}

void DmpCeffLambertWDelayCalc::loadDelaySlew(const Pin* load_pin,
                                             double drvr_slew,
                                             const RiseFall* rf,
                                             const LibertyLibrary* drvr_library,
                                             const Parasitic* parasitic,
                                             // Return values.
                                             double& wire_delay,
                                             double& load_slew)
{
  if (!parasitics_->isPiPoleResidue(parasitic)) {
    DmpCeffTwoPoleDelayCalc::loadDelaySlew(load_pin,
                                           drvr_slew,
                                           rf,
                                           drvr_library,
                                           parasitic,
                                           wire_delay,
                                           load_slew);
    return;
  }

  Parasitic* pole_residue = parasitics_->findPoleResidue(parasitic, load_pin);
  if (!pole_residue) {
    DmpCeffTwoPoleDelayCalc::loadDelaySlew(load_pin,
                                           drvr_slew,
                                           rf,
                                           drvr_library,
                                           parasitic,
                                           wire_delay,
                                           load_slew);
    return;
  }
  if (parasitics_->poleResidueCount(pole_residue) < 2) {
    DmpCeffTwoPoleDelayCalc::loadDelaySlew(load_pin,
                                           drvr_slew,
                                           rf,
                                           drvr_library,
                                           parasitic,
                                           wire_delay,
                                           load_slew);
    return;
  }

  ComplexFloat pole1, residue1;
  ComplexFloat pole2, residue2;
  parasitics_->poleResidue(pole_residue, 0, pole1, residue1);
  parasitics_->poleResidue(pole_residue, 1, pole2, residue2);

  if (pole1.imag() != 0.0 || residue1.imag() != 0.0 || pole2.imag() != 0.0
      || residue2.imag() != 0.0 || delayZero(drvr_slew, this)) {
    DmpCeffTwoPoleDelayCalc::loadDelaySlew(load_pin,
                                           drvr_slew,
                                           rf,
                                           drvr_library,
                                           parasitic,
                                           wire_delay,
                                           load_slew);
    return;
  }

  // OpenSTA's parasitic reduction engine (reduceToPiPoleResidue2) returns
  // positive decay constants (p1 > 0, p2 > 0) representing pole magnitudes.
  double p1 = pole1.real();
  double k1 = residue1.real();
  double p2 = pole2.real();
  double k2 = residue2.real();

  // k1_p1_2 = k1 / (p1^2) and k2_p2_2 = k2 / (p2^2) are the
  // coefficients of the exponential decay terms in the second-order
  // ramp response. They represent the contribution of each pole to
  // the overall system delay.
  double k1_p1_2 = k1 / (p1 * p1);
  double k2_p2_2 = k2 / (p2 * p2);
  // B is the sum of these coefficients, representing the Elmore
  // delay (first moment) of the two-pole transfer function.
  double B = k1_p1_2 + k2_p2_2;

  float tt = delayAsFloat(drvr_slew) * slew_derate_lambert_
             / (vh_lambert_ - vl_lambert_);
  double y_tt
      = (tt - B + k1_p1_2 * std::exp(-p1 * tt) + k2_p2_2 * std::exp(-p2 * tt))
        / tt;

  double C = k1_p1_2;
  double D_vth = vth_lambert_ * tt + B;
  double arg_vth = -p1 * C * std::exp(-p1 * D_vth);

  double D_vl = vl_lambert_ * tt + B;
  double arg_vl = -p1 * C * std::exp(-p1 * D_vl);
  double D_vh = vh_lambert_ * tt + B;
  double arg_vh = -p1 * C * std::exp(-p1 * D_vh);

  static constexpr double inv_e = -1.0 / std::numbers::e;

  auto is_within_lambert_range
      = [](double arg) constexpr { return (arg >= inv_e && arg < 0.0); };

  // If we're outside the Lambert W function range, fall back to the normal
  // DMP solver.
  if ((y_tt >= vth_lambert_ && !is_within_lambert_range(arg_vth))
      || (y_tt >= vl_lambert_ && !is_within_lambert_range(arg_vl))
      || (y_tt >= vh_lambert_ && !is_within_lambert_range(arg_vh))) {
    DmpCeffTwoPoleDelayCalc::loadDelaySlew(load_pin,
                                           drvr_slew,
                                           rf,
                                           drvr_library,
                                           parasitic,
                                           wire_delay,
                                           load_slew);
    return;
  }

  wire_delay = loadDelay(vth_lambert_, p1, k1, k1_p1_2, B, tt, y_tt, arg_vth)
               - tt * vth_lambert_;

  float tl = loadDelay(vl_lambert_, p1, k1, k1_p1_2, B, tt, y_tt, arg_vl);
  float th = loadDelay(vh_lambert_, p1, k1, k1_p1_2, B, tt, y_tt, arg_vh);
  load_slew = (th - tl) / slew_derate_lambert_;

  thresholdAdjust(load_pin, drvr_library, rf, wire_delay, load_slew);
}

float DmpCeffLambertWDelayCalc::loadDelay(double vth,
                                          double p1,
                                          double k1,
                                          double k1_p1_2,
                                          double B,
                                          double tt,
                                          double y_tt,
                                          double arg)
{
  if (y_tt < vth) {
    double exp_arg
        = k1 * (std::exp(p1 * tt) - 1.0) / ((1.0 - vth) * p1 * p1 * tt);
    if (exp_arg <= 0.0) {
      return 0.0;
    }
    return std::log(exp_arg) / p1;
  } else {
    double D = vth * tt + B;
    double w = boost::math::lambert_w0(arg);
    double delay = D + w / p1;
    return static_cast<float>(delay);
  }
}

CeffResult DmpCeffLambertWDelayCalc::calculateCeff(
    const LibertyLibrary*,
    const LibertyCell*,
    const Pvt* pvt,
    const GateTableModel* gate_model,
    const RiseFall*,
    double rd,
    double in_slew,
    double c2,
    double rpi,
    double c1)
{
  double ceff;
  if (rd < 1e-2 || rpi < rd * 1e-3 || c1 == 0.0 || c1 < c2 * 1e-3
      || rpi == 0.0) {
    ceff = c2 + c1;
  } else {
    // Normalized coordinates with reciprocals to avoid divisions
    double inv_tot = 1.0 / (c1 + c2);
    double inv_rd = 1.0 / rd;
    double x = rpi * inv_rd;
    double y = c2 * inv_tot;
    double z = in_slew * inv_rd * inv_tot;

    double y_sqrt = std::sqrt(std::max(y, 0.0));
    double z2 = z * z;
    double yz = y_sqrt * z;

    // 18-coeff Padé coefficients.
    // To regenerate these coefficients:
    //   bazel build \
    //     //third_party/open_road/src/dbSta:generate_pade_coefficients
    static constexpr double a1_coef[6] = {-4.60512811e-01,
                                          6.91701256e-01,
                                          7.09008866e+00,
                                          3.24039351e+00,
                                          -4.63636243e+00,
                                          3.62412342e-02};
    static constexpr double b1_coef[6] = {6.66946102e-02,
                                          1.84900104e+00,
                                          5.89372830e+00,
                                          9.97500731e-01,
                                          -4.00536240e+00,
                                          8.14833452e-02};
    static constexpr double b2_coef[6] = {12.6815995e+00,
                                          -15.2315308e+00,
                                          3.99960993e-01,
                                          2.56651173e+00,
                                          -2.54326578e-01,
                                          -1.36694965e-02};

    auto eval_poly = [y_sqrt, y, z, yz, z2](const double c[6]) constexpr {
      return c[0] + c[1] * y_sqrt + c[2] * z + c[3] * y + c[4] * yz + c[5] * z2;
    };

    double a1 = eval_poly(a1_coef);
    double b1 = eval_poly(b1_coef);
    double b2 = eval_poly(b2_coef);

    double num = 1.0 + a1 * x;
    double den = 1.0 + b1 * x + b2 * x * x;

    double k = num / std::max(den, 1e-9);
    k = std::clamp(k, 0.0, 1.0);
    ceff = c2 + k * c1;
  }

  float gate_delay, drvr_slew;
  gate_model->gateDelay(pvt, in_slew, ceff, gate_delay, drvr_slew);
  return {ceff, gate_delay, drvr_slew};
}

ArcDelayCalc* makeDmpCeffLambertWDelayCalc(StaState* sta)
{
  return new DmpCeffLambertWDelayCalc(sta);
}

}  // namespace sta
