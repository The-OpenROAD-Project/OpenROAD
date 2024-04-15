/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "rsz/MakeResizer.hh"

#include "SteinerRenderer.h"
#include "gui/gui.h"
#include "ord/OpenRoad.hh"
#include "rsz/Resizer.hh"
#include "sta/StaMain.hh"

extern "C" {
extern int Rsz_Init(Tcl_Interp* interp);
}

namespace sta {
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
  sta::evalTclInit(interp, sta::rsz_tcl_inits);
}

}  // namespace ord
