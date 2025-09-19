// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

extern "C" {
struct Tcl_Interp;
}

namespace utl {
class Logger;

utl::Logger* makeLogger(const char* log_filename, const char* metrics_filename);
void initLogger(utl::Logger* logger, Tcl_Interp* tcl_interp);

}  // namespace utl
