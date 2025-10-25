// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "tcl.h"

extern "C" {
extern int Odbtcl_Init(Tcl_Interp* interp);
}

int odbTclAppInit(Tcl_Interp* interp)
{
  if (Odbtcl_Init(interp) == TCL_ERROR) {
    return TCL_ERROR;
  }

  return TCL_OK;
}

int main(int argc, char* argv[])
{
  Tcl_Main(argc, argv, odbTclAppInit);
}
