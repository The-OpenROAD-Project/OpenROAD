// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once
#include "db_sta/dbSta.hh"

namespace rsz {

class Resizer;
using utl::Logger;
using dbSta = sta::dbSta;

class PreChecks
{
 public:
  PreChecks(Resizer* resizer);
  void checkSlewLimit(float ref_cap, float max_load_slew);

 private:
  Logger* logger_ = nullptr;
  dbSta* sta_ = nullptr;
  Resizer* resizer_;

  // best slew numbers to ensure the max_slew in SDC is reasonable
  float best_case_slew_ = -1.0;
  float best_case_slew_load_ = -1.0;
  bool best_case_slew_computed_ = false;
};

}  // namespace rsz
