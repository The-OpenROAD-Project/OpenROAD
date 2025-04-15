// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "utl/MakeLogger.h"

#include <tcl.h>

#include "utl/Logger.h"
#include "utl/decode.h"

namespace utl {
extern const char* utl_tcl_inits[];
}

extern "C" {
extern int Utl_Init(Tcl_Interp* interp);
}

namespace ord {

using utl::Logger;

Logger* makeLogger(const char* log_filename, const char* metrics_filename)
{
  return new Logger(log_filename, metrics_filename);
}

void initLogger(Logger* logger, Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Utl_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, utl::utl_tcl_inits);
}

}  // namespace ord
