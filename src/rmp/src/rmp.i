/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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
//
///////////////////////////////////////////////////////////////////////////////

%{
#include "rmp/Restructure.h"
#include "ord/OpenRoad.hh"

namespace ord {
// Defined in OpenRoad.i
rmp::Restructure *
getRestructure();
}

using namespace rmp;
using ord::getRestructure;
%}

%include "../../Exception.i"

%inline %{

void set_mode_cmd(const char* modeName)
{
  getRestructure()->setMode(modeName);
}

void set_logfile_cmd(const char* logfile)
{
  getRestructure()->setLogfile(logfile);
}

void set_locell_cmd(const char* val)
{
  getRestructure()->setLoCell(val);
}

void set_loport_cmd(const char* val)
{
  getRestructure()->setLoPort(val);
}

void set_hicell_cmd(const char* val)
{
  getRestructure()->setHiCell(val);
}

void set_hiport_cmd(const char* val)
{
  getRestructure()->setHiPort(val);
}
void
restructure_cmd(const char* libertyFileName, float slack_threshold, int depth_threshold)
{
  getRestructure()->run(libertyFileName, slack_threshold, depth_threshold);
}

%}