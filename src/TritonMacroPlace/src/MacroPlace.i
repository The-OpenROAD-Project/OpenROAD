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

%module mpl

%{
#include "openroad/OpenRoad.hh"
#include "mpl/MacroPlace.h"

namespace ord {
// Defined in OpenRoad.i
mpl::MacroPlacer*
getMacroPlacer();
}

using ord::getMacroPlacer;

%}

%include "../../Exception.i"

%inline %{

namespace mpl {

void
set_halo(double halo_v, double halo_h)
{
  MacroPlacer* macro_placer = getMacroPlacer();
  macro_placer->setHalo(halo_v, halo_h);
}

void
set_channel(double channel_v, double channel_h)
{
  MacroPlacer* macro_placer = getMacroPlacer();
  macro_placer->setChannel(channel_v, channel_h); 
}

void
set_fence_region(double lx, double ly, double ux, double uy)
{
  MacroPlacer* macro_placer = getMacroPlacer();
  macro_placer->setFenceRegion(lx, ly, ux, uy); 
}

void
place_macros()
{
  MacroPlacer* macro_placer = getMacroPlacer();
  macro_placer->placeMacros(); 
} 

void
set_global_config(const char* file) 
{
  MacroPlacer* macro_placer = getMacroPlacer();
  macro_placer->setGlobalConfig(file); 
}

void
set_local_config(const char* file)
{
  MacroPlacer* macro_placer = getMacroPlacer();
  macro_placer->setLocalConfig(file); 
}

}

%} // inline
