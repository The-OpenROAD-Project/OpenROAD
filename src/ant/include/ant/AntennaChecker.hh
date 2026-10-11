// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "odb/db.h"

namespace utl {
class Logger;
}

namespace ant {

struct Violation
{
  int routing_level;
  std::vector<odb::dbITerm*> gates;
  int diode_count_per_gate;
  double excess_ratio;
};

using Violations = std::vector<Violation>;

// One antenna ratio of a gate on a layer: the ratio the routing produces
// (actual), the rule's limit (required), and whether actual exceeds it.
struct RatioCheck
{
  double actual{0.0};
  double required{0.0};
  bool violated{false};
};

// The ratios checked for one gate on one layer that has an antenna rule.
// Side area ratios (psr, csr) apply to routing layers only.  car is unset
// when par already violated, as the check stops there.
struct LayerCheck
{
  odb::dbTechLayer* layer{nullptr};
  RatioCheck par;
  std::optional<RatioCheck> car;
  std::optional<RatioCheck> psr;
  std::optional<RatioCheck> csr;
  bool violated{false};
};

// The checks for one gate pin of a net, in layer order.
struct PinCheck
{
  odb::dbITerm* gate{nullptr};
  std::vector<LayerCheck> layers;
  bool violated{false};
};

// The checks for every gate pin of a net.  A net without wires has no pins.
struct NetCheck
{
  odb::dbNet* net{nullptr};
  std::vector<PinCheck> pins;
  bool violated{false};
};

class AntennaChecker
{
 public:
  AntennaChecker(odb::dbDatabase* db, utl::Logger* logger);
  ~AntennaChecker();

  // net nullptr -> check all nets
  int checkAntennas(odb::dbNet* net = nullptr,
                    int num_threads = 1,
                    bool verbose = false);
  // Number of violating nets / gate pins found by the last checkAntennas.
  int antennaViolationCount() const;
  int pinViolationCount() const;
  // Checks one net's routing and returns every ratio checked for its gates.
  NetCheck checkNet(odb::dbNet* net, float ratio_margin = 0);
  Violations getAntennaViolations(odb::dbNet* net,
                                  odb::dbMTerm* diode_mterm,
                                  float ratio_margin);
  void setReportFileName(const char* file_name);

  // Used in repair
  void initAntennaRules();
  void makeNetWiresFromGuides(const std::vector<odb::dbNet*>& nets);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace ant
