// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
}

namespace ram {

// A multi-port register file as placed standard cells: word rows by bit
// columns, every cell of its core placed by construction (see
// src/regfile.h). Reads the spec file, builds the block into `db`, whose
// libraries must hold the spec's cells, and writes each view whose path
// is not empty. `check_ports` names a Verilog file whose module the spec
// must match port for port before anything is built.
void generateRegfile(odb::dbDatabase* db,
                     utl::Logger* logger,
                     const std::string& spec_path,
                     const std::string& verilog_path,
                     const std::string& def_path,
                     const std::string& lef_path,
                     const std::string& liberty_path,
                     const std::string& check_ports);

}  // namespace ram
