// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "dft/MakeDft.hh"

#include "tcl.h"
#include "utl/decode.h"

namespace dft {
extern const char* dft_tcl_inits[];

extern "C" {
extern int Dft_Init(Tcl_Interp* interp);
}

void initDft(Tcl_Interp* tcl_interp)
{
  Dft_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, dft::dft_tcl_inits);
}

}  // namespace dft
