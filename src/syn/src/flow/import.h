// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

namespace sta {
class Network;
}

namespace utl {
class Logger;
}

namespace syn {

class Graph;

void importTargets(Graph& g, sta::Network* network, utl::Logger* logger);

}  // namespace syn
