/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, OpenROAD
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

#include <tcl.h>

#include "openroad/OpenRoad.hh"
#include "pdn/MakePdnGen.hh"
#include "pdn/PdnGen.hh"

namespace sta {

extern const char *pdn_tcl_inits[];
extern void evalTclInit(Tcl_Interp*, const char*[]);

}

namespace pdn {
extern "C" {
extern int Pdn_Init(Tcl_Interp *interp);
}
}


namespace ord {

void
initPdnGen(OpenRoad *openroad)
{
  Tcl_Interp *interp = openroad->tclInterp();
  // Define swig TCL commands.
  pdn::Pdn_Init(interp);
  // Eval encoded sta TCL sources.
  sta::evalTclInit(interp, sta::pdn_tcl_inits);

  openroad->getPdnGen()->init(openroad->getDb(), openroad->getLogger());
}

pdn::PdnGen* makePdnGen()
{
  return new pdn::PdnGen();
}


void deletePdnGen(pdn::PdnGen* pdngen)
{
  delete pdngen;
}

} // namespace ord
