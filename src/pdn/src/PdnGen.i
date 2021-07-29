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

%{
#include "pdn/PdnGen.hh"
#include <regex>
#include <memory>

namespace ord {
// Defined in OpenRoad.i
pdn::PdnGen* getPdnGen();
utl::Logger* getLogger();
} // namespace ord

using std::regex;
using utl::PDN;

%}

%include "../../Exception.i"

%inline %{

namespace pdn {

void
set_special_net_iterms() {
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->setSpecialITerms();
}

void
set_special_net_iterms(odb::dbNet* net) {
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->setSpecialITerms(net);
}

void
add_global_connect(const char* inst_pattern, const char* pin_pattern, odb::dbNet* net) {
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->addGlobalConnect(inst_pattern, pin_pattern, net);
}

void
clear_global_connect() {
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->clearGlobalConnect();
}

void
add_global_connect(odb::dbBlock* block, const char* region_name, const char* inst_pattern, const char* pin_pattern, odb::dbNet* net) {
  PdnGen* pdngen = ord::getPdnGen();
  
  odb::dbRegion* region = block->findRegion(region_name);
  if (region == nullptr) {
    ord::getLogger()->error(PDN, 53, "Region {} not found.", region_name);
    return;
  }
  
  for (dbBox* regionBox : region->getBoundaries())
    pdngen->addGlobalConnect(regionBox, inst_pattern, pin_pattern, net);
}

void
global_connect(odb::dbBlock* block) {
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->globalConnect(block);
}

void
global_connect(odb::dbBlock* block, const char* inst_pattern, const char* pin_pattern, odb::dbNet* net) {
  PdnGen* pdngen = ord::getPdnGen();
  
  std::shared_ptr<regex> instReg = std::make_shared<regex>(inst_pattern);
  std::shared_ptr<regex> pinReg = std::make_shared<regex>(pin_pattern);
  
  pdngen->globalConnect(block, instReg, pinReg, net);
}

void
global_connect_region(odb::dbBlock* block, const char* region_name, const char* inst_pattern, const char* pin_pattern, odb::dbNet* net) {
  PdnGen* pdngen = ord::getPdnGen();

  odb::dbRegion* region = block->findRegion(region_name);
  if (region == nullptr) {
    ord::getLogger()->error(PDN, 54, "Region {} not foundi.", region_name);
    return;
  }
  std::shared_ptr<regex> instReg = std::make_shared<regex>(inst_pattern);
  std::shared_ptr<regex> pinReg = std::make_shared<regex>(pin_pattern);
  
  for (dbBox* regionBox : region->getBoundaries())
    pdngen->globalConnectRegion(block, regionBox, instReg, pinReg, net);
}

} // namespace

%} // inline
