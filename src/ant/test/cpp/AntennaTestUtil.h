// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ant/AntennaChecker.hh"
#include "gtest/gtest.h"
#include "odb/db.h"

namespace ant::test {

// An expected RatioCheck.  Reports print ratios with two decimals, so
// values taken from a report match to within kReportTolerance.
struct Ratio
{
  double actual;
  double required;
  bool violated;
};

inline constexpr double kReportTolerance = 0.005;

struct ExpectedLayer
{
  std::string layer;
  Ratio par;
  std::optional<Ratio> car;
  std::optional<Ratio> psr;
  std::optional<Ratio> csr;
};

struct ExpectedPin
{
  std::string pin;  // "inst/mterm"
  std::vector<ExpectedLayer> layers;
};

inline std::string pinName(const odb::dbITerm* iterm)
{
  return iterm->getInst()->getName() + "/" + iterm->getMTerm()->getName();
}

inline void expectRatio(const std::optional<RatioCheck>& actual,
                        const std::optional<Ratio>& expected,
                        const std::string& where)
{
  ASSERT_EQ(actual.has_value(), expected.has_value()) << where;
  if (!expected) {
    return;
  }
  EXPECT_NEAR(actual->actual, expected->actual, kReportTolerance) << where;
  EXPECT_NEAR(actual->required, expected->required, kReportTolerance) << where;
  EXPECT_EQ(actual->violated, expected->violated) << where;
}

// The check of the named net in design; fails the test if it is missing.
inline const NetCheck& netCheck(const DesignCheck& design,
                                std::string_view net_name)
{
  static const NetCheck missing;
  for (const NetCheck& net_check : design.nets) {
    if (net_check.net->getName() == net_name) {
      return net_check;
    }
  }
  ADD_FAILURE() << "No check for net " << net_name;
  return missing;
}

// Expects net_check to hold exactly the expected pins and layers, in order.
inline void expectNetCheck(const NetCheck& net_check,
                           const std::vector<ExpectedPin>& expected)
{
  ASSERT_EQ(net_check.pins.size(), expected.size());
  for (size_t p = 0; p < expected.size(); ++p) {
    const PinCheck& pin = net_check.pins[p];
    const ExpectedPin& expected_pin = expected[p];
    EXPECT_EQ(pinName(pin.gate), expected_pin.pin);
    ASSERT_EQ(pin.layers.size(), expected_pin.layers.size())
        << expected_pin.pin;
    bool pin_violated = false;
    for (size_t l = 0; l < expected_pin.layers.size(); ++l) {
      const LayerCheck& layer = pin.layers[l];
      const ExpectedLayer& expected_layer = expected_pin.layers[l];
      const std::string where = expected_pin.pin + " " + expected_layer.layer;
      EXPECT_EQ(layer.layer->getName(), expected_layer.layer) << where;
      expectRatio(layer.par, expected_layer.par, where + " par");
      expectRatio(layer.car, expected_layer.car, where + " car");
      expectRatio(layer.psr, expected_layer.psr, where + " psr");
      expectRatio(layer.csr, expected_layer.csr, where + " csr");
      const bool layer_violated
          = expected_layer.par.violated
            || (expected_layer.car && expected_layer.car->violated)
            || (expected_layer.psr && expected_layer.psr->violated)
            || (expected_layer.csr && expected_layer.csr->violated);
      EXPECT_EQ(layer.violated, layer_violated) << where;
      pin_violated = pin_violated || layer_violated;
    }
    EXPECT_EQ(pin.violated, pin_violated) << expected_pin.pin;
  }
}

}  // namespace ant::test
