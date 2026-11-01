// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <memory>

#include "db_sta/dbSta.hh"
#include "utl/Logger.h"

namespace sta {
class Scene;
}

namespace cgt {

class ClockGating
{
 public:
  ClockGating(utl::Logger* logger, sta::dbSta* open_sta);
  ~ClockGating();

  // Main function that performs the clock gating process. The scene's Liberty
  // libraries are used for logic analysis; null is only valid with one scene.
  void run(sta::Scene* scene = nullptr);

  void setMinInstances(int min_instances);
  void setMaxCover(int max_cover);
  void setDumpDir(const char* dir);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace cgt
