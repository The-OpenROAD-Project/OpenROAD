// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace utl {
class Logger;
}

namespace odb {

class dbBlock;

void normalizeWireConnectivity(utl::Logger* logger, dbBlock* block);

}  // namespace odb
