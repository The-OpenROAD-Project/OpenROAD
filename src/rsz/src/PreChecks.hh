// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once
#include "db_sta/dbSta.hh"
#include "utl/Logger.h"

namespace rsz {

class Resizer;
using dbSta = sta::dbSta;

class PreChecks
{
 public:
  PreChecks(Resizer* resizer);
  void checkSlewLimit(float ref_cap, float max_load_slew);
  void checkCapLimit(const sta::Pin* drvr_pin);

 private:
  utl::Logger* logger_ = nullptr;
  dbSta* sta_ = nullptr;
  Resizer* resizer_;

  // best slew numbers to ensure the max_slew in SDC is reasonable
  float best_case_slew_ = -1.0;
  float best_case_slew_load_ = -1.0;
  bool best_case_slew_computed_ = false;

  // smallest buffer/inv cap to ensure the max_cap in Liberty/SDC is reasonable
  float min_cap_load_ = default_min_cap_load;
  bool min_cap_load_computed_ = false;

  static constexpr float default_min_cap_load = 1e-18;  // 1aF
};

}  // namespace rsz
