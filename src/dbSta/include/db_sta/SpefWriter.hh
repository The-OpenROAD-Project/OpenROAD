// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <map>

#include "db_sta/dbSta.hh"
#include "utl/Logger.h"

namespace sta {

using utl::Logger;

class SpefWriter
{
 public:
  SpefWriter(Logger* logger,
             dbSta* sta,
             std::map<Corner*, std::ostream*>& spef_streams);
  void writeHeader();
  void writePorts();
  void writeNet(Corner* corner, const Net* net, Parasitic* parasitic);

 private:
  Logger* logger_;
  dbSta* sta_;
  dbNetwork* network_;
  Parasitics* parasitics_;

  std::map<Corner*, std::ostream*> spef_streams_;
};

}  // namespace sta
