///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2020, The Regents of the University of California
// All rights reserved.
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include <tcl.h>
#include "sta/StaMain.hh"
#include "ord/OpenRoad.hh"
#include "gpl/MakeReplace.h"
#include "gpl/Replace.h"

namespace sta {
extern const char *gpl_tcl_inits[];
}

extern "C" {
extern int Gpl_Init(Tcl_Interp* interp);
}

namespace ord {

gpl::Replace* 
makeReplace() {
  return new gpl::Replace();
}

void
initReplace(OpenRoad* openroad) {
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  Gpl_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::gpl_tcl_inits);
  // Replace should define an init function that takes these args.
  // There is no reason for all these set functions to exist. -cherry
  openroad->getReplace()->setDb(openroad->getDb());
  openroad->getReplace()->setLogger(openroad->getLogger());
  openroad->getReplace()->setGlobalRouter(openroad->getGlobalRouter());
  openroad->getReplace()->setResizer(openroad->getResizer());
}

void
deleteReplace(gpl::Replace *replace) {
  delete replace;
}

}
