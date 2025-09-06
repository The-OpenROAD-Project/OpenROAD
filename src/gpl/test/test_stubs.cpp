// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Stubs for objects / functions required to link against openroad_lib.
struct Tcl_Interp;

int cmd_argc = 0;
char** cmd_argv = nullptr;

namespace ord {

int tclInit(Tcl_Interp* interp)
{
  return -1;
}

}  // namespace ord
