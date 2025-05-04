// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "cts/MakeTritoncts.h"

#include "CtsOptions.h"
#include "cts/TritonCTS.h"
#include "odb/db.h"
#include "utl/decode.h"

extern "C" {
extern int Cts_Init(Tcl_Interp* interp);
}

namespace cts {

extern const char* cts_tcl_inits[];

cts::TritonCTS* makeTritonCts()
{
  return new cts::TritonCTS();
}

void initTritonCts(cts::TritonCTS* cts,
                   odb::dbDatabase* db,
                   sta::dbNetwork* network,
                   sta::dbSta* sta,
                   stt::SteinerTreeBuilder* stt_builder,
                   rsz::Resizer* resizer,
                   utl::Logger* logger,
                   Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Cts_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, cts::cts_tcl_inits);
  cts->init(logger, db, network, sta, stt_builder, resizer);
}

void deleteTritonCts(cts::TritonCTS* tritoncts)
{
  delete tritoncts;
}

}  // namespace cts
