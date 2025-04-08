// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "dft/MakeDft.hh"

#include "DftConfig.hh"
#include "ScanReplace.hh"
#include "dft/Dft.hh"
#include "ord/OpenRoad.hh"

namespace sta {
extern const char* dft_tcl_inits[];
extern void evalTclInit(Tcl_Interp*, const char*[]);
}  // namespace sta

namespace dft {

extern "C" {
extern int Dft_Init(Tcl_Interp* interp);
}

dft::Dft* makeDft()
{
  return new dft::Dft();
}

void initDft(ord::OpenRoad* openroad)
{
  Tcl_Interp* interp = openroad->tclInterp();
  Dft_Init(interp);
  sta::evalTclInit(interp, sta::dft_tcl_inits);
  openroad->getDft()->init(
      openroad->getDb(), openroad->getSta(), openroad->getLogger());
}

void deleteDft(dft::Dft* dft)
{
  delete dft;
}

}  // namespace dft
