// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <vector>

#include "stt/SteinerTreeBuilder.h"

namespace utl {
class Logger;
}

namespace pdr {

stt::Tree primDijkstra(const std::vector<int>& x,
                       const std::vector<int>& y,
                       int driver_index,
                       float alpha,
                       utl::Logger* logger);

}  // namespace pdr
