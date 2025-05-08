// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rsz/MakeResizer.hh"

#include <memory>
#include <utility>

#include "SteinerRenderer.h"
#include "gui/gui.h"
#include "rsz/Resizer.hh"
#include "utl/decode.h"

extern "C" {
extern int Rsz_Init(Tcl_Interp* interp);
}

namespace rsz {
extern const char* rsz_tcl_inits[];

rsz::Resizer* makeResizer()
{
  return new rsz::Resizer;
}

void deleteResizer(rsz::Resizer* resizer)
{
  delete resizer;
}

void initResizer(rsz::Resizer* resizer,
                 Tcl_Interp* tcl_interp,
                 utl::Logger* logger,
                 odb::dbDatabase* db,
                 sta::dbSta* sta,
                 stt::SteinerTreeBuilder* stt_builder,
                 grt::GlobalRouter* global_router,
                 dpl::Opendp* dp)
{
  std::unique_ptr<rsz::AbstractSteinerRenderer> steiner_renderer;
  if (gui::Gui::enabled()) {
    steiner_renderer = std::make_unique<rsz::SteinerRenderer>();
  }
  resizer->init(logger,
                db,
                sta,
                stt_builder,
                global_router,
                dp,
                std::move(steiner_renderer));
  // Define swig TCL commands.
  Rsz_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, rsz::rsz_tcl_inits);
}

}  // namespace rsz
