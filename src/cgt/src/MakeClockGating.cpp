// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "cgt/MakeClockGating.h"

#include "cgt/ClockGating.h"
#include "utl/decode.h"

extern "C" {
extern int Cgt_Init(Tcl_Interp* interp);
}

namespace cgt {

extern const char* cgt_tcl_inits[];

ClockGating* makeClockGating()
{
  return new ClockGating();
}

void initClockGating(ClockGating* const cgt,
                     Tcl_Interp* const tcl_interp,
                     utl::Logger* const logger,
                     sta::dbSta* const sta)
{
  Cgt_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, cgt::cgt_tcl_inits);
  cgt->init(logger, sta);
}

void deleteClockGating(cgt::ClockGating* const clock_gating)
{
  delete clock_gating;
}

}  // namespace cgt
