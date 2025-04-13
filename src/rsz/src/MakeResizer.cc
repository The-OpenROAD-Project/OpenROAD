// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rsz/MakeResizer.hh"

#include <memory>
#include <utility>

#include "SteinerRenderer.h"
#include "gui/gui.h"
#include "ord/OpenRoad.hh"
#include "rsz/Resizer.hh"
#include "utl/decode.h"

extern "C" {
extern int Rsz_Init(Tcl_Interp* interp);
}

namespace rsz {
extern const char* rsz_tcl_inits[];
}

namespace ord {

rsz::Resizer* makeResizer()
{
  return new rsz::Resizer;
}

void deleteResizer(rsz::Resizer* resizer)
{
  delete resizer;
}

void initResizer(OpenRoad* openroad)
{
  std::unique_ptr<rsz::AbstractSteinerRenderer> steiner_renderer;
  if (gui::Gui::enabled()) {
    steiner_renderer = std::make_unique<rsz::SteinerRenderer>();
  }
  Tcl_Interp* interp = openroad->tclInterp();
  openroad->getResizer()->init(openroad->getLogger(),
                               openroad->getDb(),
                               openroad->getSta(),
                               openroad->getSteinerTreeBuilder(),
                               openroad->getGlobalRouter(),
                               openroad->getOpendp(),
                               std::move(steiner_renderer));
  // Define swig TCL commands.
  Rsz_Init(interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(interp, rsz::rsz_tcl_inits);
}

}  // namespace ord
