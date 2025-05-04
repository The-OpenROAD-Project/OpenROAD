// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "stt/MakeSteinerTreeBuilder.h"

#include "stt/SteinerTreeBuilder.h"
#include "utl/decode.h"

extern "C" {
extern int Stt_Init(Tcl_Interp* interp);
}

namespace stt {
// Tcl files encoded into strings.
extern const char* stt_tcl_inits[];

stt::SteinerTreeBuilder* makeSteinerTreeBuilder()
{
  return new stt::SteinerTreeBuilder();
}

void deleteSteinerTreeBuilder(stt::SteinerTreeBuilder* stt_builder)
{
  delete stt_builder;
}

void initSteinerTreeBuilder(stt::SteinerTreeBuilder* stt_builder,
                            odb::dbDatabase* db,
                            utl::Logger* logger,
                            Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Stt_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, stt::stt_tcl_inits);
  stt_builder->init(db, logger);
}

}  // namespace stt
