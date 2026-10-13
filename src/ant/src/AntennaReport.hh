// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>

#include "ant/AntennaChecker.hh"

namespace ant {

// The text check_antennas prints (with -verbose) or writes to its report
// file for one net.
std::string formatNetCheck(const NetCheck& net_check);

}  // namespace ant
