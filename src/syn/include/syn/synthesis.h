// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "syn/ir/Graph.h"
#include "utl/Logger.h"

namespace syn {

class Synthesis : public sta::dbStaState
{
 public:
  Synthesis(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);
  ~Synthesis();

  // Parse and elaborate SystemVerilog sources into the internal Graph.
  // Arguments are forwarded to slang (file paths, -I, --top, defines, etc.).
  bool elaborate(const std::vector<std::string>& args);

  Graph* graph() { return graph_ ? &*graph_ : nullptr; }

 private:
  odb::dbDatabase* db_ = nullptr;
  utl::Logger* logger_ = nullptr;
  std::optional<Graph> graph_;
};

}  // namespace syn
