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

#include "Utils.hh"

#include <iostream>

#include "db_sta/dbNetwork.hh"

namespace dft::utils {

namespace {
constexpr char kTmpScanFlopName[] = "tmp_scan_flop";

void PopulatePortNameToNet(
    odb::dbInst* instance,
    std::vector<std::tuple<std::string, odb::dbNet*>>& port_name_to_net)
{
  for (odb::dbITerm* iterm : instance->getITerms()) {
    port_name_to_net.push_back({iterm->getMTerm()->getName(), iterm->getNet()});
  }
}

void ConnectPinsToNets(
    odb::dbInst* instance,
    const std::vector<std::tuple<std::string, odb::dbNet*>>& port_name_to_net,
    const std::unordered_map<std::string, std::string>& port_mapping)
{
  for (const auto& [port_name_old, net] : port_name_to_net) {
    if (net == nullptr) {
      continue;
    }
    std::string port_name_new = port_mapping.find(port_name_old)->second;
    instance->findITerm(port_name_new.c_str())->connect(net);
  }
}

}  // namespace

bool IsSequentialCell(sta::dbNetwork* db_network, odb::dbInst* instance)
{
  odb::dbMaster* master = instance->getMaster();
  sta::Cell* master_cell = db_network->dbToSta(master);
  sta::LibertyCell* liberty_cell = db_network->libertyCell(master_cell);
  return liberty_cell->hasSequentials();
}

odb::dbInst* ReplaceCell(
    odb::dbBlock* top_block,
    odb::dbInst* old_instance,
    odb::dbMaster* new_master,
    const std::unordered_map<std::string, std::string>& port_mapping)
{
  std::vector<std::tuple<std::string, odb::dbNet*>> port_name_to_net;
  PopulatePortNameToNet(old_instance, port_name_to_net);

  odb::dbInst* new_instance
      = odb::dbInst::create(top_block, new_master, kTmpScanFlopName);
  std::string old_cell_name = old_instance->getName();

  // Delete the old cell
  odb::dbInst::destroy(old_instance);

  // Connect the new cell to the old instance's nets
  ConnectPinsToNets(new_instance, port_name_to_net, port_mapping);

  // Rename as the old cell
  new_instance->rename(old_cell_name.c_str());

  return new_instance;
}
}  // namespace dft::utils
