// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

extern "C" {
struct Tcl_Interp;
}

namespace utl {

void initLogger(Tcl_Interp* tcl_interp);

}  // namespace utl
