// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "rcx/MakeOpenRCX.h"

#include "ord/OpenRoad.hh"
#include "rcx/ext.h"
#include "utl/decode.h"

namespace rcx {
// Tcl files encoded into strings.
extern const char* rcx_tcl_inits[];
}  // namespace rcx

namespace rcx {
extern "C" {
extern int Rcx_Init(Tcl_Interp* interp);
}
}  // namespace rcx

namespace ord {

rcx::Ext* makeOpenRCX()
{
  return new rcx::Ext();
}

void deleteOpenRCX(rcx::Ext* extractor)
{
  delete extractor;
}

void initOpenRCX(OpenRoad* openroad)
{
  openroad->getOpenRCX()->init(openroad->getDb(),
                               openroad->getLogger(),
                               ord::OpenRoad::getVersion(),
                               [openroad] {
                                 rcx::Rcx_Init(openroad->tclInterp());
                                 utl::evalTclInit(openroad->tclInterp(),
                                                  rcx::rcx_tcl_inits);
                               });
}

}  // namespace ord
