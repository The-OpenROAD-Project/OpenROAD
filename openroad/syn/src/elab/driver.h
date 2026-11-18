// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

//
// syn IR backend for sv-elab
//
// Entry point: parse SystemVerilog files and elaborate into a syn::Graph.
//
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "syn/ir/Graph.h"

namespace utl {
class Logger;
}

namespace sta {
class dbSta;
}

namespace syn {

// Defined in error.cc to work around fmt version conflict, see error.cc
[[noreturn]] void reportError(utl::Logger* logger,
                              int code,
                              std::string_view message);

// Parse and elaborate SystemVerilog sources into a Graph.
// Arguments are passed through to slang's command-line parser
// (file paths, -I, --top, defines, etc.).
// If sta is non-null, Liberty cell definitions are imported as blackboxes.
std::unique_ptr<Graph> elaborate(utl::Logger* logger,
                                 const std::vector<std::string>& args,
                                 sta::dbSta* sta = nullptr);
std::unique_ptr<Graph> elaborateText(const std::string& source,
                                     const std::vector<std::string>& args = {},
                                     sta::dbSta* sta = nullptr);

}  // namespace syn
