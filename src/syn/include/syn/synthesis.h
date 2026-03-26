// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "utl/Logger.h"

namespace syn {

class Synthesis : public sta::dbStaState
{
 public:
  Synthesis(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);
  ~Synthesis();

 private:
  odb::dbDatabase* db_ = nullptr;
  utl::Logger* logger_ = nullptr;
};

}  // namespace syn
