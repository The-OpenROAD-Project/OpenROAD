///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "dft/MakeDft.hh"

#include "DftConfig.hh"
#include "ScanReplace.hh"
#include "dft/Dft.hh"
#include "ord/OpenRoad.hh"

namespace sta {
extern const char* dft_tcl_inits[];
extern void evalTclInit(Tcl_Interp*, const char*[]);
}  // namespace sta

namespace dft {

extern "C" {
extern int Dft_Init(Tcl_Interp* interp);
}

dft::Dft* makeDft()
{
  return new dft::Dft();
}

void initDft(ord::OpenRoad* openroad)
{
  Tcl_Interp* interp = openroad->tclInterp();
  Dft_Init(interp);
  sta::evalTclInit(interp, sta::dft_tcl_inits);
  openroad->getDft()->init(
      openroad->getDb(), openroad->getSta(), openroad->getLogger());
}

void deleteDft(dft::Dft* dft)
{
  delete dft;
}

}  // namespace dft
