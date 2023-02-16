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
#pragma once

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Liberty.hh"
#include "utl/Logger.h"

namespace dft::utils {

// Replace the old_instance cell with a new master. The connections of the old
// cell will be preserved by using the given port mapping from
// <old_port_name, new_port_name>. Returns the new instance and deletes the old
// one. The name of the new instance is going to be the same as the old one.
odb::dbInst* ReplaceCell(
    odb::dbBlock* top_block,
    odb::dbInst* old_instance,
    odb::dbMaster* new_master,
    const std::unordered_map<std::string, std::string>& port_mapping);

// Find the net that has the ground in the given block.
odb::dbNet* FindGroundNet(sta::dbNetwork* db_network, odb::dbBlock* block);

// Connects the scan pins of a cell to ground to make it functional equivalent
// to a non-scan cell
void TieScanPins(sta::dbNetwork* db_network,
                 odb::dbInst* instance,
                 sta::LibertyCell* scan_cell,
                 odb::dbNet* ground_net);

// Returns true if the given instance cell's is a sequential cell, false
// otherwise
bool IsSequentialCell(sta::dbNetwork* db_network, odb::dbInst* instance);

}  // namespace dft::utils
