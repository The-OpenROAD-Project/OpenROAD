// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <ostream>

#include "db_sta/dbSta.hh"
#include "sta/Corner.hh"
#include "sta/Parasitics.hh"
#include "sta/ParasiticsClass.hh"
#include "utl/Logger.h"

namespace sta {

class SpefWriter
{
 public:
  SpefWriter(utl::Logger* logger,
             dbSta* sta,
             std::map<Corner*, std::ostream*>& spef_streams);
  void writeHeader();
  void writePorts();
  void writeNet(Corner* corner, const Net* net, Parasitic* parasitic);

 private:
  utl::Logger* logger_;
  dbSta* sta_;
  dbNetwork* network_;
  Parasitics* parasitics_;

  std::map<Corner*, std::ostream*> spef_streams_;
};

}  // namespace sta
