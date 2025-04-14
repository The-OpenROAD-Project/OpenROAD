// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include "odb/db.h"

namespace utl {
class Logger;
}

namespace fin {

using utl::Logger;

////////////////////////////////////////////////////////////////

class Finale
{
 public:
  void init(odb::dbDatabase* db, Logger* logger);

  void densityFill(const char* rules_filename, const odb::Rect& fill_area);

  void setDebug();

 private:
  odb::dbDatabase* db_ = nullptr;
  Logger* logger_ = nullptr;
  bool debug_ = false;
};

}  // namespace fin
