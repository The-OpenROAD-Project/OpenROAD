// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Clock.hh"
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

// Returns true if the given instance cell's is a sequential cell, false
// otherwise
bool IsSequentialCell(sta::dbNetwork* db_network, odb::dbInst* instance);

// Returns a vector of dbITerm for every clock that there is in the instance.
// For black boxes or CTLs (Core Test Language), we can have more than one clock
std::vector<odb::dbITerm*> GetClockPin(odb::dbInst* inst);

// Returns a sta::Clock of the given iterm
std::optional<sta::Clock*> GetClock(sta::dbSta* sta, odb::dbITerm* iterm);

// Checks if the given LibertyCell is really a Scan Cell with a Scan In and a
// Scan Enable
bool IsScanCell(const sta::LibertyCell* libertyCell);

// Convenience method to create a new port
odb::dbBTerm* CreateNewPort(odb::dbBlock* block,
                            const std::string& port_name,
                            utl::Logger* logger,
                            odb::dbNet* net = nullptr);

}  // namespace dft::utils
