// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <random>

#include "DmpCeffLambertWDelayCalc.hh"
#include "db_sta/dbSta.hh"
#include "tst/IntegratedFixture.h"

namespace sta {

class TestLambertW : public tst::IntegratedFixture
{
 protected:
  TestLambertW()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/dbSta/test/")
  {
  }
};

// Subclasses to expose protected members for unit testing
class ExposedLambertWDelayCalc : public DmpCeffLambertWDelayCalc
{
 public:
  using DmpCeffLambertWDelayCalc::DmpCeffLambertWDelayCalc;

  float evaluateLoadDelay(double threshold_voltage,
                          double pole1,
                          double residue1,
                          double residue1_div_pole1_squared,
                          double elmore_delay,
                          double transition_time,
                          double output_voltage_at_transition)
  {
    return loadDelay(threshold_voltage,
                     pole1,
                     residue1,
                     residue1_div_pole1_squared,
                     elmore_delay,
                     transition_time,
                     output_voltage_at_transition);
  }
};

class ExposedTwoPoleDelayCalc : public DmpCeffTwoPoleDelayCalc
{
 public:
  using DmpCeffTwoPoleDelayCalc::DmpCeffTwoPoleDelayCalc;

  float evaluateLoadDelay(double threshold_voltage,
                          double pole1,
                          double pole2,
                          double residue1,
                          double residue2,
                          double elmore_delay,
                          double residue1_div_pole1_squared,
                          double residue2_div_pole2_squared,
                          double transition_time,
                          double output_voltage_at_transition)
  {
    return loadDelay(threshold_voltage,
                     pole1,
                     pole2,
                     residue1,
                     residue2,
                     elmore_delay,
                     residue1_div_pole1_squared,
                     residue2_div_pole2_squared,
                     transition_time,
                     output_voltage_at_transition);
  }
};

struct PiModelParameters
{
  double pole1;
  double pole2;
  double residue1;
  double residue2;
  double residue1_div_pole1_squared;
  double residue2_div_pole2_squared;
  double elmore_delay;
  double transition_time;
  double threshold_voltage;
  double output_voltage_at_transition;
  double residue_coeff_C;
  double crossing_offset_D;
  double lambert_argument;
};

// Calculates the two-pole transfer function coefficients and initial response
// parameters from randomized physical properties of a Pi-model circuit.
// The transfer function assumes a DC gain of 1.0 (residue1/pole1 +
// residue2/pole2 = 1.0).
PiModelParameters calculatePiModelParameters(double pole1,
                                             double pole2,
                                             double residue1_fraction,
                                             double transition_time,
                                             double threshold_voltage)
{
  PiModelParameters params;
  params.pole1 = pole1;
  params.pole2 = pole2;
  params.residue1 = residue1_fraction * pole1;
  params.residue2 = (1.0 - residue1_fraction) * pole2;

  params.residue1_div_pole1_squared = params.residue1 / (pole1 * pole1);
  params.residue2_div_pole2_squared = params.residue2 / (pole2 * pole2);
  params.elmore_delay
      = params.residue1_div_pole1_squared + params.residue2_div_pole2_squared;

  params.transition_time = transition_time;
  params.threshold_voltage = threshold_voltage;

  // output_voltage_at_transition is the value of the output voltage at the end
  // of the input ramp.
  params.output_voltage_at_transition
      = (transition_time - params.elmore_delay
         + params.residue1_div_pole1_squared
               * std::exp(-pole1 * transition_time)
         + params.residue2_div_pole2_squared
               * std::exp(-pole2 * transition_time))
        / transition_time;

  params.residue_coeff_C = params.residue1_div_pole1_squared;
  params.crossing_offset_D
      = threshold_voltage * transition_time + params.elmore_delay;

  // lambert_argument is the argument to the Lambert W function for checking
  // boundary limits.
  params.lambert_argument = -pole1 * params.residue_coeff_C
                            * std::exp(-pole1 * params.crossing_offset_D);
  return params;
}

TEST_F(TestLambertW, ValidateRandomPImodels)
{
  readVerilogAndSetup("TestDbSta_0.v");

  std::unique_ptr<ExposedLambertWDelayCalc> lambert_calc
      = std::make_unique<ExposedLambertWDelayCalc>(sta_.get());
  std::unique_ptr<ExposedTwoPoleDelayCalc> twopole_calc
      = std::make_unique<ExposedTwoPoleDelayCalc>(sta_.get());

  std::mt19937 gen(42);  // fixed seed for reproducibility
  std::uniform_real_distribution<double> dis_p1(1.0, 50.0);
  std::uniform_real_distribution<double> dis_p2(60.0, 500.0);  // p2 > p1
  std::uniform_real_distribution<double> dis_k1(0.1, 0.9);
  // transition times in ns
  std::uniform_real_distribution<double> dis_tt(0.01, 1.0);
  std::uniform_real_distribution<double> dis_vth(0.1, 0.9);

  int samples = 0;
  while (samples < 100) {
    double pole1 = dis_p1(gen);
    double pole2 = dis_p2(gen);
    double residue1_fraction = dis_k1(gen);
    double transition_time = dis_tt(gen);
    double threshold_voltage = dis_vth(gen);

    PiModelParameters params = calculatePiModelParameters(
        pole1, pole2, residue1_fraction, transition_time, threshold_voltage);
    static constexpr double inv_e = -1.0 / std::numbers::e;

    if (params.output_voltage_at_transition < threshold_voltage
        || (params.lambert_argument >= inv_e
            && params.lambert_argument < 0.0)) {
      float t_lambert = lambert_calc->evaluateLoadDelay(
          threshold_voltage,
          pole1,
          params.residue1,
          params.residue1_div_pole1_squared,
          params.elmore_delay,
          transition_time,
          params.output_voltage_at_transition);
      float t_twopole = twopole_calc->evaluateLoadDelay(
          threshold_voltage,
          pole1,
          pole2,
          params.residue1,
          params.residue2,
          params.elmore_delay,
          params.residue1_div_pole1_squared,
          params.residue2_div_pole2_squared,
          transition_time,
          params.output_voltage_at_transition);

      float diff = std::abs(t_lambert - t_twopole);
      float max_val = std::max(std::abs(t_lambert), std::abs(t_twopole));

      if (max_val > 0.05) {
        float rel_err = diff / max_val;
        EXPECT_LT(rel_err, 0.06)
            << "Mismatch at sample " << samples << ": Lambert=" << t_lambert
            << ", TwoPole=" << t_twopole << ", RelErr=" << rel_err;
      } else {
        EXPECT_LT(diff, 0.01) << "Absolute error at sample " << samples
                              << ": Lambert=" << t_lambert
                              << ", TwoPole=" << t_twopole << ", Diff=" << diff;
      }

      samples++;
    }
  }
}

}  // namespace sta
