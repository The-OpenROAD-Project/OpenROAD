// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include "src/odb/include/odb/db.h"
#include "src/odb/include/odb/geom.h"
#include "src/utl/include/utl/Logger.h"

namespace fin {

////////////////////////////////////////////////////////////////

class Finale
{
 public:
  Finale(odb::dbDatabase* db, utl::Logger* logger);

  void densityFill(const char* rules_filename, const odb::Rect& fill_area);

  void setDebug();

 private:
  odb::dbDatabase* db_ = nullptr;
  utl::Logger* logger_ = nullptr;
  bool debug_ = false;
};

}  // namespace fin
