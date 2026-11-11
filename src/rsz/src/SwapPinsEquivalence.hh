// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "sta/LibertyClass.hh"
#include "utl/Logger.h"

namespace rsz {

// The inputs of cell interchangeable with input_port on every output
// function, in port id order, found by simulating the functions' truth
// tables. Empty when cell is not a plain combinational gate: sequential,
// isolation, with a bus, bundle or bidirectional port, an output without a
// function, more than 16 inputs, or no output that depends on input_port.
std::vector<sta::LibertyPort*> equivalentSwapInputs(
    utl::Logger* logger,
    const sta::LibertyCell* cell,
    const sta::LibertyPort* input_port);

}  // namespace rsz
