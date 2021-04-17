/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, James Cherry, Parallax Software, Inc.
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

#include "pdn/PdnGen.hh"

// Defined by OpenRoad.i
namespace ord {

odb::dbDatabase* getDb();

}

%}

%inline %{

namespace pdn {

void
set_special_net_iterms(const char* net_name) {
  odb::dbNet* net = ord::getDb()->getChip()->getBlock()->findNet(net_name);
  
  pdn::setSpecialITerms(net);
}

void
set_special_net_iterms(odb::dbNet* net) {
  pdn::setSpecialITerms(net);
}

void
global_connect(odb::dbBlock* block, const char* inst_pattern, const char* pin_pattern, odb::dbNet* net) {
  pdn::globalConnect(block, inst_pattern, pin_pattern, net);
}

void
global_connect_region(odb::dbBlock* block, const char* region, const char* inst_pattern, const char* pin_pattern, odb::dbNet* net) {
  pdn::globalConnectRegion(block, region, inst_pattern, pin_pattern, net);
}

} // namespace

%} // inline
