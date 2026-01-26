// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <memory>
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

class AntennaChecker
{
 public:
  AntennaChecker(odb::dbDatabase* db, utl::Logger* logger);
  ~AntennaChecker();

  // net nullptr -> check all nets
  int checkAntennas(odb::dbNet* net = nullptr,
                    int num_threads = 1,
                    bool verbose = false);
  int antennaViolationCount() const;
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
