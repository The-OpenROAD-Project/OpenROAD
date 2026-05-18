// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>

namespace utl {
class Logger;
}

namespace syn {

class Graph;

void abcRoundtrip(Graph& g, const std::string& commands, utl::Logger* logger);

}  // namespace syn
