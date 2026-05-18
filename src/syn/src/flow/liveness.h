// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

namespace utl {
class Logger;
}

namespace syn {

class Graph;

// Liveness analysis to detect registers which can be replaced with a constant
// tie
void livenessOpt(Graph& g, utl::Logger* logger, bool replace_combinational);

}  // namespace syn
