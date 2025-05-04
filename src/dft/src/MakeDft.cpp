// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "dft/MakeDft.hh"

#include "DftConfig.hh"
#include "ScanReplace.hh"
#include "dft/Dft.hh"
#include "utl/decode.h"

namespace dft {
extern const char* dft_tcl_inits[];

extern "C" {
extern int Dft_Init(Tcl_Interp* interp);
}

dft::Dft* makeDft()
{
  return new dft::Dft();
}

void initDft(dft::Dft* dft,
             odb::dbDatabase* db,
             sta::dbSta* sta,
             utl::Logger* logger,
             Tcl_Interp* tcl_interp)
{
  Dft_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, dft::dft_tcl_inits);
  dft->init(db, sta, logger);
}

void deleteDft(dft::Dft* dft)
{
  delete dft;
}

}  // namespace dft
