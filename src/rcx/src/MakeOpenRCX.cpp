// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "rcx/MakeOpenRCX.h"

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

void initOpenRCX(rcx::Ext* extractor,
                 odb::dbDatabase* db,
                 utl::Logger* logger,
                 const char* spef_version,
                 Tcl_Interp* tcl_interp)
{
  rcx::Rcx_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, rcx::rcx_tcl_inits);

  extractor->init(db, logger, spef_version);
}

}  // namespace ord
