// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <ostream>

#include "db_sta/dbSta.hh"
#include "sta/Parasitics.hh"
#include "sta/ParasiticsClass.hh"
#include "utl/Logger.h"

namespace sta {

class Scene;
class Parasitic;

class SpefWriter
{
 public:
  SpefWriter(utl::Logger* logger,
             dbSta* sta,
             std::map<Scene*, std::ostream*>& spef_streams);
  void writeHeader();
  void writePorts();
  void writeNet(Scene* scene,
                const Net* net,
                Parasitic* parasitic,
                Parasitics* parasitics);

 private:
  utl::Logger* logger_;
  dbSta* sta_;
  dbNetwork* network_;

  std::map<Scene*, std::ostream*> spef_streams_;
};

}  // namespace sta
