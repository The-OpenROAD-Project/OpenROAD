///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019-2020, The Regents of the University of California
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

#include "mpl2/MakeMacroPlacer.h"

#include <tcl.h>

#include "mpl2/rtl_mp.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"

namespace sta {
extern const char* mpl2_tcl_inits[];
}

extern "C" {
extern int Mpl2_Init(Tcl_Interp* interp);
}

namespace ord {

mpl::MacroPlacer2* makeMacroPlacer2()
{
  return new mpl::MacroPlacer2;
}

void initMacroPlacer2(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  Mpl2_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::mpl2_tcl_inits);
  openroad->getMacroPlacer2()->init(openroad->getLogger());
}

void deleteMacroPlacer2(mpl::MacroPlacer2* macro_placer)
{
  delete macro_placer;
}

}  // namespace ord
