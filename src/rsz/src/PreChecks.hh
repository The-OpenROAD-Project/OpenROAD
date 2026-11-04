// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <map>

#include "db_sta/dbSta.hh"
#include "utl/Logger.h"

namespace sta {
class Pin;
class Scene;
}  // namespace sta

namespace rsz {

class Resizer;
using dbSta = sta::dbSta;

class PreChecks
{
 public:
  PreChecks(Resizer* resizer);
  void checkSlewLimit(const sta::Pin* load_pin,
                      float ref_cap,
                      float max_load_slew,
                      const sta::Scene* corner);
  void checkCapLimit(const sta::Pin* drvr_pin);

 private:
  utl::Logger* logger_ = nullptr;
  dbSta* sta_ = nullptr;
  Resizer* resizer_;

  struct SlewCheckCache
  {
    float best_case_slew = -1.0;
    float best_case_slew_load = -1.0;
    bool best_case_slew_computed = false;
  };

  // Best slew numbers per corner to ensure the max_slew is reasonable.
  std::map<const sta::Scene*, SlewCheckCache> slew_check_by_corner_;

  // smallest buffer/inv cap to ensure the max_cap in Liberty/SDC is reasonable
  float min_cap_load_ = default_min_cap_load;
  bool min_cap_load_computed_ = false;

  static constexpr float default_min_cap_load = 1e-18;  // 1aF
};

}  // namespace rsz
