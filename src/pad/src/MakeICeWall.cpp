// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "pad/MakeICeWall.h"

#include <tcl.h>

#include "pad/ICeWall.h"
#include "utl/decode.h"

extern "C" {
extern int Pad_Init(Tcl_Interp* interp);
}

namespace pad {

extern const char* pad_tcl_inits[];

pad::ICeWall* makeICeWall()
{
  return new pad::ICeWall();
}

void deleteICeWall(pad::ICeWall* icewall)
{
  delete icewall;
}

void initICeWall(pad::ICeWall* icewall,
                 odb::dbDatabase* db,
                 utl::Logger* logger,
                 Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Pad_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, pad::pad_tcl_inits);

  icewall->init(db, logger);
}

}  // namespace pad
